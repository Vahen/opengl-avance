#include "Application.hpp"

#include <iostream>
#include <unordered_set>
#include <algorithm>

#include <imgui.h>
#include <glmlv/imgui_impl_glfw_gl3.hpp>
#include <glmlv/Image2DRGBA.hpp>
#include <glmlv/load_obj.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtx/matrix_interpolation.hpp>

#include <chrono>

using Clock=std::chrono::high_resolution_clock;
using namespace glmlv;
using namespace glm;
using namespace std;

int Application::run() {
    float clearColor[3] = {0.5, 0.5, 0.5};
    glClearColor(clearColor[0], clearColor[1], clearColor[2], 1.f);
    startTime = Clock::now();
    // todo -> Changer la position de base de la camera une fois l'animation défini
    // Définir une trajectoire pour la camera -> modif le viewController pour rajouter des fonctions pour que camera puisse bouger librement sans controle humain

    m_viewController.setPosition(m_SceneCenter);
    // Loop until the user closes the window
    for (auto iterationCount = 0u; !m_GLFWHandle.shouldClose(); ++iterationCount) {
        const auto seconds = glfwGetTime();

        const auto projMatrix = perspective(glm::radians(70.f), float(m_nWindowWidth) / m_nWindowHeight,
                                            0.01f * m_SceneSizeLength, m_SceneSizeLength);
        const auto viewMatrix = m_viewController.getViewMatrix();
        const auto rcpViewMatrix = m_viewController.getRcpViewMatrix();

        const float sceneRadius = m_SceneSizeLength * 0.5f;

        const auto dirLightUpVector = computeDirectionVectorUp(radians(m_DirLightPhiAngleDegrees),
                                                               radians(m_DirLightThetaAngleDegrees));
        const auto dirLightViewMatrix = lookAt(m_SceneCenter + m_DirLightDirection * sceneRadius, m_SceneCenter,
                                               dirLightUpVector); // Will not work if m_DirLightDirection is colinear to lightUpVector
        const auto dirLightProjMatrix = ortho(-sceneRadius, sceneRadius, -sceneRadius, sceneRadius,
                                              0.01f * sceneRadius, 2.f * sceneRadius);

        // Shadow map computation if necessary
        {
            if (m_directionalSMResolutionDirty) {
                cleanShadowMap();
                m_directionalSMResolutionDirty = false;
                m_directionalSMDirty = true; // The shadow map must also be recomputed
            }

            if (m_directionalSMDirty) {
                m_directionalSMProgram.use();
                computShadowMap(dirLightViewMatrix, dirLightProjMatrix);
                m_directionalSMDirty = false;
            }
        }

        // Geometry pass
        {
            m_geometryPassProgram.use();

            geometryPass(projMatrix, viewMatrix);
        }

        // Put here rendering code
        renderScene(projMatrix, viewMatrix, rcpViewMatrix, dirLightViewMatrix, dirLightProjMatrix);

        // GUI code:
        ImGui_ImplGlfwGL3_NewFrame();

        {
            GUIDisplay(clearColor);
        }


        ImGui::Render();

        /* Poll for and process events */
        glfwPollEvents();

        /* Swap front and back buffers*/
        m_GLFWHandle.swapBuffers();

        auto ellapsedTime = glfwGetTime() - seconds;
        auto guiHasFocus = ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
        if (!guiHasFocus) {
            m_viewController.update(float(ellapsedTime));
        }
    }

    return 0;
}

void Application::GUIDisplay(float *clearColor) {
    ImGui::Begin("GUI");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    ImGui::SetColorEditOptions(ImGuiColorEditFlags_RGB);
    if (ImGui::ColorEdit3("clearColor", clearColor)) {
        glClearColor(clearColor[0], clearColor[1], clearColor[2], 1.f);
    }
//            if (ImGui::Button("Sort shapes wrt materialID")) {
//                sort(m_data.materialIDPerShape.begin(),m_data.materialIDPerShape.end(), [&](auto lhs, auto rhs) {
//                    return lhs.materialID < rhs.materialID;
//                });
//            }
    if (ImGui::CollapsingHeader("Directional Light")) {
        ImGui::ColorEdit3("DirLightColor", value_ptr(m_DirLightColor));
        ImGui::DragFloat("DirLightIntensity", &m_DirLightIntensity, 0.1f, 0.f, 100.f);
        bool angleChanged = ImGui::DragFloat("Phi Angle", &m_DirLightPhiAngleDegrees, 1.0f, 0.0f, 360.f);
        angleChanged = ImGui::DragFloat("Theta Angle", &m_DirLightThetaAngleDegrees, 1.0f, 0.0f, 180.f) ||
                       angleChanged;

        if (angleChanged) {
            m_DirLightDirection = computeDirectionVector(radians(m_DirLightPhiAngleDegrees),
                                                         radians(m_DirLightThetaAngleDegrees));
            m_directionalSMDirty = true;
        }

        if (ImGui::InputInt("DirShadowMap Res", &m_nDirectionalSMResolution)) {
            if (m_nDirectionalSMResolution <= 0)
                m_nDirectionalSMResolution = 1;
            m_directionalSMResolutionDirty = true;
        }

        ImGui::InputFloat("DirShadowMap Bias", &m_DirLightSMBias);
        ImGui::InputInt("DirShadowMap SampleCount", &m_DirLightSMSampleCount);
        ImGui::InputFloat("DirShadowMap Spread", &m_DirLightSMSpread);
    }

    if (ImGui::CollapsingHeader("Point Light")) {
        ImGui::ColorEdit3("PointLightColor", value_ptr(m_PointLightColor));
        ImGui::DragFloat("PointLightIntensity", &m_PointLightIntensity, 0.1f, 0.f, 16000.f);
        ImGui::InputFloat3("Position", value_ptr(m_PointLightPosition));
    }

    if (ImGui::CollapsingHeader("Display")) {
        for (int32_t i = Display_Beauty; i < Display_Count; ++i) {
            ImGui::RadioButton(m_DisplayNames[i], &m_CurrentlyDisplayed, i);
        }
    }

    ImGui::InputFloat("MovementSpeed", &m_speed);
    ImGui::InputFloat("RotationSpeed", &m_RotationSpeed);
    auto cameraPos = m_viewController.getM_position();
    ImGui::InputFloat3("Position camera", value_ptr(cameraPos));
    ImGui::End();
}

