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

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO);
        // Put here rendering code
        //
        //
        drawScene();

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
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

//            ImGui::RadioButton("Albedo");
//            ImGui::RadioButton("Normal");
//            ImGui::RadioButton("Depth");
//            ImGui::RadioButton("Specular");
//            ImGui::RadioButton("Shadow");

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
    m_program = glmlv::compileProgram({m_ShadersRootPath / m_AppName / "geometryPass.vs.glsl",
                                       m_ShadersRootPath / m_AppName / "geometryPass.fs.glsl"});
    m_program.use();

    setUniformLocations();

    const GLenum m_GBufferTextureFormat[GBufferTextureCount] = {GL_RGB32F, GL_RGB32F, GL_RGB32F, GL_RGB32F, GL_RGBA32F,
                                                                GL_DEPTH_COMPONENT32F};
    glGenTextures(GBufferTextureCount, m_GBufferTextures);

    glBindTexture(GL_TEXTURE_2D, m_GBufferTextures[GPosition]);
    glTexStorage2D(GL_TEXTURE_2D, 1, m_GBufferTextureFormat[GPosition], m_nWindowWidth, m_nWindowHeight);

    glBindTexture(GL_TEXTURE_2D, m_GBufferTextures[GNormal]);
    glTexStorage2D(GL_TEXTURE_2D, 1, m_GBufferTextureFormat[GNormal], m_nWindowWidth, m_nWindowHeight);

    glBindTexture(GL_TEXTURE_2D, m_GBufferTextures[GAmbient]);
    glTexStorage2D(GL_TEXTURE_2D, 1, m_GBufferTextureFormat[GAmbient], m_nWindowWidth, m_nWindowHeight);

    glBindTexture(GL_TEXTURE_2D, m_GBufferTextures[GDiffuse]);
    glTexStorage2D(GL_TEXTURE_2D, 1, m_GBufferTextureFormat[GDiffuse], m_nWindowWidth, m_nWindowHeight);

    glBindTexture(GL_TEXTURE_2D, m_GBufferTextures[GGlossyShininess]);
    glTexStorage2D(GL_TEXTURE_2D, 1, m_GBufferTextureFormat[GGlossyShininess], m_nWindowWidth, m_nWindowHeight);

    glBindTexture(GL_TEXTURE_2D, m_GBufferTextures[GDepth]);
    glTexStorage2D(GL_TEXTURE_2D, 1, m_GBufferTextureFormat[GDepth], m_nWindowWidth, m_nWindowHeight);

    glGenFramebuffers(1, &m_FBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO);

    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + GPosition, GL_TEXTURE_2D,
                           m_GBufferTextures[GPosition], 0);

    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + GNormal, GL_TEXTURE_2D,
                           m_GBufferTextures[GNormal], 0);

    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + GAmbient,
                           GL_TEXTURE_2D, m_GBufferTextures[GAmbient], 0);

    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + GDiffuse,
                           GL_TEXTURE_2D, m_GBufferTextures[GDiffuse], 0);

    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + GGlossyShininess, GL_TEXTURE_2D,
                           m_GBufferTextures[GGlossyShininess], 0);

    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_GBufferTextures[GDepth], 0);

    GLenum drawBuffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3,
                            GL_COLOR_ATTACHMENT4};
    glDrawBuffers(5, drawBuffers);

    // Securité
    glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER,0);
    ////////

    initScene();

}


void Application::initLights() {
    pointLight.position = vec3(0.f, 1.f, 0.f);
    pointLight.intensity = vec3(1E5, 0, 1E5);

    dirLight.position = vec3(1.f, 1.f, 1.f);
    dirLight.intensity = vec3(1.f, 1.f, 1.f);

}

