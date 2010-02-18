// OpenGL Shader Language resource.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// Modified by Anders Bach Nielsen <abachn@daimi.au.dk> - 21. Nov 2007
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#define printinfo true

#include <Resources/OpenGLShader.h>

#include <Logging/Logger.h>
#include <Meta/OpenGL.h>
#include <Resources/Exceptions.h>
#include <Resources/File.h>
#include <Resources/ResourceManager.h>
#include <Resources/ITexture2D.h>
#include <Resources/ITexture3D.h>

namespace OpenEngine {
    namespace Resources {

        OpenGLShader::OpenGLShader() {
            resource.clear();
            nextTexUnit = 0;
            shaderProgram = 0;
        }

        OpenGLShader::OpenGLShader(string filename)
            : resource(filename) {
            nextTexUnit = 0;
            shaderProgram = 0;
        }

        void OpenGLShader::Load() {
            // Load the resource and its attributes from a file, if a
            // file is available.
            if (!resource.empty()){
                logger.info << "Load the shader from disk " << logger.end;
                LoadResource(resource);
            }

            // Load the shader onto the gpu.
#ifdef OE_SAFE
            if (vertexShaders.empty() && 
                geometryShaders.empty() && 
                fragmentShaders.empty())
                throw ResourceException("No shaders specified");
#endif
      
            logger.info << "Bind the shader program to gpu " << logger.end;
            BindShaderPrograms();
        }
        
        void OpenGLShader::Unload() {
            glDeleteShader(shaderProgram);
            shaderProgram = 0;
        }

        void OpenGLShader::ApplyShader(){
#ifdef OE_SAFE
            if (shaderProgram == 0)
                throw ResourceException("No shader to apply. Perhaps loading failed.");
#endif

            // Bind the shader program.
            glUseProgram(shaderProgram);

            BindUniforms();
            BindTextures();
        }

        void OpenGLShader::ReleaseShader(){
            glUseProgram(0);
        }

#undef UNIFORM1
#define UNIFORM1(type, extension)                                   \
        void OpenGLShader::SetUniform(string name, type value){     \
            type *data = new type[1];                               \
            data[0] = value;                                        \
            uniform uni;                                            \
            uni.loc = 0;                                            \
            uni.kind = UNIFORM##extension ;                         \
            uni.data = data;                                        \
            unboundUniforms[name] = uni;                            \
        }                                                       
            
#undef UNIFORMn
#define UNIFORMn(params, type, extension)                               \
        void OpenGLShader::SetUniform(string name, Vector<params, type> value){ \
            type *data = new type[params];                              \
            value.ToArray(data);                                        \
            uniform uni;                                                \
            uni.loc = 0;                                                \
            uni.kind = UNIFORM##params##extension ;                     \
            uni.data = data;                                            \
            unboundUniforms[name] = uni;                                \
        }
#include "UniformList.h"


        void OpenGLShader::SetTexture(string name, ITexture2DPtr tex){
            sampler2D sam;
            sam.loc = 0;
            sam.texUnit = 0;
            sam.tex = tex;
            unboundTex2Ds[name] = sam;
        }

        void OpenGLShader::SetTexture(string name, ITexture3DPtr tex){
            sampler3D sam;
            sam.loc = 0;
            sam.texUnit = 0;
            sam.tex = tex;
            unboundTex3Ds[name] = sam;
        }

        TextureList OpenGLShader::GetTextures(){
            TextureList list = TextureList();
            map<string, sampler2D>::iterator itr = boundTex2Ds.begin();
            while(itr != boundTex2Ds.end()){
                list.push_back(itr->second.tex);
                itr++;
            }

            itr = unboundTex2Ds.begin();
            while(itr != unboundTex2Ds.end()){
                list.push_back(itr->second.tex);
                itr++;
            }

            return list;
        }

        void OpenGLShader::SetAttribute(string name, Vector<3,float> value){
            throw Exception("Not implemented!");
        }

        void OpenGLShader::BindAttribute(int id, string name) {
            throw Exception("Not implemented!");            
        }

        void OpenGLShader::VertexAttribute(int id, Vector<3,float> vec) {
            throw Exception("Not implemented!");
        }

        int OpenGLShader::GetAttributeID(const string name) {
            throw Exception("Not implemented!");
        }

        //  *** Private helper methods ***
        
