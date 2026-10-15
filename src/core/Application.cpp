#include "core/Application.hpp"
#include "core/Input.hpp"
#include <cmath>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace game::core {
namespace {

constexpr float SOURCE_NODE_X = 500.0f;
constexpr float MIDDLE_NODE_X = 830.0f;
constexpr float DESTINATION_NODE_X = 1160.0f;
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
    m_RobotoMonoValueTextRenderer = std::make_unique<TextRenderer>(
        "assets/fonts/RobotoMono-Regular.ttf",
        48);
    m_RobotoMonoPointerTextRenderer = std::make_unique<TextRenderer>(
        "assets/fonts/RobotoMono-Regular.ttf",
        20);
    m_RobotoRegularTextRenderer = std::make_unique<TextRenderer>(
        "assets/fonts/Roboto-Regular.ttf",
        32);
    const model::NodeId nodeA = m_List.emplaceNode("A");
    const model::NodeId nodeB = m_List.emplaceNode("B");
    const model::NodeId nodeC = m_List.emplaceNode("C");
    m_List.setHead(nodeA);
    m_List.setNext(nodeA, nodeB);
    m_List.setNext(nodeB, nodeC);

    m_NodeVisuals = {
        {.id = nodeA, .layout = render::makeNodeLayout(SOURCE_NODE_X, NODE_CARD_Y)},
        {.id = nodeB, .layout = render::makeNodeLayout(MIDDLE_NODE_X, NODE_CARD_Y)},
        {.id = nodeC, .layout = render::makeNodeLayout(DESTINATION_NODE_X, NODE_CARD_Y)},
    };
    if (!m_List.isConsistent()) {
        throw std::logic_error("The fixed linked-list example is inconsistent");
    }
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
    drawListPointers();
    drawHeadIndicator();
    for (const NodeVisual& node : m_NodeVisuals) {
        drawNodeCard(node);
        drawNodeText(node);
    }
    drawNullEndpoints();
    m_RobotoRegularTextRenderer->draw(
        "Static list preview",
        80.0f,
        100.0f,
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

void Application::drawNodeCard(const NodeVisual& node) const {
    constexpr Color nodeColor{0.07f, 0.10f, 0.15f};

    drawRectangle(node.layout.card, nodeColor);
    drawCircle(
        node.layout.outputSocket.centerX,
        node.layout.outputSocket.centerY,
        node.layout.outputSocket.radius,
        nodeColor);
}

void Application::drawNodeText(const NodeVisual& node) const {
    constexpr Color nodeTextColor{0.92f, 0.96f, 1.0f};
    constexpr std::string_view nextLabel = "next";
    const std::string& value = m_List.node(node.id).value;
    const float valueX = node.layout.card.x +
                         (node.layout.card.width -
                          m_RobotoMonoValueTextRenderer->measureWidth(value)) /
                             2.0f;
    const float pointerLabelX = node.layout.outputSocket.centerX -
                                m_RobotoMonoPointerTextRenderer->measureWidth(nextLabel) / 2.0f;

    m_RobotoMonoValueTextRenderer->draw(
        value,
        valueX,
        node.layout.card.y + (node.layout.card.height + 32.0f) / 2.0f,
        nodeTextColor.red,
        nodeTextColor.green,
        nodeTextColor.blue,
        nodeTextColor.alpha);
    m_RobotoMonoPointerTextRenderer->draw(
        nextLabel,
        pointerLabelX,
        node.layout.outputSocket.centerY + 7.0f,
        nodeTextColor.red,
        nodeTextColor.green,
        nodeTextColor.blue,
        nodeTextColor.alpha);
}

void Application::drawListPointers() const {
    for (const NodeVisual& source : m_NodeVisuals) {
        const std::optional<model::NodeId> destinationId = m_List.node(source.id).next;
        if (!destinationId) {
            continue;
        }

        const NodeVisual& destination = findNodeVisual(*destinationId);
        drawDirectionalPointer(
            source.layout.outputSocket.centerX,
            source.layout.outputSocket.centerY,
            destination.layout.card.x,
            destination.layout.card.y + destination.layout.card.height / 2.0f);
    }
}

void Application::drawHeadIndicator() const {
    constexpr Color labelColor{0.07f, 0.10f, 0.15f};
    constexpr std::string_view headLabel = "head";

    const std::optional<model::NodeId> headId = m_List.head();
    if (!headId) {
        return;
    }

    const NodeVisual& head = findNodeVisual(*headId);
    const float centerX = head.layout.card.x + head.layout.card.width / 2.0f;
    drawDirectionalPointer(centerX, head.layout.card.y - 80.0f, centerX, head.layout.card.y);
    m_RobotoRegularTextRenderer->draw(
        headLabel,
        centerX - m_RobotoRegularTextRenderer->measureWidth(headLabel) / 2.0f,
        head.layout.card.y - 100.0f,
        labelColor.red,
        labelColor.green,
        labelColor.blue,
        labelColor.alpha);
}

void Application::drawNullEndpoints() const {
    for (const NodeVisual& node : m_NodeVisuals) {
        if (!m_List.node(node.id).next) {
            drawNullEndpoint(node);
        }
    }
}

void Application::drawDirectionalPointer(
    float sourceX,
    float sourceY,
    float destinationX,
    float destinationY) const {
    constexpr Color pointerColor{0.16f, 0.29f, 0.42f};
    constexpr float lineHalfWidth = 5.0f;
    constexpr float arrowLength = 28.0f;
    constexpr float arrowHalfWidth = 16.0f;

    const float deltaX = destinationX - sourceX;
    const float deltaY = destinationY - sourceY;
    const float length = std::sqrt(deltaX * deltaX + deltaY * deltaY);
    if (length <= arrowLength) {
        return;
    }

    const float directionX = deltaX / length;
    const float directionY = deltaY / length;
    const float perpendicularX = -directionY;
    const float perpendicularY = directionX;
    const float shaftEndX = destinationX - directionX * arrowLength;
    const float shaftEndY = destinationY - directionY * arrowLength;

    const float vertices[] = {
        toNormalizedDeviceX(sourceX + perpendicularX * lineHalfWidth),
        toNormalizedDeviceY(sourceY + perpendicularY * lineHalfWidth),
        toNormalizedDeviceX(shaftEndX + perpendicularX * lineHalfWidth),
        toNormalizedDeviceY(shaftEndY + perpendicularY * lineHalfWidth),
        toNormalizedDeviceX(shaftEndX - perpendicularX * lineHalfWidth),
        toNormalizedDeviceY(shaftEndY - perpendicularY * lineHalfWidth),
        toNormalizedDeviceX(sourceX + perpendicularX * lineHalfWidth),
        toNormalizedDeviceY(sourceY + perpendicularY * lineHalfWidth),
        toNormalizedDeviceX(shaftEndX - perpendicularX * lineHalfWidth),
        toNormalizedDeviceY(shaftEndY - perpendicularY * lineHalfWidth),
        toNormalizedDeviceX(sourceX - perpendicularX * lineHalfWidth),
        toNormalizedDeviceY(sourceY - perpendicularY * lineHalfWidth),
        toNormalizedDeviceX(destinationX),
        toNormalizedDeviceY(destinationY),
        toNormalizedDeviceX(shaftEndX + perpendicularX * arrowHalfWidth),
        toNormalizedDeviceY(shaftEndY + perpendicularY * arrowHalfWidth),
        toNormalizedDeviceX(shaftEndX - perpendicularX * arrowHalfWidth),
        toNormalizedDeviceY(shaftEndY - perpendicularY * arrowHalfWidth),
    };

    m_ColorShader->use();
    m_ColorShader->setColor(
        pointerColor.red,
        pointerColor.green,
        pointerColor.blue,
        pointerColor.alpha);
    glBindVertexArray(m_RectangleVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, m_RectangleVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 9);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Application::drawNullEndpoint(const NodeVisual& node) const {
    constexpr Color pointerColor{0.16f, 0.29f, 0.42f};
    constexpr Color labelColor{0.07f, 0.10f, 0.15f};
    constexpr float lineLength = 100.0f;
    constexpr float lineHeight = 8.0f;
    constexpr float labelGap = 20.0f;

    const float lineX = node.layout.outputSocket.centerX + node.layout.outputSocket.radius;
    const float lineY = node.layout.outputSocket.centerY - lineHeight / 2.0f;
    drawRectangle({lineX, lineY, lineLength, lineHeight}, pointerColor);
    m_RobotoMonoPointerTextRenderer->draw(
        "nullptr",
        lineX + lineLength + labelGap,
        node.layout.outputSocket.centerY + 7.0f,
        labelColor.red,
        labelColor.green,
        labelColor.blue,
        labelColor.alpha);
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

const Application::NodeVisual& Application::findNodeVisual(model::NodeId id) const {
    for (const NodeVisual& node : m_NodeVisuals) {
        if (node.id == id) {
            return node;
        }
    }
    throw std::logic_error("Missing layout for linked-list node");
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
