#include "application.h"

#include <framework/image.h>

Application::Application()
    : m_window("Final Project", glm::ivec2(1024, 1024), OpenGLVersion::GL41)
    , m_texture(RESOURCE_ROOT "resources/checkerboard.png")
    , m_firstCamera(&m_window, glm::vec3(0,2,-8), glm::vec3(0,0,4), glm::perspective(glm::radians(80.0f), 1.0f, 0.1f, 50.0f))
    , m_secondCamera(&m_window, glm::vec3(5, 5, 5), glm::vec3(-5, -5, -5), glm::perspective(glm::radians(80.0f), 1.0f, 0.1f, 50.0f))
    , m_sunLight(DirectionalLight(glm::vec3(-1, -1, -1), glm::vec3(1, 1, 1), glm::vec3(1,1,1)))
{
    m_window.registerKeyCallback([this](int key, int scancode, int action, int mods) {
        if (action == GLFW_PRESS)
            onKeyPressed(key, mods);
        else if (action == GLFW_RELEASE)
            onKeyReleased(key, mods);
    });
    m_window.registerMouseMoveCallback(std::bind(&Application::onMouseMove, this, std::placeholders::_1));
    m_window.registerMouseButtonCallback([this](int button, int action, int mods) {
        if (action == GLFW_PRESS)
            onMouseClicked(button, mods);
        else if (action == GLFW_RELEASE)
            onMouseReleased(button, mods);
    });

    initShaders();
    initMeshes();
    initLights();
    initBezierPath();
    initSkybox();
}

void Application::initMeshes() 
{
    m_renderable.emplace_back(mergeMeshes(loadMesh("resources/brickwall.obj")), glm::mat4(1.0f),
        Texture("resources/alley-brick-wall_albedo.png"), Texture("resources/alley-brick-wall_normal-ogl.png"));
    m_renderable.emplace_back(mergeMeshes(loadMesh("resources/dragoon.obj")), glm::scale(glm::translate(glm::mat4{1.0f}, { 0,0, -3 }), { 3,3,3 }), 
        std::nullopt, std::nullopt);
    m_renderable.emplace_back(mergeMeshes(loadMesh("resources/grassy_terrain.obj")), glm::mat4{1.0f},
        Texture("resources/grass1-albedo3.png"), std::nullopt);

    // ====== INITIALIZING HIERARCHICAL TRANSFORM MESHES ========
    sun = &m_renderable.emplace_back(mergeMeshes(loadMesh("resources/sphere.obj")), glm::mat4{ 1.0f },
        Texture("resources/2k_sun.jpg"), std::nullopt);
    m_sunTransform = { glm::translate(glm::mat4{ 1.0f }, { 0,8,0 }) * glm::scale(glm::mat4{1.0f}, {2,2,2}) , nullptr };
    sun->modelMat = m_sunTransform.getGlobalTransform();

    // Put planet relative to the sun
    planet = &m_renderable.emplace_back(mergeMeshes(loadMesh("resources/sphere.obj")), glm::mat4{ 1.0f } ,
        std::nullopt, std::nullopt);
    m_planetTransform = { glm::translate(glm::mat4{ 1.0f }, { 0, 0, -4 }) * glm::scale(glm::mat4{1.0f}, {0.5, 0.5, 0.5}) , &m_sunTransform };
    planet->modelMat = m_planetTransform.getGlobalTransform();

    // Put moon relative to the planet
    moon = &m_renderable.emplace_back(mergeMeshes(loadMesh("resources/sphere.obj")), glm::mat4{ 1.0f },
        std::nullopt, std::nullopt);
    m_moonTransform = { glm::translate(glm::mat4{ 1.0f }, { 0,2,0 }) * glm::scale(glm::mat4{1.0f}, {0.5, 0.5, 0.5}) , &m_planetTransform };
    moon->modelMat = m_moonTransform.getGlobalTransform();
}

void Application::initLights()
{
    // ======== INITIALIZING BEZIER POINT LIGHT =============
    // First index is always the Bezier path light
    m_pointLights.emplace_back(glm::vec3(0, 0, 0), glm::vec3(0.7, 0, 0), glm::vec3(1, 0, 0), 50);

    // (Other) point lights
    m_pointLights.emplace_back(glm::vec3(3, 2, -4), glm::vec3(0.6,0.6,0), glm::vec3(1, 1, 0), 50);
    m_pointLights.emplace_back(glm::vec3(-3, 2, -4), glm::vec3(0.2, 0.7, 0.7), glm::vec3(0.7, 1, 1), 50);

    // Spot lights
    m_spotLights.emplace_back(glm::vec3(0,2, 4), glm::vec3(0,-1,-3), glm::radians(12.5f), glm::radians(17.5f), glm::vec3(0.8, 0.8, 0.8), glm::vec3(1, 1, 1), 50);
}

