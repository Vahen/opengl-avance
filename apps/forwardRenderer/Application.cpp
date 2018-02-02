#include "Application.hpp"

#include <iostream>

#include <imgui.h>
#include <glmlv/imgui_impl_glfw_gl3.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtc/random.hpp>
#include <glmlv/Image2DRGBA.hpp>

using namespace glmlv;
using namespace glm;
using namespace std;

int Application::run() {
    float clearColor[3] = {0, 0, 0};
    float diffuseColor[3] = {1, 1, 1};
    float pointLightposition[3] = {pointLight.position.x/255.f, pointLight.position.y/255.f, pointLight.position.z/255.f};
    float dirLightposition[3] = {dirLight.position.x/255.f, dirLight.position.y/255.f, dirLight.position.z/255.f};
    //float pointLightposition[3] = {pointLight.position.x/255.f, pointLight.position.y/255.f, pointLight.position.z/255.f};
    // Loop until the user closes the window
    m_program.use();
    for (auto iterationCount = 0u; !m_GLFWHandle.shouldClose(); ++iterationCount) {
        const auto seconds = glfwGetTime();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Put here rendering code
        //
        //
        //
        drawScene();
        

        // GUI code:
        ImGui_ImplGlfwGL3_NewFrame();

        {
            ImGui::Begin("GUI");
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                        ImGui::GetIO().Framerate);
            ImGui::SetColorEditOptions(ImGuiColorEditFlags_RGB);//ColorEditMode(ImGuiColorEditMode_RGB);
            if (ImGui::ColorEdit3("BackgroundColor", clearColor)) {
                glClearColor(clearColor[0], clearColor[1], clearColor[2], 1.f);
            }
            
            if (ImGui::ColorEdit3("pointLightPos", pointLightposition)) {
                pointLight.position = vec3(pointLightposition[0],pointLightposition[1],pointLightposition[2]);
            }

            if (ImGui::ColorEdit3("dirLightPos", dirLightposition)) {
                dirLight.position = vec3(dirLightposition[0],dirLightposition[1],dirLightposition[2]);
            }
            
            if (ImGui::ColorEdit3("diffuseColor", diffuseColor)) {
                coloruKd = vec3(diffuseColor[0],diffuseColor[1],diffuseColor[2]);
            }


            ImGui::End();
        }
        const auto viewportSize = m_GLFWHandle.framebufferSize();
        glViewport(0, 0, viewportSize.x, viewportSize.y);
        ImGui::Render();

        /* Poll for and process events */
        glfwPollEvents();

        /* Swap front and back buffers*/
        m_GLFWHandle.swapBuffers();

        auto ellapsedTime = glfwGetTime() - seconds;
        auto guiHasFocus = ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
        if (!guiHasFocus) {
            viewController.update(float(ellapsedTime));
        }
    }

    return 0;
}

Application::Application(int argc, char **argv) :
        m_AppPath{glmlv::fs::path{argv[0]}},
        m_AppName{m_AppPath.stem().string()},
        m_ImGuiIniFilename{m_AppName + ".imgui.ini"},
        m_ShadersRootPath{m_AppPath.parent_path() / "shaders"},
        m_AssetsRootPath{ m_AppPath.parent_path() / "assets" }
        {
    ImGui::GetIO().IniFilename = m_ImGuiIniFilename.c_str(); // At exit, ImGUI will store its windows positions in this file

    glEnable(GL_DEPTH_TEST);

    // Here we load and compile shaders from the library
    m_program = glmlv::compileProgram({m_ShadersRootPath / "forwardRenderer" / "forward.vs.glsl",
                                       m_ShadersRootPath / "forwardRenderer" / "forward.fs.glsl"});
    m_program.use();
    setUniformLocations();
    
    pointLight.position = vec3(1.f,10.f,1.f);
    pointLight.intensity = vec3(5.f,5.f,5.f);

    dirLight.position = vec3(1.f,1.f,1.f);
    dirLight.intensity = vec3(5.f,5.f,5.f);

    coloruKd = vec3(1.f,1.f,1.f);


    glGenTextures(2,textures);

    createSphere();
    createCube();

}

void Application::setUniformLocations(){
    uModelViewMatrix = glGetUniformLocation(m_program.glId(), "uModelViewMatrix");
    uModelViewProjMatrix = glGetUniformLocation(m_program.glId(), "uModelViewProjMatrix");
    uNormalMatrix = glGetUniformLocation(m_program.glId(), "uNormalMatrix");

    uPointLightPosition = glGetUniformLocation(m_program.glId(), "uPointLighPosition");
    uPointLightIntensity = glGetUniformLocation(m_program.glId(), "uPointLightIntensity");

    uDirectionalLightDir = glGetUniformLocation(m_program.glId(), "uDirectionalLightDir");
    uDirectionalLightIntensity = glGetUniformLocation(m_program.glId(), "uDirectionalLightIntensity");

    uKd = glGetUniformLocation(m_program.glId(), "uKd");

    uTexture = glGetUniformLocation(m_program.glId(), "uTexture");
}