void Application::renderScene(const mat4 &projMatrix, const mat4 &viewMatrix, const mat4 &rcpViewMatrix,
                              const mat4 &dirLightViewMatrix, const mat4 &dirLightProjMatrix) const {
    const auto viewportSize = m_GLFWHandle.framebufferSize();
    glViewport(0, 0, viewportSize.x, viewportSize.y);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_CurrentlyDisplayed == Display_Beauty) // Beauty
    {
        // Shading pass
        {
            m_shadingPassProgram.use();
            shadingPass(viewMatrix, rcpViewMatrix, dirLightViewMatrix, dirLightProjMatrix);
        }
    } else if (m_CurrentlyDisplayed == Display_GDepth) {
        m_displayDepthProgram.use();
        displayDepth();

    } else if (m_CurrentlyDisplayed == Display_GPosition) {
        m_displayPositionProgram.use();
        displayPosition(projMatrix);

    } else if (m_CurrentlyDisplayed == Display_DirectionalLightDepthMap) {
        m_displayDepthProgram.use();
        displayDirectionalLightDepthMap();

    } else {
        // GBuffer display
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_GBufferFBO);
        const auto gBufferIndex = m_CurrentlyDisplayed - Display_GPosition;
        glReadBuffer(GL_COLOR_ATTACHMENT0 + gBufferIndex);
        glBlitFramebuffer(0, 0, m_nWindowWidth, m_nWindowHeight,
                          0, 0, m_nWindowWidth, m_nWindowHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    }
}

void Application::cleanShadowMap() {
    glDeleteTextures(1, &m_directionalSMTexture);

    // Realocate texture
    glGenTextures(1, &m_directionalSMTexture);
    glBindTexture(GL_TEXTURE_2D, m_directionalSMTexture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, m_nDirectionalSMResolution,
                   m_nDirectionalSMResolution);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Attach new texture to FBO
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_directionalSMFBO);
    glBindTexture(GL_TEXTURE_2D, m_directionalSMTexture);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_directionalSMTexture,
                           0);

    const auto fboStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
        cerr << "Error on building directional shadow mapping framebuffer. Error code = " << fboStatus << endl;
        throw runtime_error("Error on building directional shadow mapping framebuffer.");
    }

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void Application::computShadowMap(const mat4 &dirLightViewMatrix, const mat4 &dirLightProjMatrix) const {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_directionalSMFBO);
    glViewport(0, 0, m_nDirectionalSMResolution, m_nDirectionalSMResolution);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//    glUniformMatrix4fv(m_uDirLightViewProjMatrix, 1, GL_FALSE,
//                       value_ptr(dirLightProjMatrix * dirLightViewMatrix));
// todo Ajouter les transformations des objets

    glBindVertexArray(m_SceneVAO);

    // We draw each shape by specifying how much indices it carries, and with an offset in the global index buffer
    uint32_t offset = 0;
    int j = 0;
    int countShapePassed = 0;
    for (int i = 0; i < m_data.shapeCount; ++i) {
        if (i - countShapePassed >= m_tabIndexShape[j]) {
            countShapePassed += m_tabIndexShape[j];
            j++;
        }
        auto transformMatrix = mat4();
        switch (j) {
            // todo -> Ajouter les rotations
            case 1:
                transformMatrix = glm::translate(transformMatrix,m_coordBigShip);
//                transformMatrix = glm::translate(transformMatrix,m_coordBigShip);
//                glUniformMatrix4fv(m_uDirLightViewProjMatrix, 1, GL_FALSE, value_ptr(
//                        dirLightProjMatrix * dirLightViewMatrix *
//                        m_coordBigShip));
                transformMatrix = glm::rotate(transformMatrix,m_RotationBigShip.x,vec3(-1,0,0));
                transformMatrix = glm::rotate(transformMatrix,m_RotationBigShip.y,vec3(0,1,0));
                transformMatrix = glm::rotate(transformMatrix,m_RotationBigShip.z,vec3(0,0,-1));
                sendLightProjInfo(dirLightViewMatrix, dirLightProjMatrix, transformMatrix);
                break;
            case 2:
                transformMatrix = glm::translate(transformMatrix,m_coordAWing1);
                transformMatrix = glm::rotate(transformMatrix,m_RotationAWing1.x,vec3(-1,0,0));
                transformMatrix = glm::rotate(transformMatrix,m_RotationAWing1.y,vec3(0,1,0));
                transformMatrix = glm::rotate(transformMatrix,m_RotationAWing1.z,vec3(0,0,-1));
                sendLightProjInfo(dirLightViewMatrix, dirLightProjMatrix, transformMatrix);
                break;
            case 3:
                transformMatrix = glm::translate(transformMatrix,m_coordAWing2);
                transformMatrix = glm::rotate(transformMatrix,m_RotationAWing2.x,vec3(-1,0,0));
                transformMatrix = glm::rotate(transformMatrix,m_RotationAWing2.y,vec3(0,1,0));
                transformMatrix = glm::rotate(transformMatrix,m_RotationAWing3.z,vec3(0,0,-1));
                sendLightProjInfo(dirLightViewMatrix, dirLightProjMatrix, transformMatrix);
                break;
            case 4:
                transformMatrix = glm::translate(transformMatrix,m_coordAWing3);
                transformMatrix = glm::rotate(transformMatrix,m_RotationAWing3.x,vec3(-1,0,0));
                transformMatrix = glm::rotate(transformMatrix,m_RotationAWing3.y,vec3(0,1,0));
                transformMatrix = glm::rotate(transformMatrix,m_RotationAWing3.z,vec3(0,0,-1));
                sendLightProjInfo(dirLightViewMatrix, dirLightProjMatrix, transformMatrix);
                break;
        }
        int val = m_data.indexCountPerShape[i];
        glDrawElements(GL_TRIANGLES, val, GL_UNSIGNED_INT, (const GLvoid *) (offset * sizeof(GLuint)));
        offset += val;
    }

    glBindVertexArray(0);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void Application::sendLightProjInfo(const mat4 &dirLightViewMatrix, const mat4 &dirLightProjMatrix,
                                    const mat4 &transformMatrix) const {
    glUniformMatrix4fv(m_uDirLightViewProjMatrix, 1, GL_FALSE,
                       value_ptr(dirLightProjMatrix * dirLightViewMatrix * transformMatrix));
}