        /**
         * Resets the uniforms. This means preparing the known
         * uniforms to be bound again.
         */
        void OpenGLShader::ResetProperties(){
            // Move bound uniforms to unbound, to preserve attributes not
            // specified in the glsl file.
            boundUniforms.insert(unboundUniforms.begin(), unboundUniforms.end());
            unboundUniforms = map<string, uniform>(boundUniforms);
            boundTex2Ds.insert(unboundTex2Ds.begin(), unboundTex2Ds.end());
            unboundTex2Ds = map<string, sampler2D>(boundTex2Ds);
            boundTex3Ds.insert(unboundTex3Ds.begin(), unboundTex3Ds.end());
            unboundTex3Ds = map<string, sampler3D>(boundTex3Ds);
            boundUniforms.clear();
            boundTex2Ds.clear();
            boundTex3Ds.clear();

            // Set all their loc's to 0 since we no longer know where they are.
            map<string, uniform>::iterator itr = unboundUniforms.begin();
            while (itr != unboundUniforms.end()){
                itr->second.loc = 0;
                itr++;
            }
            map<string, sampler2D>::iterator itr2 = unboundTex2Ds.begin();
            while (itr2 != unboundTex2Ds.end()){
                itr2->second.loc = 0;
                itr2++;
            }
            map<string, sampler3D>::iterator itr3 = unboundTex3Ds.begin();
            while (itr3 != unboundTex3Ds.end()){
                itr3->second.loc = 0;
                itr3++;
            }
        }

        void OpenGLShader::LoadResource(string resource){
            ResetProperties();

            // Load the file.
            ifstream* in = File::Open(resource);
            
            char buf[255], file[255];
            int line = 0;
            while (!in->eof()){
                ++line;
                in->getline(buf, 255);
                string type = string(buf,5);

                // Empty lines and comments can be ignored.
                if (type.empty() ||
                    buf[0] == '#')
                    continue;

                if (type == "vert:") {
                    if (sscanf(buf, "vert: %s", file) == 1)
                        vertexShaders.push_back(file);
                    else
                        logger.warning << "Line("<<line<<") Invalid vertex shader." << logger.end;
                }else if (type == "geom:") {
                    if (sscanf(buf, "geom: %s", file) == 1)
                        geometryShaders.push_back(file);
                    else
                        logger.warning << "Line("<<line<<") Invalid geometry shader." << logger.end;
                }else if (type == "frag:") {
                    if (sscanf(buf, "frag: %s", file) == 1)
                        fragmentShaders.push_back(file);
                    else
                        logger.warning << "Line("<<line<<") Invalid fragment shader." << logger.end;
                }else if (type == "text:" || type == "tex2D" || type == "tex3D") {
                    const int maxlength = 300;
                    char fileandname[maxlength];
                    if (sscanf(buf, "text: %s", fileandname) == 1) {
                        int seperator=0;
                        for(int i=0;i<maxlength;i++) {
                            if(fileandname[i]=='|')
                                seperator=i;
                            if(fileandname[i]=='\0')
                                break;
                        }
                        if(seperator==0) {
                            logger.error << "no separetor(|) between texture name and file, texture not loaded" << logger.end;
                            continue;
                        }
                        string texname = string(fileandname,seperator);
                        string texfile = string(fileandname+seperator+1);
                        if (type == "text:" || type == "tex2D") {
                            ITexture2DPtr t = ResourceManager<ITexture2D>::Create(texfile);
                            SetTexture(texname, t);
                        }else{
                            ITexture3DPtr t = ResourceManager<ITexture3D>::Create(texfile);
                            SetTexture(texname, t);
                        }
                    }else
                        logger.warning << "Line("<<line<<") Invalid texture resource: '" << file << "'" << logger.end;
                }else if (type == "attr:" || type == "unif:") {
                    char name[255];
                    float attr[4];
                    int n = sscanf(buf, "attr: %s = %f %f %f %f", name, &attr[0], &attr[1], &attr[2], &attr[3]) - 1;
                    switch(n){
                    case 1:
                        SetUniform(string(name), attr[0]);
                        break;
                    case 2:
                        SetUniform(string(name), Vector<2, float>(attr[0], attr[1]));
                        break;
                    case 3:
                        SetUniform(string(name), Vector<3, float>(attr[0], attr[1], attr[2]));
                        break;
                    case 4:
                        SetUniform(string(name), Vector<4, float>(attr[0], attr[1], attr[2], attr[3]));
                        break;
                    }
                }
            }
        
            in->close();
            delete in;
        }     

