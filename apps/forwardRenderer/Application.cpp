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
    float pointLightposition[3] = {pointLight.position.x, pointLight.position.y, pointLight.position.z};
    float dirLightposition[3] = {dirLight.position.x, dirLight.position.y, dirLight.position.z};
    //float pointLightposition[3] = {pointLight.position.x/255.f, pointLight.position.y/255.f, pointLight.position.z/255.f};
    // Loop until the user closes the window
    m_program.use();
    for (auto iterationCount = 0u; !m_GLFWHandle.shouldClose(); ++iterationCount) {
        const auto seconds = glfwGetTime();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Put here rendering code
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

            if (ImGui::InputFloat3("pointLightPos", pointLightposition)) {
                pointLight.position = vec3(pointLightposition[0], pointLightposition[1], pointLightposition[2]);
            }

            if (ImGui::InputFloat3("dirLightPos", dirLightposition)) {
                dirLight.position = vec3(dirLightposition[0], dirLightposition[1], dirLightposition[2]);
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
        m_AssetsRootPath{m_AppPath.parent_path() / "assets"} {
    ImGui::GetIO().IniFilename = m_ImGuiIniFilename.c_str(); // At exit, ImGUI will store its windows positions in this file

    glEnable(GL_DEPTH_TEST);

    // Here we load and compile shaders from the library
    m_program = glmlv::compileProgram({m_ShadersRootPath / "forwardRenderer" / "forward.vs.glsl",
                                       m_ShadersRootPath / "forwardRenderer" / "forward.fs.glsl"});
    m_program.use();

    setUniformLocations();

    pointLight.position = vec3(0.f, 1.f, 0.f);
    pointLight.intensity = vec3(1E4, 0, 1E4);

    dirLight.position = vec3(1.f, 1.f, 1.f);
    dirLight.intensity = vec3(1.f, 1.f, 1.f);

    coloruKd = vec3(1.f, 1.f, 1.f);

    initScene();

}

void Application::initScene() {

    //createSphere();
    //createCube();

    glGenBuffers(1, &m_SceneVBO);
    glGenBuffers(1, &m_SceneIBO);

    glmlv::loadObj({m_AssetsRootPath / "glmlv" / "models" / "crytek-sponza" / "sponza.obj"}, data);

    // Fill VBO
    glBindBuffer(GL_ARRAY_BUFFER, m_SceneVBO);
    glBufferStorage(GL_ARRAY_BUFFER, data.vertexBuffer.size() * sizeof(Vertex3f3f2f), data.vertexBuffer.data(), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Fill IBO
    glBindBuffer(GL_ARRAY_BUFFER, m_SceneIBO);
    glBufferStorage(GL_ARRAY_BUFFER, data.indexBuffer.size() * sizeof(uint32_t), data.indexBuffer.data(), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);


    //Creation texture blanche de base
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, 1, 1);
    vec4 white(1.f, 1.f, 1.f, 1.f);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1, 1, GL_RGBA, GL_FLOAT, &white);
    glBindTexture(GL_TEXTURE_2D, 0);

    textureIds.resize(data.textures.size());
    glGenTextures(data.textures.size(), textureIds.data());
    for (auto i = 0; i < textureIds.size(); i++) {
        glBindTexture(GL_TEXTURE_2D, textureIds[i]);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, data.textures[i].width(), data.textures[i].height());
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, data.textures[i].width(), data.textures[i].height(), GL_RGBA,
                        GL_UNSIGNED_BYTE, data.textures[i].data());
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenVertexArrays(1, &m_SceneVAO);
    glBindVertexArray(m_SceneVAO);

    const GLint positionAttrLocation = 0;
    const GLint normalAttrLocation = 1;
    const GLint texCoordsAttrLocation = 2;

    glEnableVertexAttribArray(positionAttrLocation);
    glEnableVertexAttribArray(normalAttrLocation);
    glEnableVertexAttribArray(texCoordsAttrLocation);

    glBindBuffer(GL_ARRAY_BUFFER, m_SceneVBO);

    glVertexAttribPointer(positionAttrLocation, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3f3f2f),
                          (const GLvoid *) offsetof(Vertex3f3f2f, position));
    glVertexAttribPointer(normalAttrLocation, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3f3f2f),
                          (const GLvoid *) offsetof(Vertex3f3f2f, normal));
    glVertexAttribPointer(texCoordsAttrLocation, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3f3f2f),
                          (const GLvoid *) offsetof(Vertex3f3f2f, texCoords));

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_SceneIBO);

    glBindVertexArray(0);

    m_SceneSize = glm::length(data.bboxMax - data.bboxMin);
    viewController.setSpeed(m_SceneSize * 0.1f);

    const auto viewportSize = m_GLFWHandle.framebufferSize();
    projMatrix = glm::perspective(70.f, float(viewportSize.x) / viewportSize.y, 0.01f * m_SceneSize,
                                  m_SceneSize); // near = 1% de la taille de la scene, far = 100%

}

