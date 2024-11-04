#include "application.h"
#include "utils.h"

#include <framework/image.h>

Application::Application()
    : m_window("Final Project", glm::ivec2(1024, 1024), OpenGLVersion::GL41)
    , m_texture(RESOURCE_ROOT "resources/checkerboard.png")
    , m_mainCamera(&m_window, glm::vec3(0,0,-1), glm::vec3(0,0,1), glm::perspective(glm::radians(80.0f), 1.0f, 0.1f, 30.0f))
    , m_minimapCamera(&m_window, glm::vec3(0, 20, 0), glm::vec3(0, -10, 0), glm::ortho(0.f, 1024.0f, 0.f, 600.f, 0.1f, 50.f))
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
    initScene();
}

void Application::initScene() 
{
    initSkybox(); // Skybox is part of scene, supposedly
    m_meshes.emplace_back(mergeMeshes(loadMesh("resources/dragon.obj")));

    
}

void Application::initShaders()
{
    try {
        m_defaultShader = ShaderBuilder().
            addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl").
            addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shader_frag.glsl").
            build();

        m_shadowShader = ShaderBuilder().
            addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shadow_vert.glsl").
            addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "Shaders/shadow_frag.glsl").
            build();

        m_blinnOrPhongShader = ShaderBuilder().
            addStage(GL_VERTEX_SHADER, "shaders/shader_vert.glsl").
            addStage(GL_FRAGMENT_SHADER, "shaders/blinn_or_phong_frag.glsl").
            build();

        m_skyboxShader = ShaderBuilder().
            addStage(GL_VERTEX_SHADER, "shaders/skybox_vert.glsl").
            addStage(GL_FRAGMENT_SHADER, "shaders/skybox_frag.glsl").
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
    Image right{ utils::globals::SKYBOX_RIGHT_IMG};
    Image left{ utils::globals::SKYBOX_LEFT_IMG };
    Image top{ utils::globals::SKYBOX_TOP_IMG };
    Image bottom{ utils::globals::SKYBOX_BOTTOM_IMG };
    Image front{ utils::globals::SKYBOX_FRONT_IMG };
    Image back{ utils::globals::SKYBOX_BACK_IMG };

    auto getFormat = [](int channels) {return channels == 4 ? GL_RGBA : GL_RGB; };
    auto getInternalFormat = [](int channels) { return channels == 4 ? GL_RGBA8 : GL_RGB8; };

    // Set up cubemap texture for skybox
    glGenTextures(1, &m_skyboxTex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_skyboxTex);

    GLuint imageDatatype = utils::globals::SKYBOX_IMG_DATATYPE;
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, getInternalFormat(right.channels), right.width, right.height, 0, getFormat(right.channels), imageDatatype , right.get_data());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, getInternalFormat(left.channels), left.width, left.height, 0, getFormat(left.channels), imageDatatype, left.get_data());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, getInternalFormat(top.channels), top.width, top.height, 0, getFormat(top.channels), imageDatatype, top.get_data());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, getInternalFormat(bottom.channels), bottom.width, bottom.height, 0, getFormat(bottom.channels), imageDatatype, bottom.get_data());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, getInternalFormat(front.channels), front.width, front.height, 0, getFormat(front.channels), imageDatatype, front.get_data());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, getInternalFormat(back.channels), back.width, back.height, 0, getFormat(back.channels), imageDatatype, back.get_data());

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

    //GLint oldDepthMode;
    //glGetIntegerv(GL_DEPTH_FUNC, &oldDepthMode);
    glDepthFunc(GL_LEQUAL);

    glBindVertexArray(m_skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    //glDepthFunc(oldDepthMode);
    glDepthFunc(GL_LESS);
}

void Application::update()
{
    int dummyInteger = 0; // Initialized to 0
    while (!m_window.shouldClose()) {
        // This is your game loop
        // Put your real-time logic and rendering in here

        // ==== UPDATE INPUT ====
        m_window.updateInput();
        m_mainCamera.updateInput();

        // Use ImGui for easy input/output of ints, floats, strings, etc...
        ImGui::Begin("Window");
        ImGui::InputInt("This is an integer input", &dummyInteger); // Use ImGui::DragInt or ImGui::DragFloat for larger range of numbers.
        ImGui::Text("Value is: %i", dummyInteger); // Use C printf formatting rules (%i is a signed integer)
        ImGui::Checkbox("Use material if no texture", &m_useMaterial);
        ImGui::End();

        // Clear the screen
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);

        const glm::mat4 mvpMatrix = m_mainCamera.viewProjectionMatrix() * m_modelMatrix;
        const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(m_modelMatrix));

        for (GPUMesh& mesh : m_meshes) {
            m_defaultShader.bind();
            glUniformMatrix4fv(m_defaultShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
            //Uncomment this line when you use the modelMatrix (or fragmentPosition)
            //glUniformMatrix4fv(m_defaultShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(m_modelMatrix));
            glUniformMatrix3fv(m_defaultShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));
            if (mesh.hasTextureCoords()) {
                m_texture.bind(GL_TEXTURE0);
                glUniform1i(m_defaultShader.getUniformLocation("colorMap"), 0);
                glUniform1i(m_defaultShader.getUniformLocation("hasTexCoords"), GL_TRUE);
                glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), GL_FALSE);
            } else {
                glUniform1i(m_defaultShader.getUniformLocation("hasTexCoords"), GL_FALSE);
                glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), m_useMaterial);
            }
            mesh.draw(m_defaultShader);
        }

        // Render skybox last
        drawSkybox();

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
