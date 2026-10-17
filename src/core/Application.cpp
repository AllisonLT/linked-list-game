#include "core/Application.hpp"
#include "core/Input.hpp"
#include "render/PointerHitTest.hpp"
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
constexpr float CORRECT_NODE_Y = 760.0f;
constexpr float WORKBENCH_X = 60.0f;
constexpr float WORKBENCH_Y = 50.0f;
constexpr float WORKBENCH_WIDTH = Window::TARGET_WIDTH - (2.0f * WORKBENCH_X);
constexpr float WORKBENCH_HEIGHT = (Window::TARGET_HEIGHT * 3.0f) / 5.0f;
constexpr float TOOLBOX_X = WORKBENCH_X + 50.0f;
constexpr float TOOLBOX_Y = WORKBENCH_Y + 120.0f;
constexpr float TOOLBOX_WIDTH = 255.0f;
constexpr float TOOLBOX_HEIGHT = 405.0f;
constexpr float TOOL_BUTTON_X = TOOLBOX_X + 34.0f;
constexpr float TOOL_BUTTON_Y = TOOLBOX_Y + 79.0f;
constexpr float TOOL_BUTTON_WIDTH = 187.0f;
constexpr float TOOL_BUTTON_HEIGHT = 66.0f;
constexpr float TOOL_BUTTON_GAP = 17.0f;
constexpr float CLAMP_CENTER_X = Window::TARGET_WIDTH / 2.0f;
constexpr float CLAMP_Y = WORKBENCH_Y + 55.0f;
constexpr float CLAMP_SLOT_SIZE = render::NODE_CARD_WIDTH;
constexpr render::Rectangle CLAMP_SLOT{
    CLAMP_CENTER_X - (CLAMP_SLOT_SIZE / 2.0f),
    WORKBENCH_Y + 135.0f,
    CLAMP_SLOT_SIZE,
    CLAMP_SLOT_SIZE,
};
constexpr render::Rectangle TAPE_TOOL_SLOT{
    TOOL_BUTTON_X,
    TOOL_BUTTON_Y,
    TOOL_BUTTON_WIDTH,
    TOOL_BUTTON_HEIGHT,
};
constexpr render::Rectangle SCISSORS_TOOL_SLOT{
    TOOL_BUTTON_X,
    TOOL_BUTTON_Y + TOOL_BUTTON_HEIGHT + TOOL_BUTTON_GAP,
    TOOL_BUTTON_WIDTH,
    TOOL_BUTTON_HEIGHT,
};
constexpr render::Rectangle ADVANCE_TOOL_SLOT{
    TOOL_BUTTON_X,
    TOOL_BUTTON_Y + (2.0f * (TOOL_BUTTON_HEIGHT + TOOL_BUTTON_GAP)),
    TOOL_BUTTON_WIDTH,
    TOOL_BUTTON_HEIGHT,
};
constexpr int CIRCLE_SEGMENTS = 24;
constexpr float PI = 3.14159265358979323846f;
constexpr float ADVANCE_FEEDBACK_DURATION = 0.5f;
constexpr float SCISSORS_POINTER_HIT_RADIUS = 20.0f;
constexpr float TAPE_DRAG_THRESHOLD = 8.0f;