void Application::setUniformLocations() {
    uModelViewMatrix = glGetUniformLocation(m_program.glId(), "uModelViewMatrix");
    uModelViewProjMatrix = glGetUniformLocation(m_program.glId(), "uModelViewProjMatrix");
    uNormalMatrix = glGetUniformLocation(m_program.glId(), "uNormalMatrix");

    uPointLightPosition = glGetUniformLocation(m_program.glId(), "uPointLighPosition");
    uPointLightIntensity = glGetUniformLocation(m_program.glId(), "uPointLightIntensity");

    uDirectionalLightDir = glGetUniformLocation(m_program.glId(), "uDirectionalLightDir");
    uDirectionalLightIntensity = glGetUniformLocation(m_program.glId(), "uDirectionalLightIntensity");

    //uKd = glGetUniformLocation(m_program.glId(), "uKd");

    uKaTexture = glGetUniformLocation(m_program.glId(), "uKaTexture");
    uKdTexture = glGetUniformLocation(m_program.glId(), "uKdTexture");
    uKsTexture = glGetUniformLocation(m_program.glId(), "uKsTexture");
    uSnTexture = glGetUniformLocation(m_program.glId(), "uSnTexture");

    uKaLocation = glGetUniformLocation(m_program.glId(), "uKa");
    uKdLocation = glGetUniformLocation(m_program.glId(), "uKd");
    uKsLocation = glGetUniformLocation(m_program.glId(), "uKs");

    uShininessLocation = glGetUniformLocation(m_program.glId(), "uShininess");
}

