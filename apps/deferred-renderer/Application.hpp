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

    GLProgram m_program;

    GLuint uModelViewMatrixLocation;
    GLuint uModelViewProjMatrixLocation;
    GLuint uNormalMatrixLocation;

    GLuint uPointLightPositionLocation;
    GLuint uPointLightIntensityLocation;

    GLuint uDirectionalLightDirLocation;
    GLuint uDirectionalLightIntensityLocation;

    Light pointLight;
    Light dirLight;

    ViewController viewController{m_GLFWHandle.window()};
    ///////////////////////////////////////////////////

    GLuint m_SceneVBO = 0;
    GLuint m_SceneIBO = 0;
    GLuint m_SceneVAO = 0;

    GLuint m_texture;
    ObjData data;
    std::vector<GLuint> textureIds;

    glm::mat4 projMatrix;

    float m_SceneSize = 0.f;

    GLint uKaLocation;
    GLint uKdLocation;
    GLint uKsLocation;
    GLint uShininessLocation;

    GLint uKaTexture;
    GLint uKdTexture;
    GLint uKsTexture;
    GLint uSnTexture;

    ///////////////////////////////////////////////

    GLuint m_GBufferTextures[GBufferTextureCount];
    ///////////////////////////////////////////////

    void setUniformLocations();

    void drawScene();

    void initScene();

    void setMaterial(const ObjData::PhongMaterial &material) const;

    void drawLights(const mat4 &viewMatrix) const;

    void destroyScene();

    void initLights();

    void fillSceneVBO() const;

    void fillSceneIBO() const;

    void createWhiteTexture() const;

    void generateAndBindAllTexture();

    void bindDataOnVAO() const;
};