void Application::initShaders()
{
    try {
        m_lightShader = ShaderBuilder().
            addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/light_vert.glsl").
            addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/light_frag.glsl").
            build();

        m_shadowShader = ShaderBuilder().
            addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shadow_vert.glsl").
            addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shadow_frag.glsl").
            build();

        m_skyboxShader = ShaderBuilder().
            addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/skybox_vert.glsl").
            addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/skybox_frag.glsl").
            build();

        m_blinnOrPhongPointLightShader = ShaderBuilder().
            addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl").
            addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/blinn_or_phong_frag.glsl", 
                "#define LIGHT_TYPE POINT_LIGHT\n#define MAX_NUM_LIGHTS " + std::to_string(utils::globals::shader_preprocessor_params::MAX_NUM_POINT_LIGHT)).
            build();

        m_blinnOrPhongDirLightShader = ShaderBuilder().
            addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl").
            addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/blinn_or_phong_frag.glsl",
                "#define LIGHT_TYPE DIRECTIONAL_LIGHT\n#define MAX_NUM_LIGHTS " + std::to_string(utils::globals::shader_preprocessor_params::MAX_NUM_DIR_LIGHT)).
            build();

        m_blinnOrPhongSpotLightShader = ShaderBuilder().
            addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl").
            addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/blinn_or_phong_spot_frag.glsl",
                "#define LIGHT_TYPE SPOT_LIGHT\n#define MAX_NUM_LIGHTS " + std::to_string(utils::globals::shader_preprocessor_params::MAX_NUM_SPOT_LIGHT)).
            build();

        m_bezierPathShader = ShaderBuilder().
            addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/bezier_curve_vert.glsl").
            addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/bezier_curve_frag.glsl").
            build();
    }
    catch (ShaderLoadingException e) {
        std::cerr << e.what() << std::endl;
    }    
}

void Application::initBezierPath()
{
    std::vector<glm::vec3> bezierPathPointsPos;
    float t = 0;
    for (int i = 0; i < utils::globals::bezier_path::frameCount + 1; i++) {
        using namespace utils::globals::bezier_path;
        bezierPathPointsPos.push_back(utils::math::cubicBezier(t, control_point0, control_point1, control_point2, control_point3));
        t += 1.0 / frameCount;
    }

    glGenVertexArrays(1, &m_bezierPathVAO);
    glBindVertexArray(m_bezierPathVAO);

    glGenBuffers(1, &m_bezierPathVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_bezierPathVBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(bezierPathPointsPos.size() * sizeof(glm::vec3)), bezierPathPointsPos.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribDivisor(0, 0);
}

void Application::initSkybox()
{
    Image right{ utils::globals::skybox_params::SKYBOX_RIGHT_IMG};
    Image left{ utils::globals::skybox_params::SKYBOX_LEFT_IMG };
    Image top{ utils::globals::skybox_params::SKYBOX_TOP_IMG };
    Image bottom{ utils::globals::skybox_params::SKYBOX_BOTTOM_IMG };
    Image front{ utils::globals::skybox_params::SKYBOX_FRONT_IMG };
    Image back{ utils::globals::skybox_params::SKYBOX_BACK_IMG };

    auto getFormat = [](int channels) {return channels == 4 ? GL_RGBA : GL_RGB; };
    auto internalFormat = front.channels == 4 ? GL_RGBA8 : GL_RGB8; // Cubemap completeness require the same internal format
    auto imageDatatype = utils::globals::skybox_params::SKYBOX_IMG_DATATYPE;
    
    // Set up cubemap texture for skybox 
    glGenTextures(1, &m_skyboxTex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_skyboxTex);

    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, internalFormat, right.width, right.height, 0, getFormat(right.channels), imageDatatype , right.get_data());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, internalFormat, left.width, left.height, 0, getFormat(left.channels), imageDatatype, left.get_data());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, internalFormat, top.width, top.height, 0, getFormat(top.channels), imageDatatype, top.get_data());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, internalFormat, bottom.width, bottom.height, 0, getFormat(bottom.channels), imageDatatype, bottom.get_data());
    // Cubemap uses left hand coordinate system
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, internalFormat, front.width, front.height, 0, getFormat(front.channels), imageDatatype, front.get_data());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, internalFormat, back.width, back.height, 0, getFormat(back.channels), imageDatatype, back.get_data());

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // Set up VAO and VBO of skybox corners
    glGenVertexArrays(1, &m_skyboxVAO);
    glGenBuffers(1, &m_skyboxVBO);
    glBindVertexArray(m_skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(utils::skyboxVertices), &utils::skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
}