void Application::displayDirectionalLightDepthMap() const {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_directionalSMTexture);

    glUniform1i(m_uGDepthSamplerLocation, 0);

    glBindVertexArray(m_TriangleVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

void Application::displayPosition(const mat4 &projMatrix) const {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_GBufferTextures[GPosition]);

    glUniform1i(m_uGDepthSamplerLocation, 0);

    const auto rcpProjMat = inverse(projMatrix);

    const vec4 frustrumTopRight(1, 1, 1, 1);
    const auto frustrumTopRight_view = rcpProjMat * frustrumTopRight;

    glUniform3fv(m_uSceneSizeLocation, 1, value_ptr(frustrumTopRight_view / frustrumTopRight_view.w));

    glBindVertexArray(m_TriangleVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

void Application::displayDepth() const {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_GBufferTextures[GDepth]);

    glUniform1i(m_uGDepthSamplerLocation, 0);

    glBindVertexArray(m_TriangleVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

void Application::shadingPass(const mat4 &viewMatrix, const mat4 &rcpViewMatrix,
                              const mat4 &dirLightViewMatrix,
                              const mat4 &dirLightProjMatrix) const {
    glUniform3fv(m_uDirectionalLightDirLocation, 1,
                 value_ptr(vec3(viewMatrix * vec4(normalize(m_DirLightDirection), 0))));
    glUniform3fv(m_uDirectionalLightIntensityLocation, 1,
                 value_ptr(m_DirLightColor * m_DirLightIntensity));

    glUniform3fv(m_uPointLightPositionLocation, 1,
                 value_ptr(vec3(viewMatrix * vec4(m_PointLightPosition, 1))));
    glUniform3fv(m_uPointLightIntensityLocation, 1,
                 value_ptr(m_PointLightColor * m_PointLightIntensity));

    glUniform1fv(m_uDirLightShadowMapBias, 1, &m_DirLightSMBias);

    glUniform1iv(m_uDirLightShadowMapSampleCount, 1, &m_DirLightSMSampleCount);
    glUniform1fv(m_uDirLightShadowMapSpread, 1, &m_DirLightSMSpread);

    glUniformMatrix4fv(m_uDirLightViewProjMatrix_shadingPass, 1, GL_FALSE,
                       value_ptr(dirLightProjMatrix * dirLightViewMatrix * rcpViewMatrix));

    for (int32_t i = GPosition; i < GDepth; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, m_GBufferTextures[i]);

        glUniform1i(m_uGBufferSamplerLocations[i], i);
    }

    glActiveTexture(GL_TEXTURE0 + GDepth);
    glBindTexture(GL_TEXTURE_2D, m_directionalSMTexture);
    glBindSampler(GDepth, m_directionalSMSampler);
    glUniform1i(m_uDirLightShadowMap, GDepth);

    glBindVertexArray(m_TriangleVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

void Application::geometryPass(const mat4 &projMatrix, const mat4 &viewMatrix) {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_GBufferFBO);

    glViewport(0, 0, m_nWindowWidth, m_nWindowHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Same sampler for all texture units
    glBindSampler(0, m_textureSampler);
    glBindSampler(1, m_textureSampler);
    glBindSampler(2, m_textureSampler);
    glBindSampler(3, m_textureSampler);

    // Set texture unit of each sampler
    glUniform1i(m_uKaSamplerLocation, 0);
    glUniform1i(m_uKdSamplerLocation, 1);
    glUniform1i(m_uKsSamplerLocation, 2);
    glUniform1i(m_uShininessSamplerLocation, 3);

    glBindVertexArray(m_SceneVAO);

    // todo -> modif les matrices selon l'objet affiché
    uint32_t offset = 0;
    int j = 0;
    int countShapePassed = 0;

    vec3 test = vec3(10, 10, 10);
    auto now = Clock::now();
    auto timeElapsed = chrono::duration_cast<chrono::duration<double>>(now - startTime);
    cout << "Time elapsed = " << timeElapsed.count() << " seconds" << endl;
    auto time = timeElapsed.count();

    auto firstPart = time < 10. && time > 0;
    auto secondPart = time < 20. && time > 10;
    auto thirdPart = time < 30. && time > 20;
    auto fourthPart = time < 40. && time > 30;
    auto fifthPart = time < 50. && time > 40;
    auto sixthPart = time < 60. && time > 50;

    // todo
    // vec3(0,1,1) -> monte en vertical
    vec3 up = vec3(0, 1, 1);
    vec3 down = vec3(0, -1, -1);

    vec3 front = vec3(-0.5, 0, 0.5);
    vec3 back = vec3(0.5, 0, -1);

    // Modif des vecteurs de translation
    if (firstPart) {
        m_coordBigShip += up;
        m_coordAWing1 += vec3();
        m_coordAWing2 += vec3();
        m_coordAWing3 += vec3();
    }
    if (secondPart) {
        m_coordBigShip += down;
        m_coordAWing1 += vec3();
        m_coordAWing2 += vec3();
        m_coordAWing3 += vec3();
    }
    if (thirdPart) {
        m_coordBigShip += front;
        m_coordAWing1 += vec3();
        m_coordAWing2 += vec3();
        m_coordAWing3 += vec3();
    }
    if (fourthPart) {
        m_coordBigShip += back;
        m_coordAWing1 += vec3();
        m_coordAWing2 += vec3();
        m_coordAWing3 += vec3();
    }
    if (fifthPart) {
        m_coordBigShip += vec3();
        m_coordAWing1 += vec3();
        m_coordAWing2 += vec3();
        m_coordAWing3 += vec3();
    }
    if (sixthPart) {
        m_coordBigShip += vec3();
        m_coordAWing1 += vec3();
        m_coordAWing2 += vec3();
        m_coordAWing3 += vec3();
    }

    for (int i = 0; i < m_data.shapeCount; ++i) {
        if (i - countShapePassed >= m_tabIndexShape[j]) {
            countShapePassed += m_tabIndexShape[j];
            j++;
        }
        auto mvMatrix = viewMatrix;
        mvMatrix = firstPartLoop(j, firstPart, mvMatrix);
        mvMatrix = secondPartLoop(j, secondPart, mvMatrix);
        mvMatrix = thirdPartLoop(j, thirdPart, mvMatrix);
        mvMatrix = fourthPartLoop(j, fourthPart, mvMatrix);
        mvMatrix = fifthPartLoop(j, fifthPart, mvMatrix);
        mvMatrix = sixthPartLoop(j, sixthPart, mvMatrix);

//        auto mvMatrix = glm::translate(viewMatrix,m_SceneCenter) ;
        auto mvpMatrix = projMatrix * mvMatrix;
        auto normalMatrix = transpose(inverse(mvMatrix));

        sendMatrixInformation(mvMatrix, mvpMatrix, normalMatrix);

        auto materialId = m_data.materialIDPerShape[i];
        const auto &material = materialId >= 0 ? m_data.materials[materialId] : m_DefaultMaterial;
        setMaterial(material);
        int val = m_data.indexCountPerShape[i];
        glDrawElements(GL_TRIANGLES, val, GL_UNSIGNED_INT, (const GLvoid *) (offset * sizeof(GLuint)));
        offset += val;
    }

    for (const auto i : {0, 1, 2, 3})
        glBindSampler(0, m_textureSampler);

    glBindVertexArray(0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

mat4 &Application::sixthPartLoop(int j, bool sixthPart, mat4 &mvMatrix) {
    if (sixthPart) {
        switch (j) {
            case 0: // star destroyer
                mvMatrix = sixthPartBigShip(mvMatrix);
                break;
            case 1: // A-Wing 1
                mvMatrix = sixthPartAWing1(mvMatrix);
                break;
            case 2: // A-Wing 2
                mvMatrix = sixthPartAWing2(mvMatrix);
                break;
            case 3: // A-Wing 3
                mvMatrix = sixthPartAWing3(mvMatrix);
                break;
            default:
                break;
        }
    }
    return mvMatrix;
}

mat4 &Application::fifthPartLoop(int j, bool fifthPart, mat4 &mvMatrix) {
    if (fifthPart) {
        switch (j) {
            case 0: // star destroyer
                mvMatrix = fifthPartBigShip(mvMatrix);
                break;
            case 1: // A-Wing 1
                mvMatrix = fifthPartAWing1(mvMatrix);
                break;
            case 2: // A-Wing 2
                mvMatrix = fifthPartAWing2(mvMatrix);
                break;
            case 3: // A-Wing 3
                mvMatrix = fifthPartAWing3(mvMatrix);
                break;
            default:
                break;
        }
    }
    return mvMatrix;
}

mat4 &Application::fourthPartLoop(int j, bool fourthPart, mat4 &mvMatrix) {
    if (fourthPart) {
        switch (j) {
            case 0: // star destroyer
                mvMatrix = fourthPartBigShip(mvMatrix);
                break;
            case 1: // A-Wing 1
                mvMatrix = fourthPartAWing1(mvMatrix);
                break;
            case 2: // A-Wing 2
                mvMatrix = fourthPartAWing2(mvMatrix);
                break;
            case 3: // A-Wing 3
                mvMatrix = fourthPartAWing3(mvMatrix);
                break;
            default:
                break;
        }
    }
    return mvMatrix;
}

mat4 &Application::thirdPartLoop(int j, bool thirdPart, mat4 &mvMatrix) {
    if (thirdPart) {
        switch (j) {
            case 0: // star destroyer
                mvMatrix = thirdPartBigShip(mvMatrix);
                break;
            case 1: // A-Wing 1
                mvMatrix = thirdPartAWing1(mvMatrix);
                break;
            case 2: // A-Wing 2
                mvMatrix = thirdPartAWing2(mvMatrix);
                break;
            case 3: // A-Wing 3
                mvMatrix = thirdPartAWing3(mvMatrix);
                break;
            default:
                break;
        }
    }
    return mvMatrix;
}

mat4 &Application::secondPartLoop(int j, bool secondPart, mat4 &mvMatrix) {
    if (secondPart) {
        switch (j) {
            case 0: // star destroyer
                mvMatrix = secondPartBigShip(mvMatrix);
                break;
            case 1: // A-Wing 1
                mvMatrix = secondPartAWing1(mvMatrix);
                break;
            case 2: // A-Wing 2
                mvMatrix = secondPartAWing2(mvMatrix);
                break;
            case 3: // A-Wing 3
                mvMatrix = secondPartAWing3(mvMatrix);
                break;
            default:
                break;
        }
    }
    return mvMatrix;
}

mat4 &Application::firstPartLoop(int j, bool firstPart, mat4 &mvMatrix) {
    if (firstPart) {
        switch (j) {
            case 0: // star destroyer
                //mvMatrix = glm::translate(mvMatrvix,m_SceneCenter);
                //cout << mvMatrix << endl;
                //mvMatrix = glm::translate(mvMatrix,vec3(0,0,0));

                mvMatrix = firstPartBigShip(mvMatrix);
                // todo -> Se servir de l'interpolation pour regler la vitesse et avoir un mouvement fluide
                //cout << "Interpolation ->" << mvMatrix << endl;
                break;
            case 1: // A-Wing 1
                mvMatrix = firstPartAWing1(mvMatrix);

                //mvMatrix = glm::rotate(mvMatrix, radians(45.f), glm::vec3(0, 1, 0));
//                mvMatrix = glm::rotate(mvMatrix, static_cast<float>(m_speed * glfwGetTime()), glm::vec3(0, 1, 0));
                break;
            case 2: // A-Wing 2
                mvMatrix = firstPartAWing2(mvMatrix);
                //mvMatrix = glm::rotate(mvMatrix, static_cast<float>(m_speed * glfwGetTime()), glm::vec3(1, 0, 0));
                break;
            case 3: // A-Wing 3
                mvMatrix = firstPartAWing3(mvMatrix);
                break;
            default:
                break;
        }
    }
    return mvMatrix;
}

void Application::sendMatrixInformation(const mat4 &mvMatrix, const mat4 &mvpMatrix, const mat4 &normalMatrix) const {
    glUniformMatrix4fv(m_uModelViewProjMatrixLocation, 1, GL_FALSE, value_ptr(mvpMatrix));
    glUniformMatrix4fv(m_uModelViewMatrixLocation, 1, GL_FALSE, value_ptr(mvMatrix));
    glUniformMatrix4fv(m_uNormalMatrixLocation, 1, GL_FALSE, value_ptr(normalMatrix));
}

void Application::setMaterial(const ObjData::PhongMaterial &material) const {

    glUniform3fv(m_uKaLocation, 1, value_ptr(material.Ka));
    glUniform3fv(m_uKdLocation, 1, value_ptr(material.Kd));
    glUniform3fv(m_uKsLocation, 1, value_ptr(material.Ks));
    glUniform1fv(m_uShininessLocation, 1, &material.shininess);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, (material.KaTextureId >= 0) ? m_textureIds[material.KaTextureId] : m_WhiteTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, (material.KsTextureId >= 0) ? m_textureIds[material.KsTextureId] : m_WhiteTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, (material.KdTextureId >= 0) ? m_textureIds[material.KdTextureId] : m_WhiteTexture);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D,
                  (material.shininessTextureId >= 0) ? m_textureIds[material.shininessTextureId] : m_WhiteTexture);

}

Application::Application(int argc, char **argv) :
        m_AppPath{fs::path{argv[0]}},
        m_AppName{m_AppPath.stem().string()},
        m_ImGuiIniFilename{m_AppName + ".imgui.ini"},
        m_ShadersRootPath{m_AppPath.parent_path() / "shaders"},
        m_AssetsRootPath{m_AppPath.parent_path() / "assets"} {
    ImGui::GetIO().IniFilename = m_ImGuiIniFilename.c_str(); // At exit, ImGUI will store its windows positions in this file

    initShadersData();
    initScene();
    initGBuffer();
    initScreenTriangle();
    initShadowData();

    glEnable(GL_DEPTH_TEST);
    m_viewController.setSpeed(m_SceneSizeLength * 0.1f); // Let's travel 10% of the scene per second
}

void Application::initShadowData() {
    glGenTextures(1, &m_directionalSMTexture);

    glBindTexture(GL_TEXTURE_2D, m_directionalSMTexture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, m_nDirectionalSMResolution, m_nDirectionalSMResolution);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &m_directionalSMFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_directionalSMFBO);
    glBindTexture(GL_TEXTURE_2D, m_directionalSMTexture);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_directionalSMTexture, 0);

    const auto fboStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
        cerr << "Error on building directional shadow mapping framebuffer. Error code = " << fboStatus << endl;
        throw runtime_error("Error on building directional shadow mapping framebuffer.");
    }

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    glGenSamplers(1, &m_directionalSMSampler);
    glSamplerParameteri(m_directionalSMSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glSamplerParameteri(m_directionalSMSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(m_directionalSMSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glSamplerParameteri(m_directionalSMSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glSamplerParameteri(m_directionalSMSampler, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glSamplerParameteri(m_directionalSMSampler, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
}

void Application::initScene() {
    glGenBuffers(1, &m_SceneVBO);
    glGenBuffers(1, &m_SceneIBO);

    {
//        const auto objPath = m_AssetsRootPath / "glmlv" / "models" / "crytek-sponza" / "sponza.obj";
//        loadObjAndPushIndexShape(objPath);
        // todo -> charger tous les objets 3D à afficher
        // todo -> trouver d'autres modeles
//        auto objPath = m_AssetsRootPath / "glmlv" / "models" / "tieFighter" / "TieFighter.obj"; -> Pas de texture

//        loadObjAndPushIndexShape(objPath);

        auto objPath = m_AssetsRootPath / "glmlv" / "models" / "StarDestroyer" /
                       "star_wars_star_destroyer.obj"; //-> Fonctionne
        loadObjAndPushIndexShape(objPath);


        objPath = m_AssetsRootPath / "glmlv" / "models" / "A_Wing" / "Star_Wars_A_Wing.obj"; //-> Fonctionne
        loadObjAndPushIndexShape(objPath);
        loadObjAndPushIndexShape(objPath);
        loadObjAndPushIndexShape(objPath);


        m_SceneSize = m_data.bboxMax - m_data.bboxMin;
        m_SceneSizeLength = glm::length(m_SceneSize);
        m_SceneCenter = 0.5f * (m_data.bboxMax + m_data.bboxMin);

        cout << "# of shapes    : " << m_data.shapeCount << endl;
        cout << "# of materials : " << m_data.materialCount << endl;
        cout << "# of vertex    : " << m_data.vertexBuffer.size() << endl;
        cout << "# of triangles : " << m_data.indexBuffer.size() / 3 << endl;
        cout << "# of textures  : " << m_data.textures.size() << endl;
        cout << "Center of scene : " << m_SceneCenter << endl;
        cout << "SceneSize length : " << m_SceneSizeLength << endl;

        // Fill VBO
        fillSceneVBO();

        // Fill IBO
        fillSceneIBO();

        createWhiteTexture();

        // Upload all textures to the GPU
        generateAndBindAllTexture();

        initDefaultMaterial();
    }

    // Fill VAO
    bindDataOnVAO();

    // Note: no need to bind a sampler for modifying it: the sampler API is already direct_state_access
    glGenSamplers(1, &m_textureSampler);
    glSamplerParameteri(m_textureSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glSamplerParameteri(m_textureSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void Application::initDefaultMaterial() {
    m_DefaultMaterial.Ka = vec3(0);
    m_DefaultMaterial.Kd = vec3(1);
    m_DefaultMaterial.Ks = vec3(1);
    m_DefaultMaterial.shininess = 32.f;
    m_DefaultMaterial.KaTextureId = m_WhiteTexture;
    m_DefaultMaterial.KdTextureId = m_WhiteTexture;
    m_DefaultMaterial.KsTextureId = m_WhiteTexture;
    m_DefaultMaterial.shininessTextureId = m_WhiteTexture;
}

void Application::loadObjAndPushIndexShape(const fs::path &objPath) {
    int oldVal = m_data.shapeCount;
    loadObj(objPath, m_data, true);
    if (oldVal > 0) {
        m_tabIndexShape.push_back(m_data.shapeCount - oldVal);
    } else {
        m_tabIndexShape.push_back(m_data.shapeCount);
    }
}

void Application::fillSceneIBO() const {
    glBindBuffer(GL_ARRAY_BUFFER, m_SceneIBO);
    glBufferStorage(GL_ARRAY_BUFFER, m_data.indexBuffer.size() * sizeof(uint32_t), m_data.indexBuffer.data(), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Application::fillSceneVBO() const {
    glBindBuffer(GL_ARRAY_BUFFER, m_SceneVBO);
    glBufferStorage(GL_ARRAY_BUFFER, m_data.vertexBuffer.size() * sizeof(Vertex3f3f2f), m_data.vertexBuffer.data(), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Application::bindDataOnVAO() {
    glGenVertexArrays(1, &m_SceneVAO);
    glBindVertexArray(m_SceneVAO);

    const GLint positionAttrLocation = 0;
    const GLint normalAttrLocation = 1;
    const GLint texCoordsAttrLocation = 2;

    // We tell OpenGL what vertex attributes our VAO is describing:
    glEnableVertexAttribArray(positionAttrLocation);
    glEnableVertexAttribArray(normalAttrLocation);
    glEnableVertexAttribArray(texCoordsAttrLocation);

    // We bind the VBO because the next 3 calls will read what VBO is bound in order to know where the data is stored
    glBindBuffer(GL_ARRAY_BUFFER, m_SceneVBO);

    glVertexAttribPointer(positionAttrLocation, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3f3f2f),
                          (const GLvoid *) offsetof(Vertex3f3f2f, position));
    glVertexAttribPointer(normalAttrLocation, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3f3f2f),
                          (const GLvoid *) offsetof(Vertex3f3f2f, normal));
    glVertexAttribPointer(texCoordsAttrLocation, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3f3f2f),
                          (const GLvoid *) offsetof(Vertex3f3f2f, texCoords));

    // We can unbind the VBO because OpenGL has "written" in the VAO what VBO it needs to read when the VAO will be drawn
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Binding the IBO to GL_ELEMENT_ARRAY_BUFFER while a VAO is bound "writes" it in the VAO for usage when the VAO will be drawn
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_SceneIBO);

    glBindVertexArray(0);
}

void Application::createWhiteTexture() {
    glGenTextures(1, &m_WhiteTexture);
    glBindTexture(GL_TEXTURE_2D, m_WhiteTexture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, 1, 1);
    vec4 white(1.f, 1.f, 1.f, 1.f);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1, 1, GL_RGBA, GL_FLOAT, &white);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Application::generateAndBindAllTexture() {
    m_textureIds.resize(m_data.textures.size());
    glGenTextures(m_data.textures.size(), m_textureIds.data());
    for (auto i = 0; i < m_textureIds.size(); i++) {
        glBindTexture(GL_TEXTURE_2D, m_textureIds[i]);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, m_data.textures[i].width(), m_data.textures[i].height());
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_data.textures[i].width(), m_data.textures[i].height(), GL_RGBA,
                        GL_UNSIGNED_BYTE, m_data.textures[i].data());
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Application::initShadersData() {
    initGeometryPassData();

    initShadingPassData();

    initDisplayDepthData();

    initDisplayPositionData();

    initDirectionnalSMData();
}

void Application::initDirectionnalSMData() {
    m_directionalSMProgram = compileProgram({m_ShadersRootPath / m_AppName / "directionalSM.vs.glsl",
                                             m_ShadersRootPath / m_AppName / "directionalSM.fs.glsl"});
    m_uDirLightViewProjMatrix = glGetUniformLocation(m_directionalSMProgram.glId(), "uDirLightViewProjMatrix");
}

void Application::initDisplayPositionData() {
    m_displayPositionProgram = compileProgram({m_ShadersRootPath / m_AppName / "shadingPass.vs.glsl",
                                               m_ShadersRootPath / m_AppName / "displayPosition.fs.glsl"});

    m_uGPositionSamplerLocation = glGetUniformLocation(m_displayPositionProgram.glId(), "uGPosition");
    m_uSceneSizeLocation = glGetUniformLocation(m_displayPositionProgram.glId(), "uSceneSize");
}

void Application::initDisplayDepthData() {
    m_displayDepthProgram = compileProgram({m_ShadersRootPath / m_AppName / "shadingPass.vs.glsl",
                                            m_ShadersRootPath / m_AppName / "displayDepth.fs.glsl"});

    m_uGDepthSamplerLocation = glGetUniformLocation(m_displayDepthProgram.glId(), "uGDepth");
}

void Application::initShadingPassData() {
    m_shadingPassProgram = compileProgram({m_ShadersRootPath / m_AppName / "shadingPass.vs.glsl",
                                           m_ShadersRootPath / m_AppName / "shadingPass.fs.glsl"});

    m_uGBufferSamplerLocations[GPosition] = glGetUniformLocation(m_shadingPassProgram.glId(), "uGPosition");
    m_uGBufferSamplerLocations[GNormal] = glGetUniformLocation(m_shadingPassProgram.glId(), "uGNormal");
    m_uGBufferSamplerLocations[GAmbient] = glGetUniformLocation(m_shadingPassProgram.glId(), "uGAmbient");
    m_uGBufferSamplerLocations[GDiffuse] = glGetUniformLocation(m_shadingPassProgram.glId(), "uGDiffuse");
    m_uGBufferSamplerLocations[GGlossyShininess] = glGetUniformLocation(m_shadingPassProgram.glId(),
                                                                        "uGGlossyShininess");

    m_uDirLightViewProjMatrix_shadingPass = glGetUniformLocation(m_shadingPassProgram.glId(),
                                                                 "uDirLightViewProjMatrix");
    m_uDirLightShadowMap = glGetUniformLocation(m_shadingPassProgram.glId(), "uDirLightShadowMap");
    m_uDirLightShadowMapBias = glGetUniformLocation(m_shadingPassProgram.glId(), "uDirLightShadowMapBias");
    m_uDirLightShadowMapSampleCount = glGetUniformLocation(m_shadingPassProgram.glId(),
                                                           "uDirLightShadowMapSampleCount");
    m_uDirLightShadowMapSpread = glGetUniformLocation(m_shadingPassProgram.glId(), "uDirLightShadowMapSpread");

    m_uDirectionalLightDirLocation = glGetUniformLocation(m_shadingPassProgram.glId(), "uDirectionalLightDir");
    m_uDirectionalLightIntensityLocation = glGetUniformLocation(m_shadingPassProgram.glId(),
                                                                "uDirectionalLightIntensity");

    m_uPointLightPositionLocation = glGetUniformLocation(m_shadingPassProgram.glId(), "uPointLightPosition");
    m_uPointLightIntensityLocation = glGetUniformLocation(m_shadingPassProgram.glId(), "uPointLightIntensity");
}

void Application::initGeometryPassData() {
    m_geometryPassProgram = compileProgram({m_ShadersRootPath / m_AppName / "geometryPass.vs.glsl",
                                            m_ShadersRootPath / m_AppName / "geometryPass.fs.glsl"});

    m_uModelViewProjMatrixLocation = glGetUniformLocation(m_geometryPassProgram.glId(), "uModelViewProjMatrix");
    m_uModelViewMatrixLocation = glGetUniformLocation(m_geometryPassProgram.glId(), "uModelViewMatrix");
    m_uNormalMatrixLocation = glGetUniformLocation(m_geometryPassProgram.glId(), "uNormalMatrix");

    m_uKaLocation = glGetUniformLocation(m_geometryPassProgram.glId(), "uKa");
    m_uKdLocation = glGetUniformLocation(m_geometryPassProgram.glId(), "uKd");
    m_uKsLocation = glGetUniformLocation(m_geometryPassProgram.glId(), "uKs");
    m_uShininessLocation = glGetUniformLocation(m_geometryPassProgram.glId(), "uShininess");
    m_uKaSamplerLocation = glGetUniformLocation(m_geometryPassProgram.glId(), "uKaSampler");
    m_uKdSamplerLocation = glGetUniformLocation(m_geometryPassProgram.glId(), "uKdSampler");
    m_uKsSamplerLocation = glGetUniformLocation(m_geometryPassProgram.glId(), "uKsSampler");
    m_uShininessSamplerLocation = glGetUniformLocation(m_geometryPassProgram.glId(), "uShininessSampler");
}

void Application::initGBuffer() {
    // Init GBuffer
    glGenTextures(GBufferTextureCount, m_GBufferTextures);

    for (int32_t i = GPosition; i < GBufferTextureCount; ++i) {
        glBindTexture(GL_TEXTURE_2D, m_GBufferTextures[i]);
        glTexStorage2D(GL_TEXTURE_2D, 1, m_GBufferTextureFormat[i], m_nWindowWidth, m_nWindowHeight);
    }

    glGenFramebuffers(1, &m_GBufferFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_GBufferFBO);
    for (int32_t i = GPosition; i < GDepth; ++i) {
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, m_GBufferTextures[i], 0);
    }
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_GBufferTextures[GDepth], 0);

    // we will write into 5 textures from the fragment shader
    GLenum drawBuffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3,
                            GL_COLOR_ATTACHMENT4};
    glDrawBuffers(5, drawBuffers);

    GLenum status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);

    if (status != GL_FRAMEBUFFER_COMPLETE) {
        cerr << "FB error, status: " << status << endl;
        throw runtime_error("FBO error");
    }

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void Application::initScreenTriangle() {
    glGenBuffers(1, &m_TriangleVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_TriangleVBO);

    GLfloat data[] = {-1, -1, 3, -1, -1, 3};
    glBufferStorage(GL_ARRAY_BUFFER, sizeof(data), data, 0);

    glGenVertexArrays(1, &m_TriangleVAO);
    glBindVertexArray(m_TriangleVAO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

////////////////////////////////////////////////////////////////////////////
///////////////////////// First Part animation /////////////////////////////
////////////////////////////////////////////////////////////////////////////

mat4 &Application::firstPartBigShip(mat4 &mvMatrix) {

    mvMatrix = interpolate(mvMatrix, translate(mvMatrix, m_coordBigShip), m_speed);
    return mvMatrix;
}

mat4 &Application::firstPartAWing1(mat4 &mvMatrix) {
    mvMatrix = translate(mvMatrix, m_SceneCenter);
    mvMatrix = glm::interpolate(mvMatrix, glm::translate(mvMatrix, m_coordAWing1), m_speed);
//    mvMatrix = glm::interpolate(mvMatrix,
//                                glm::rotate(mvMatrix, static_cast<float>(m_RotationSpeed * glfwGetTime()), glm::vec3(0, 1, 0)),
//                                1.f);
    return mvMatrix;
}

mat4 &Application::firstPartAWing3(mat4 &mvMatrix) {
    mvMatrix = translate(mvMatrix, m_SceneCenter);
    mvMatrix = glm::interpolate(mvMatrix, glm::translate(mvMatrix, m_coordAWing2), m_speed);
    return mvMatrix;
}

mat4 &Application::firstPartAWing2(mat4 &mvMatrix) {
    mvMatrix = translate(mvMatrix, m_SceneCenter);
    mvMatrix = glm::interpolate(mvMatrix, glm::translate(mvMatrix, m_coordAWing3), m_speed);
    return mvMatrix;
}

////////////////////////////////////////////////////////////////////////////
//////////////////////// Second Part animation /////////////////////////////
////////////////////////////////////////////////////////////////////////////

mat4 &Application::secondPartBigShip(mat4 &mvMatrix) {
    mvMatrix = firstPartBigShip(mvMatrix);
    // todo
//    mvMatrix = interpolate(mvMatrix, translate(mvMatrix, m_coordBigShip), m_speed);

    return mvMatrix;
}

mat4 &Application::secondPartAWing1(mat4 &mvMatrix) {
    mvMatrix = firstPartAWing1(mvMatrix);
    // todo
    mvMatrix = glm::interpolate(mvMatrix, glm::translate(mvMatrix, m_coordAWing1), m_speed);

    return mvMatrix;
}

mat4 &Application::secondPartAWing2(mat4 &mvMatrix) {
    mvMatrix = firstPartAWing2(mvMatrix);
    // todo
    mvMatrix = glm::interpolate(mvMatrix, glm::translate(mvMatrix, m_coordAWing2), m_speed);

    return mvMatrix;
}

mat4 &Application::secondPartAWing3(mat4 &mvMatrix) {
    mvMatrix = firstPartAWing3(mvMatrix);
    // todo
    mvMatrix = glm::interpolate(mvMatrix, glm::translate(mvMatrix, m_coordAWing3), m_speed);

    return mvMatrix;
}

////////////////////////////////////////////////////////////////////////////
///////////////////////// Third Part animation /////////////////////////////
////////////////////////////////////////////////////////////////////////////

mat4 &Application::thirdPartBigShip(mat4 &mvMatrix) {
    mvMatrix = secondPartBigShip(mvMatrix);
    // todo
    return mvMatrix;
}

mat4 &Application::thirdPartAWing1(mat4 &mvMatrix) {
    mvMatrix = secondPartAWing1(mvMatrix);
    // todo
    return mvMatrix;
}

mat4 &Application::thirdPartAWing2(mat4 &mvMatrix) {
    mvMatrix = secondPartAWing2(mvMatrix);
    // todo
    return mvMatrix;
}

mat4 &Application::thirdPartAWing3(mat4 &mvMatrix) {
    mvMatrix = secondPartAWing3(mvMatrix);
    // todo
    return mvMatrix;
}

////////////////////////////////////////////////////////////////////////////
///////////////////////// Fourth Part animation ////////////////////////////
////////////////////////////////////////////////////////////////////////////

mat4 &Application::fourthPartBigShip(mat4 &mvMatrix) {
    mvMatrix = thirdPartBigShip(mvMatrix);
    // todo
    return mvMatrix;
}

mat4 &Application::fourthPartAWing1(mat4 &mvMatrix) {
    mvMatrix = thirdPartAWing1(mvMatrix);
    // todo
    return mvMatrix;
}

mat4 &Application::fourthPartAWing2(mat4 &mvMatrix) {
    mvMatrix = thirdPartAWing2(mvMatrix);
    // todo
    return mvMatrix;
}

mat4 &Application::fourthPartAWing3(mat4 &mvMatrix) {
    mvMatrix = thirdPartAWing3(mvMatrix);
    // todo
    return mvMatrix;
}
////////////////////////////////////////////////////////////////////////////
///////////////////////// Fifth Part animation /////////////////////////////
////////////////////////////////////////////////////////////////////////////

mat4 &Application::fifthPartBigShip(mat4 &mvMatrix) {
    mvMatrix = fourthPartBigShip(mvMatrix);
    // todo
    return mvMatrix;
}

mat4 &Application::fifthPartAWing1(mat4 &mvMatrix) {
    mvMatrix = fourthPartAWing1(mvMatrix);
    // todo
    return mvMatrix;
}

mat4 &Application::fifthPartAWing2(mat4 &mvMatrix) {
    mvMatrix = fourthPartAWing2(mvMatrix);
    // todo
    return mvMatrix;
}

mat4 &Application::fifthPartAWing3(mat4 &mvMatrix) {
    mvMatrix = fourthPartAWing3(mvMatrix);
    // todo
    return mvMatrix;
}
////////////////////////////////////////////////////////////////////////////
///////////////////////// Sixth Part animation /////////////////////////////
////////////////////////////////////////////////////////////////////////////


mat4 &Application::sixthPartBigShip(mat4 &mvMatrix) {
    mvMatrix = fifthPartBigShip(mvMatrix);
    // todo
    return mvMatrix;
}

mat4 &Application::sixthPartAWing1(mat4 &mvMatrix) {
    mvMatrix = fifthPartAWing1(mvMatrix);
    // todo
    return mvMatrix;
}

mat4 &Application::sixthPartAWing2(mat4 &mvMatrix) {
    mvMatrix = fifthPartAWing2(mvMatrix);
    // todo
    return mvMatrix;
}

mat4 &Application::sixthPartAWing3(mat4 &mvMatrix) {
    mvMatrix = fifthPartAWing3(mvMatrix);
    // todo
    return mvMatrix;
}
