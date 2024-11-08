#include "application.h"

#include <imgui/imgui.h>

#include <framework/image.h>

Application::Application()
    : m_window("Final Project", glm::ivec2(1024, 1024), OpenGLVersion::GL41)
    , m_texture(RESOURCE_ROOT "resources/checkerboard.png")
    , m_mainCamera(&m_window, glm::vec3(0,0,-2), glm::vec3(0,0,2), glm::perspective(glm::radians(80.0f), 1.0f, 0.1f, 30.0f))
    , m_minimapCamera(&m_window, glm::vec3(0, 20, 0), glm::vec3(0, -10, 0), glm::ortho(0.f, 1024.0f, 0.f, 600.f, 0.1f, 50.f))
    , m_sunLight(DirectionalLight(glm::vec3(-1, -1, -1), glm::vec3(0.5, 0.5, 0.1), glm::vec3(1,1,1)))
    , m_bezierPathLight(PointLight(glm::vec3(0,0,0), glm::vec3(0.4, 0.4, 0.4), glm::vec3(0.8, 0.8, 0.8), 20))
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
    initSkybox();
    initLights();
}

void Application::initMeshes() 
{
    m_renderable.emplace_back(mergeMeshes(loadMesh("resources/dragon.obj")), MeshMovement::Static, glm::mat4{1.0f});
}

void Application::initLights()
{
    m_pointLights.emplace_back(glm::vec3(1, 0, 0), glm::vec3(0, 0.5, 0), glm::vec3(0, 1, 0), 20);
    m_pointLights.emplace_back(glm::vec3(-1, 0, 0), glm::vec3(0.5, 0, 0), glm::vec3(1, 0, 0), 20);
    m_pointLights.emplace_back(glm::vec3(0, 3, 0), glm::vec3(0, 0, 0.5), glm::vec3(0, 0, 1), 30);
}

void Application::initShaders()
{
    try {
        //m_defaultShader = ShaderBuilder().
        //    addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl").
        //    addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shader_frag.glsl").
        //    build();

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
            addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/blinn_or_phong_frag.glsl",
                "#define LIGHT_TYPE SPOT_LIGHT\n#define MAX_NUM_LIGHTS " + std::to_string(utils::globals::shader_preprocessor_params::MAX_NUM_SPOT_LIGHT)).
            build();


    }
    catch (ShaderLoadingException e) {
        std::cerr << e.what() << std::endl;
    }    
}