void Application::drawScene() {
    const auto viewMatrix = viewController.getViewMatrix();
    // todo
    drawLights(viewMatrix);

    const auto modelMatrix = glm::mat4(1);

    const auto mvMatrix = viewMatrix * modelMatrix;
    const auto mvpMatrix = projMatrix * mvMatrix;
    const auto normalMatrix = glm::transpose(glm::inverse(mvMatrix));

    glUniformMatrix4fv(uModelViewProjMatrix, 1, GL_FALSE, glm::value_ptr(mvpMatrix));
    glUniformMatrix4fv(uModelViewMatrix, 1, GL_FALSE, glm::value_ptr(mvMatrix));
    glUniformMatrix4fv(uNormalMatrix, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    glBindVertexArray(m_SceneVAO);
    uint32_t offset = 0;

    glUniform1i(uKaTexture, 0);
    glUniform1i(uKsTexture, 1);
    glUniform1i(uKdTexture, 2);
    glUniform1i(uSnTexture, 3);

    for (int i = 0; i < data.shapeCount; ++i) {

        // todo -> add materiaux
        auto materialId = data.materialIDPerShape[i];
        const auto &material = data.materials[materialId];

        setMaterial(material);


        int val = data.indexCountPerShape[i];
        glDrawElements(GL_TRIANGLES, val, GL_UNSIGNED_INT, (const GLvoid *) (offset * sizeof(GLuint)));
        offset += val;
    }
    //drawCube();
    //drawSphere();


}

void Application::drawLights(const mat4 &viewMatrix) const {
    vec3 pointLightPos_vs = vec3(viewMatrix * vec4(pointLight.position, 1));
    glUniform3f(uPointLightPosition, pointLightPos_vs.x, pointLightPos_vs.y, pointLightPos_vs.z);

    vec3 dirLightPos_vs = vec3(viewMatrix * vec4(dirLight.position, 0));
    glUniform3f(uDirectionalLightDir, dirLightPos_vs.x, dirLightPos_vs.y, dirLightPos_vs.z);

    glUniform3f(uPointLightIntensity, pointLight.intensity.x, pointLight.intensity.y, pointLight.intensity.z);
    glUniform3f(uDirectionalLightIntensity, dirLight.intensity.x, dirLight.intensity.y, dirLight.intensity.z);
}

void Application::setMaterial(const ObjData::PhongMaterial &material) const {
    glUniform3fv(uKaLocation, 1, value_ptr(material.Ka));
    glUniform3fv(uKdLocation, 1, value_ptr(material.Kd));
    glUniform3fv(uKsLocation, 1, value_ptr(material.Ks));
    glUniform1fv(uShininessLocation, 1, &material.shininess);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, (material.KaTextureId < 0) ? 0 : textureIds[material.KaTextureId]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, (material.KsTextureId < 0) ? 0 : textureIds[material.KsTextureId]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, (material.KdTextureId < 0) ? 0 : textureIds[material.KdTextureId]);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, (material.shininessTextureId < 0) ? 0 : textureIds[material.shininessTextureId]);
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
    const GLint positionAttrLocation = glGetAttribLocation(m_program.glId(), "aPosition");
    const GLint normalAttrLocation = glGetAttribLocation(m_program.glId(), "aNormal");
    const GLint uTexture = glGetUniformLocation(m_program.glId(), "uTexture");
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
    glUniform1i(uKaTexture, ind_texture_cube);

    glBindTexture(GL_TEXTURE_2D, textures[ind_texture_cube]);// Bind sur GL_TEXTURE_2D
    // Met les info de l'image dans GL_TEXTURE_2D
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, textureImg.width(), textureImg.height());
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, textureImg.width(), textureImg.height(), GL_RGBA, GL_UNSIGNED_BYTE,
                    textureImg.data());

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
    glm::mat4 cubeMVMatrix = glm::translate(viewMatrix, vec3(0.f, 1.f, 0.f));

    glBindVertexArray(m_cubeVAO);

    glUniformMatrix4fv(uModelViewMatrix, 1, GL_FALSE, glm::value_ptr(cubeMVMatrix));
    glUniformMatrix4fv(uNormalMatrix, 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(cubeMVMatrix))));
    glUniformMatrix4fv(uModelViewProjMatrix, 1, GL_FALSE, glm::value_ptr(projMatrix * cubeMVMatrix));

    glUniform1i(uKaTexture, ind_texture_cube);
    glActiveTexture(GL_TEXTURE0 + ind_texture_cube);
    glBindTexture(GL_TEXTURE_2D, textures[ind_texture_cube]);

    glDrawElements(GL_TRIANGLES, cube.indexBuffer.size(), GL_UNSIGNED_INT, nullptr);

    glActiveTexture(GL_TEXTURE0 + ind_texture_cube);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindVertexArray(0);
}

void Application::drawSphere() {
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

Application::~Application() {
    destroyScene();
//    destroySphere();
//    destroyCube();

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

void Application::destroyScene() {
    if (m_SceneVBO) {
        glDeleteBuffers(1, &m_SceneVBO);
    }

    if (m_SceneIBO) {
        glDeleteBuffers(1, &m_SceneIBO);
    }

    if (m_SceneVAO) {
        glDeleteBuffers(1, &m_SceneVAO);
    }
}
