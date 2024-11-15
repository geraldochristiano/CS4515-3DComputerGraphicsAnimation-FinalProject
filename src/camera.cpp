#include "camera.h"
// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
DISABLE_WARNINGS_POP()

#include <imgui/imgui.h>

Camera::Camera(Window* pWindow, glm::mat4 projectionMatrix)
    : Camera(pWindow, glm::vec3(0), glm::vec3(0, 0, -1), projectionMatrix)
{
}

Camera::Camera(Window* pWindow, const glm::vec3& pos, const glm::vec3& forward, glm::mat4 projectionMatrix)
    : m_position(pos)
    , m_forward(glm::normalize(forward))
    , m_pWindow(pWindow)
    , m_projectionMatrixCached(projectionMatrix)
{
}

void Camera::setUserInteraction(bool enabled)
{
    m_userInteraction = enabled;
}

glm::vec3 Camera::cameraPos() const
{
    return m_position;
}

glm::vec3 Camera::cameraForward() const
{
    return glm::normalize(m_forward);
}


glm::mat4 Camera::viewMatrix() const
{
    return glm::lookAt(m_position, m_position + m_forward, m_up);
}

glm::mat4 Camera::projectionMatrix() const
{
    return m_projectionMatrixCached;
}

glm::mat4 Camera::viewProjectionMatrix() const
{
    return m_projectionMatrixCached * viewMatrix();
}

void Camera::rotateX(float angle)
{
    const glm::vec3 horAxis = glm::cross(s_yAxis, m_forward);

    m_forward = glm::normalize(glm::angleAxis(angle, horAxis) * m_forward);
    m_up = glm::normalize(glm::cross(m_forward, horAxis));
}

void Camera::rotateY(float angle)
{
    const glm::vec3 horAxis = glm::cross(s_yAxis, m_forward);

    m_forward = glm::normalize(glm::angleAxis(angle, s_yAxis) * m_forward);
    m_up = glm::normalize(glm::cross(m_forward, horAxis));
}

void Camera::updateInput()
{
    constexpr float moveSpeed = 0.1f;
    constexpr float lookSpeed = 0.0035f;

    if (m_userInteraction) {
        glm::vec3 localMoveDelta{ 0 };
        const glm::vec3 right = glm::normalize(glm::cross(m_forward, m_up));
        const float movementMultiplier = m_pWindow->isKeyPressed(GLFW_KEY_LEFT_SHIFT) ? 0.1 : 1;
        if (m_pWindow->isKeyPressed(GLFW_KEY_A))
            m_position -= moveSpeed * right * movementMultiplier;
        if (m_pWindow->isKeyPressed(GLFW_KEY_D))
            m_position += moveSpeed * right * movementMultiplier;
        if (m_pWindow->isKeyPressed(GLFW_KEY_W))
            m_position += moveSpeed * m_forward * movementMultiplier;
        if (m_pWindow->isKeyPressed(GLFW_KEY_S))
            m_position -= moveSpeed * m_forward * movementMultiplier;
        if (m_pWindow->isKeyPressed(GLFW_KEY_R))
            m_position += moveSpeed * m_up * movementMultiplier;
        if (m_pWindow->isKeyPressed(GLFW_KEY_F))
            m_position -= moveSpeed * m_up * movementMultiplier;

        const glm::dvec2 cursorPos = m_pWindow->getCursorPos();
        const glm::vec2 delta = lookSpeed * glm::vec2(cursorPos - m_prevCursorPos);
        m_prevCursorPos = cursorPos;

        ImGuiIO io = ImGui::GetIO();
        if (!io.WantCaptureMouse) {
            if (m_pWindow->isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
                if (delta.x != 0.0f)
                    rotateY(delta.x);
                if (delta.y != 0.0f)
                    rotateX(delta.y);
            }
        }
    }
    else {
        m_prevCursorPos = m_pWindow->getCursorPos();
    }
}