constexpr bool isPointInsideRectangle(
    const render::Rectangle& rectangle,
    float x,
    float y) {
    return x >= rectangle.x && x <= rectangle.x + rectangle.width &&
           y >= rectangle.y && y <= rectangle.y + rectangle.height;
}

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

    if (!m_CorrectExample.list.isConsistent()) {
        throw std::logic_error("A fixed linked-list example is inconsistent");
    }
    for (const view::NodeViewState& node : m_CorrectExample.nodes) {
        m_DrawOrder.push_back({.example = &m_CorrectExample, .id = node.id});
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

void Application::update(float deltaTime) {
    if (m_AdvanceFeedbackRemaining > 0.0f) {
        m_AdvanceFeedbackRemaining = std::max(0.0f, m_AdvanceFeedbackRemaining - deltaTime);
        if (m_AdvanceFeedbackRemaining == 0.0f) {
            m_AdvanceFeedback = AdvanceFeedback::None;
        }
    }

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
    drawWorkbench();
    drawExampleConnections(m_CorrectExample);
    for (const NodeReference& reference : m_DrawOrder) {
        const view::NodeViewState& node = findNodeVisual(reference.example->nodes, reference.id);
        drawNodeCard(*reference.example, node);
        drawNodeText(*reference.example, node);
    }
    drawHeadIndicator(m_CorrectExample);
    drawCurrClamp();
    drawTapePreview();
    drawHeldTool();
    m_Window->endRender();
}

void Application::updateInteraction() {
    const glm::vec2 mouse = Input::getMousePosition();
    const std::optional<HeldTool> clickedTool =
        Input::isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)
            ? toolAt(mouse.x, mouse.y)
            : std::nullopt;
    if (clickedTool) {
        m_HeldTool = m_HeldTool == *clickedTool ? HeldTool::None : *clickedTool;
        m_DragState.reset();
        m_TapeDrag.reset();
    } else if (
        Input::isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT) &&
        isAdvanceToolAt(mouse.x, mouse.y)) {
        triggerAdvanceFeedback();
        m_DragState.reset();
        m_TapeDrag.reset();
    } else if (
        Input::isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT) &&
        m_HeldTool == HeldTool::Scissors) {
        if (const std::optional<NodeReference> scissorsTarget =
                findScissorsTarget(mouse.x, mouse.y)) {
            scissorsTarget->example->list.setNext(scissorsTarget->id, std::nullopt);
        }
    } else if (m_HeldTool == HeldTool::Tape) {
        if (Input::isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
            if (const std::optional<NodeReference> source = findTapeOutput(mouse.x, mouse.y)) {
                m_TapeDrag = {
                    .source = *source,
                    .pressX = mouse.x,
                    .pressY = mouse.y,
                };
            } else if (m_TapeDrag) {
                if (const std::optional<NodeReference> target = findTopmostCard(mouse.x, mouse.y)) {
                    m_TapeDrag->source.example->list.setNext(m_TapeDrag->source.id, target->id);
                }
                m_TapeDrag.reset();
            } else {
                m_TapeDrag.reset();
            }
        }
        if (m_TapeDrag && Input::isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)) {
            const float offsetX = mouse.x - m_TapeDrag->pressX;
            const float offsetY = mouse.y - m_TapeDrag->pressY;
            m_TapeDrag->isDragging = m_TapeDrag->isDragging ||
                                     (offsetX * offsetX + offsetY * offsetY >=
                                      TAPE_DRAG_THRESHOLD * TAPE_DRAG_THRESHOLD);
        }
        if (
            m_TapeDrag && m_TapeDrag->isDragging &&
            Input::isMouseButtonJustReleased(GLFW_MOUSE_BUTTON_LEFT)) {
            if (const std::optional<NodeReference> target = findTopmostCard(mouse.x, mouse.y)) {
                m_TapeDrag->source.example->list.setNext(m_TapeDrag->source.id, target->id);
            }
            m_TapeDrag.reset();
        }
    } else if (
        Input::isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT) &&
        m_HeldTool == HeldTool::None) {
        const std::optional<NodeReference> hitNode = findTopmostCard(mouse.x, mouse.y);
        if (hitNode) {
            bringToFront(*hitNode);
            m_SelectedNode = *hitNode;
            view::NodeViewState& node = findNodeVisual(hitNode->example->nodes, hitNode->id);
            std::vector<DraggedNode> downstreamNodes;
            for (const model::NodeId downstreamId :
                 hitNode->example->list.downstreamNodeIds(hitNode->id)) {
                const view::NodeViewState& downstreamNode = findNodeVisual(
                    hitNode->example->nodes,
                    downstreamId);
                downstreamNodes.push_back(
                    {
                        .id = downstreamId,
                        .initialX = downstreamNode.x,
                        .initialY = downstreamNode.y,
                    });
            }
            m_DragState = {
                .node = *hitNode,
                .offsetX = mouse.x - node.x,
                .offsetY = mouse.y - node.y,
                .initialX = node.x,
                .initialY = node.y,
                .downstreamNodes = std::move(downstreamNodes),
            };
        } else {
            m_SelectedNode.reset();
            m_DragState.reset();
        }
    }

    if (m_DragState && m_HeldTool == HeldTool::None) {
        const bool mouseReleased = Input::isMouseButtonJustReleased(GLFW_MOUSE_BUTTON_LEFT);
        if (Input::isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT) || mouseReleased) {
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
            const float deltaX = node.x - m_DragState->initialX;
            const float deltaY = node.y - m_DragState->initialY;
            for (const DraggedNode& downstreamNode : m_DragState->downstreamNodes) {
                view::NodeViewState& visual = findNodeVisual(
                    m_DragState->node.example->nodes,
                    downstreamNode.id);
                visual.x = downstreamNode.initialX + deltaX;
                visual.y = downstreamNode.initialY + deltaY;
            }
        }
        if (mouseReleased) {
            view::NodeViewState& node = findNodeVisual(
                m_DragState->node.example->nodes,
                m_DragState->node.id);
            const float centerX = node.x + (render::NODE_CARD_WIDTH / 2.0f);
            const float centerY = node.y + (render::NODE_CARD_HEIGHT / 2.0f);
            if (isPointInsideRectangle(CLAMP_SLOT, centerX, centerY)) {
                if (
                    m_CurrNode &&
                    (m_CurrNode->example != m_DragState->node.example ||
                     m_CurrNode->id != m_DragState->node.id)) {
                    view::NodeViewState& ejectedNode = findNodeVisual(
                        m_CurrNode->example->nodes,
                        m_CurrNode->id);
                    const render::Rectangle ejectionPosition = randomClampEjectionPosition();
                    ejectedNode.x = ejectionPosition.x;
                    ejectedNode.y = ejectionPosition.y;
                }
                const float snapDeltaX = CLAMP_SLOT.x - node.x;
                const float snapDeltaY = CLAMP_SLOT.y - node.y;
                node.x = CLAMP_SLOT.x;
                node.y = CLAMP_SLOT.y;
                for (const DraggedNode& downstreamNode : m_DragState->downstreamNodes) {
                    view::NodeViewState& visual = findNodeVisual(
                        m_DragState->node.example->nodes,
                        downstreamNode.id);
                    visual.x += snapDeltaX;
                    visual.y += snapDeltaY;
                }
                m_CurrNode = m_DragState->node;
            }
            m_DragState.reset();
        }
    }

    updateHoveredScissorsTarget();
    updateHoveredTapeSource();
    updateHoveredTapeTarget();
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

