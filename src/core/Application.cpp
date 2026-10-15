#include "core/Application.hpp"
#include "core/Input.hpp"
#include <algorithm>
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
constexpr float CORRECT_NODE_Y = 300.0f;
constexpr float INCORRECT_NODE_Y = 700.0f;
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
    const model::NodeId correctA = m_CorrectExample.list.emplaceNode("A");
    const model::NodeId correctB = m_CorrectExample.list.emplaceNode("B");
    const model::NodeId correctC = m_CorrectExample.list.emplaceNode("C");
    m_CorrectExample.list.setHead(correctA);
    m_CorrectExample.list.setNext(correctA, correctB);
    m_CorrectExample.list.setNext(correctB, correctC);
    m_CorrectExample.nodes = {
        {.id = correctA, .x = SOURCE_NODE_X, .y = CORRECT_NODE_Y},
        {.id = correctB, .x = MIDDLE_NODE_X, .y = CORRECT_NODE_Y},
        {.id = correctC, .x = DESTINATION_NODE_X, .y = CORRECT_NODE_Y},
    };
    m_CorrectExample.description = "Correct: A -> B -> C -> nullptr";
    m_CorrectExample.descriptionBaselineY = 120.0f;

    const model::NodeId incorrectA = m_IncorrectExample.list.emplaceNode("A");
    const model::NodeId incorrectB = m_IncorrectExample.list.emplaceNode("B");
    const model::NodeId incorrectC = m_IncorrectExample.list.emplaceNode("C");
    m_IncorrectExample.list.setHead(incorrectA);
    m_IncorrectExample.list.setNext(incorrectA, incorrectB);
    m_IncorrectExample.nodes = {
        {.id = incorrectA, .x = SOURCE_NODE_X, .y = INCORRECT_NODE_Y},
        {.id = incorrectB, .x = MIDDLE_NODE_X, .y = INCORRECT_NODE_Y},
        {.id = incorrectC, .x = DESTINATION_NODE_X, .y = INCORRECT_NODE_Y},
    };
    m_IncorrectExample.description = "Incorrect: B ends early; C is disconnected";
    m_IncorrectExample.descriptionBaselineY = 520.0f;

    if (!m_CorrectExample.list.isConsistent() || !m_IncorrectExample.list.isConsistent()) {
        throw std::logic_error("A fixed linked-list example is inconsistent");
    }
    for (const view::NodeViewState& node : m_CorrectExample.nodes) {
        m_DrawOrder.push_back({.example = &m_CorrectExample, .id = node.id});
    }
    for (const view::NodeViewState& node : m_IncorrectExample.nodes) {
        m_DrawOrder.push_back({.example = &m_IncorrectExample, .id = node.id});
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
    updateInteraction();

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
    drawExampleConnections(m_CorrectExample);
    drawExampleConnections(m_IncorrectExample);
    for (const NodeReference& reference : m_DrawOrder) {
        const view::NodeViewState& node = findNodeVisual(reference.example->nodes, reference.id);
        drawNodeCard(*reference.example, node);
        drawNodeText(*reference.example, node);
    }
    drawHeadIndicator(m_CorrectExample);
    drawHeadIndicator(m_IncorrectExample);
    m_Window->endRender();
}

void Application::updateInteraction() {
    const glm::vec2 mouse = Input::getMousePosition();
    if (Input::isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        const std::optional<NodeReference> hitNode = findTopmostCard(mouse.x, mouse.y);
        if (hitNode) {
            bringToFront(*hitNode);
            m_SelectedNode = *hitNode;
            view::NodeViewState& node = findNodeVisual(hitNode->example->nodes, hitNode->id);
            m_DragState = {
                .node = *hitNode,
                .offsetX = mouse.x - node.x,
                .offsetY = mouse.y - node.y,
            };
        } else {
            m_SelectedNode.reset();
            m_DragState.reset();
        }
    }

    if (m_DragState) {
        if (Input::isMouseButtonJustReleased(GLFW_MOUSE_BUTTON_LEFT)) {
            m_DragState.reset();
        } else if (Input::isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)) {
            view::NodeViewState& node = findNodeVisual(
                m_DragState->node.example->nodes,
                m_DragState->node.id);
            const render::Rectangle clampedCard = render::clampCardToCanvas(
                mouse.x - m_DragState->offsetX,
                mouse.y - m_DragState->offsetY,
                Window::TARGET_WIDTH,
                Window::TARGET_HEIGHT);
            node.x = clampedCard.x;
            node.y = clampedCard.y;
        }
    }

    updateHoveredNode();
}