void Application::initEnvironmentMapping()
{
    glGenFramebuffers(1, &m_mirrorFBO);

    glGenTextures(1, &m_mirrorTex);
}

void Application::drawSkybox() 
{
    Camera& activeCamera = m_firstCameraActive ? m_firstCamera : m_secondCamera;

    m_skyboxShader.bind();
    glm::mat4 skyboxMatrix = activeCamera.projectionMatrix() * glm::mat4(glm::mat3(activeCamera.viewMatrix()));
    glUniformMatrix4fv(m_skyboxShader.getUniformLocation("viewProjMatrix"), 1, GL_FALSE, glm::value_ptr(skyboxMatrix));

    int textureUnit = 0;
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_skyboxTex);
    glUniform1i(m_skyboxShader.getUniformLocation("skybox"), textureUnit);

    glBindVertexArray(m_skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void Application::drawBezierPath() {
    Camera& activeCamera = m_firstCameraActive ? m_firstCamera : m_secondCamera;

    m_bezierPathShader.bind();
    const glm::mat4 mvpMatrix = activeCamera.viewProjectionMatrix() * glm::mat4(1.0f);
    const glm::vec3 lineColor{ 1,0,0 };

    glUniformMatrix4fv(m_bezierPathShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
    glUniform3fv(m_bezierPathShader.getUniformLocation("color"), 1, glm::value_ptr(lineColor));

    glLineWidth(10.0f); // Driver implementation dependent, only 1.0f is guaranteed by the spec
    glBindVertexArray(m_bezierPathVAO);
    glDrawArrays(GL_LINE_STRIP, 0, utils::globals::bezier_path::frameCount + 1);
}


void Application::drawScene()
{
    Camera& activeCamera = m_firstCameraActive ? m_firstCamera : m_secondCamera;

    // Fill depth buffer, but disable color writes
    glDepthFunc(GL_LEQUAL); 
    glDepthMask(GL_TRUE); 
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    m_shadowShader.bind();
    for (auto& renderable : m_renderable) {
        const glm::mat4 modelMatrix = renderable.modelMat;
        const glm::mat4 mvpMatrix = activeCamera.viewProjectionMatrix() * modelMatrix;

        glUniformMatrix4fv(m_shadowShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
        renderable.mesh.draw(m_shadowShader);
    }

    glDepthFunc(GL_LEQUAL);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glEnable(GL_BLEND); // Blending for multiple lights
    glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ZERO);

     //bool firstPassCompleted = false;
    
    // Each light type (point, spot, directional) has its own shader
    // Each shader can handle a certain maximum number of lights defined in "utils.h"
    // Iterating through mesh first (i.e. outer for-loop) will introduce many state changes (shader program binding)
    // To minimize state changes, we iterate through each light type first

    // ======== POINT LIGHT =========
    const int& maxPointLight = utils::globals::shader_preprocessor_params::MAX_NUM_POINT_LIGHT;
    m_blinnOrPhongPointLightShader.bind();
    for (int idx = 0; idx < m_pointLights.size(); idx += maxPointLight) {
        for (auto& renderable : m_renderable) {
            const glm::mat4 modelMatrix = renderable.modelMat;
            const glm::mat4 mvpMatrix = activeCamera.viewProjectionMatrix() * modelMatrix;
            const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(modelMatrix));
            glUniformMatrix4fv(m_blinnOrPhongPointLightShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
            glUniformMatrix4fv(m_blinnOrPhongPointLightShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
            glUniformMatrix3fv(m_blinnOrPhongPointLightShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));
            glUniform3fv(m_blinnOrPhongPointLightShader.getUniformLocation("viewPos"), 1, glm::value_ptr(activeCamera.cameraPos()));
            glUniform1i(m_blinnOrPhongPointLightShader.getUniformLocation("useBlinnCorrection"), utils::globals::useBlinnCorrection);
            // ======= DIFFUSE MAP AND NORMAL MAP UNIFORMS ========
            if (renderable.diffuseMap.has_value() && utils::globals::useDiffuseMap) {
                glUniform1i(m_blinnOrPhongPointLightShader.getUniformLocation("hasDiffuseMap"), GL_TRUE);
                renderable.diffuseMap.value().bind(GL_TEXTURE0);
                glUniform1i(m_blinnOrPhongPointLightShader.getUniformLocation("diffuseMap"), 0);
            } else {
                glUniform1i(m_blinnOrPhongPointLightShader.getUniformLocation("hasDiffuseMap"), GL_FALSE);
            }

            if (renderable.normalMap.has_value() && utils::globals::useNormalMap) {
                glUniform1i(m_blinnOrPhongPointLightShader.getUniformLocation("hasNormalMap"), GL_TRUE);
                renderable.normalMap.value().bind(GL_TEXTURE1);
                glUniform1i(m_blinnOrPhongPointLightShader.getUniformLocation("normalMap"), 1);
            } else {
                glUniform1i(m_blinnOrPhongPointLightShader.getUniformLocation("hasNormalMap"), GL_FALSE);
            }
            // ======== LIGHT UNIFORMS ==========
            int j = 0;
            for (; j < maxPointLight && idx + j < m_pointLights.size(); j++) {
                PointLight& current = m_pointLights[idx + j];
                glUniform3fv(m_blinnOrPhongPointLightShader.getUniformLocation("lights[" + std::to_string(j) + "].lightPos"), 1, glm::value_ptr(current.position));
                glUniform1f(m_blinnOrPhongPointLightShader.getUniformLocation("lights[" + std::to_string(j) + "].linearAttenuationCoeff"), current.attenuationCoefficients.x);
                glUniform1f(m_blinnOrPhongPointLightShader.getUniformLocation("lights[" + std::to_string(j) + "].quadraticAttenuationCoeff"), current.attenuationCoefficients.y);
                glUniform3fv(m_blinnOrPhongPointLightShader.getUniformLocation("lights[" + std::to_string(j) + "].lightDiffuseColor"), 1, glm::value_ptr(current.diffuseColor));
                glUniform3fv(m_blinnOrPhongPointLightShader.getUniformLocation("lights[" + std::to_string(j) + "].lightSpecularColor"), 1, glm::value_ptr(current.specularColor));
            }
            glUniform1i(m_blinnOrPhongPointLightShader.getUniformLocation("numLights"), j);

            // With opaque mesh blending, don't blend with background on first pass
            //if (!firstPassCompleted) {
            //    glDepthFunc(GL_LESS);
            //    glBlendFunc(GL_ONE, GL_ZERO);
            //}

            renderable.mesh.draw(m_blinnOrPhongPointLightShader);

            //if (!firstPassCompleted) {
            //    glDepthFunc(GL_LEQUAL);
            //    glBlendFunc(GL_ONE, GL_ONE);
            //    firstPassCompleted = true;
            //}
        }
    }

    // ======== SPOT LIGHT =========
    const int& maxSpotLight = utils::globals::shader_preprocessor_params::MAX_NUM_SPOT_LIGHT;
    m_blinnOrPhongSpotLightShader.bind();
    for (int idx = 0; idx < m_spotLights.size(); idx += maxSpotLight) {
        for (auto& renderable : m_renderable) {
            const glm::mat4 modelMatrix = renderable.modelMat;
            const glm::mat4 mvpMatrix = activeCamera.viewProjectionMatrix() * modelMatrix;
            const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(modelMatrix));
            glUniformMatrix4fv(m_blinnOrPhongSpotLightShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
            glUniformMatrix4fv(m_blinnOrPhongSpotLightShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
            glUniformMatrix3fv(m_blinnOrPhongSpotLightShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));
            glUniform3fv(m_blinnOrPhongSpotLightShader.getUniformLocation("viewPos"), 1, glm::value_ptr(activeCamera.cameraPos()));
            glUniform1i(m_blinnOrPhongSpotLightShader.getUniformLocation("useBlinnCorrection"), utils::globals::useBlinnCorrection);
            // ======= DIFFUSE MAP AND NORMAL MAP UNIFORMS ========
            if (renderable.diffuseMap.has_value() && utils::globals::useDiffuseMap) {
                glUniform1i(m_blinnOrPhongSpotLightShader.getUniformLocation("hasDiffuseMap"), GL_TRUE);
                renderable.diffuseMap.value().bind(GL_TEXTURE0);
                glUniform1i(m_blinnOrPhongSpotLightShader.getUniformLocation("diffuseMap"), 0);
            }
            else {
                glUniform1i(m_blinnOrPhongSpotLightShader.getUniformLocation("hasDiffuseMap"), GL_FALSE);
            }

            if (renderable.normalMap.has_value() && utils::globals::useNormalMap) {
                glUniform1i(m_blinnOrPhongSpotLightShader.getUniformLocation("hasNormalMap"), GL_TRUE);
                renderable.normalMap.value().bind(GL_TEXTURE1);
                glUniform1i(m_blinnOrPhongSpotLightShader.getUniformLocation("normalMap"), 1);
            }
            else {
                glUniform1i(m_blinnOrPhongSpotLightShader.getUniformLocation("hasNormalMap"), GL_FALSE);
            }
            // ======== LIGHT UNIFORMS ==========
            int j = 0;
            for (; j < maxSpotLight && idx + j < m_spotLights.size(); j++) {
                SpotLight& current = m_spotLights[idx + j];
                glUniform3fv(m_blinnOrPhongSpotLightShader.getUniformLocation("lights[" + std::to_string(j) + "].lightPos"), 1, glm::value_ptr(current.position));
                glUniform3fv(m_blinnOrPhongSpotLightShader.getUniformLocation("lights[" + std::to_string(j) + "].lightDir"), 1, glm::value_ptr(current.direction));
                glUniform1f(m_blinnOrPhongSpotLightShader.getUniformLocation("lights[" + std::to_string(j) + "].innerCutoff"), glm::cos(current.innerCutoffAngle));
                glUniform1f(m_blinnOrPhongSpotLightShader.getUniformLocation("lights[" + std::to_string(j) + "].outerCutoff"), glm::cos(current.outerCutoffAngle));
                glUniform1f(m_blinnOrPhongSpotLightShader.getUniformLocation("lights[" + std::to_string(j) + "].linearAttenuationCoeff"), current.attenuationCoefficients.x);
                glUniform1f(m_blinnOrPhongSpotLightShader.getUniformLocation("lights[" + std::to_string(j) + "].quadraticAttenuationCoeff"), current.attenuationCoefficients.y);
                glUniform3fv(m_blinnOrPhongSpotLightShader.getUniformLocation("lights[" + std::to_string(j) + "].lightDiffuseColor"), 1, glm::value_ptr(current.diffuseColor));
                glUniform3fv(m_blinnOrPhongSpotLightShader.getUniformLocation("lights[" + std::to_string(j) + "].lightSpecularColor"), 1, glm::value_ptr(current.specularColor));
            }
            glUniform1i(m_blinnOrPhongSpotLightShader.getUniformLocation("numLights"), j);

            renderable.mesh.draw(m_blinnOrPhongSpotLightShader);
        }
    }

     // ==== DIRECTIONAL LIGHT : SUNLIGHT =====
    if (utils::globals::sunlight)
    {
        m_blinnOrPhongDirLightShader.bind();
        for (auto& renderable : m_renderable) {
            const glm::mat4 modelMatrix = renderable.modelMat;
            const glm::mat4 mvpMatrix = activeCamera.viewProjectionMatrix() * modelMatrix;
            const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(modelMatrix));
            glUniformMatrix4fv(m_blinnOrPhongDirLightShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
            glUniformMatrix4fv(m_blinnOrPhongDirLightShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
            glUniformMatrix3fv(m_blinnOrPhongDirLightShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));
            glUniform3fv(m_blinnOrPhongDirLightShader.getUniformLocation("viewPos"), 1, glm::value_ptr(activeCamera.cameraPos()));
            glUniform1i(m_blinnOrPhongDirLightShader.getUniformLocation("useBlinnCorrection"), utils::globals::useBlinnCorrection);

            // ======= DIFFUSE MAP AND NORMAL MAP UNIFORMS ========
            if (renderable.diffuseMap.has_value() && utils::globals::useDiffuseMap) {
                glUniform1i(m_blinnOrPhongDirLightShader.getUniformLocation("hasDiffuseMap"), GL_TRUE);
                renderable.diffuseMap.value().bind(GL_TEXTURE0);
                glUniform1i(m_blinnOrPhongDirLightShader.getUniformLocation("diffuseMap"), 0);
            }
            else {
                glUniform1i(m_blinnOrPhongDirLightShader.getUniformLocation("hasDiffuseMap"), GL_FALSE);
            }

            if (renderable.normalMap.has_value() && utils::globals::useNormalMap) {
                glUniform1i(m_blinnOrPhongDirLightShader.getUniformLocation("hasNormalMap"), GL_TRUE);
                renderable.normalMap.value().bind(GL_TEXTURE1);
                glUniform1i(m_blinnOrPhongDirLightShader.getUniformLocation("normalMap"), 1);
            }
            else {
                glUniform1i(m_blinnOrPhongDirLightShader.getUniformLocation("hasNormalMap"), GL_FALSE);
            }

            glUniform3fv(m_blinnOrPhongDirLightShader.getUniformLocation("lights[0].lightDir"), 1, glm::value_ptr(m_sunLight.direction));
            glUniform3fv(m_blinnOrPhongDirLightShader.getUniformLocation("lights[0].lightDiffuseColor"), 1, glm::value_ptr(m_sunLight.diffuseColor));
            glUniform3fv(m_blinnOrPhongDirLightShader.getUniformLocation("lights[0].lightSpecularColor"), 1, glm::value_ptr(m_sunLight.specularColor));
            glUniform1i(m_blinnOrPhongDirLightShader.getUniformLocation("numLights"), 1);

            renderable.mesh.draw(m_blinnOrPhongDirLightShader);
        }
    }

    glDisable(GL_BLEND);
}

void Application::drawLightsAsPoints() 
{
    Camera& activeCamera = m_firstCameraActive ? m_firstCamera : m_secondCamera;

    glDepthFunc(GL_LESS);
    m_lightShader.bind();
    for (const PointLight& pointLight : m_pointLights) {
        const glm::vec4 screenPos = activeCamera.viewProjectionMatrix() * glm::vec4(pointLight.position, 1.0f);
        glUniform4fv(m_lightShader.getUniformLocation("pos"), 1, glm::value_ptr(screenPos));

        glPointSize(utils::globals::lightPointSize);
        glUniform3fv(m_lightShader.getUniformLocation("color"), 1, glm::value_ptr(pointLight.specularColor));
        glDrawArrays(GL_POINTS, 0, 1);
    }

    for (const SpotLight& spotLight : m_spotLights) {
        const glm::vec4 screenPos = activeCamera.viewProjectionMatrix() * glm::vec4(spotLight.position, 1.0f);
        glUniform4fv(m_lightShader.getUniformLocation("pos"), 1, glm::value_ptr(screenPos));

        glPointSize(utils::globals::lightPointSize);
        glUniform3fv(m_lightShader.getUniformLocation("color"), 1, glm::value_ptr(spotLight.specularColor));
        glDrawArrays(GL_POINTS, 0, 1);
    }
}
void Application::updateBezierLightPosition() 
{
    if (utils::globals::pauseBezierPath) return;
    //for (int i = 0; i < curveCount; i += 1) {d
//    if (timestep <= (i + 1.0)) {
//        bezierLight.position = utils::math::cubicBezier(timestep - i, control_points[3 * i], 
//                                                                    control_points[3 * i + 1], 
//                                                                    control_points[3 * i + 2], 
//                                                                    control_points[3 * i + 3]);

//    }
//}
    PointLight& bezierLight = m_pointLights[0];
    using namespace utils::globals::bezier_path;

    if (timestep <= 1.0)
        bezierLight.position = utils::math::cubicBezier(timestep, control_point0, control_point1, control_point2, control_point3);
    else if (timestep <= 2.0)
        bezierLight.position = utils::math::cubicBezier(timestep - 1.0, control_point3, control_point4, control_point5, control_point6);
    else if (timestep <= 3.0)
        bezierLight.position = utils::math::cubicBezier(timestep - 2.0, control_point6, control_point7, control_point8, control_point9);
    else if (timestep <= 4.0)
        bezierLight.position = utils::math::cubicBezier(timestep - 3.0, control_point9, control_point10, control_point11, control_point12);

    timestep += 1.0 / frameCount;
    if (timestep >= float(curveCount)) {
        timestep = 0;
    }
}

void Application::updateHierarchyTransform() 
{
    if (utils::globals::pauseHierarchyTransform) return;

    m_planetTransform.localModelMatrix = glm::rotate(glm::mat4{ 1.0f }, glm::radians(1.0f), {0,1,0}) * m_planetTransform.localModelMatrix;
    m_moonTransform.localModelMatrix = glm::rotate(glm::mat4{ 1.0f }, glm::radians(1.0f), {1,0,0}) * m_moonTransform.localModelMatrix;

    planet->modelMat = m_planetTransform.getGlobalTransform();
    moon->modelMat = m_moonTransform.getGlobalTransform();
}

void Application::update()
{
    glEnable(GL_DEPTH_TEST);

    int dummyInteger = 0; // Initialized to 0
    while (!m_window.shouldClose()) {
        // This is your game loop
        // Put your real-time logic and rendering in here

        // ==== UPDATE STUFF ====
        m_window.updateInput();
        Camera& activeCamera = m_firstCameraActive ? m_firstCamera : m_secondCamera;
        activeCamera.updateInput();
        updateBezierLightPosition();
        updateHierarchyTransform();

        // Use ImGui for easy input/output of ints, floats, strings, etc...
        ImGui::Begin("Window");

        ImGui::Separator();
        ImGui::Checkbox("Show lights as points", &utils::globals::showLightsAsPoints);

        std::string bezierPathPlaybackTxt = utils::globals::pauseBezierPath ? "Resume Bezier path" : "Pause Bezier path";
        if (ImGui::Button(bezierPathPlaybackTxt.c_str())) { utils::globals::pauseBezierPath = !utils::globals::pauseBezierPath; };

        std::string hierarchyTransformPlaybackTxt = utils::globals::pauseHierarchyTransform ? "Resume hierarchy transform" : "Pause hierarchy transform";
        if (ImGui::Button(hierarchyTransformPlaybackTxt.c_str())) { utils::globals::pauseHierarchyTransform = !utils::globals::pauseHierarchyTransform; };

        ImGui::Separator();
        ImGui::Text("SHADING");
        ImGui::Checkbox("Use Blinn-Phong", &utils::globals::useBlinnCorrection);
        ImGui::Checkbox("Use diffuse map", &utils::globals::useDiffuseMap);
        ImGui::Checkbox("Use normal map", &utils::globals::useNormalMap);
        ImGui::Checkbox("Sunlight", &utils::globals::sunlight);

        ImGui::End();

        // Clear the screen
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        drawScene();
        drawBezierPath();

        // Draw point lights and spotlights as square points
        if (utils::globals::showLightsAsPoints) {
            drawLightsAsPoints();
        }

        // Draw skybox last for optimization
        glDepthFunc(GL_LEQUAL);
        drawSkybox();
        glDepthFunc(GL_LESS);

        // Processes input and swaps the window buffer
        m_window.swapBuffers();
    }
}

// In here you can handle key presses
// key - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__keys.html
// mods - Any modifier keys pressed, like shift or control
void Application::onKeyPressed(int key, int mods)
{
    if (key == GLFW_KEY_SPACE) {
        m_firstCameraActive = !m_firstCameraActive;
    }
}

// In here you can handle key releases
// key - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__keys.html
// mods - Any modifier keys pressed, like shift or control
void Application::onKeyReleased(int key, int mods)
{
    std::cout << "Key released: " << key << std::endl;
}

// If the mouse is moved this function will be called with the x, y screen-coordinates of the mouse
void Application::onMouseMove(const glm::dvec2& cursorPos)
{
    std::cout << "Mouse at position: " << cursorPos.x << " " << cursorPos.y << std::endl;
}

// If one of the mouse buttons is pressed this function will be called
// button - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__buttons.html
// mods - Any modifier buttons pressed
void Application::onMouseClicked(int button, int mods)
{
    std::cout << "Pressed mouse button: " << button << std::endl;
}

// If one of the mouse buttons is released this function will be called
// button - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__buttons.html
// mods - Any modifier buttons pressed
void Application::onMouseReleased(int button, int mods)
{
    std::cout << "Released mouse button: " << button << std::endl;
}

int main()
{
    Application app;
    app.update();

    return 0;
}