void Application::updateHoveredScissorsTarget() {
    if (m_HeldTool != HeldTool::Scissors || !Input::isMouseInsideCanvas()) {
        m_HoveredScissorsTarget.reset();
        return;
    }

    const glm::vec2 mouse = Input::getMousePosition();
    m_HoveredScissorsTarget = findScissorsTarget(mouse.x, mouse.y);
}

void Application::updateHoveredTapeTarget() {
    if (!m_TapeDrag || m_HeldTool != HeldTool::Tape || !Input::isMouseInsideCanvas()) {
        m_HoveredTapeTarget.reset();
        return;
    }

    const glm::vec2 mouse = Input::getMousePosition();
    m_HoveredTapeTarget = findTopmostCard(mouse.x, mouse.y);
}

void Application::updateHoveredTapeSource() {
    if (m_HeldTool != HeldTool::Tape || !Input::isMouseInsideCanvas()) {
        m_HoveredTapeSource.reset();
        return;
    }

    const glm::vec2 mouse = Input::getMousePosition();
    m_HoveredTapeSource = findTapeOutput(mouse.x, mouse.y);
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

void Application::drawWorkbench() const {
    constexpr Color tableBorderColor{0.26f, 0.15f, 0.08f};
    constexpr Color tableSurfaceColor{0.68f, 0.48f, 0.28f};
    constexpr Color titleColor{0.17f, 0.10f, 0.05f};
    constexpr float tableBorderWidth = 14.0f;

    drawRectangle(
        {WORKBENCH_X, WORKBENCH_Y, WORKBENCH_WIDTH, WORKBENCH_HEIGHT},
        tableBorderColor);
    drawRectangle(
        {
            WORKBENCH_X + tableBorderWidth,
            WORKBENCH_Y + tableBorderWidth,
            WORKBENCH_WIDTH - (2.0f * tableBorderWidth),
            WORKBENCH_HEIGHT - (2.0f * tableBorderWidth),
        },
        tableSurfaceColor);
    m_RobotoRegularTextRenderer->draw(
        "Pointer Workbench",
        WORKBENCH_X + 32.0f,
        WORKBENCH_Y + 45.0f,
        titleColor.red,
        titleColor.green,
        titleColor.blue,
        titleColor.alpha);
    drawClampSlot();
    drawToolbox();
}

void Application::drawToolbox() const {
    constexpr Color toolboxBorderColor{0.20f, 0.12f, 0.07f};
    constexpr Color toolboxSurfaceColor{0.40f, 0.24f, 0.12f};
    constexpr Color toolButtonColor{0.73f, 0.55f, 0.30f};
    constexpr Color toolTextColor{0.95f, 0.91f, 0.82f};
    constexpr Color inactiveTextColor{0.32f, 0.19f, 0.08f};
    constexpr Color selectedToolColor{0.96f, 0.72f, 0.24f};
    constexpr Color errorToolColor{0.68f, 0.15f, 0.13f};
    constexpr Color errorGlowColor{1.0f, 0.24f, 0.19f};

    drawRectangle({TOOLBOX_X, TOOLBOX_Y, TOOLBOX_WIDTH, TOOLBOX_HEIGHT}, toolboxBorderColor);
    drawRectangle(
        {
            TOOLBOX_X + 10.0f,
            TOOLBOX_Y + 10.0f,
            TOOLBOX_WIDTH - 20.0f,
            TOOLBOX_HEIGHT - 20.0f,
        },
        toolboxSurfaceColor);
    m_RobotoMonoPointerTextRenderer->draw(
        "TOOLBOX",
        TOOLBOX_X + 36.0f,
        TOOLBOX_Y + 40.0f,
        toolTextColor.red,
        toolTextColor.green,
        toolTextColor.blue,
        toolTextColor.alpha);

    drawRectangle(TAPE_TOOL_SLOT, toolButtonColor);
    drawRectangle(SCISSORS_TOOL_SLOT, toolButtonColor);
    if (m_HeldTool != HeldTool::Tape) {
        m_RobotoMonoPointerTextRenderer->draw(
            "TAPE",
            TOOL_BUTTON_X + 56.0f,
            TOOL_BUTTON_Y + 44.0f,
            inactiveTextColor.red,
            inactiveTextColor.green,
            inactiveTextColor.blue,
            inactiveTextColor.alpha);
    }
    if (m_HeldTool != HeldTool::Scissors) {
        m_RobotoMonoPointerTextRenderer->draw(
            "SCISSORS",
            TOOL_BUTTON_X + 20.0f,
            TOOL_BUTTON_Y + TOOL_BUTTON_HEIGHT + TOOL_BUTTON_GAP + 44.0f,
            inactiveTextColor.red,
            inactiveTextColor.green,
            inactiveTextColor.blue,
            inactiveTextColor.alpha);
    }
    if (m_AdvanceFeedback == AdvanceFeedback::Error) {
        drawRectangle(
            {
                ADVANCE_TOOL_SLOT.x - 6.0f,
                ADVANCE_TOOL_SLOT.y - 6.0f,
                ADVANCE_TOOL_SLOT.width + 12.0f,
                ADVANCE_TOOL_SLOT.height + 12.0f,
            },
            errorGlowColor);
    }
    drawRectangle(
        ADVANCE_TOOL_SLOT,
        m_AdvanceFeedback == AdvanceFeedback::Error
            ? errorToolColor
            : (m_AdvanceFeedback == AdvanceFeedback::Activated
                   ? selectedToolColor
                   : toolButtonColor));
    m_RobotoMonoPointerTextRenderer->draw(
        "ADVANCE",
        TOOL_BUTTON_X + 25.0f,
        TOOL_BUTTON_Y + (2.0f * (TOOL_BUTTON_HEIGHT + TOOL_BUTTON_GAP)) + 44.0f,
        inactiveTextColor.red,
        inactiveTextColor.green,
        inactiveTextColor.blue,
        inactiveTextColor.alpha);
}

void Application::drawClampSlot() const {
    constexpr Color dashColor{0.95f, 0.95f, 0.95f};
    constexpr float dashLength = 18.0f;
    constexpr float dashGap = 12.0f;
    constexpr float dashWidth = 4.0f;
    const float right = CLAMP_SLOT.x + CLAMP_SLOT.width;
    const float bottom = CLAMP_SLOT.y + CLAMP_SLOT.height;

    for (float x = CLAMP_SLOT.x; x < right; x += dashLength + dashGap) {
        const float width = std::min(dashLength, right - x);
        drawRectangle({x, CLAMP_SLOT.y, width, dashWidth}, dashColor);
        drawRectangle({x, bottom - dashWidth, width, dashWidth}, dashColor);
    }
    for (float y = CLAMP_SLOT.y; y < bottom; y += dashLength + dashGap) {
        const float height = std::min(dashLength, bottom - y);
        drawRectangle({CLAMP_SLOT.x, y, dashWidth, height}, dashColor);
        drawRectangle({right - dashWidth, y, dashWidth, height}, dashColor);
    }
}

void Application::drawHeldTool() const {
    if (m_HeldTool == HeldTool::None || !Input::isMouseInsideCanvas()) {
        return;
    }

    constexpr Color heldToolColor{0.97f, 0.82f, 0.40f};
    constexpr std::string_view tapeLabel = "tape";
    constexpr std::string_view scissorsLabel = "scissors";
    const std::string_view label =
        m_HeldTool == HeldTool::Tape ? tapeLabel : scissorsLabel;
    const glm::vec2 mouse = Input::getMousePosition();
    m_RobotoMonoPointerTextRenderer->draw(
        label,
        mouse.x + 16.0f,
        mouse.y + 24.0f,
        heldToolColor.red,
        heldToolColor.green,
        heldToolColor.blue,
        heldToolColor.alpha);
}

void Application::drawTapePreview() const {
    if (!m_TapeDrag || !Input::isMouseInsideCanvas()) {
        return;
    }

    constexpr Color tapePreviewColor{0.96f, 0.72f, 0.24f};
    const view::NodeViewState& source = findNodeVisual(
        m_TapeDrag->source.example->nodes,
        m_TapeDrag->source.id);
    const render::NodeLayout layout = layoutFor(source);
    const glm::vec2 mouse = Input::getMousePosition();
    drawDirectionalPointer(
        layout.outputSocket.centerX,
        layout.outputSocket.centerY,
        mouse.x,
        mouse.y,
        tapePreviewColor,
        1.0f);
}

void Application::triggerAdvanceFeedback() {
    m_AdvanceFeedback = AdvanceFeedback::Activated;
    if (m_CurrNode && !m_CurrNode->example->list.node(m_CurrNode->id).next) {
        m_AdvanceFeedback = AdvanceFeedback::Error;
    }
    m_AdvanceFeedbackRemaining = ADVANCE_FEEDBACK_DURATION;
}

void Application::drawCurrClamp() const {
    constexpr Color clampBodyColor{0.88f, 0.64f, 0.20f};
    constexpr Color clampInsetColor{0.27f, 0.18f, 0.08f};
    constexpr Color clampLabelColor{0.98f, 0.94f, 0.82f};
    constexpr Color pointerColor{0.16f, 0.29f, 0.42f};
    std::optional<render::NodeLayout> attachedLayout;

    if (m_CurrNode) {
        const view::NodeViewState& node = findNodeVisual(
            m_CurrNode->example->nodes,
            m_CurrNode->id);
        attachedLayout = layoutFor(node);
    }

    const float clampX = CLAMP_CENTER_X - 78.0f;
    constexpr float clampWidth = 156.0f;
    constexpr float clampHeight = 56.0f;

    drawRectangle({clampX, CLAMP_Y, clampWidth, clampHeight}, clampBodyColor);
    drawRectangle(
        {clampX + 10.0f, CLAMP_Y + 10.0f, clampWidth - 20.0f, clampHeight - 20.0f},
        clampInsetColor);
    drawRectangle(
        {CLAMP_CENTER_X - 9.0f, CLAMP_Y + clampHeight, 18.0f, 34.0f},
        clampBodyColor);
    m_RobotoMonoPointerTextRenderer->draw(
        "curr",
        CLAMP_CENTER_X - m_RobotoMonoPointerTextRenderer->measureWidth("curr") / 2.0f,
        CLAMP_Y + 36.0f,
        clampLabelColor.red,
        clampLabelColor.green,
        clampLabelColor.blue,
        clampLabelColor.alpha);
    if (attachedLayout) {
        drawDirectionalPointer(
            CLAMP_CENTER_X,
            CLAMP_Y + clampHeight + 34.0f,
            attachedLayout->referenceDock.centerX,
            attachedLayout->referenceDock.centerY + attachedLayout->referenceDock.radius,
            pointerColor,
            1.0f);
    }
}

void Application::drawNodeCard(
    const ListExample& example,
    const view::NodeViewState& node) const {
    constexpr Color nodeColor{0.07f, 0.10f, 0.15f};
    constexpr Color hoverOutline{0.18f, 0.58f, 0.95f};
    constexpr Color selectedOutline{0.08f, 0.37f, 0.88f};
    constexpr Color selectedNodeColor{0.05f, 0.15f, 0.29f};
    constexpr Color tapeSourceOutline{0.91f, 0.50f, 0.17f};
    constexpr Color tapeTargetOutline{0.96f, 0.72f, 0.24f};
    constexpr float hoverOutlineWidth = 5.0f;
    constexpr float selectedOutlineWidth = 8.0f;
    constexpr float tapeSourceOutlineWidth = 7.0f;
    constexpr float tapeTargetOutlineWidth = 8.0f;
    const render::NodeLayout layout = layoutFor(node);
    const bool selected = isNodeSelected(example, node.id);
    const bool tapeSource = isTapeSource(example, node.id);
    const bool hovered = isNodeHovered(example, node.id) && !tapeSource;
    const bool tapeTarget = isTapeTarget(example, node.id);
    const float outlineWidth = tapeTarget
                                   ? tapeTargetOutlineWidth
                                   : (selected ? selectedOutlineWidth
                                               : (hovered ? hoverOutlineWidth : 0.0f));
    const Color& cardColor = selected ? selectedNodeColor : nodeColor;

    if (tapeSource) {
        drawCircle(
            layout.outputSocket.centerX,
            layout.outputSocket.centerY,
            layout.outputSocket.radius + tapeSourceOutlineWidth,
            tapeSourceOutline);
    }

    if (outlineWidth > 0.0f) {
        const Color& outlineColor =
            tapeTarget ? tapeTargetOutline : (selected ? selectedOutline : hoverOutline);
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
    drawListPointers(example);
    drawNullEndpoints(example);
}

void Application::drawListPointers(const ListExample& example) const {
    constexpr Color pointerColor{0.16f, 0.29f, 0.42f};
    constexpr Color scissorsGlowColor{1.0f, 0.59f, 0.24f};
    constexpr Color scissorsPointerColor{1.0f, 0.83f, 0.41f};

    for (const view::NodeViewState& source : example.nodes) {
        const std::optional<model::NodeId> destinationId = example.list.node(source.id).next;
        if (!destinationId) {
            continue;
        }

        const render::NodeLayout sourceLayout = layoutFor(source);
        const view::NodeViewState& destination = findNodeVisual(example.nodes, *destinationId);
        const render::NodeLayout destinationLayout = layoutFor(destination);
        if (isScissorsTarget(example, source.id)) {
            drawDirectionalPointer(
                sourceLayout.outputSocket.centerX,
                sourceLayout.outputSocket.centerY,
                destinationLayout.card.x,
                destinationLayout.card.y + destinationLayout.card.height / 2.0f,
                scissorsGlowColor,
                1.7f);
        }
        drawDirectionalPointer(
            sourceLayout.outputSocket.centerX,
            sourceLayout.outputSocket.centerY,
            destinationLayout.card.x,
            destinationLayout.card.y + destinationLayout.card.height / 2.0f,
            isScissorsTarget(example, source.id) ? scissorsPointerColor : pointerColor,
            1.0f);
    }
}

void Application::drawHeadIndicator(const ListExample& example) const {
    constexpr Color labelColor{0.07f, 0.10f, 0.15f};
    constexpr Color pointerColor{0.16f, 0.29f, 0.42f};
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
        layout.referenceDock.centerY + layout.referenceDock.radius,
        pointerColor,
        1.0f);
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
    float destinationY,
    const Color& color,
    float sizeScale) const {
    constexpr float baseLineHalfWidth = 5.0f;
    constexpr float baseArrowLength = 28.0f;
    constexpr float baseArrowHalfWidth = 16.0f;
    const float lineHalfWidth = baseLineHalfWidth * sizeScale;
    const float arrowLength = baseArrowLength * sizeScale;
    const float arrowHalfWidth = baseArrowHalfWidth * sizeScale;

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
        color.red,
        color.green,
        color.blue,
        color.alpha);
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

std::optional<Application::HeldTool> Application::toolAt(float x, float y) const {
    if (isPointInsideRectangle(TAPE_TOOL_SLOT, x, y)) {
        return HeldTool::Tape;
    }
    if (isPointInsideRectangle(SCISSORS_TOOL_SLOT, x, y)) {
        return HeldTool::Scissors;
    }
    return std::nullopt;
}

bool Application::isAdvanceToolAt(float x, float y) const {
    return isPointInsideRectangle(ADVANCE_TOOL_SLOT, x, y);
}

std::optional<Application::NodeReference> Application::findScissorsTarget(float x, float y) const {
    for (auto iterator = m_DrawOrder.rbegin(); iterator != m_DrawOrder.rend(); ++iterator) {
        const view::NodeViewState& source = findNodeVisual(iterator->example->nodes, iterator->id);
        const std::optional<model::NodeId> destinationId =
            iterator->example->list.node(iterator->id).next;
        if (!destinationId) {
            continue;
        }

        const view::NodeViewState& destination = findNodeVisual(
            iterator->example->nodes,
            *destinationId);
        const render::NodeLayout sourceLayout = layoutFor(source);
        const render::NodeLayout destinationLayout = layoutFor(destination);
        if (render::isPointInsidePointerHitArea(
                sourceLayout.outputSocket.centerX,
                sourceLayout.outputSocket.centerY,
                destinationLayout.card.x,
                destinationLayout.card.y + destinationLayout.card.height / 2.0f,
                x,
                y,
                SCISSORS_POINTER_HIT_RADIUS)) {
            return *iterator;
        }
    }
    return std::nullopt;
}

std::optional<Application::NodeReference> Application::findTapeOutput(float x, float y) const {
    for (auto iterator = m_DrawOrder.rbegin(); iterator != m_DrawOrder.rend(); ++iterator) {
        const view::NodeViewState& node = findNodeVisual(iterator->example->nodes, iterator->id);
        if (render::isPointInsideNextOutput(node, x, y)) {
            return *iterator;
        }
    }
    return std::nullopt;
}

bool Application::isScissorsTarget(const ListExample& example, model::NodeId id) const {
    return m_HoveredScissorsTarget && m_HoveredScissorsTarget->example == &example &&
           m_HoveredScissorsTarget->id == id;
}

bool Application::isTapeSource(const ListExample& example, model::NodeId id) const {
    const auto matches = [&example, id](const std::optional<NodeReference>& reference) {
        return reference && reference->example == &example && reference->id == id;
    };
    return matches(m_HoveredTapeSource) ||
           (m_TapeDrag && m_TapeDrag->source.example == &example &&
            m_TapeDrag->source.id == id);
}

bool Application::isTapeTarget(const ListExample& example, model::NodeId id) const {
    return m_HoveredTapeTarget && m_HoveredTapeTarget->example == &example &&
           m_HoveredTapeTarget->id == id;
}

render::Rectangle Application::randomClampEjectionPosition() {
    constexpr float minimumHorizontalOffset = 50.0f;
    constexpr float maximumHorizontalOffset = 400.0f;
    constexpr float minimumVerticalOffset = 50.0f;
    constexpr float maximumVerticalOffset = 200.0f;
    std::uniform_real_distribution<float> horizontalOffsetDistribution(
        minimumHorizontalOffset,
        maximumHorizontalOffset);
    std::uniform_real_distribution<float> verticalOffsetDistribution(
        minimumVerticalOffset,
        maximumVerticalOffset);
    std::bernoulli_distribution directionDistribution;
    const float horizontalOffset = horizontalOffsetDistribution(m_RandomEngine);

    return {
        .x = CLAMP_SLOT.x + (directionDistribution(m_RandomEngine)
                                  ? horizontalOffset
                                  : -horizontalOffset),
        .y = CLAMP_SLOT.y + CLAMP_SLOT.height + verticalOffsetDistribution(m_RandomEngine),
        .width = render::NODE_CARD_WIDTH,
        .height = render::NODE_CARD_HEIGHT,
    };
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

    if (m_CurrNode && m_CurrNode->example == node.example && m_CurrNode->id == node.id) {
        m_CurrNode.reset();
    }
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
