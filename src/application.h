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


enum class StateType {
    Static,
    Dynamic
};

enum class DrawingMode {
    Opaque,
    Reflective,
};

struct Light {
    glm::vec3 diffuseColor;
    glm::vec3 specularColor;
    StateType lightType;
};

struct DirectionalLight : Light {
    glm::vec3 direction;

    DirectionalLight(const glm::vec3& dir,
        const glm::vec3& diffuse,
        const glm::vec3& specular,
        StateType type = StateType::Static)
        : direction(glm::normalize(dir))
    {
        diffuseColor = diffuse;
        specularColor = specular;
        lightType = type;
    }
};

// vec2 attenuation coefficients: first component is linear coefficient, second is quadratic coefficient, constant coefficient is 1.0
struct PointLight : Light {
    glm::vec3 position;
    glm::vec2 attenuationCoefficients;

    PointLight(const glm::vec3& pos,
        const glm::vec3& diffuse,
        const glm::vec3& specular,
        const float& maxDistance,
        StateType type = StateType::Static)
        : position(pos)
        , attenuationCoefficients(utils::math::getAttenuationCoefficient(maxDistance))
    {
        diffuseColor = diffuse;
        specularColor = specular;
        lightType = type;
    }
};

struct SpotLight : Light {
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec2 attenuationCoefficients;
    float innerCutoffAngle;
    float outerCutoffAngle;

    SpotLight(const glm::vec3& pos,
        const glm::vec3& dir,
        const float& innerCutoff,
        const float& outerCutoff,
        const glm::vec3& diffuse,
        const glm::vec3& specular,
        const float& maxDistance,
        StateType type = StateType::Static)
        : position(pos)
        , direction(glm::normalize(dir))
        , attenuationCoefficients(utils::math::getAttenuationCoefficient(maxDistance))
        , innerCutoffAngle(innerCutoff)
        , outerCutoffAngle(outerCutoff)
    {
        diffuseColor = diffuse;
        specularColor = specular;
        lightType = type;
    }
};

struct Transform {
    glm::mat4 localModelMatrix;
    Transform* parent; // nullptr when "this" transform is root

    glm::mat4 getGlobalTransform(){
        glm::mat4 globMatrix = localModelMatrix;
        Transform* iter = parent;
        while (iter != nullptr) {
            globMatrix = iter->localModelMatrix * globMatrix;
            iter = iter->parent;
        }
        return globMatrix;
    }
};

struct Renderable {
    GPUMesh mesh;
    glm::mat4 modelMat;
    std::optional<Texture> diffuseMap;
    std::optional<Texture> normalMap;
    StateType meshType;
    DrawingMode drawMode;
};


class Application {
public:
    Application();

    void initMeshes();
    void initShaders();
    void initBezierPath();
    void initSkybox();
    void initLights();
    void initEnvironmentMapping();
    void initHierarchicalTransform();

    void drawScene();
    void drawSkybox();
    void drawBezierPath();
    void drawLightsAsPoints();
    void updateBezierLightPosition();
    void updateHierarchicalTransform();
    void update();

    void onKeyPressed(int key, int mods);
    void onKeyReleased(int key, int mods);
    void onMouseMove(const glm::dvec2& cursorPos);
    void onMouseClicked(int button, int mods);
    void onMouseReleased(int button, int mods);

private:
    Window m_window;

    Shader m_lightShader;
    Shader m_shadowShader;
    Shader m_skyboxShader;
    Shader m_blinnOrPhongPointLightShader;
    Shader m_blinnOrPhongDirLightShader;
    Shader m_blinnOrPhongSpotLightShader;
    Shader m_bezierPathShader;
    Shader m_reflectionMapShader;

    Texture m_texture;
    bool m_useMaterial{ true };
    
    std::vector < Renderable> m_renderable;

    std::vector<PointLight> m_pointLights;
    std::vector<SpotLight> m_spotLights;
    DirectionalLight m_sunLight;

    bool m_firstCameraActive = true;
    Camera m_firstCamera;
    Camera m_secondCamera;

    // Bezier path
    GLuint m_bezierPathVAO;
    GLuint m_bezierPathVBO;

    // Skybox
    GLuint m_skyboxVAO;
    GLuint m_skyboxVBO;
    GLuint m_skyboxTex;

    // Hierarchical transform
    Renderable* sun;
    Renderable* planet;
    Renderable* moon;
    Transform m_sunTransform;
    Transform m_planetTransform;
    Transform m_moonTransform;

    // Environment mapping
    GLuint m_envMapFBO;
    GLuint m_preRenderedEnvMapTex;
    GLuint m_envMapTex;
};