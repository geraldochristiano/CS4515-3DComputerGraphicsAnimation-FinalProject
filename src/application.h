#pragma once

#include "mesh.h"
#include "texture.h"
#include "camera.h"
// Always include window first (because it includes glfw, which includes GL which needs to be included AFTER glew).
// Can't wait for modules to fix this stuff...
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glad/glad.h>
// Include glad before glfw3
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <imgui/imgui.h>
DISABLE_WARNINGS_POP()
#include <framework/shader.h>
#include <framework/window.h>

#include <functional>
#include <iostream>
#include <vector>


struct Light {
    glm::vec3 diffuseColor;
    glm::vec3 specularColor;

    glm::vec3 position;

    Light(const glm::vec3& pos, 
        const glm::vec3& diffuse = glm::vec3(0.5, 0.5, 0.5), 
        const glm::vec3& specular = glm::vec3(1, 1, 1))
        : position(pos)
        , diffuseColor(diffuse)
        , specularColor(specular)
    {}
};

enum class MeshMovement {
    Static, 
    Dynamic
};

struct Renderable {
    GPUMesh mesh;
    MeshMovement movementType;
    int instanceCount;
    glm::mat4 modelMatrix;
};

class Application {
public:
    Application();

    void initScene();
    void initShaders();
    void initBuffers();
    void initSkybox();

    void drawSkybox();
    void update();

    void onKeyPressed(int key, int mods);
    void onKeyReleased(int key, int mods);
    void onMouseMove(const glm::dvec2& cursorPos);
    void onMouseClicked(int button, int mods);
    void onMouseReleased(int button, int mods);

private:
    Window m_window;

    Shader m_defaultShader;
    Shader m_blinnOrPhongShader;
    Shader m_shadowShader;
    Shader m_skyboxShader;

    Texture m_texture;
    bool m_useMaterial{ true };
    std::vector<GPUMesh> m_meshes;
    std::vector<Light> m_lights;

    Camera m_mainCamera;
    Camera m_minimapCamera;

    // Skybox
    GLuint m_skyboxVAO;
    GLuint m_skyboxVBO;
    GLuint m_skyboxTex;

    glm::mat4 m_modelMatrix{ 1.0f };
};