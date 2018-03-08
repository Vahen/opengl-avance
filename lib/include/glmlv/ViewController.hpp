#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct GLFWwindow;

namespace glmlv {

    class ViewController {
    public:
        ViewController(GLFWwindow *window, float speed = 1.f) :
                m_pWindow(window), m_fSpeed(speed) {
        }

        void setSpeed(float speed) {
            m_fSpeed = speed;
        }

        float getSpeed() const {
            return m_fSpeed;
        }

        void increaseSpeed(float delta) {
            m_fSpeed += delta;
            m_fSpeed = glm::max(m_fSpeed, 0.f);
        }

        float getCameraSpeed() const {
            return m_fSpeed;
        }

        bool update(float elapsedTime);

        void setViewMatrix(const glm::mat4 &viewMatrix) {
            m_ViewMatrix = viewMatrix;
            m_RcpViewMatrix = glm::inverse(viewMatrix);
        }

        const glm::mat4 &getViewMatrix() const {
            return m_ViewMatrix;
        }

        const glm::mat4 &getRcpViewMatrix() const {
            return m_RcpViewMatrix;
        }

        // /!\ Utiliser des valeurs très faibles
        void translateFront(float dist) {
            auto frontVector = -glm::vec3(m_RcpViewMatrix[2]);
            auto leftVector = -glm::vec3(m_RcpViewMatrix[0]);
            auto upVector = glm::vec3(m_RcpViewMatrix[1]);
            glm::vec3 localTranslationVector(0.f);
            localTranslationVector += m_fSpeed * dist * 0.01f * frontVector;
            m_position += localTranslationVector;
            setViewMatrix(lookAt(m_position, m_position + frontVector, upVector));
        }

        void translateLeft(float dist) {
            auto frontVector = -glm::vec3(m_RcpViewMatrix[2]);
            auto leftVector = -glm::vec3(m_RcpViewMatrix[0]);
            auto upVector = glm::vec3(m_RcpViewMatrix[1]);
            glm::vec3 localTranslationVector(0.f);
            localTranslationVector += m_fSpeed * dist * 0.01f * leftVector;
            m_position += localTranslationVector;
            setViewMatrix(lookAt(m_position, m_position + frontVector, upVector));
        }

        void rotateOnSelf(float deg) {
            auto frontVector = -glm::vec3(m_RcpViewMatrix[2]);
            auto leftVector = -glm::vec3(m_RcpViewMatrix[0]);
            auto upVector = glm::vec3(m_RcpViewMatrix[1]);
            float lateralAngleDelta = 0.f;
            lateralAngleDelta += deg * 0.001f;
            m_Orientation += glm::vec3(0,0,lateralAngleDelta);
            doNotGoOverFullOrientation();
            auto newRcpViewMatrix = m_RcpViewMatrix;
            newRcpViewMatrix = rotate(newRcpViewMatrix, lateralAngleDelta, glm::vec3(0, 0, 1));
            frontVector = -glm::vec3(newRcpViewMatrix[2]);
            leftVector = -glm::vec3(newRcpViewMatrix[0]);
            upVector = cross(frontVector, leftVector);
            setViewMatrix(lookAt(m_position, m_position + frontVector, upVector));
        }

        void rotateUp(float deg) {
            auto frontVector = -glm::vec3(m_RcpViewMatrix[2]);
            auto leftVector = -glm::vec3(m_RcpViewMatrix[0]);
            auto upVector = glm::vec3(m_RcpViewMatrix[1]);
            float lateralAngleDelta = 0.f;
            lateralAngleDelta += deg * 0.001f;
            m_Orientation += glm::vec3(lateralAngleDelta,0,0);
            doNotGoOverFullOrientation();
            auto newRcpViewMatrix = m_RcpViewMatrix;
            newRcpViewMatrix = rotate(newRcpViewMatrix, lateralAngleDelta, glm::vec3(1, 0, 0));
            frontVector = -glm::vec3(newRcpViewMatrix[2]);
            leftVector = -glm::vec3(newRcpViewMatrix[0]);
            upVector = cross(frontVector, leftVector);
            setViewMatrix(lookAt(m_position, m_position + frontVector, upVector));
        }

        void rotateLeft(float deg) {
            auto frontVector = -glm::vec3(m_RcpViewMatrix[2]);
            auto leftVector = -glm::vec3(m_RcpViewMatrix[0]);
            auto upVector = glm::vec3(m_RcpViewMatrix[1]);
            float lateralAngleDelta = 0.f;
            lateralAngleDelta += deg * 0.001f;
            m_Orientation += glm::vec3(0,lateralAngleDelta,0);
            doNotGoOverFullOrientation();
            auto newRcpViewMatrix = m_RcpViewMatrix;
            newRcpViewMatrix = rotate(newRcpViewMatrix, lateralAngleDelta, glm::vec3(0, 1, 0));
            frontVector = -glm::vec3(newRcpViewMatrix[2]);
            leftVector = -glm::vec3(newRcpViewMatrix[0]);
            upVector = cross(frontVector, leftVector);
            setViewMatrix(lookAt(m_position, m_position + frontVector, upVector));
        }

        void setPosition(glm::vec3 position) {
            auto frontVector = -glm::vec3(m_RcpViewMatrix[2]);
            auto leftVector = -glm::vec3(m_RcpViewMatrix[0]);
            auto upVector = cross(frontVector, leftVector);
            m_position = position;
            setViewMatrix(lookAt(m_position, m_position + frontVector, upVector));
        }

        void doNotGoOverFullOrientation() {
            m_Orientation.x = glm::mod(m_Orientation.x,360.f);
            m_Orientation.y = glm::mod(m_Orientation.y,360.f);
            m_Orientation.z = glm::mod(m_Orientation.z,360.f);
        }
        const glm::vec3 &getM_position() const {
            return m_position;
        }

        const glm::vec3 &getM_Orientation() const {
            return m_Orientation;
        }

    private:
        GLFWwindow *m_pWindow = nullptr;
        float m_fSpeed = 0.f;
        bool m_LeftButtonPressed = false;
        glm::dvec2 m_LastCursorPosition;

        glm::mat4 m_ViewMatrix = glm::mat4(1);
        glm::mat4 m_RcpViewMatrix = glm::mat4(1);
        glm::vec3 m_position = glm::vec3();
        glm::vec3 m_Orientation = glm::vec3();
    };

}