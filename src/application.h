#pragma once

#include "mesh.h"
#include "texture.h"
#include "camera.h"
#include "utils.h"
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
};

struct DirectionalLight : Light {
    glm::vec3 direction;

    DirectionalLight(const glm::vec3& dir,
        const glm::vec3& diffuse,
        const glm::vec3& specular)
        : direction(dir)
    {
        diffuseColor = diffuse;
        specularColor = specular;
    }
};

// vec2 attenuation coefficients: first component is linear coefficient, second is quadratic coefficient, constant coefficient is 1.0
struct PointLight : Light {
    glm::vec3 position;
    glm::vec2 attenuationCoefficients;

    PointLight(const glm::vec3& pos,
        const glm::vec3& diffuse,
        const glm::vec3& specular,
        const float& maxDistance)
        : position(pos)
        , attenuationCoefficients(utils::math::getAttenuationCoefficient(maxDistance))
    {
        diffuseColor = diffuse;
        specularColor = specular;
    }
};

struct SpotLight : Light {
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec2 attenuationCoefficients;
    float innerConeCutoffAngle;
    float outerConeCutoffAngle;

    SpotLight(const glm::vec3& pos,
        const glm::vec3& dir,
        const float& innerConeCutoff,
        const float& outerConeCutoff,
        const glm::vec3& diffuse,
        const glm::vec3& specular,
        const float& maxDistance)
        : position(pos)
        , direction(dir)
        , attenuationCoefficients(utils::math::getAttenuationCoefficient(maxDistance))
        , innerConeCutoffAngle(innerConeCutoff)
        , outerConeCutoffAngle(outerConeCutoff)
    {
        diffuseColor = diffuse;
        specularColor = specular;
    }
};



enum class MeshMovement {
    Static, 
    Dynamic
};

class Application {
public:
    Application();

    void initMeshes();
    void initShaders();
    void initBuffers();
    void initSkybox();
    void initLights();

    void drawSkybox();
    void update();

    void onKeyPressed(int key, int mods);
    void onKeyReleased(int key, int mods);
    void onMouseMove(const glm::dvec2& cursorPos);
    void onMouseClicked(int button, int mods);
    void onMouseReleased(int button, int mods);

private:
    Window m_window;

    //Shader m_defaultShader;
    Shader m_lightShader;
    Shader m_shadowShader;
    Shader m_skyboxShader;
    Shader m_blinnOrPhongPointLightShader;
    Shader m_blinnOrPhongDirLightShader;
    Shader m_blinnOrPhongSpotLightShader;

    Texture m_texture;
    bool m_useMaterial{ true };
    
    std::vector < std::tuple<GPUMesh, MeshMovement, glm::mat4> > m_renderable;
    std::vector<PointLight> m_pointLights;
    std::vector<SpotLight> m_spotLights;
    DirectionalLight m_sunLight;
    PointLight m_bezierPathLight;

    Camera m_mainCamera;
    Camera m_minimapCamera;

    // Skybox
    GLuint m_skyboxVAO;
    GLuint m_skyboxVBO;
    GLuint m_skyboxTex;


};