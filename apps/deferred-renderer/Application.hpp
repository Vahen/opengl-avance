#pragma once

#include <glmlv/filesystem.hpp>
#include <glmlv/GLFWHandle.hpp>
#include <glmlv/GLProgram.hpp>
#include <glmlv/simple_geometry.hpp>
#include <glmlv/ViewController.hpp>
#include <glmlv/load_obj.hpp>

using namespace glmlv;
using namespace glm;

struct Light {
    vec3 position;
    vec3 intensity;
};

enum GBufferTextureType{
    GPosition = 0,
    GNormal,
    GAmbient,
    GDiffuse,
    GGlossyShininess,
    GDepth, // On doit créer une texture de depth mais on écrit pas directement dedans dans le FS. OpenGL le fait pour nous (et l'utilise).
    GBufferTextureCount
};

class Application {
public:
    Application(int argc, char **argv);

    ~Application();

    int run();

private:
    const int m_nWindowWidth = 1280;
    const int m_nWindowHeight = 720;
    GLFWHandle m_GLFWHandle{m_nWindowWidth, m_nWindowHeight,
                            "Template"}; // Note: the handle must be declared before the creation of any object managing OpenGL resource (e.g. GLProgram, GLShader)

    const fs::path m_AppPath;
    const std::string m_AppName;
    const std::string m_ImGuiIniFilename;
    const fs::path m_ShadersRootPath;
    const fs::path m_AssetsRootPath;

    GLProgram m_programGeometryPass;
    GLProgram m_programShadingPass;

    GLint uModelViewMatrixLocation;
    GLint uModelViewProjMatrixLocation;
    GLint uNormalMatrixLocation;

    GLint uPointLightPositionLocation;
    GLint uPointLightIntensityLocation;

    GLint uDirectionalLightDirLocation;
    GLint uDirectionalLightIntensityLocation;

    Light pointLight;
    Light dirLight;

    ViewController viewController{m_GLFWHandle.window()};
    ///////////////////////////////////////////////////

    GLuint m_SceneVBO = 0;
    GLuint m_SceneIBO = 0;
    GLuint m_SceneVAO = 0;

    GLuint m_texture;
    ObjData m_data;
    std::vector<GLuint> textureIds;

    glm::mat4 projMatrix;

    float m_SceneSize = 0.f;

    GLint uKaLocation;
    GLint uKdLocation;
    GLint uKsLocation;
    GLint uShininessLocation;

    GLint uKaTextureLocation;
    GLint uKdTextureLocation;
    GLint uKsTextureLocation;
    GLint uSnTextureLocation;

    ///////////////////////////////////////////////

    GLuint m_GBufferTextures[GBufferTextureCount];
    const char * m_GBufferTexNames[GBufferTextureCount] = { "position", "normal", "ambient", "diffuse", "glossyShininess", "depth" };

    const GLenum m_GBufferTextureFormat[GBufferTextureCount] = {GL_RGB32F, GL_RGB32F, GL_RGB32F, GL_RGB32F, GL_RGBA32F,
                                                                GL_DEPTH_COMPONENT32F};
    GLuint m_FBO;

    GLint uGPositionLocation;
    GLint uGNormalLocation;
    GLint uGAmbientLocation;
    GLint uGDiffuseLocation;
    GLint uGlossyShininessLocation;

    GLuint m_triangleVBO;
    GLuint m_triangleIBO;
    GLuint m_triangleVAO;
    ///////////////////////////////////////////////

    void setUniformLocationsGeometry();
    void setUniformLocationsShading();

    void drawScene();

    void initScene();
    void initTriangle();

    void setMaterial(const ObjData::PhongMaterial &material) const;

    void sendLights(const mat4 &viewMatrix) const;

    void destroyScene();

    void initLights();

    void fillSceneVBO() const;

    void fillSceneIBO() const;

    void createWhiteTexture() ;

    void generateAndBindAllTexture();

    void bindDataOnVAO() ;
};