Application::~Application() {
    destroySphere();
    destroyCube();

    ImGui_ImplGlfwGL3_Shutdown();
    glfwTerminate();
}

void Application::destroySphere() {
    if (m_sphereVBO) {
        glDeleteBuffers(1, &m_sphereVBO);
    }

    if (m_sphereIBO) {
        glDeleteBuffers(1, &m_sphereIBO);
    }

    if (m_sphereVAO) {
        glDeleteBuffers(1, &m_sphereVAO);
    }
}

void Application::destroyCube() {
    if (m_cubeVBO) {
        glDeleteBuffers(1, &m_cubeVBO);
    }

    if (m_cubeIBO) {
        glDeleteBuffers(1, &m_cubeIBO);
    }

    if (m_cubeVAO) {
        glDeleteBuffers(1, &m_cubeVAO);
    }
}

void Application::createSphere() {
    sphere = makeSphere(5);

    glGenBuffers(1, &m_sphereVBO);

    glBindBuffer(GL_ARRAY_BUFFER, m_sphereVBO);

    glBufferStorage(GL_ARRAY_BUFFER, sphere.vertexBuffer.size() * sizeof(glmlv::Vertex3f3f2f),
                    sphere.vertexBuffer.data(), 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &m_sphereIBO);

    glBindBuffer(GL_ARRAY_BUFFER, m_sphereIBO);

    glBufferStorage(GL_ARRAY_BUFFER, sphere.indexBuffer.size() * sizeof(sphere.indexBuffer[0]),
                    sphere.indexBuffer.data(), 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Here we use glGetAttribLocation(program, attribname) to obtain attrib locations; We could also directly use locations if they are set in the vertex shader (cf. triangle app)
    GLint positionAttrLocation = glGetAttribLocation(m_program.glId(), "aPosition");
    const GLint normalAttrLocation = glGetAttribLocation(m_program.glId(), "aNormal");
    // const GLint colorAttrLocation = glGetAttribLocation(m_program.glId(), "aColor");

    glGenVertexArrays(1, &m_sphereVAO);

    glBindVertexArray(m_sphereVAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_sphereVBO);

    glEnableVertexAttribArray(positionAttrLocation);
    glVertexAttribPointer(positionAttrLocation,
                          3,
                          GL_FLOAT, GL_FALSE,
                          sizeof(Vertex3f3f2f),
                          (const GLvoid *) offsetof(Vertex3f3f2f, position)
    );

    glEnableVertexAttribArray(normalAttrLocation);
    glVertexAttribPointer(normalAttrLocation,
                          3,
                          GL_FLOAT, GL_FALSE,
                          sizeof(Vertex3f3f2f),
                          (const GLvoid *) offsetof(Vertex3f3f2f, normal)
    );

    // glEnableVertexAttribArray(colorAttrLocation);
    // glVertexAttribPointer(colorAttrLocation, 
    //                         3,
    //                         GL_FLOAT, GL_FALSE, 
    //                         sizeof(Vertex3f3f2f), 
    //                         (const GLvoid*) offsetof(Vertex3f3f2f, color)
    //                     );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_sphereIBO);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);
}

void Application::createCube() {
    cube = makeCube();

    glGenBuffers(1, &m_cubeVBO);

    glBindBuffer(GL_ARRAY_BUFFER, m_cubeVBO);

    glBufferStorage(GL_ARRAY_BUFFER, cube.vertexBuffer.size() * sizeof(glmlv::Vertex3f3f2f), cube.vertexBuffer.data(),
                    0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &m_cubeIBO);

    glBindBuffer(GL_ARRAY_BUFFER, m_cubeIBO);

    glBufferStorage(GL_ARRAY_BUFFER, cube.indexBuffer.size() * sizeof(cube.indexBuffer[0]), cube.indexBuffer.data(), 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Here we use glGetAttribLocation(program, attribname) to obtain attrib locations; We could also directly use locations if they are set in the vertex shader (cf. triangle app)
    const GLint positionAttrLocation = glGetAttribLocation(m_program.glId(), "aPosition");
    const GLint normalAttrLocation = glGetAttribLocation(m_program.glId(), "aNormal");
    const GLint texCoordAttrLocation = glGetAttribLocation(m_program.glId(), "aTexCoords");
    
    auto textureImg = readImage(m_AssetsRootPath / m_AppName / "textures" / "opengl-logo.png");
    glUniform1i(uTexture,ind_texture_cube);

    cout << textures[ind_texture_cube] << endl;
    glBindTexture(GL_TEXTURE_2D, textures[ind_texture_cube]);// Bind sur GL_TEXTURE_2D
    // Met les info de l'image dans GL_TEXTURE_2D
    cout << "deb" << endl;
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, textureImg.width(), textureImg.height());
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, textureImg.width(), textureImg.height(), GL_RGBA, GL_UNSIGNED_BYTE, textureImg.data());
 
    cout << "fin" << endl;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glActiveTexture(GL_TEXTURE0 + ind_texture_cube);
    glBindTexture(GL_TEXTURE_2D, textures[ind_texture_cube]);

    glGenVertexArrays(1, &m_cubeVAO);

    glBindVertexArray(m_cubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_cubeVBO);

    glEnableVertexAttribArray(positionAttrLocation);
    glVertexAttribPointer(positionAttrLocation, 3,
                          GL_FLOAT, GL_FALSE,
                          sizeof(Vertex3f3f2f),
                          (const GLvoid *) offsetof(Vertex3f3f2f, position)
    );

    glEnableVertexAttribArray(normalAttrLocation);
    glVertexAttribPointer(normalAttrLocation, 3,
                          GL_FLOAT, GL_FALSE,
                          sizeof(Vertex3f3f2f),
                          (const GLvoid *) offsetof(Vertex3f3f2f, normal)
    );

    glEnableVertexAttribArray(texCoordAttrLocation);
    glVertexAttribPointer(texCoordAttrLocation, 2,
                          GL_FLOAT, GL_FALSE,
                          sizeof(Vertex3f3f2f),
                          (const GLvoid *) offsetof(Vertex3f3f2f, texCoords)
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_cubeIBO);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);
}