        void OpenGLShader::PrintShaderInfoLog(GLuint shader){
            if(!printinfo) return;
            GLint infologLength = 0, charsWritten = 0;
            GLchar *infoLog;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLength);
            if (infologLength > 0) {
                infoLog = (GLchar *)malloc(infologLength);
                if (infoLog==NULL) {
                    throw Exception("Could not allocate InfoLog buffer");
                }
                glGetShaderInfoLog(shader, infologLength, &charsWritten, infoLog);
                logger.info << "Shader InfoLog:\n \"" << infoLog << "\"" << logger.end;
                free(infoLog);
            }
        }
        
        void OpenGLShader::PrintProgramInfoLog(GLuint program){
            if(!printinfo) return;
            GLint infologLength = 0, charsWritten = 0;
            GLchar* infoLog;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infologLength);
            if (infologLength > 0) {
                infoLog = (GLchar *)malloc(infologLength);
                if (infoLog==NULL) {
                    logger.error << "Could not allocate InfoLog buffer" << logger.end;
                    return;
                }
                glGetProgramInfoLog(program, infologLength, &charsWritten, infoLog);
                logger.info << "Program InfoLog:\n \"" << infoLog << "\"" << logger.end;
                free(infoLog);
            }
        }
        
        GLint OpenGLShader::GetUniLoc(GLuint program, const GLchar *name){
            GLint loc = glGetUniformLocation(program, name);
#ifdef OE_SAFE
            if (loc == -1)
                throw Exception( string("No such uniform named \"") + name + "\"");
#endif
            return loc;
        }
        
        void OpenGLShader::BindShaderPrograms(){
            shaderProgram = glCreateProgram();

            // attach vertex shader
            if (!vertexShaders.empty()){
                GLuint shader = LoadShader(vertexShaders, GL_VERTEX_SHADER);
                if (shader !=0)
                    glAttachShader(shaderProgram, shader);
                else {
                    logger.error << "Failed loading vertexshader" << logger.end;
                    Unload();
                    return;
                }
            }

            /*        
            // attach geometry shader
            if (!geometryShaders.empty()){
                GLuint shader = LoadShader(geometryShaders, GEOMETRY_SHADER_ARB);
                if (shader !=0)
                    glAttachShader(shaderProgram, shader);
                else {
                    logger.error << "Failed loading geometryshader" << logger.end;
                    Unload();
                    return;
                }
            }
            */

            // attach fragment shader
            if (!fragmentShaders.empty()){
                GLuint shader = LoadShader(fragmentShaders, GL_FRAGMENT_SHADER);
                if (shader !=0)
                    glAttachShader(shaderProgram, shader);
                else {
                    logger.error << "Failed loading fragmentshader" << logger.end;
                    Unload();
                    return;
                }
            }
            
            // Link the program object and print out the info log
            glLinkProgram(shaderProgram);
            GLint linked;
            glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linked);
            
            CHECK_FOR_GL_ERROR();
            PrintProgramInfoLog(shaderProgram);
            
            if(linked == 0){
                logger.error << "could not link shader program" << logger.end;
                Unload();
                return;
            }
            
        }

        /**
         * Loads the given shader. OpenGL 2.0 and above.
         */        
        GLuint OpenGLShader::LoadShader(vector<string> files, int type){
            GLuint shader = glCreateShader(type);
            
            unsigned int size = files.size();
            const GLchar* shaderBits[size];

            // Read all the shaders from disk
            for (unsigned int i = 0; i < size; ++i){
                if (printinfo)
                    logger.info << "Loading shader: " << files[i] << logger.end;
                shaderBits[i] = File::ReadShader<GLchar>(DirectoryManager::FindFileInPath(files[i]));
                if (shaderBits[i] == NULL) return 0;
            }

            glShaderSource(shader, size, shaderBits, NULL);

            // Compile shader
            glCompileShader(shader);
            GLint  compiled;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
            if (compiled==0) {
                logger.error << "failed compiling shader program consisting of: " << logger.end;
                for (unsigned int i = 0; i< size; ++i)
                    logger.error << files[i] << logger.end;
                GLsizei bufsize;
                const int maxBufSize = 100;
                char buffer[maxBufSize];
                glGetShaderInfoLog(shader, maxBufSize, &bufsize, buffer);
                logger.error << "compile errors: " << buffer << logger.end;
                glDeleteShader(shader);
                return 0;
            }
            PrintShaderInfoLog(shader);
            return shader;
        }
                
        /**
         * Binds the unbound uniforms to the shader.
         *
         * Assumes the shader is already applied.
         */
        void OpenGLShader::BindUniforms(){
            
            // Apply the unbound uniforms.
            map<string, uniform>::iterator unbound = unboundUniforms.begin();
            while (unbound != unboundUniforms.end()){
                string name = unbound->first;
                uniform uni = unbound->second;
                map<string, uniform>::iterator bound = boundUniforms.find(name);
                if (bound != boundUniforms.end()){
                    // If the uniform has already been bound, then
                    // delete the old data, copy the new and bind it.
                    DeleteData(bound->second);
                    bound->second.data = uni.data;
                    BindUniform(bound->second);
                }else{
                    // Else get the location of the uniform, bind it
                    // and copy it to the bound array.
                    uni.loc = GetUniLoc(shaderProgram, name.c_str());
                    BindUniform(uni);
                    boundUniforms[name] = uni;
                }
                unbound++;
            }
            unboundUniforms.clear();
        }
              
        /**
         * Bind the uniform to the gpu.
         */
        void OpenGLShader::BindUniform(uniform uni){
            switch(uni.kind){
                
#undef UNIFORM1
#define UNIFORM1(type, extension)                                       \
                case UNIFORM##extension :                               \
                    glUniform1##extension##v (uni.loc, 1, (const GL##type*) uni.data); \
                    break;
#undef UNIFORMn
#define UNIFORMn(params, type, extension)                               \
                case UNIFORM##params##extension :                       \
                    glUniform##params##extension##v (uni.loc, 1, (const GL##type*) uni.data); \
                    break;
#include "UniformList.h"

            default:
                throw Exception("Unsupported uniform type. How did you manage that?");
            }
        }

        void OpenGLShader::DeleteData(uniform uni){
            switch(uni.kind){

#undef UNIFORM1
#define UNIFORM1(type, extension)                                    \
                case UNIFORM##extension :                            \
                    delete [] ( type* ) uni.data;                     \
                    break;
#undef UNIFORMn
#define UNIFORMn(params, type, extension)                               \
                case UNIFORM##params##extension :                       \
                    delete [] ( type* ) uni.data;                        \
                    break;
#include "UniformList.h"

            default:
                throw Exception("Unsupported uniform type. How did you manage that?");
            }
        }

        /**
         * Binds the unbound textures to the shader. Then bind them to the gpu.
         *
         * Assumes the shader is already applied.
         */
        void OpenGLShader::BindTextures(){

            // Apply the unbound textures.
            map<string, sampler2D>::iterator unbound = unboundTex2Ds.begin();
            while(unbound != unboundTex2Ds.end()){
                // Check if the texture already exists
                string name = unbound->first;
                sampler2D sam = unbound->second;
                map<string, sampler2D>::iterator bound = boundTex2Ds.find(name);
                if (bound != boundTex2Ds.end()){ 
                    // sampler already bound, replace it's texture
                    bound->second.tex = sam.tex;
                }else{
                    // Set the samplers values and add it to the bound
                    // map.
                    sam.loc = GetUniLoc(shaderProgram, name.c_str());
                    sam.texUnit = nextTexUnit++;
                    glUniform1i(sam.loc, sam.texUnit);
                    boundTex2Ds[name] = sam;
                }
                unbound++;
            }
            unboundTex2Ds.clear();

            map<string, sampler3D>::iterator lazy = unboundTex3Ds.begin();
            while(lazy != unboundTex3Ds.end()){
                // Check if the texture already exists
                string name = lazy->first;
                sampler3D sam = lazy->second;
                map<string, sampler3D>::iterator bound = boundTex3Ds.find(name);
                if (bound != boundTex3Ds.end()){ 
                    // sampler already bound, replace it's texture
                    bound->second.tex = sam.tex;
                }else{
                    // Set the samplers values and add it to the bound
                    // map.
                    sam.loc = GetUniLoc(shaderProgram, name.c_str());
                    sam.texUnit = nextTexUnit++;
                    glUniform1i(sam.loc, sam.texUnit);
                    boundTex3Ds[name] = sam;
                }
                lazy++;
            }
            unboundTex3Ds.clear();

            // Bind all the textures
            map<string, sampler2D>::iterator itr2 = boundTex2Ds.begin();
            while(itr2 != boundTex2Ds.end()){
                glActiveTexture(GL_TEXTURE0 + itr2->second.texUnit);
                glBindTexture(GL_TEXTURE_2D, itr2->second.tex->GetID());
                itr2++;
            }
            map<string, sampler3D>::iterator itr3 = boundTex3Ds.begin();
            while(itr3 != boundTex3Ds.end()){
                glActiveTexture(GL_TEXTURE0 + itr3->second.texUnit);
                glBindTexture(GL_TEXTURE_3D, itr3->second.tex->GetID());
                itr3++;
            }

            // reset the active texture
            glActiveTexture(GL_TEXTURE0);
        }

    }
}