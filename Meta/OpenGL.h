// Meta header for OpenGL components.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#ifndef _OPENENGINE_OPENGL_H_
#define _OPENENGINE_OPENGL_H_

#include <Core/Exceptions.h>
#include <Utils/Convert.h>
#include <string>

using OpenEngine::Core::Exception;
using OpenEngine::Utils::Convert;
using std::string;

#ifdef _WIN32
#ifndef GLEW_STATIC
#define GLEW_STATIC
#endif // GLEW_STATIC
#endif

#include <GL/glew.h>

#if defined __APPLE__
    #include <OpenGL/gl.h>
    #include <OpenGL/glu.h>
#else
  #ifdef _WIN32
    #include <Windows.h>
  #endif
  #include <GL/gl.h>
  #include <GL/glu.h>
#endif

#ifndef GL_RGB32F
#define GL_RGB32F 34837
#endif
#ifndef GL_RGBA32F
#define GL_RGBA32F 34838
#endif
#ifndef GL_R32F
#define GL_R32F 0x822E
#endif


/**
 *  Should never be used in the code, use CHECK_FOR_GL_ERROR(); instead
 */
inline void CHECK_FOR_GL_ERROR(const std::string file, const int line) {
    GLenum errorCode = glGetError();
    if (errorCode != GL_NO_ERROR) {
        const GLubyte* errorString = gluErrorString(errorCode);
        throw Exception("[file:" + file +
                        " line:" + Convert::ToString(line) +
                        "] OpenGL Error: " +
                        std::string((const char*)errorString));
    }
}

/**
 *  Should never be used in the code, use CHECK_FOR_GL_ERROR(); instead
 */
inline void CHECK_FRAMEBUFFER_STATUS(const std::string file, const int line) {
    GLenum errorCode = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    if (errorCode != GL_FRAMEBUFFER_COMPLETE_EXT) { 
        string s;
        switch (errorCode) {
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
            s = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
            s = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT";
            break;
            // case GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT:
            //     s = "GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT";
            //     break; 
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
            s = "GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
            s = "GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
            s = "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
            s = "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT";
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
            s = "GL_FRAMEBUFFER_UNSUPPORTED_EXT";
            break;
        default:
            s = "Unknown";
        }
        throw Exception("[file:" + file +
                        " line:" + Convert::ToString(line) +
                        "] OpenGL Framebuffer Error: " + s
                        );
    }
}


/**
 *  Checks for Open GL errors and throws an exception if
 *  an error was detected, is only available in debug mode.
 *  NOTE: This call must not be used between calls to glBegin and
 *  glEnd. 
 */
#if OE_DEBUG_GL
#define CHECK_FOR_GL_ERROR(); CHECK_FOR_GL_ERROR(__FILE__,__LINE__);
#define CHECK_FRAMEBUFFER_STATUS(); CHECK_FRAMEBUFFER_STATUS(__FILE__,__LINE__);
#else
#define CHECK_FOR_GL_ERROR();
#define CHECK_FRAMEBUFFER_STATUS();
#endif

#endif // _OPENENGINE_OPENGL_H_