void Application::initBuffers()
{

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

void Application::drawSkybox() 
{
    m_skyboxShader.bind();
    glm::mat4 skyboxMatrix = m_mainCamera.projectionMatrix() * glm::mat4(glm::mat3(m_mainCamera.viewMatrix()));
    glUniformMatrix4fv(m_skyboxShader.getUniformLocation("viewProjMatrix"), 1, GL_FALSE, glm::value_ptr(skyboxMatrix));
    glUniform1i(m_skyboxShader.getUniformLocation("skybox"), 0);

    glBindVertexArray(m_skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void Application::update()
{
    glEnable(GL_DEPTH_TEST);

    int dummyInteger = 0; // Initialized to 0
    while (!m_window.shouldClose()) {
        // This is your game loop
        // Put your real-time logic and rendering in here

        // ==== UPDATE INPUT ====
        ImGuiIO io = ImGui::GetIO();
        m_window.updateInput();
        if (!io.WantCaptureMouse){
            m_mainCamera.updateInput();
        }

        // Use ImGui for easy input/output of ints, floats, strings, etc...
        ImGui::Begin("Window");
        ImGui::InputInt("This is an integer input", &dummyInteger); // Use ImGui::DragInt or ImGui::DragFloat for larger range of numbers.
        ImGui::Text("Value is: %i", dummyInteger); // Use C printf formatting rules (%i is a signed integer)
        ImGui::Checkbox("Use material if no texture", &m_useMaterial);
        
        ImGui::Checkbox("Show lights as points", &utils::globals::showLightsAsPoints);

        ImGui::End();

        // Clear the screen
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //// Fill depth buffer, but disable color writes
        //glDepthFunc(GL_LEQUAL); 
        ////glDepthMask(GL_TRUE); 
        //glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        //m_shadowShader.bind();
        //for (auto& renderable : m_renderable) {
        //    const glm::mat4& modelMatrix = std::get<2>(renderable);
        //    const glm::mat4 mvpMatrix = m_mainCamera.viewProjectionMatrix() * modelMatrix;

        //    glUniformMatrix4fv(m_shadowShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
        //    std::get<0>(renderable).draw(m_shadowShader);
        //}

        // //Now fill the color buffer, the depth buffer removes blending with the background
        //glDepthFunc(GL_EQUAL);
        //glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        glEnable(GL_BLEND); // Blending for multiple lights

        // Each light type (point, spot, directional) has its own shader
        // Each shader can handle a certain maximum number of lights defined in "utils.h"
        // Iterating through mesh first (i.e. outer for-loop) will introduce many state changes (shader program binding)
        // To minimize state changes, we iterate through each light type first
        bool firstPassCompleted = false;
        const int& maxPointLight = utils::globals::shader_preprocessor_params::MAX_NUM_POINT_LIGHT;
        for (int idx = 0; idx < m_pointLights.size(); idx += maxPointLight) {
            for (auto& renderable : m_renderable) {
                const glm::mat4& modelMatrix = std::get<2>(renderable);
                const glm::mat4 mvpMatrix = m_mainCamera.viewProjectionMatrix() * modelMatrix;
                const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(modelMatrix));

                m_blinnOrPhongPointLightShader.bind();
                glUniformMatrix4fv(m_blinnOrPhongPointLightShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
                glUniformMatrix4fv(m_blinnOrPhongPointLightShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
                glUniformMatrix3fv(m_blinnOrPhongPointLightShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));
                glUniform3fv(m_blinnOrPhongPointLightShader.getUniformLocation("viewPos"), 1, glm::value_ptr(m_mainCamera.cameraPos()));
                glUniform1i(m_blinnOrPhongPointLightShader.getUniformLocation("useBlinnCorrection"), GL_FALSE);

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
                if (!firstPassCompleted) {
                    glDepthFunc(GL_LESS);
                    glBlendFunc(GL_ONE, GL_ZERO);
                    //firstPassCompleted = true;
                }
                //else {
                //    glDepthFunc(GL_EQUAL);
                //    glBlendFunc(GL_ONE, GL_ONE);
                //}

                std::get<0>(renderable).draw(m_blinnOrPhongPointLightShader);

                if (!firstPassCompleted) {
                    glDepthFunc(GL_EQUAL);
                    glBlendFunc(GL_ONE, GL_ONE);
                    firstPassCompleted = true;
                }
            }
        }

        const int& maxSpotLight = utils::globals::shader_preprocessor_params::MAX_NUM_SPOT_LIGHT;


        glDisable(GL_BLEND);

        // Draw point lights as square points
        if (utils::globals::showLightsAsPoints) {
            glDepthFunc(GL_LESS);
            m_lightShader.bind();
            for (const PointLight& pointLight : m_pointLights) {
                const glm::vec4 screenPos = m_mainCamera.viewProjectionMatrix() * glm::vec4(pointLight.position, 1.0f);

                glPointSize(10.0f);
                glUniform4fv(m_lightShader.getUniformLocation("pos"), 1, glm::value_ptr(screenPos));
                glUniform3fv(m_lightShader.getUniformLocation("color"), 1, glm::value_ptr(pointLight.specularColor));
                glDrawArrays(GL_POINTS, 0, 1);
            }
        }

        // Render skybox last
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
    std::cout << "Key pressed: " << key << std::endl;
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