void Application::initScene() {
    initLights();

    glGenBuffers(1, &m_SceneVBO);
    glGenBuffers(1, &m_SceneIBO);

    glmlv::loadObj({m_AssetsRootPath / "glmlv" / "models" / "crytek-sponza" / "sponza.obj"}, data);

    // Fill VBO
    fillSceneVBO();

    // Fill IBO
    fillSceneIBO();

    //Creation texture blanche de base
    createWhiteTexture();

    generateAndBindAllTexture();

    bindDataOnVAO();

    m_SceneSize = glm::length(data.bboxMax - data.bboxMin);
    viewController.setSpeed(m_SceneSize * 0.1f);

    const auto viewportSize = m_GLFWHandle.framebufferSize();
    projMatrix = glm::perspective(70.f, float(viewportSize.x) / viewportSize.y, 0.01f * m_SceneSize,
                                  m_SceneSize); // near = 1% de la taille de la scene, far = 100%

}

void Application::bindDataOnVAO() {
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
}

void Application::generateAndBindAllTexture() {
    textureIds.resize(data.textures.size());
    glGenTextures(data.textures.size(), textureIds.data());
    for (auto i = 0; i < textureIds.size(); i++) {
        glBindTexture(GL_TEXTURE_2D, textureIds[i]);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, data.textures[i].width(), data.textures[i].height());
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, data.textures[i].width(), data.textures[i].height(), GL_RGBA,
                        GL_UNSIGNED_BYTE, data.textures[i].data());
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Application::createWhiteTexture() {
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, 1, 1);
    vec4 white(1.f, 1.f, 1.f, 1.f);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1, 1, GL_RGBA, GL_FLOAT, &white);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Application::fillSceneIBO() const {
    glBindBuffer(GL_ARRAY_BUFFER, m_SceneIBO);
    glBufferStorage(GL_ARRAY_BUFFER, data.indexBuffer.size() * sizeof(uint32_t), data.indexBuffer.data(), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Application::fillSceneVBO() const {
    glBindBuffer(GL_ARRAY_BUFFER, m_SceneVBO);
    glBufferStorage(GL_ARRAY_BUFFER, data.vertexBuffer.size() * sizeof(Vertex3f3f2f), data.vertexBuffer.data(), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Application::setUniformLocations() {
    uModelViewMatrixLocation = glGetUniformLocation(m_program.glId(), "uModelViewMatrixLocation");
    uModelViewProjMatrixLocation = glGetUniformLocation(m_program.glId(), "uModelViewProjMatrixLocation");
    uNormalMatrixLocation = glGetUniformLocation(m_program.glId(), "uNormalMatrixLocation");

    uPointLightPositionLocation = glGetUniformLocation(m_program.glId(), "uPointLighPosition");
    uPointLightIntensityLocation = glGetUniformLocation(m_program.glId(), "uPointLightIntensityLocation");

    uDirectionalLightDirLocation = glGetUniformLocation(m_program.glId(), "uDirectionalLightDirLocation");
    uDirectionalLightIntensityLocation = glGetUniformLocation(m_program.glId(), "uDirectionalLightIntensityLocation");

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

    glUniformMatrix4fv(uModelViewProjMatrixLocation, 1, GL_FALSE, glm::value_ptr(mvpMatrix));
    glUniformMatrix4fv(uModelViewMatrixLocation, 1, GL_FALSE, glm::value_ptr(mvMatrix));
    glUniformMatrix4fv(uNormalMatrixLocation, 1, GL_FALSE, glm::value_ptr(normalMatrix));

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
}

void Application::drawLights(const mat4 &viewMatrix) const {
    vec3 pointLightPos_vs = vec3(viewMatrix * vec4(pointLight.position, 1));
    glUniform3f(uPointLightPositionLocation, pointLightPos_vs.x, pointLightPos_vs.y, pointLightPos_vs.z);

    vec3 dirLightPos_vs = vec3(viewMatrix * vec4(dirLight.position, 0));
    glUniform3f(uDirectionalLightDirLocation, dirLightPos_vs.x, dirLightPos_vs.y, dirLightPos_vs.z);

    glUniform3f(uPointLightIntensityLocation, pointLight.intensity.x, pointLight.intensity.y, pointLight.intensity.z);
    glUniform3f(uDirectionalLightIntensityLocation, dirLight.intensity.x, dirLight.intensity.y, dirLight.intensity.z);
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

Application::~Application() {
    destroyScene();
    ImGui_ImplGlfwGL3_Shutdown();
    glfwTerminate();
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
