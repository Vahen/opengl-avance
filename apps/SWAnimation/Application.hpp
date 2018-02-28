#pragma once

#include <glmlv/filesystem.hpp>
#include <glmlv/GLFWHandle.hpp>
#include <glmlv/GLProgram.hpp>
#include <glmlv/ViewController.hpp>
#include <glmlv/simple_geometry.hpp>
#include <glmlv/load_obj.hpp>
#include <glm/glm.hpp>

#include <limits>
#include <chrono>


using namespace glmlv;
using namespace glm;

// GBuffer:
enum GBufferTextureType {
    GPosition = 0,
    GNormal,
    GAmbient,
    GDiffuse,
    GGlossyShininess,
    GDepth,
    GBufferTextureCount
};

enum DisplayType {
    Display_Beauty,
    Display_GPosition,
    Display_GNormal,
    Display_GAmbient,
    Display_GDiffuse,
    Display_GGlossyShininess,
    Display_GDepth,
    Display_DirectionalLightDepthMap,
    Display_Count
};

class Application {
public:
    Application(int argc, char **argv);

    int run();

private:
    void initScene();

    void initShadersData();

    void initGBuffer();

    void initScreenTriangle();

    void initShadowData();

    static glm::vec3 computeDirectionVector(float phiRadians, float thetaRadians) {
        const auto cosPhi = glm::cos(phiRadians);
        const auto sinPhi = glm::sin(phiRadians);
        const auto sinTheta = glm::sin(thetaRadians);
        return glm::vec3(sinPhi * sinTheta, glm::cos(thetaRadians), cosPhi * sinTheta);
    }

    static glm::vec3 computeDirectionVectorUp(float phiRadians, float thetaRadians) {
        const auto cosPhi = glm::cos(phiRadians);
        const auto sinPhi = glm::sin(phiRadians);
        const auto cosTheta = glm::cos(thetaRadians);
        return -glm::normalize(glm::vec3(sinPhi * cosTheta, -glm::sin(thetaRadians), cosPhi * cosTheta));
    }

    const int m_nWindowWidth = 1280;
    const int m_nWindowHeight = 720;
    glmlv::GLFWHandle m_GLFWHandle{m_nWindowWidth, m_nWindowHeight,
                                   "SW Animation"}; // Note: the handle must be declared before the creation of any object managing OpenGL resource (e.g. GLProgram, GLShader)

    const glmlv::fs::path m_AppPath;
    const std::string m_AppName;
    const std::string m_ImGuiIniFilename;
    const glmlv::fs::path m_ShadersRootPath;
    const glmlv::fs::path m_AssetsRootPath;

    const GLenum m_GBufferTextureFormat[GBufferTextureCount] = {GL_RGB32F, GL_RGB32F, GL_RGB32F, GL_RGB32F, GL_RGBA32F,
                                                                GL_DEPTH_COMPONENT32F};
    GLuint m_GBufferTextures[GBufferTextureCount];
    GLuint m_GBufferFBO; // Framebuffer object

    const char *m_DisplayNames[Display_Count] = {"beauty", "position", "normal", "ambient", "diffuse",
                                                 "glossyShininess", "depth", "directionalLightDepth"};
    int32_t m_CurrentlyDisplayed = Display_Beauty;

    // Triangle covering the whole screen, for the shading pass:
    GLuint m_TriangleVBO = 0;
    GLuint m_TriangleVAO = 0;

    // Scene data in GPU:
    GLuint m_SceneVBO = 0;
    GLuint m_SceneIBO = 0;
    GLuint m_SceneVAO = 0;

    // Required data about the scene in CPU in order to send draw calls
    ObjData m_data;
    std::vector<GLuint> m_textureIds;
    std::vector<int> m_tabIndexShape; // tableau d'int pour indiquer le nombre de shape par objet
    // exemple : tabIndexShape[0] == 5 indique qu'il y a 5 shapeCount dans le 1er objet
    // -> dans la boucle de rendu faire if(i
    ObjData::PhongMaterial m_DefaultMaterial;

    ///////////////////////////////////////////
    // vecteur des translation des objets
    glm::vec3 m_coordBigShip = vec3();
    glm::vec3 m_RotationBigShip = vec3();

    glm::vec3 m_coordAWing1 = vec3();
    glm::vec3 m_RotationAWing1 = vec3();

    glm::vec3 m_coordAWing2 = vec3();
    glm::vec3 m_RotationAWing2 = vec3();

    glm::vec3 m_coordAWing3 = vec3();
    glm::vec3 m_RotationAWing3 = vec3();

    glm::vec3 rotateOnX = vec3(1,0,0);
    glm::vec3 rotateOnY = vec3(0,1,0);
    glm::vec3 rotateOnZ = vec3(0,0,1);

    float m_RotationSpeed = 0.01f;
    float m_speed = 0.01f;
    ///////////////////////////////////////////

    glm::vec3 m_SceneSize = glm::vec3(0.f); // Used for camera speed and projection matrix parameters
    float m_SceneSizeLength = 0.f;
    glm::vec3 m_SceneCenter = glm::vec3(0.f);

    GLuint m_WhiteTexture; // A white 1x1 texture

