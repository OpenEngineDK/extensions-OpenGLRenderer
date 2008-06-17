// OpenGL renderer implementation.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

#ifndef _RENDERER_H_
#define _RENDERER_H_

#include <Renderers/IRenderer.h>
#include <Renderers/IRenderingView.h>
#include <Scene/ISceneNode.h>
#include <Math/Matrix.h>
#include <Geometry/Face.h>
#include <vector>


namespace OpenEngine {

    //forward declarations
    namespace Scene {
        class TransformationNode;
        class PointLightNode;
        class DirectionalLightNode;
        class SpotLightNode;
    }

namespace Renderers {

/**
 * OpenGL Rendering implementation.
 *
 * @namespace OpenEngine::Renderers::OpenGL
 */
namespace OpenGL {

/**
 * OpenGL Shader Language versions
 */
enum GLSLVersion { GLSL_UNKNOWN, GLSL_NONE, GLSL_14, GLSL_20 };

using namespace std;
using namespace OpenEngine::Scene;
    //using OpenEngine::Scene::ISceneNode;
using OpenEngine::Math::Matrix;
using OpenEngine::Geometry::FacePtr;

class TextureLoader;

/**
 * Renderer using OpenGL
 *
 * @class Renderer Renderer.h Renderers/OpenGL/IRenderer.h
 */
class Renderer : public IRenderer {
private:
    class LightVisitor: public ISceneNodeVisitor {
    private:
        float pos[4], dir[4];
    public:
        int count;

        LightVisitor(); 
        ~LightVisitor();
        
        void VisitTransformationNode(TransformationNode* node);

        void VisitDirectionalLightNode(DirectionalLightNode* node);

        void VisitPointLightNode(PointLightNode* node);

        void VisitSpotLightNode(SpotLightNode* node);

    };

    LightVisitor lv;

    static GLSLVersion glslversion;

    void InitializeGLSLVersion();

    float farPlane;

public:
    Renderer();
    ~Renderer();

    void Initialize();
    void Process(const float deltaTime, const float percent);
    void Deinitialize();
    bool IsTypeOf(const std::type_info& inf);

    /**
     * Test if OpenGL Shader Language is supported.
     *
     * @return True if support is found.
     */
    static bool IsGLSLSupported();

    /**
     * Get the supported version of OpenGL Shader Language.
     *
     * @return Version
     */
    static GLSLVersion GetGLSLVersion();

    virtual void DrawFace(FacePtr face, Vector<3,float> color, float width = 1);
    virtual void DrawLine(Line line, Vector<3,float> color, float width = 1);
    virtual void DrawPoint(Vector<3,float> point, Vector<3,float> color , float size);

    virtual void SetFarPlane(float farPlane);
    virtual float GetFarPlane();
};

} // NS OpenGL
} // NS OpenEngine
} // NS Renderers

#endif
