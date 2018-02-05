#pragma once

#include <glmlv/filesystem.hpp>
#include <glmlv/GLFWHandle.hpp>
#include <glmlv/GLProgram.hpp>
#include <glmlv/simple_geometry.hpp>
#include <glmlv/ViewController.hpp>
#include <glmlv/load_obj.hpp>

using namespace glmlv;
using namespace glm;

struct Light{
    vec3 position;
    vec3 intensity;    
};


class Application{
    public:
        Application(int argc, char** argv);
        ~Application();
        int run();
    private:
        const int m_nWindowWidth = 1280;
        const int m_nWindowHeight = 720;
        GLFWHandle m_GLFWHandle{ m_nWindowWidth, m_nWindowHeight, "Template" }; // Note: the handle must be declared before the creation of any object managing OpenGL resource (e.g. GLProgram, GLShader)

        const fs::path m_AppPath;
        const std::string m_AppName;
        const std::string m_ImGuiIniFilename;
        const fs::path m_ShadersRootPath;
        const fs::path m_AssetsRootPath;

        GLuint m_sphereVBO = 0;
        GLuint m_sphereIBO = 0;
        GLuint m_sphereVAO = 0;

        GLuint m_cubeVBO = 0;
        GLuint m_cubeIBO = 0;
        GLuint m_cubeVAO = 0;

        SimpleGeometry sphere;
        SimpleGeometry cube;

        GLProgram m_program;

        GLuint uModelViewMatrix;
        GLuint uModelViewProjMatrix;
        GLuint uNormalMatrix;

        GLuint uPointLightPosition;
        GLuint uPointLightIntensity;

        GLuint uDirectionalLightDir;
        GLuint uDirectionalLightIntensity;

        GLuint uKd;
        
        Light pointLight;
        Light dirLight;

        vec3 coloruKd;


        GLuint textures[2];
        
        int ind_texture_cube = 0;

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

        //GLuint m_textureSampler = 0;

        GLint uKaLocation;
        GLint uKdLocation;
        GLint uKsLocation;
        GLint uShininessLocation;

        GLint uKaTexture;
        GLint uKdTexture;
        GLint uKsTexture;
        GLint uSnTexture;

        ///////////////////////////////////////////////
        void setUniformLocations();
        void createCube();
        void createSphere();
        void destroyCube();
        void destroySphere();

        void drawSphere();
        void drawCube();
        void drawScene();

        void initScene();
};