    GLuint m_textureSampler = 0; // Only one sampler object since we will use the same sampling parameters for all textures

    // Camera
    glmlv::ViewController m_viewController{m_GLFWHandle.window(), 3.f};

    // GLSL programs
    glmlv::GLProgram m_geometryPassProgram;
    glmlv::GLProgram m_shadingPassProgram;
    glmlv::GLProgram m_displayDepthProgram;
    glmlv::GLProgram m_displayPositionProgram;

    // Geometry pass uniforms
    GLint m_uModelViewProjMatrixLocation;
    GLint m_uModelViewMatrixLocation;
    GLint m_uNormalMatrixLocation;
    GLint m_uKaLocation;
    GLint m_uKdLocation;
    GLint m_uKsLocation;
    GLint m_uShininessLocation;
    GLint m_uKaSamplerLocation;
    GLint m_uKdSamplerLocation;
    GLint m_uKsSamplerLocation;
    GLint m_uShininessSamplerLocation;

    // Shading pass uniforms
    GLint m_uGBufferSamplerLocations[GDepth];
    GLint m_uDirectionalLightDirLocation;
    GLint m_uDirectionalLightIntensityLocation;
    GLint m_uPointLightPositionLocation;
    GLint m_uPointLightIntensityLocation;
    GLint m_uDirLightViewProjMatrix_shadingPass; // Suffix because the variable m_uDirLightViewProjMatrix is already used for the uniform of m_directionalSMProgram.
    GLint m_uDirLightShadowMap;
    GLint m_uDirLightShadowMapBias;
    GLint m_uDirLightShadowMapSampleCount;
    GLint m_uDirLightShadowMapSpread;

    // Display depth pass uniforms
    GLint m_uGDepthSamplerLocation;

    // Display position pass uniforms
    GLint m_uGPositionSamplerLocation;
    GLint m_uSceneSizeLocation;

    // Lights
    float m_DirLightPhiAngleDegrees = 100.f;
    float m_DirLightThetaAngleDegrees = 30.f;
    glm::vec3 m_DirLightDirection = computeDirectionVector(glm::radians(m_DirLightPhiAngleDegrees),
                                                           glm::radians(m_DirLightThetaAngleDegrees));
    glm::vec3 m_DirLightColor = glm::vec3(1, 1, 1);
    float m_DirLightIntensity = 2.f;
    float m_DirLightSMBias = 0.01f;
    int m_DirLightSMSampleCount = 16;
    float m_DirLightSMSpread = 0.0005;

    glm::vec3 m_PointLightPosition = glm::vec3(0, 1, 0);
    glm::vec3 m_PointLightColor = glm::vec3(1, 1, 1);
    float m_PointLightIntensity = 1.f;

    // Shadow mapping data
    glmlv::GLProgram m_directionalSMProgram;
    GLint m_uDirLightViewProjMatrix;

    GLuint m_directionalSMSampler;

    GLuint m_directionalSMFBO;
    GLuint m_directionalSMTexture;
    int32_t m_nDirectionalSMResolution = 4096;


    bool m_directionalSMResolutionDirty = false;
    bool m_directionalSMDirty = true;

    void geometryPass(const glm::mat4 &projMatrix, const glm::mat4 &viewMatrix);

    void shadingPass(const glm::mat4 &viewMatrix, const glm::mat4 &rcpViewMatrix,
                     const glm::mat4 &dirLightViewMatrix, const glm::mat4 &dirLightProjMatrix) const;

    void displayDepth() const;

    void displayPosition(const glm::mat4 &projMatrix) const;

    void displayDirectionalLightDepthMap() const;

    void initGeometryPassData();

    void initShadingPassData();

    void initDisplayDepthData();

    void initDisplayPositionData();

    void initDirectionnalSMData();

    void generateAndBindAllTexture();

    void createWhiteTexture();

    void bindDataOnVAO();

    void fillSceneVBO() const;

    void fillSceneIBO() const;

    void setMaterial(const ObjData::PhongMaterial &material) const;

    void computShadowMap(const mat4 &dirLightViewMatrix, const mat4 &dirLightProjMatrix) ;

    void cleanShadowMap();

    void renderScene(const mat4 &projMatrix,
                     const mat4 &viewMatrix,
                     const mat4 &rcpViewMatrix,
                     const mat4 &dirLightViewMatrix,
                     const mat4 &dirLightProjMatrix) const;

    void GUIDisplay(float *clearColor);

    void loadObjAndPushIndexShape(const fs::path &objPath);

    void initDefaultMaterial();

    void sendMatrixInformation(const mat4 &mvMatrix, const mat4 &mvpMatrix, const mat4 &normalMatrix) const;

    std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();

    // Fonction pour les differentes partie de l'animations //

    mat4 &applyTransformBigShip(mat4 &mvMatrix);

    mat4 &applyTransformAWing2(mat4 &mvMatrix);

    mat4 &applyTransformAWing3(mat4 &mvMatrix);

    mat4 &applyTransformAWing1(mat4 &mvMatrix);

    void sendLightProjInfo(const mat4 &dirLightViewMatrix, const mat4 &dirLightProjMatrix, const mat4 &transformMatrix) const;

    void updateShipMovements();
};