void Application::updateHoveredNode() {
    if (!Input::isMouseInsideCanvas()) {
        m_HoveredNode.reset();
        return;
    }

    const glm::vec2 mouse = Input::getMousePosition();
    m_HoveredNode = findTopmostCard(mouse.x, mouse.y);
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

void Application::drawNodeCard(
    const ListExample& example,
    const view::NodeViewState& node) const {
    constexpr Color nodeColor{0.07f, 0.10f, 0.15f};
    constexpr Color hoverOutline{0.18f, 0.58f, 0.95f};
    constexpr Color selectedOutline{0.08f, 0.37f, 0.88f};
    constexpr Color selectedNodeColor{0.05f, 0.15f, 0.29f};
    constexpr float hoverOutlineWidth = 5.0f;
    constexpr float selectedOutlineWidth = 8.0f;
    const render::NodeLayout layout = layoutFor(node);
    const bool selected = isNodeSelected(example, node.id);
    const bool hovered = isNodeHovered(example, node.id);
    const float outlineWidth = selected ? selectedOutlineWidth : (hovered ? hoverOutlineWidth : 0.0f);
    const Color& cardColor = selected ? selectedNodeColor : nodeColor;

    if (outlineWidth > 0.0f) {
        const Color& outlineColor = selected ? selectedOutline : hoverOutline;
        drawNodeSilhouette(layout, outlineWidth, outlineColor);
    }

    drawNodeSilhouette(layout, 0.0f, cardColor);
}

void Application::drawNodeText(
    const ListExample& example,
    const view::NodeViewState& node) const {
    constexpr Color nodeTextColor{0.92f, 0.96f, 1.0f};
    constexpr std::string_view nextLabel = "next";
    const render::NodeLayout layout = layoutFor(node);
    const std::string& value = example.list.node(node.id).value;
    const float valueX = layout.card.x +
                         (layout.card.width -
                          m_RobotoMonoValueTextRenderer->measureWidth(value)) /
                             2.0f;
    const float pointerLabelX = layout.outputSocket.centerX -
                                m_RobotoMonoPointerTextRenderer->measureWidth(nextLabel) / 2.0f;

    m_RobotoMonoValueTextRenderer->draw(
        value,
        valueX,
        layout.card.y + (layout.card.height + 32.0f) / 2.0f,
        nodeTextColor.red,
        nodeTextColor.green,
        nodeTextColor.blue,
        nodeTextColor.alpha);
    m_RobotoMonoPointerTextRenderer->draw(
        nextLabel,
        pointerLabelX,
        layout.outputSocket.centerY + 7.0f,
        nodeTextColor.red,
        nodeTextColor.green,
        nodeTextColor.blue,
        nodeTextColor.alpha);
}

void Application::drawExampleConnections(const ListExample& example) const {
    constexpr Color labelColor{0.07f, 0.10f, 0.15f};

    drawListPointers(example);
    drawNullEndpoints(example);
    m_RobotoRegularTextRenderer->draw(
        example.description,
        80.0f,
        example.descriptionBaselineY,
        labelColor.red,
        labelColor.green,
        labelColor.blue,
        labelColor.alpha);
}

void Application::drawListPointers(const ListExample& example) const {
    for (const view::NodeViewState& source : example.nodes) {
        const std::optional<model::NodeId> destinationId = example.list.node(source.id).next;
        if (!destinationId) {
            continue;
        }

        const render::NodeLayout sourceLayout = layoutFor(source);
        const view::NodeViewState& destination = findNodeVisual(example.nodes, *destinationId);
        const render::NodeLayout destinationLayout = layoutFor(destination);
        drawDirectionalPointer(
            sourceLayout.outputSocket.centerX,
            sourceLayout.outputSocket.centerY,
            destinationLayout.card.x,
            destinationLayout.card.y + destinationLayout.card.height / 2.0f);
    }
}

void Application::drawHeadIndicator(const ListExample& example) const {
    constexpr Color labelColor{0.07f, 0.10f, 0.15f};
    constexpr std::string_view headLabel = "head";

    const std::optional<model::NodeId> headId = example.list.head();
    if (!headId) {
        return;
    }

    const view::NodeViewState& head = findNodeVisual(example.nodes, *headId);
    const render::NodeLayout layout = layoutFor(head);
    const float centerX = layout.referenceDock.centerX;
    drawDirectionalPointer(
        centerX,
        layout.card.y - 80.0f,
        centerX,
        layout.referenceDock.centerY + layout.referenceDock.radius);
    m_RobotoRegularTextRenderer->draw(
        headLabel,
        centerX - m_RobotoRegularTextRenderer->measureWidth(headLabel) / 2.0f,
        layout.card.y - 100.0f,
        labelColor.red,
        labelColor.green,
        labelColor.blue,
        labelColor.alpha);
}

void Application::drawNullEndpoints(const ListExample& example) const {
    for (const view::NodeViewState& node : example.nodes) {
        if (!example.list.node(node.id).next) {
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

void Application::drawNullEndpoint(const view::NodeViewState& node) const {
    constexpr Color pointerColor{0.16f, 0.29f, 0.42f};
    constexpr Color labelColor{0.07f, 0.10f, 0.15f};
    constexpr float lineLength = 100.0f;
    constexpr float lineHeight = 8.0f;
    constexpr float labelGap = 20.0f;

    const render::NodeLayout layout = layoutFor(node);
    const float lineX = layout.outputSocket.centerX + layout.outputSocket.radius;
    const float lineY = layout.outputSocket.centerY - lineHeight / 2.0f;
    drawRectangle({lineX, lineY, lineLength, lineHeight}, pointerColor);
    m_RobotoMonoPointerTextRenderer->draw(
        "nullptr",
        lineX + lineLength + labelGap,
        layout.outputSocket.centerY + 7.0f,
        labelColor.red,
        labelColor.green,
        labelColor.blue,
        labelColor.alpha);
}

void Application::drawNodeSilhouette(
    const render::NodeLayout& layout,
    float glowExpansion,
    const Color& fillColor) const {
    const render::Rectangle expandedCard{
        .x = layout.card.x - glowExpansion,
        .y = layout.card.y - glowExpansion,
        .width = layout.card.width + (2.0f * glowExpansion),
        .height = layout.card.height + (2.0f * glowExpansion),
    };
    const render::Circle dockCutout{
        .centerX = layout.referenceDock.centerX,
        .centerY = layout.referenceDock.centerY,
        .radius = std::max(0.0f, layout.referenceDock.radius - glowExpansion),
    };

    drawRectangleWithCircularCutout(expandedCard, dockCutout, fillColor);
    drawCircle(
        layout.outputSocket.centerX,
        layout.outputSocket.centerY,
        layout.outputSocket.radius + glowExpansion,
        fillColor);
}

void Application::drawRectangleWithCircularCutout(
    const render::Rectangle& rectangle,
    const render::Circle& cutout,
    const Color& color) const {
    std::vector<float> vertices;
    vertices.reserve((CIRCLE_SEGMENTS * 12) + 24);
    const float rectangleLeft = rectangle.x;
    const float rectangleRight = rectangle.x + rectangle.width;
    const float rectangleTop = rectangle.y;
    const float rectangleBottom = rectangle.y + rectangle.height;
    const float cutoutTop = cutout.centerY - cutout.radius;
    const float cutoutBottom = cutout.centerY + cutout.radius;

    const auto appendRectangle = [&vertices](float left, float top, float right, float bottom) {
        vertices.insert(
            vertices.end(),
            {
                toNormalizedDeviceX(left), toNormalizedDeviceY(top),
                toNormalizedDeviceX(right), toNormalizedDeviceY(top),
                toNormalizedDeviceX(right), toNormalizedDeviceY(bottom),
                toNormalizedDeviceX(left), toNormalizedDeviceY(top),
                toNormalizedDeviceX(right), toNormalizedDeviceY(bottom),
                toNormalizedDeviceX(left), toNormalizedDeviceY(bottom),
            });
    };
    const auto appendQuadrilateral = [&vertices](
                                        float firstLeft,
                                        float firstRight,
                                        float firstY,
                                        float secondLeft,
                                        float secondRight,
                                        float secondY) {
        vertices.insert(
            vertices.end(),
            {
                toNormalizedDeviceX(firstLeft), toNormalizedDeviceY(firstY),
                toNormalizedDeviceX(firstRight), toNormalizedDeviceY(firstY),
                toNormalizedDeviceX(secondRight), toNormalizedDeviceY(secondY),
                toNormalizedDeviceX(firstLeft), toNormalizedDeviceY(firstY),
                toNormalizedDeviceX(secondRight), toNormalizedDeviceY(secondY),
                toNormalizedDeviceX(secondLeft), toNormalizedDeviceY(secondY),
            });
    };

    if (rectangleTop < cutoutTop) {
        appendRectangle(rectangleLeft, rectangleTop, rectangleRight, std::min(rectangleBottom, cutoutTop));
    }
    if (rectangleBottom > cutoutBottom) {
        appendRectangle(rectangleLeft, std::max(rectangleTop, cutoutBottom), rectangleRight, rectangleBottom);
    }

    const float cutoutVisibleTop = std::max(rectangleTop, cutoutTop);
    const float cutoutVisibleBottom = std::min(rectangleBottom, cutoutBottom);
    if (cutoutVisibleTop < cutoutVisibleBottom) {
        for (int segment = 0; segment < CIRCLE_SEGMENTS; ++segment) {
            const float startY = cutoutVisibleTop +
                                 (cutoutVisibleBottom - cutoutVisibleTop) * segment / CIRCLE_SEGMENTS;
            const float endY = cutoutVisibleTop +
                               (cutoutVisibleBottom - cutoutVisibleTop) * (segment + 1) / CIRCLE_SEGMENTS;
            const float startDistanceY = startY - cutout.centerY;
            const float endDistanceY = endY - cutout.centerY;
            const float startHalfWidth = std::sqrt(std::max(
                0.0f,
                (cutout.radius * cutout.radius) - (startDistanceY * startDistanceY)));
            const float endHalfWidth = std::sqrt(std::max(
                0.0f,
                (cutout.radius * cutout.radius) - (endDistanceY * endDistanceY)));

            appendQuadrilateral(
                rectangleLeft,
                cutout.centerX - startHalfWidth,
                startY,
                rectangleLeft,
                cutout.centerX - endHalfWidth,
                endY);
            appendQuadrilateral(
                cutout.centerX + startHalfWidth,
                rectangleRight,
                startY,
                cutout.centerX + endHalfWidth,
                rectangleRight,
                endY);
        }
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
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size() / 2));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
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

void Application::bringToFront(const NodeReference& node) {
    const auto iterator = std::find_if(
        m_DrawOrder.begin(),
        m_DrawOrder.end(),
        [&node](const NodeReference& candidate) {
            return candidate.example == node.example && candidate.id == node.id;
        });
    if (iterator == m_DrawOrder.end()) {
        throw std::logic_error("Missing node from draw order");
    }

    const NodeReference selected = *iterator;
    m_DrawOrder.erase(iterator);
    m_DrawOrder.push_back(selected);
}

std::optional<Application::NodeReference> Application::findTopmostCard(float x, float y) const {
    for (auto iterator = m_DrawOrder.rbegin(); iterator != m_DrawOrder.rend(); ++iterator) {
        const view::NodeViewState& node = findNodeVisual(iterator->example->nodes, iterator->id);
        if (render::isPointInsideCard(node, x, y)) {
            return *iterator;
        }
    }
    return std::nullopt;
}

bool Application::isNodeHovered(const ListExample& example, model::NodeId id) const {
    return m_HoveredNode && m_HoveredNode->example == &example && m_HoveredNode->id == id;
}

bool Application::isNodeSelected(const ListExample& example, model::NodeId id) const {
    return m_SelectedNode && m_SelectedNode->example == &example && m_SelectedNode->id == id;
}

render::NodeLayout Application::layoutFor(const view::NodeViewState& node) {
    return render::makeNodeLayout(node.x, node.y);
}

view::NodeViewState& Application::findNodeVisual(
    std::vector<view::NodeViewState>& nodes,
    model::NodeId id) {
    for (view::NodeViewState& node : nodes) {
        if (node.id == id) {
            return node;
        }
    }
    throw std::logic_error("Missing layout for linked-list node");
}

const view::NodeViewState& Application::findNodeVisual(
    const std::vector<view::NodeViewState>& nodes,
    model::NodeId id) const {
    for (const view::NodeViewState& node : nodes) {
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
