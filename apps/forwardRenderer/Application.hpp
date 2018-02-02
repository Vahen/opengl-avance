#pragma once

#include <glmlv/filesystem.hpp>
#include <glmlv/GLFWHandle.hpp>
#include <glmlv/GLProgram.hpp>
#include <glmlv/simple_geometry.hpp>
#include <glmlv/ViewController.hpp>

using namespace glmlv;

class Application
{
public:
    Application(int argc, char** argv);
    ~Application();
    int run();
private:
    const size_t m_nWindowWidth = 1280;
    const size_t m_nWindowHeight = 720;
    glmlv::GLFWHandle m_GLFWHandle{ m_nWindowWidth, m_nWindowHeight, "Template" }; // Note: the handle must be declared before the creation of any object managing OpenGL resource (e.g. GLProgram, GLShader)

    const glmlv::fs::path m_AppPath;
    const std::string m_AppName;
    const std::string m_ImGuiIniFilename;
    const glmlv::fs::path m_ShadersRootPath;

    GLuint m_sphereVBO = 0;
    GLuint m_sphereIBO = 0;
    GLuint m_sphereVAO = 0;

    GLuint m_cubeVBO = 0;
    GLuint m_cubeIBO = 0;
    GLuint m_cubeVAO = 0;

    SimpleGeometry sphere;
    SimpleGeometry cube;

    glmlv::GLProgram m_program;

    GLint uModelViewMatrix;
    GLint uModelViewProjMatrix;
    GLint uNormalMatrix;

    ViewController viewController{m_GLFWHandle.window()};

    void createCube();
    void createSphere();
    void destroyCube();
    void destroySphere();

    void drawSphere();
    void drawCube();
};