void Application::drawCube() {
    glm::mat4 projMatrix = glm::perspective(glm::radians(70.f), (float) (m_nWindowWidth) / m_nWindowHeight, 0.1f,
                                            100.f);
    glm::mat4 viewMatrix = viewController.getViewMatrix();
    glm::mat4 cubeMVMatrix = glm::translate(viewMatrix,vec3(0.f,1.f,0.f));

    glBindVertexArray(m_cubeVAO);

    glUniformMatrix4fv(uModelViewMatrix, 1, GL_FALSE, glm::value_ptr(cubeMVMatrix));
    glUniformMatrix4fv(uNormalMatrix, 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(cubeMVMatrix))));
    glUniformMatrix4fv(uModelViewProjMatrix, 1, GL_FALSE, glm::value_ptr(projMatrix * cubeMVMatrix));

    glUniform1i(uTexture, ind_texture_cube);
    glActiveTexture(GL_TEXTURE0 + ind_texture_cube);
    glBindTexture(GL_TEXTURE_2D, textures[ind_texture_cube]);

    glDrawElements(GL_TRIANGLES, cube.indexBuffer.size(), GL_UNSIGNED_INT, nullptr);

    glActiveTexture(GL_TEXTURE0 + ind_texture_cube);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindVertexArray(0);
}

void Application::drawSphere() {
    glm::mat4 projMatrix = glm::perspective(glm::radians(70.f), (float) (m_nWindowWidth) / m_nWindowHeight, 0.1f,
                                            100.f);
    //glm::mat4 viewMatrix = glm::translate(glm::mat4(1.f), glm::vec3(0.0f, 0.0f, -5.f));
    
    glm::mat4 viewMatrix = viewController.getViewMatrix();
    glm::mat4 sphereMVMatrix = viewMatrix;
    glBindVertexArray(m_sphereVAO); // bind
    glUniformMatrix4fv(uModelViewMatrix, 1, GL_FALSE, glm::value_ptr(sphereMVMatrix));
    glUniformMatrix4fv(uNormalMatrix, 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(sphereMVMatrix))));
    glUniformMatrix4fv(uModelViewProjMatrix, 1, GL_FALSE, glm::value_ptr(projMatrix * sphereMVMatrix));
    glDrawElements(GL_TRIANGLES, sphere.indexBuffer.size(), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void Application::drawScene(){
    
    vec3 pointLightPos_vs = vec3(viewController.getViewMatrix() * vec4(pointLight.position,1));
    glUniform3f(uPointLightPosition,pointLightPos_vs.x,pointLightPos_vs.y,pointLightPos_vs.z);
    
    vec3 pointLightInt_vs = vec3(viewController.getViewMatrix() * vec4(pointLight.intensity,1));
    glUniform3f(uPointLightIntensity,pointLightInt_vs.x,pointLightInt_vs.y,pointLightInt_vs.z);
    
    vec3 dirLightPos_vs = vec3(viewController.getViewMatrix() * vec4(dirLight.position,1)); 
    glUniform3f(uDirectionalLightDir,dirLightPos_vs.x,dirLightPos_vs.y,dirLightPos_vs.z);
    
    vec3 dirLightInt_vs = vec3(viewController.getViewMatrix() * vec4(dirLight.intensity,1));
    glUniform3f(uDirectionalLightIntensity,dirLightInt_vs.x,dirLightInt_vs.y,dirLightInt_vs.z);
    
    glUniform3f(uKd,coloruKd.x,coloruKd.y,coloruKd.z);
    
    drawCube();
    drawSphere();
}