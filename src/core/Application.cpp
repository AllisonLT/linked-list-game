#include "core/Application.hpp"
#include "core/Input.hpp"
#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>

namespace game::core {
namespace {

constexpr float NODE_CARD_X = 780.0f;
constexpr float NODE_CARD_Y = 440.0f;
constexpr int CIRCLE_SEGMENTS = 24;
constexpr float PI = 3.14159265358979323846f;

constexpr float toNormalizedDeviceX(float canvasX) {
    return (canvasX / Window::TARGET_WIDTH) * 2.0f - 1.0f;
}

constexpr float toNormalizedDeviceY(float canvasY) {
    return 1.0f - (canvasY / Window::TARGET_HEIGHT) * 2.0f;
}

} // namespace

Application::Application() {
    m_Window = std::make_unique<Window>(1280, 720, "Linked List Game");
    Input::init(m_Window.get());
    m_ColorShader = std::make_unique<ShaderProgram>(
        "assets/shaders/solid_color.vert",
        "assets/shaders/solid_color.frag");
    m_TextRenderer = std::make_unique<TextRenderer>("assets/fonts/Roboto-Regular.ttf", 48);
    createRectangleRenderer();
}

Application::~Application() {
    if (m_RectangleVertexBuffer != 0) {
        glDeleteBuffers(1, &m_RectangleVertexBuffer);
    }
    if (m_RectangleVertexArray != 0) {
        glDeleteVertexArrays(1, &m_RectangleVertexArray);
    }
}

void Application::processInput() {
    if (Input::isKeyJustPressed(GLFW_KEY_ESCAPE)) {
        glfwSetWindowShouldClose(m_Window->getNativeWindow(), true);
    }

    if (Input::isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        glm::vec2 mousePos = Input::getMousePosition();
        std::cout << "[Click] Virtual Canvas: (" << mousePos.x << ", " << mousePos.y << ")"
                  << " | Inside Canvas: " << (Input::isMouseInsideCanvas() ? "YES" : "NO") << "\n";
    }
}

void Application::update(float /*deltaTime*/) {
    const glm::vec2 mouse = Input::getMousePosition();
    std::ostringstream title;
    title << "Linked List Game | Canvas mouse: (" << static_cast<int>(mouse.x) << ", "
          << static_cast<int>(mouse.y) << ")"
          << (Input::isMouseInsideCanvas() ? "" : " - outside canvas");
    m_Window->setTitle(title.str());

    Input::update();
}

void Application::render() {
    m_Window->beginRender();
    drawNodeCard();
    const render::NodeLayout layout = render::makeNodeLayout(NODE_CARD_X, NODE_CARD_Y);
    m_TextRenderer->draw(
        "Hello World!",
        layout.outputSocket.centerX + layout.outputSocket.radius + 32.0f,
        layout.outputSocket.centerY + 16.0f,
        0.07f,
        0.10f,
        0.15f,
        1.0f);
    m_Window->endRender();
}

void Application::createRectangleRenderer() {
    glGenVertexArrays(1, &m_RectangleVertexArray);
    glGenBuffers(1, &m_RectangleVertexBuffer);

    glBindVertexArray(m_RectangleVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, m_RectangleVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Application::drawNodeCard() const {
    constexpr Color nodeColor{0.07f, 0.10f, 0.15f};

    const render::NodeLayout layout = render::makeNodeLayout(NODE_CARD_X, NODE_CARD_Y);
    drawRectangle(layout.card, nodeColor);
    drawCircle(
        layout.outputSocket.centerX,
        layout.outputSocket.centerY,
        layout.outputSocket.radius,
        nodeColor);
}

void Application::drawCircle(float centerX, float centerY, float radius, const Color& color) const {
    std::vector<float> vertices;
    vertices.reserve((CIRCLE_SEGMENTS + 2) * 2);
    vertices.push_back(toNormalizedDeviceX(centerX));
    vertices.push_back(toNormalizedDeviceY(centerY));

    for (int segment = 0; segment <= CIRCLE_SEGMENTS; ++segment) {
        const float angle = (static_cast<float>(segment) / CIRCLE_SEGMENTS) * 2.0f * PI;
        const float x = centerX + std::cos(angle) * radius;
        const float y = centerY + std::sin(angle) * radius;
        vertices.push_back(toNormalizedDeviceX(x));
        vertices.push_back(toNormalizedDeviceY(y));
    }

    m_ColorShader->use();
    m_ColorShader->setColor(color.red, color.green, color.blue, color.alpha);
    glBindVertexArray(m_RectangleVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, m_RectangleVertexBuffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
        vertices.data(),
        GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(vertices.size() / 2));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Application::drawRectangle(const render::Rectangle& rectangle, const Color& color) const {
    const float left = toNormalizedDeviceX(rectangle.x);
    const float right = toNormalizedDeviceX(rectangle.x + rectangle.width);
    const float top = toNormalizedDeviceY(rectangle.y);
    const float bottom = toNormalizedDeviceY(rectangle.y + rectangle.height);

    const float vertices[] = {
        left, top,
        right, top,
        right, bottom,
        left, top,
        right, bottom,
        left, bottom,
    };

    m_ColorShader->use();
    m_ColorShader->setColor(color.red, color.green, color.blue, color.alpha);
    glBindVertexArray(m_RectangleVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, m_RectangleVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

int Application::run() {
    float lastTime = static_cast<float>(glfwGetTime());

    while (!m_Window->shouldClose()) {
        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        m_Window->pollEvents();
        processInput();
        update(deltaTime);
        render();
    }

    return 0;
}

} // namespace game::core
