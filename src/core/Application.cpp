#include "core/Application.hpp"
#include "core/Input.hpp"
#include "render/PointerHitTest.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace game::core {
namespace {

constexpr float WORKBENCH_X = 60.0f;
constexpr float WORKBENCH_Y = 50.0f;
constexpr float WORKBENCH_WIDTH = Window::TARGET_WIDTH - (2.0f * WORKBENCH_X);
constexpr float WORKBENCH_HEIGHT = (Window::TARGET_HEIGHT * 3.0f) / 5.0f;
constexpr float DRAWER_Y = WORKBENCH_Y + WORKBENCH_HEIGHT;
constexpr float DRAWER_HEIGHT = 50.0f;
constexpr render::Rectangle HINT_DRAWER_HANDLE{
    1595.0f,
    DRAWER_Y + 20.0f,
    90.0f,
    10.0f,
};
constexpr render::Rectangle HINT_DRAWER_KEY_TARGET{
    HINT_DRAWER_HANDLE.x - 6.0f,
    HINT_DRAWER_HANDLE.y - 6.0f,
    HINT_DRAWER_HANDLE.width + 12.0f,
    HINT_DRAWER_HANDLE.height + 12.0f,
};
const std::filesystem::path INITIAL_SANDBOX_PATH{"assets/sandbox/initial.json"};
const std::filesystem::path SANDBOX_SAVE_PATH{"saves/sandbox.json"};
constexpr render::Rectangle INIT_BUTTON{
    WORKBENCH_X,
    Window::TARGET_HEIGHT - 58.0f,
    82.0f,
    34.0f,
};
constexpr float TOOLBOX_WIDTH = 255.0f;
constexpr float TOOLBOX_HEIGHT = 488.0f;
constexpr float TOOLBOX_X = WORKBENCH_X + WORKBENCH_WIDTH - TOOLBOX_WIDTH - 50.0f;
constexpr float TOOLBOX_Y = WORKBENCH_Y + 120.0f;
constexpr float TOOL_BUTTON_X = TOOLBOX_X + 34.0f;
constexpr float TOOL_BUTTON_Y = TOOLBOX_Y + 79.0f;
constexpr float TOOL_BUTTON_WIDTH = 187.0f;
constexpr float TOOL_BUTTON_HEIGHT = 66.0f;
constexpr float TOOL_BUTTON_GAP = 17.0f;
constexpr float TOOL_ICON_SIZE = 54.0f;
constexpr float HELD_TOOL_ICON_SIZE = 76.0f;
constexpr float CURR_CLAMP_CENTER_X = Window::TARGET_WIDTH / 2.0f;
constexpr float TEMP_CLAMP_CENTER_X = CURR_CLAMP_CENTER_X + 290.0f;
constexpr float CLAMP_Y = WORKBENCH_Y + 55.0f;
constexpr float CLAMP_SLOT_SIZE = render::NODE_CARD_WIDTH;
constexpr render::Rectangle CURR_CLAMP_SLOT{
    CURR_CLAMP_CENTER_X - (CLAMP_SLOT_SIZE / 2.0f),
    WORKBENCH_Y + 135.0f,
    CLAMP_SLOT_SIZE,
    CLAMP_SLOT_SIZE,
};
constexpr render::Rectangle KEY_TOOL_SLOT{
    TOOL_BUTTON_X,
    TOOL_BUTTON_Y + (2.0f * (TOOL_BUTTON_HEIGHT + TOOL_BUTTON_GAP)),
    TOOL_BUTTON_WIDTH,
    TOOL_BUTTON_HEIGHT,
};
constexpr render::Rectangle PAPERCLIP_TOOL_SLOT{
    TOOL_BUTTON_X,
    TOOL_BUTTON_Y + (3.0f * (TOOL_BUTTON_HEIGHT + TOOL_BUTTON_GAP)),
    TOOL_BUTTON_WIDTH,
    TOOL_BUTTON_HEIGHT,
};
constexpr render::Rectangle TEMP_CLAMP_SLOT{
    TEMP_CLAMP_CENTER_X - (CLAMP_SLOT_SIZE / 2.0f),
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
constexpr float ADVANCE_TOOL_WIDTH = 156.0f;
constexpr float ADVANCE_TOOL_HEIGHT = 38.0f;
constexpr float ADVANCE_TOOL_Y = WORKBENCH_Y + 8.0f;
constexpr render::Rectangle CURR_ADVANCE_TOOL_SLOT{
    CURR_CLAMP_CENTER_X - (ADVANCE_TOOL_WIDTH / 2.0f),
    ADVANCE_TOOL_Y,
    ADVANCE_TOOL_WIDTH,
    ADVANCE_TOOL_HEIGHT,
};
constexpr render::Rectangle TEMP_ADVANCE_TOOL_SLOT{
    TEMP_CLAMP_CENTER_X - (ADVANCE_TOOL_WIDTH / 2.0f),
    ADVANCE_TOOL_Y,
    ADVANCE_TOOL_WIDTH,
    ADVANCE_TOOL_HEIGHT,
};
constexpr int CIRCLE_SEGMENTS = 24;
constexpr float PI = 3.14159265358979323846f;
constexpr float ADVANCE_FEEDBACK_DURATION = 0.5f;
constexpr float SCISSORS_POINTER_HIT_RADIUS = 20.0f;
constexpr float TAPE_DRAG_THRESHOLD = 8.0f;
constexpr float DECK_OFFSET_STEP = 16.0f;

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
    m_Theme = std::make_unique<Theme>(Theme::loadFromFile("assets/themes/test/colors.json"));
    m_Window = std::make_unique<Window>(1280, 720, "Linked List Game", *m_Theme);
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
    m_TapeInactiveTexture = std::make_unique<TextureRenderer>(
        "assets/themes/test/tools/tape_inactive.png");
    m_TapeActiveTexture = std::make_unique<TextureRenderer>(
        "assets/themes/test/tools/tape_active.png");
    m_ScissorsInactiveTexture = std::make_unique<TextureRenderer>(
        "assets/themes/test/tools/scissors_inactive.png");
    m_ScissorsActiveTexture = std::make_unique<TextureRenderer>(
        "assets/themes/test/tools/scissors_active.png");
    m_KeyInactiveTexture = std::make_unique<TextureRenderer>(
        "assets/themes/test/tools/key_inactive.png");
    m_KeyActiveTexture = std::make_unique<TextureRenderer>(
        "assets/themes/test/tools/key_active.png");
    m_PaperclipInactiveTexture = std::make_unique<TextureRenderer>(
        "assets/themes/test/tools/paperclip_inactive.png");
    m_PaperclipActiveTexture = std::make_unique<TextureRenderer>(
        "assets/themes/test/tools/paperclip_active.png");
    std::error_code error;
    const bool hasSave = std::filesystem::exists(SANDBOX_SAVE_PATH, error);
    if (error) {
        throw std::runtime_error("Failed to inspect sandbox save state: " + error.message());
    }
    loadSandbox(hasSave ? SANDBOX_SAVE_PATH : INITIAL_SANDBOX_PATH);
    if (!hasSave) {
        saveSandbox();
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
    const auto updateAdvanceFeedback = [deltaTime](
                                           AdvanceFeedback& feedback,
                                           float& feedbackRemaining) {
        if (feedbackRemaining > 0.0f) {
            feedbackRemaining = std::max(0.0f, feedbackRemaining - deltaTime);
            if (feedbackRemaining == 0.0f) {
                feedback = AdvanceFeedback::None;
            }
        }
    };
    updateAdvanceFeedback(m_CurrAdvanceFeedback, m_CurrAdvanceFeedbackRemaining);
    updateAdvanceFeedback(m_TempAdvanceFeedback, m_TempAdvanceFeedbackRemaining);

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
    const auto drawNode = [this](const NodeReference& reference) {
        const view::NodeViewState& node = findNodeVisual(reference.example->nodes, reference.id);
        drawNodeCard(*reference.example, node);
        drawNodeText(*reference.example, node);
    };
    for (const NodeReference& reference : m_DrawOrder) {
        if (isDeckMember(reference)) {
            continue;
        }
        if (const std::optional<std::size_t> deckIndex = deckIndexForLead(reference)) {
            const DeckState& deck = m_Decks.at(*deckIndex);
            for (auto member = deck.members.rbegin(); member != deck.members.rend(); ++member) {
                drawNode({.example = deck.lead.example, .id = member->id});
            }
        }
        drawNode(reference);
    }
    drawHeadIndicator(m_CorrectExample);
    drawClamp(CURR_CLAMP_CENTER_X, "curr", m_CurrNode);
    drawClamp(TEMP_CLAMP_CENTER_X, "temp", m_TempNode);
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
    const std::optional<ClampTarget> clickedAdvance =
        Input::isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)
            ? advanceToolAt(mouse.x, mouse.y)
            : std::nullopt;
    const std::optional<ClampTarget> clickedEmptyClampSlot =
        Input::isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT) &&
                m_HeldTool == HeldTool::None
            ? emptyClampSlotAt(mouse.x, mouse.y)
            : std::nullopt;
    const bool clickedInit =
        Input::isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT) &&
        isInitButtonAt(mouse.x, mouse.y);
    const bool nodeHasInputPriority =
        Input::isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT) &&
        m_HeldTool == HeldTool::None &&
        findTopmostCard(mouse.x, mouse.y).has_value();
    if (clickedTool && !nodeHasInputPriority) {
        m_HeldTool = m_HeldTool == *clickedTool ? HeldTool::None : *clickedTool;
        m_DragState.reset();
        m_TapeDrag.reset();
    } else if (clickedAdvance && !nodeHasInputPriority) {
        if (*clickedAdvance == ClampTarget::Curr) {
            triggerAdvance(
                m_CurrNode,
                m_CurrAdvanceFeedback,
                m_CurrAdvanceFeedbackRemaining);
        } else {
            triggerAdvance(
                m_TempNode,
                m_TempAdvanceFeedback,
                m_TempAdvanceFeedbackRemaining);
        }
        m_DragState.reset();
        m_TapeDrag.reset();
    } else if (clickedEmptyClampSlot && !nodeHasInputPriority) {
        spawnNodeAtClamp(*clickedEmptyClampSlot);
        m_SelectedNode.reset();
        m_DragState.reset();
    } else if (clickedInit && !nodeHasInputPriority) {
        resetSandbox();
    } else if (
        Input::isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT) &&
        m_HeldTool == HeldTool::Scissors) {
        if (const std::optional<NodeReference> scissorsTarget =
                findScissorsTarget(mouse.x, mouse.y)) {
            expandDeckContaining(*scissorsTarget);
            scissorsTarget->example->list.setNext(scissorsTarget->id, std::nullopt);
            saveSandbox();
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
                if (const std::optional<NodeReference> target = findTapeTarget(mouse.x, mouse.y)) {
                    expandDeckContaining(m_TapeDrag->source);
                    expandDeckContaining(*target);
                    m_TapeDrag->source.example->list.setNext(m_TapeDrag->source.id, target->id);
                    saveSandbox();
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
            if (const std::optional<NodeReference> target = findTapeTarget(mouse.x, mouse.y)) {
                expandDeckContaining(m_TapeDrag->source);
                expandDeckContaining(*target);
                m_TapeDrag->source.example->list.setNext(m_TapeDrag->source.id, target->id);
                saveSandbox();
            }
            m_TapeDrag.reset();
        }
    } else if (
        Input::isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT) &&
        m_HeldTool == HeldTool::Paperclip) {
        if (const std::optional<NodeReference> lead = findTopmostCard(mouse.x, mouse.y)) {
            toggleDeck(*lead);
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
                if (isNodeClamped({.example = hitNode->example, .id = downstreamId})) {
                    break;
                }
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
            if (isPointInsideRectangle(CURR_CLAMP_SLOT, centerX, centerY)) {
                attachDraggedNodeToClamp(m_CurrNode, CURR_CLAMP_SLOT);
            } else if (isPointInsideRectangle(TEMP_CLAMP_SLOT, centerX, centerY)) {
                attachDraggedNodeToClamp(m_TempNode, TEMP_CLAMP_SLOT);
            }
            updateDeckOriginalPositions(*m_DragState);
            m_DragState.reset();
            saveSandbox();
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
    m_HoveredTapeTarget = findTapeTarget(mouse.x, mouse.y);
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
    constexpr float tableBorderWidth = 14.0f;
    const Theme& theme = *m_Theme;

    drawRectangle(
        {WORKBENCH_X, WORKBENCH_Y, WORKBENCH_WIDTH, WORKBENCH_HEIGHT},
        theme.workbench.border);
    drawRectangle(
        {
            WORKBENCH_X + tableBorderWidth,
            WORKBENCH_Y + tableBorderWidth,
            WORKBENCH_WIDTH - (2.0f * tableBorderWidth),
            WORKBENCH_HEIGHT - (2.0f * tableBorderWidth),
        },
        theme.workbench.surface);
    m_RobotoRegularTextRenderer->draw(
        "Pointer Workbench",
        WORKBENCH_X + 32.0f,
        WORKBENCH_Y + 45.0f,
        theme.workbench.title.red,
        theme.workbench.title.green,
        theme.workbench.title.blue,
        theme.workbench.title.alpha);
    if (isInitButtonHovered()) {
        drawRectangle(
            {
                INIT_BUTTON.x - 6.0f,
                INIT_BUTTON.y - 6.0f,
                INIT_BUTTON.width + 12.0f,
                INIT_BUTTON.height + 12.0f,
            },
            theme.workbench.toolbox.selected);
    }
    drawRectangle(
        INIT_BUTTON,
        isInitButtonHovered() ? theme.workbench.toolbox.selected : theme.workbench.toolbox.button);
    m_RobotoMonoPointerTextRenderer->draw(
        "INIT",
        INIT_BUTTON.x + 17.0f,
        INIT_BUTTON.y + 25.0f,
        theme.workbench.toolbox.inactiveLabel.red,
        theme.workbench.toolbox.inactiveLabel.green,
        theme.workbench.toolbox.inactiveLabel.blue,
        theme.workbench.toolbox.inactiveLabel.alpha);
    drawAdvanceTool(CURR_ADVANCE_TOOL_SLOT, "curr", m_CurrAdvanceFeedback);
    drawAdvanceTool(TEMP_ADVANCE_TOOL_SLOT, "temp", m_TempAdvanceFeedback);
    drawClampSlot(CURR_CLAMP_SLOT, m_CurrNode.has_value());
    drawClampSlot(TEMP_CLAMP_SLOT, m_TempNode.has_value());
    drawTemplateScroll();
    drawToolbox();
    drawRubbishBin();
    drawDrawers();
}

void Application::drawToolbox() const {
    const Theme::Workbench::Toolbox& toolbox = m_Theme->workbench.toolbox;

    drawRectangle({TOOLBOX_X, TOOLBOX_Y, TOOLBOX_WIDTH, TOOLBOX_HEIGHT}, toolbox.border);
    drawRectangle(
        {
            TOOLBOX_X + 10.0f,
            TOOLBOX_Y + 10.0f,
            TOOLBOX_WIDTH - 20.0f,
            TOOLBOX_HEIGHT - 20.0f,
        },
        toolbox.surface);
    m_RobotoMonoPointerTextRenderer->draw(
        "TOOLBOX",
        TOOLBOX_X + 36.0f,
        TOOLBOX_Y + 40.0f,
        toolbox.label.red,
        toolbox.label.green,
        toolbox.label.blue,
        toolbox.label.alpha);

    drawRectangle(TAPE_TOOL_SLOT, toolbox.button);
    drawRectangle(SCISSORS_TOOL_SLOT, toolbox.button);
    drawRectangle(KEY_TOOL_SLOT, toolbox.button);
    drawRectangle(PAPERCLIP_TOOL_SLOT, toolbox.button);
    if (m_HeldTool != HeldTool::Tape) {
        m_TapeInactiveTexture->draw(
            TAPE_TOOL_SLOT.x + 6.0f,
            TAPE_TOOL_SLOT.y + 6.0f,
            TOOL_ICON_SIZE,
            TOOL_ICON_SIZE);
        m_RobotoMonoPointerTextRenderer->draw(
            "ASSIGN",
            TAPE_TOOL_SLOT.x + 74.0f,
            TOOL_BUTTON_Y + 44.0f,
            toolbox.inactiveLabel.red,
            toolbox.inactiveLabel.green,
            toolbox.inactiveLabel.blue,
            toolbox.inactiveLabel.alpha);
    }
    if (m_HeldTool != HeldTool::Key) {
        m_KeyInactiveTexture->draw(
            KEY_TOOL_SLOT.x + 6.0f,
            KEY_TOOL_SLOT.y + 6.0f,
            TOOL_ICON_SIZE,
            TOOL_ICON_SIZE);
        m_RobotoMonoPointerTextRenderer->draw(
            "HINT",
            KEY_TOOL_SLOT.x + 74.0f,
            KEY_TOOL_SLOT.y + 44.0f,
            toolbox.inactiveLabel.red,
            toolbox.inactiveLabel.green,
            toolbox.inactiveLabel.blue,
            toolbox.inactiveLabel.alpha);
    }
    if (m_HeldTool != HeldTool::Scissors) {
        m_ScissorsInactiveTexture->draw(
            SCISSORS_TOOL_SLOT.x + 6.0f,
            SCISSORS_TOOL_SLOT.y + 6.0f,
            TOOL_ICON_SIZE,
            TOOL_ICON_SIZE);
        m_RobotoMonoPointerTextRenderer->draw(
            "TO nullptr",
            SCISSORS_TOOL_SLOT.x + 65.0f,
            TOOL_BUTTON_Y + TOOL_BUTTON_HEIGHT + TOOL_BUTTON_GAP + 44.0f,
            toolbox.inactiveLabel.red,
            toolbox.inactiveLabel.green,
            toolbox.inactiveLabel.blue,
            toolbox.inactiveLabel.alpha);
    }
    if (m_HeldTool != HeldTool::Paperclip) {
        m_PaperclipInactiveTexture->draw(
            PAPERCLIP_TOOL_SLOT.x + 6.0f,
            PAPERCLIP_TOOL_SLOT.y + 6.0f,
            TOOL_ICON_SIZE,
            TOOL_ICON_SIZE);
        m_RobotoMonoPointerTextRenderer->draw(
            "CLIP",
            PAPERCLIP_TOOL_SLOT.x + 74.0f,
            PAPERCLIP_TOOL_SLOT.y + 44.0f,
            toolbox.inactiveLabel.red,
            toolbox.inactiveLabel.green,
            toolbox.inactiveLabel.blue,
            toolbox.inactiveLabel.alpha);
    }
}

void Application::drawAdvanceTool(
    const render::Rectangle& slot,
    const char* clampLabel,
    AdvanceFeedback feedback) const {
    const Theme::Workbench::Toolbox& toolbox = m_Theme->workbench.toolbox;
    if (feedback == AdvanceFeedback::Error) {
        drawRectangle(
            {slot.x - 6.0f, slot.y - 6.0f, slot.width + 12.0f, slot.height + 12.0f},
            toolbox.errorGlow);
    }
    drawRectangle(
        slot,
        feedback == AdvanceFeedback::Error
            ? toolbox.error
            : (feedback == AdvanceFeedback::Activated ? toolbox.selected : toolbox.button));
    const std::string label = std::string("ADVANCE ") + clampLabel;
    m_RobotoMonoPointerTextRenderer->draw(
        label,
        slot.x + 6.0f,
        slot.y + 27.0f,
        toolbox.inactiveLabel.red,
        toolbox.inactiveLabel.green,
        toolbox.inactiveLabel.blue,
        toolbox.inactiveLabel.alpha);
}

void Application::drawClampSlot(const render::Rectangle& slot, bool isOccupied) const {
    if (m_DragState && m_HeldTool == HeldTool::None) {
        const view::NodeViewState& draggedNode = findNodeVisual(
            m_DragState->node.example->nodes,
            m_DragState->node.id);
        const float centerX = draggedNode.x + (render::NODE_CARD_WIDTH / 2.0f);
        const float centerY = draggedNode.y + (render::NODE_CARD_HEIGHT / 2.0f);
        if (isPointInsideRectangle(slot, centerX, centerY)) {
            drawDashedNodeSilhouette(
                render::makeNodeLayout(slot.x, slot.y),
                m_Theme->workbench.clamp.dropPreview);
            return;
        }
    }

    constexpr float dashLength = 18.0f;
    constexpr float dashGap = 12.0f;
    constexpr float dashWidth = 4.0f;
    const float right = slot.x + slot.width;
    const float bottom = slot.y + slot.height;

    for (float x = slot.x; x < right; x += dashLength + dashGap) {
        const float width = std::min(dashLength, right - x);
        drawRectangle({x, slot.y, width, dashWidth}, m_Theme->workbench.clamp.slotOutline);
        drawRectangle(
            {x, bottom - dashWidth, width, dashWidth},
            m_Theme->workbench.clamp.slotOutline);
    }
    for (float y = slot.y; y < bottom; y += dashLength + dashGap) {
        const float height = std::min(dashLength, bottom - y);
        drawRectangle({slot.x, y, dashWidth, height}, m_Theme->workbench.clamp.slotOutline);
        drawRectangle(
            {right - dashWidth, y, dashWidth, height},
            m_Theme->workbench.clamp.slotOutline);
    }
    if (!isOccupied) {
        constexpr float plusLength = 44.0f;
        constexpr float plusWidth = 6.0f;
        const float centerX = slot.x + (slot.width / 2.0f);
        const float centerY = slot.y + (slot.height / 2.0f);
        drawRectangle(
            {
                centerX - (plusLength / 2.0f),
                centerY - (plusWidth / 2.0f),
                plusLength,
                plusWidth,
            },
            m_Theme->workbench.clamp.slotOutline);
        drawRectangle(
            {
                centerX - (plusWidth / 2.0f),
                centerY - (plusLength / 2.0f),
                plusWidth,
                plusLength,
            },
            m_Theme->workbench.clamp.slotOutline);
    }
}

void Application::drawDashedNodeSilhouette(
    const render::NodeLayout& layout,
    const Color& color) const {
    constexpr float dashLength = 18.0f;
    constexpr float dashGap = 12.0f;
    constexpr float dashWidth = 4.0f;
    constexpr float dashSpacing = dashLength + dashGap;
    constexpr int circleSegments = 24;

    const auto drawHorizontalDashes = [this, &color, dashLength](float left, float right, float y) {
        for (float x = left; x < right; x += dashSpacing) {
            drawRectangle(
                {x, y - (dashWidth / 2.0f), std::min(dashLength, right - x), dashWidth},
                color);
        }
    };
    const auto drawVerticalDashes = [this, &color, dashLength](float x, float top, float bottom) {
        for (float y = top; y < bottom; y += dashSpacing) {
            drawRectangle(
                {x - (dashWidth / 2.0f), y, dashWidth, std::min(dashLength, bottom - y)},
                color);
        }
    };
    const auto drawDashedCircle = [this, &color](const render::Circle& circle) {
        constexpr int dashCount = 9;
        for (int dash = 0; dash < dashCount; ++dash) {
            const float angle = (2.0f * PI * dash) / dashCount;
            drawCircle(
                circle.centerX + (circle.radius * std::cos(angle)),
                circle.centerY + (circle.radius * std::sin(angle)),
                dashWidth / 2.0f,
                color);
        }
    };

    const float cardLeft = layout.card.x;
    const float cardRight = layout.card.x + layout.card.width;
    const float cardTop = layout.card.y;
    const float cardBottom = layout.card.y + layout.card.height;
    const float dockTopOffset = cardTop - layout.referenceDock.centerY;
    const float dockIntersectionOffset = std::sqrt(
        (layout.referenceDock.radius * layout.referenceDock.radius) -
        (dockTopOffset * dockTopOffset));
    const float dockLeft = layout.referenceDock.centerX - dockIntersectionOffset;
    const float dockRight = layout.referenceDock.centerX + dockIntersectionOffset;

    drawHorizontalDashes(cardLeft, dockLeft, cardTop);
    drawHorizontalDashes(dockRight, cardRight, cardTop);
    drawHorizontalDashes(cardLeft, cardRight, cardBottom);
    drawVerticalDashes(cardLeft, cardTop, cardBottom);
    drawVerticalDashes(cardRight, cardTop, cardBottom);

    const float dockStartAngle = std::atan2(
        cardTop - layout.referenceDock.centerY,
        dockLeft - layout.referenceDock.centerX);
    const float dockEndAngle = std::atan2(
        cardTop - layout.referenceDock.centerY,
        dockRight - layout.referenceDock.centerX) -
                               (2.0f * PI);
    for (int segment = 0; segment <= circleSegments; ++segment) {
        if (segment % 2 != 0) {
            continue;
        }
        const float progress = static_cast<float>(segment) / circleSegments;
        const float angle = dockStartAngle + ((dockEndAngle - dockStartAngle) * progress);
        drawCircle(
            layout.referenceDock.centerX + (layout.referenceDock.radius * std::cos(angle)),
            layout.referenceDock.centerY + (layout.referenceDock.radius * std::sin(angle)),
            dashWidth / 2.0f,
            color);
    }
    drawDashedCircle(layout.outputSocket);
}

void Application::drawTemplateScroll() const {
    constexpr render::Rectangle scroll{
        WORKBENCH_X + 42.0f,
        WORKBENCH_Y + 105.0f,
        350.0f,
        390.0f,
    };

    drawRectangle(
        {scroll.x + 9.0f, scroll.y + 10.0f, scroll.width, scroll.height},
        m_Theme->workbench.templateScroll.shadow);
    drawRectangle(scroll, m_Theme->workbench.templateScroll.paper);
    drawRectangle(
        {scroll.x - 8.0f, scroll.y - 10.0f, scroll.width + 16.0f, 20.0f},
        m_Theme->workbench.templateScroll.roll);
    drawRectangle(
        {scroll.x - 8.0f, scroll.y + scroll.height - 10.0f, scroll.width + 16.0f, 20.0f},
        m_Theme->workbench.templateScroll.roll);

    constexpr std::array<std::string_view, 5> placeholderLines{
        "// placeholder",
        "struct Node_T {",
        "    int val;",
        "    Node_T* next;",
        "};",
    };
    constexpr float textX = scroll.x + 28.0f;
    constexpr float textY = scroll.y + 70.0f;
    constexpr float lineHeight = 42.0f;
    const Color& textColor = m_Theme->workbench.templateScroll.text;
    for (std::size_t line = 0; line < placeholderLines.size(); ++line) {
        m_RobotoMonoPointerTextRenderer->draw(
            placeholderLines[line],
            textX,
            textY + static_cast<float>(line) * lineHeight,
            textColor.red,
            textColor.green,
            textColor.blue,
            textColor.alpha);
    }
}

void Application::drawRubbishBin() const {
    constexpr float binX = 1660.0f;
    constexpr float binY = DRAWER_Y + DRAWER_HEIGHT + 34.0f;
    constexpr float binWidth = 150.0f;
    constexpr float binHeight = 160.0f;

    drawRectangle(
        {binX - 12.0f, binY, binWidth + 24.0f, 18.0f},
        m_Theme->workbench.rubbishBin.lid);
    drawRectangle({binX, binY + 18.0f, binWidth, binHeight}, m_Theme->workbench.rubbishBin.body);
    drawRectangle(
        {binX + 36.0f, binY - 12.0f, binWidth - 72.0f, 12.0f},
        m_Theme->workbench.rubbishBin.lid);
    drawRectangle(
        {binX + 40.0f, binY + 38.0f, 10.0f, binHeight - 38.0f},
        m_Theme->workbench.rubbishBin.detail);
    drawRectangle(
        {binX + 70.0f, binY + 38.0f, 10.0f, binHeight - 38.0f},
        m_Theme->workbench.rubbishBin.detail);
    drawRectangle(
        {binX + 100.0f, binY + 38.0f, 10.0f, binHeight - 38.0f},
        m_Theme->workbench.rubbishBin.detail);
    m_RobotoMonoPointerTextRenderer->draw(
        "BIN",
        binX + 56.0f,
        binY + binHeight + 46.0f,
        m_Theme->workbench.rubbishBin.label.red,
        m_Theme->workbench.rubbishBin.label.green,
        m_Theme->workbench.rubbishBin.label.blue,
        m_Theme->workbench.rubbishBin.label.alpha);
}

void Application::drawDrawers() const {
    constexpr render::Rectangle codeDrawer{80.0f, DRAWER_Y, 1320.0f, DRAWER_HEIGHT};
    constexpr render::Rectangle hintDrawer{1450.0f, DRAWER_Y, 380.0f, DRAWER_HEIGHT};

    drawRectangle(
        {
            codeDrawer.x - 6.0f,
            codeDrawer.y - 6.0f,
            codeDrawer.width + 12.0f,
            codeDrawer.height + 12.0f,
        },
        m_Theme->workbench.drawers.frame);
    drawRectangle(codeDrawer, m_Theme->workbench.drawers.face);
    drawRectangle(
        {
            hintDrawer.x - 6.0f,
            hintDrawer.y - 6.0f,
            hintDrawer.width + 12.0f,
            hintDrawer.height + 12.0f,
        },
        m_Theme->workbench.drawers.frame);
    drawRectangle(hintDrawer, m_Theme->workbench.drawers.face);
    drawRectangle(
        {
            codeDrawer.x + (codeDrawer.width / 2.0f) - 70.0f,
            codeDrawer.y + 20.0f,
            140.0f,
            10.0f,
        },
        m_Theme->workbench.drawers.handle);
    if (isKeyTargetHovered()) {
        drawRectangle(
            HINT_DRAWER_KEY_TARGET,
            m_Theme->workbench.toolbox.selected);
    }
    drawRectangle(
        HINT_DRAWER_HANDLE,
        isKeyTargetHovered()
            ? m_Theme->workbench.toolbox.selected
            : m_Theme->workbench.drawers.handle);
    m_RobotoMonoPointerTextRenderer->draw(
        "CODE DRAWER",
        codeDrawer.x + 36.0f,
        codeDrawer.y + 37.0f,
        m_Theme->workbench.drawers.label.red,
        m_Theme->workbench.drawers.label.green,
        m_Theme->workbench.drawers.label.blue,
        m_Theme->workbench.drawers.label.alpha);
    m_RobotoMonoPointerTextRenderer->draw(
        "[LOCKED] HINTS",
        hintDrawer.x + 38.0f,
        hintDrawer.y + 37.0f,
        m_Theme->workbench.drawers.label.red,
        m_Theme->workbench.drawers.label.green,
        m_Theme->workbench.drawers.label.blue,
        m_Theme->workbench.drawers.label.alpha);
}

void Application::drawHeldTool() const {
    if (m_HeldTool == HeldTool::None || !Input::isMouseInsideCanvas()) {
        return;
    }

    const glm::vec2 mouse = Input::getMousePosition();
    const TextureRenderer& activeTexture =
        m_HeldTool == HeldTool::Tape
            ? *m_TapeActiveTexture
            : (m_HeldTool == HeldTool::Scissors
                   ? *m_ScissorsActiveTexture
                   : (m_HeldTool == HeldTool::Key ? *m_KeyActiveTexture
                                                  : *m_PaperclipActiveTexture));
    activeTexture.draw(
        mouse.x - (HELD_TOOL_ICON_SIZE / 2.0f),
        mouse.y - (HELD_TOOL_ICON_SIZE / 2.0f),
        HELD_TOOL_ICON_SIZE,
        HELD_TOOL_ICON_SIZE);
}

void Application::drawTapePreview() const {
    if (!m_TapeDrag || !Input::isMouseInsideCanvas()) {
        return;
    }

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
        m_Theme->workbench.toolbox.selected,
        1.0f);
}

void Application::triggerAdvance(
    std::optional<NodeReference>& attachedNode,
    AdvanceFeedback& feedback,
    float& feedbackRemaining) {
    feedback = AdvanceFeedback::Error;
    if (attachedNode) {
        const std::optional<model::NodeId> nextNodeId =
            attachedNode->example->list.node(attachedNode->id).next;
        if (nextNodeId) {
            attachedNode = {.example = attachedNode->example, .id = *nextNodeId};
            feedback = AdvanceFeedback::Activated;
            saveSandbox();
        }
    }
    feedbackRemaining = ADVANCE_FEEDBACK_DURATION;
}

void Application::drawClamp(
    float centerX,
    const char* label,
    const std::optional<NodeReference>& attachedNode) const {
    std::optional<render::NodeLayout> attachedLayout;

    if (attachedNode) {
        const view::NodeViewState& node = findNodeVisual(
            attachedNode->example->nodes,
            attachedNode->id);
        attachedLayout = layoutFor(node);
    }

    const float clampX = centerX - 78.0f;
    constexpr float clampWidth = 156.0f;
    constexpr float clampHeight = 56.0f;

    drawRectangle({clampX, CLAMP_Y, clampWidth, clampHeight}, m_Theme->workbench.clamp.body);
    drawRectangle(
        {clampX + 10.0f, CLAMP_Y + 10.0f, clampWidth - 20.0f, clampHeight - 20.0f},
        m_Theme->workbench.clamp.inset);
    drawRectangle(
        {centerX - 9.0f, CLAMP_Y + clampHeight, 18.0f, 34.0f},
        m_Theme->workbench.clamp.body);
    m_RobotoMonoPointerTextRenderer->draw(
        label,
        centerX - m_RobotoMonoPointerTextRenderer->measureWidth(label) / 2.0f,
        CLAMP_Y + 36.0f,
        m_Theme->workbench.clamp.label.red,
        m_Theme->workbench.clamp.label.green,
        m_Theme->workbench.clamp.label.blue,
        m_Theme->workbench.clamp.label.alpha);
    if (attachedLayout) {
        drawDirectionalPointer(
            centerX,
            CLAMP_Y + clampHeight + 34.0f,
            attachedLayout->referenceDock.centerX,
            attachedLayout->referenceDock.centerY + attachedLayout->referenceDock.radius,
            m_Theme->pointer.normal,
            1.0f);
    }
}

void Application::drawNodeCard(
    const ListExample& example,
    const view::NodeViewState& node) const {
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
    const std::vector<Color>& palette = m_Theme->nodeCard.palette;
    const Color& cardColor = palette.empty()
                                 ? m_Theme->nodeCard.defaultFill
                                 : palette[node.paletteIndex % palette.size()];

    if (tapeSource) {
        drawCircle(
            layout.outputSocket.centerX,
            layout.outputSocket.centerY,
            layout.outputSocket.radius + tapeSourceOutlineWidth,
            m_Theme->nodeCard.tapeSourceGlow);
    }

    if (outlineWidth > 0.0f) {
        const Color& outlineColor =
            tapeTarget ? m_Theme->nodeCard.tapeTargetGlow
                       : (selected ? m_Theme->nodeCard.selectedGlow
                                   : m_Theme->nodeCard.hoverGlow);
        drawNodeSilhouette(layout, outlineWidth, outlineColor);
    }

    drawNodeSilhouette(layout, 0.0f, cardColor);
}

void Application::drawNodeText(
    const ListExample& example,
    const view::NodeViewState& node) const {
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
        m_Theme->nodeCard.text.red,
        m_Theme->nodeCard.text.green,
        m_Theme->nodeCard.text.blue,
        m_Theme->nodeCard.text.alpha);
    m_RobotoMonoPointerTextRenderer->draw(
        nextLabel,
        pointerLabelX,
        layout.outputSocket.centerY + 7.0f,
        m_Theme->nodeCard.text.red,
        m_Theme->nodeCard.text.green,
        m_Theme->nodeCard.text.blue,
        m_Theme->nodeCard.text.alpha);
}

void Application::drawExampleConnections(const ListExample& example) const {
    drawListPointers(example);
    drawNullEndpoints(example);
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
        if (isScissorsTarget(example, source.id)) {
            drawDirectionalPointer(
                sourceLayout.outputSocket.centerX,
                sourceLayout.outputSocket.centerY,
                destinationLayout.card.x,
                destinationLayout.card.y + destinationLayout.card.height / 2.0f,
                m_Theme->pointer.scissorsGlow,
                1.7f);
        }
        drawDirectionalPointer(
            sourceLayout.outputSocket.centerX,
            sourceLayout.outputSocket.centerY,
            destinationLayout.card.x,
            destinationLayout.card.y + destinationLayout.card.height / 2.0f,
            isScissorsTarget(example, source.id)
                ? m_Theme->pointer.scissorsHighlight
                : m_Theme->pointer.normal,
            1.0f);
    }
}

void Application::drawHeadIndicator(const ListExample& example) const {
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
        m_Theme->pointer.normal,
        1.0f);
    m_RobotoRegularTextRenderer->draw(
        headLabel,
        centerX - m_RobotoRegularTextRenderer->measureWidth(headLabel) / 2.0f,
        layout.card.y - 100.0f,
        m_Theme->pointer.label.red,
        m_Theme->pointer.label.green,
        m_Theme->pointer.label.blue,
        m_Theme->pointer.label.alpha);
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
    constexpr float lineLength = 100.0f;
    constexpr float lineHeight = 8.0f;
    constexpr float dashLength = 16.0f;
    constexpr float dashGap = 10.0f;
    constexpr float labelGap = 20.0f;

    const render::NodeLayout layout = layoutFor(node);
    const float lineX = layout.outputSocket.centerX + layout.outputSocket.radius;
    const float lineY = layout.outputSocket.centerY - lineHeight / 2.0f;
    for (float offset = 0.0f; offset < lineLength; offset += dashLength + dashGap) {
        drawRectangle(
            {
                lineX + offset,
                lineY,
                std::min(dashLength, lineLength - offset),
                lineHeight,
            },
            m_Theme->pointer.normal);
    }
    m_RobotoMonoPointerTextRenderer->draw(
        "nullptr",
        lineX + lineLength + labelGap,
        layout.outputSocket.centerY + 7.0f,
        m_Theme->pointer.label.red,
        m_Theme->pointer.label.green,
        m_Theme->pointer.label.blue,
        m_Theme->pointer.label.alpha);
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
    if (isPointInsideRectangle(KEY_TOOL_SLOT, x, y)) {
        return HeldTool::Key;
    }
    if (isPointInsideRectangle(PAPERCLIP_TOOL_SLOT, x, y)) {
        return HeldTool::Paperclip;
    }
    return std::nullopt;
}

std::optional<Application::ClampTarget> Application::advanceToolAt(float x, float y) const {
    if (isPointInsideRectangle(CURR_ADVANCE_TOOL_SLOT, x, y)) {
        return ClampTarget::Curr;
    }
    if (isPointInsideRectangle(TEMP_ADVANCE_TOOL_SLOT, x, y)) {
        return ClampTarget::Temp;
    }
    return std::nullopt;
}

std::optional<Application::ClampTarget> Application::emptyClampSlotAt(float x, float y) const {
    if (!m_CurrNode && isPointInsideRectangle(CURR_CLAMP_SLOT, x, y)) {
        return ClampTarget::Curr;
    }
    if (!m_TempNode && isPointInsideRectangle(TEMP_CLAMP_SLOT, x, y)) {
        return ClampTarget::Temp;
    }
    return std::nullopt;
}

bool Application::isInitButtonAt(float x, float y) const {
    return isPointInsideRectangle(INIT_BUTTON, x, y);
}

bool Application::isInitButtonHovered() const {
    if (!Input::isMouseInsideCanvas()) {
        return false;
    }

    const glm::vec2 mouse = Input::getMousePosition();
    return isInitButtonAt(mouse.x, mouse.y);
}

bool Application::isKeyTargetHovered() const {
    if (m_HeldTool != HeldTool::Key || !Input::isMouseInsideCanvas()) {
        return false;
    }

    const glm::vec2 mouse = Input::getMousePosition();
    return isPointInsideRectangle(HINT_DRAWER_KEY_TARGET, mouse.x, mouse.y);
}

std::optional<Application::NodeReference> Application::findScissorsTarget(float x, float y) const {
    for (auto iterator = m_DrawOrder.rbegin(); iterator != m_DrawOrder.rend(); ++iterator) {
        if (!isExternallyReferenced(*iterator)) {
            continue;
        }
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
        if (!isExternallyReferenced(*iterator)) {
            continue;
        }
        const view::NodeViewState& node = findNodeVisual(iterator->example->nodes, iterator->id);
        if (render::isPointInsideNextOutput(node, x, y)) {
            return *iterator;
        }
    }
    return std::nullopt;
}

std::optional<Application::NodeReference> Application::findTapeTarget(float x, float y) const {
    for (auto iterator = m_DrawOrder.rbegin(); iterator != m_DrawOrder.rend(); ++iterator) {
        if (!isExternallyReferenced(*iterator)) {
            continue;
        }
        const view::NodeViewState& node = findNodeVisual(iterator->example->nodes, iterator->id);
        if (render::isPointInsideNextInput(node, x, y)) {
            return *iterator;
        }
    }
    return std::nullopt;
}

bool Application::isExternallyReferenced(const NodeReference& node) const {
    return node.example->list.head() == node.id ||
           (m_CurrNode && m_CurrNode->example == node.example && m_CurrNode->id == node.id) ||
           (m_TempNode && m_TempNode->example == node.example && m_TempNode->id == node.id);
}

bool Application::isNodeClamped(const NodeReference& node) const {
    return (m_CurrNode && m_CurrNode->example == node.example && m_CurrNode->id == node.id) ||
           (m_TempNode && m_TempNode->example == node.example && m_TempNode->id == node.id);
}

std::optional<std::size_t> Application::deckIndexForLead(const NodeReference& node) const {
    for (std::size_t index = 0; index < m_Decks.size(); ++index) {
        const DeckState& deck = m_Decks.at(index);
        if (deck.lead.example == node.example && deck.lead.id == node.id) {
            return index;
        }
    }
    return std::nullopt;
}

std::optional<std::size_t> Application::deckIndexContaining(const NodeReference& node) const {
    if (const std::optional<std::size_t> leadIndex = deckIndexForLead(node)) {
        return leadIndex;
    }
    for (std::size_t index = 0; index < m_Decks.size(); ++index) {
        const DeckState& deck = m_Decks.at(index);
        for (const DeckMember& member : deck.members) {
            if (deck.lead.example == node.example && member.id == node.id) {
                return index;
            }
        }
    }
    return std::nullopt;
}

bool Application::isDeckMember(const NodeReference& node) const {
    const std::optional<std::size_t> deckIndex = deckIndexContaining(node);
    return deckIndex && m_Decks.at(*deckIndex).lead.id != node.id;
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

void Application::attachDraggedNodeToClamp(
    std::optional<NodeReference>& attachedNode,
    const render::Rectangle& slot) {
    if (!m_DragState) {
        throw std::logic_error("Cannot attach a node without an active drag");
    }

    view::NodeViewState& node = findNodeVisual(
        m_DragState->node.example->nodes,
        m_DragState->node.id);
    if (
        attachedNode &&
        (attachedNode->example != m_DragState->node.example ||
         attachedNode->id != m_DragState->node.id)) {
        view::NodeViewState& ejectedNode = findNodeVisual(
            attachedNode->example->nodes,
            attachedNode->id);
        const render::Rectangle ejectionPosition = randomClampEjectionPosition(slot);
        ejectedNode.x = ejectionPosition.x;
        ejectedNode.y = ejectionPosition.y;
    }

    const float snapDeltaX = slot.x - node.x;
    const float snapDeltaY = slot.y - node.y;
    node.x = slot.x;
    node.y = slot.y;
    for (const DraggedNode& downstreamNode : m_DragState->downstreamNodes) {
        view::NodeViewState& visual = findNodeVisual(
            m_DragState->node.example->nodes,
            downstreamNode.id);
        visual.x += snapDeltaX;
        visual.y += snapDeltaY;
    }
    attachedNode = m_DragState->node;
}

void Application::spawnNodeAtClamp(ClampTarget target) {
    const render::Rectangle& slot =
        target == ClampTarget::Curr ? CURR_CLAMP_SLOT : TEMP_CLAMP_SLOT;
    std::optional<NodeReference>& attachedNode =
        target == ClampTarget::Curr ? m_CurrNode : m_TempNode;
    if (attachedNode) {
        throw std::logic_error("Cannot spawn a node into an occupied clamp");
    }

    std::uniform_int_distribution<int> letterDistribution('A', 'Z');
    std::size_t paletteIndex = 0;
    if (!m_Theme->nodeCard.palette.empty()) {
        std::uniform_int_distribution<std::size_t> paletteDistribution(
            0,
            m_Theme->nodeCard.palette.size() - 1);
        paletteIndex = paletteDistribution(m_RandomEngine);
    }

    const model::NodeId id = m_CorrectExample.list.emplaceNode(
        std::string(1, static_cast<char>(letterDistribution(m_RandomEngine))));
    m_CorrectExample.nodes.push_back(
        {
            .id = id,
            .x = slot.x,
            .y = slot.y,
            .paletteIndex = paletteIndex,
        });
    const NodeReference node{.example = &m_CorrectExample, .id = id};
    m_DrawOrder.push_back(node);
    attachedNode = node;
    saveSandbox();
}

void Application::toggleDeck(const NodeReference& lead) {
    if (const std::optional<std::size_t> index = deckIndexForLead(lead)) {
        expandDeck(*index);
        saveSandbox();
        return;
    }
    if (isDeckMember(lead)) {
        return;
    }

    DeckState deck{
        .lead = lead,
    };
    for (const model::NodeId downstreamId : lead.example->list.downstreamNodeIds(lead.id)) {
        const NodeReference candidate{.example = lead.example, .id = downstreamId};
        if (isExternallyReferenced(candidate)) {
            break;
        }
        if (deckIndexForLead(candidate) || isDeckMember(candidate)) {
            return;
        }
        const view::NodeViewState& visual = findNodeVisual(lead.example->nodes, downstreamId);
        deck.members.push_back(
            {
                .id = downstreamId,
                .originalX = visual.x,
                .originalY = visual.y,
            });
    }
    if (deck.members.empty()) {
        return;
    }

    m_Decks.push_back(std::move(deck));
    applyDeckLayout(m_Decks.back());
    saveSandbox();
}

void Application::expandDeck(std::size_t index) {
    if (index >= m_Decks.size()) {
        throw std::out_of_range("Unknown deck");
    }

    const DeckState& deck = m_Decks.at(index);
    for (const DeckMember& member : deck.members) {
        view::NodeViewState& visual = findNodeVisual(deck.lead.example->nodes, member.id);
        visual.x = member.originalX;
        visual.y = member.originalY;
    }
    m_Decks.erase(m_Decks.begin() + static_cast<std::ptrdiff_t>(index));
}

void Application::expandDeckContaining(const NodeReference& node) {
    if (const std::optional<std::size_t> index = deckIndexContaining(node)) {
        expandDeck(*index);
    }
}

void Application::applyDeckLayout(const DeckState& deck) {
    const view::NodeViewState& leadVisual = findNodeVisual(deck.lead.example->nodes, deck.lead.id);
    for (std::size_t index = 0; index < deck.members.size(); ++index) {
        view::NodeViewState& visual =
            findNodeVisual(deck.lead.example->nodes, deck.members.at(index).id);
        const float offset = DECK_OFFSET_STEP * static_cast<float>(index + 1);
        visual.x = leadVisual.x + offset;
        visual.y = leadVisual.y - offset;
    }
}

void Application::updateDeckOriginalPositions(const DragState& dragState) {
    const auto initialPositionFor = [&dragState](model::NodeId id)
        -> std::optional<glm::vec2> {
        if (dragState.node.id == id) {
            return glm::vec2{dragState.initialX, dragState.initialY};
        }
        for (const DraggedNode& downstreamNode : dragState.downstreamNodes) {
            if (downstreamNode.id == id) {
                return glm::vec2{downstreamNode.initialX, downstreamNode.initialY};
            }
        }
        return std::nullopt;
    };

    for (DeckState& deck : m_Decks) {
        for (DeckMember& member : deck.members) {
            const std::optional<glm::vec2> initialPosition = initialPositionFor(member.id);
            if (!initialPosition) {
                continue;
            }
            const view::NodeViewState& visual =
                findNodeVisual(deck.lead.example->nodes, member.id);
            member.originalX += visual.x - initialPosition->x;
            member.originalY += visual.y - initialPosition->y;
        }
    }
}

render::Rectangle Application::randomClampEjectionPosition(const render::Rectangle& slot) {
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
        .x = slot.x + (directionDistribution(m_RandomEngine) ? horizontalOffset : -horizontalOffset),
        .y = slot.y + slot.height + verticalOffsetDistribution(m_RandomEngine),
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
    if (m_TempNode && m_TempNode->example == node.example && m_TempNode->id == node.id) {
        m_TempNode.reset();
    }
}

std::optional<Application::NodeReference> Application::findTopmostCard(float x, float y) const {
    for (auto iterator = m_DrawOrder.rbegin(); iterator != m_DrawOrder.rend(); ++iterator) {
        if (isDeckMember(*iterator)) {
            continue;
        }
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

void Application::loadSandbox(const std::filesystem::path& path) {
    applySandboxState(SandboxStateStore::load(path));
}

void Application::resetSandbox() {
    loadSandbox(INITIAL_SANDBOX_PATH);
    saveSandbox();
}

void Application::saveSandbox() const {
    SandboxStateStore::save(SANDBOX_SAVE_PATH, sandboxState());
}

void Application::applySandboxState(const SandboxState& state) {
    m_CorrectExample = {};
    m_DrawOrder.clear();
    m_HoveredNode.reset();
    m_SelectedNode.reset();
    m_CurrNode.reset();
    m_TempNode.reset();
    m_DragState.reset();
    m_HoveredScissorsTarget.reset();
    m_TapeDrag.reset();
    m_HoveredTapeSource.reset();
    m_HoveredTapeTarget.reset();
    m_Decks.clear();
    m_HeldTool = HeldTool::None;
    m_CurrAdvanceFeedback = AdvanceFeedback::None;
    m_TempAdvanceFeedback = AdvanceFeedback::None;
    m_CurrAdvanceFeedbackRemaining = 0.0f;
    m_TempAdvanceFeedbackRemaining = 0.0f;

    std::unordered_map<std::uint32_t, model::NodeId> nodeIds;
    nodeIds.reserve(state.nodes.size());
    for (const SandboxNodeState& nodeState : state.nodes) {
        const model::NodeId id = m_CorrectExample.list.emplaceNode(nodeState.value);
        nodeIds.emplace(nodeState.id, id);
        m_CorrectExample.nodes.push_back(
            {
                .id = id,
                .x = nodeState.x,
                .y = nodeState.y,
                .paletteIndex = nodeState.paletteIndex,
            });
        m_DrawOrder.push_back({.example = &m_CorrectExample, .id = id});
    }

    const auto resolveId = [&nodeIds](std::uint32_t persistedId) {
        const auto iterator = nodeIds.find(persistedId);
        if (iterator == nodeIds.end()) {
            throw std::logic_error("Sandbox state references a missing node");
        }
        return iterator->second;
    };
    for (const SandboxNodeState& nodeState : state.nodes) {
        if (nodeState.nextId) {
            m_CorrectExample.list.setNext(resolveId(nodeState.id), resolveId(*nodeState.nextId));
        }
    }
    if (state.headId) {
        m_CorrectExample.list.setHead(resolveId(*state.headId));
    }
    const auto makeReference = [this, &resolveId](std::optional<std::uint32_t> persistedId)
        -> std::optional<NodeReference> {
        if (!persistedId) {
            return std::nullopt;
        }
        return NodeReference{.example = &m_CorrectExample, .id = resolveId(*persistedId)};
    };
    m_CurrNode = makeReference(state.currId);
    m_TempNode = makeReference(state.tempId);
    for (const SandboxDeckState& deckState : state.decks) {
        DeckState deck{
            .lead = {
                .example = &m_CorrectExample,
                .id = resolveId(deckState.leadId),
            },
        };
        for (const SandboxDeckMemberState& memberState : deckState.members) {
            deck.members.push_back(
                {
                    .id = resolveId(memberState.id),
                    .originalX = memberState.originalX,
                    .originalY = memberState.originalY,
                });
        }
        m_Decks.push_back(std::move(deck));
        applyDeckLayout(m_Decks.back());
    }

    if (!m_CorrectExample.list.isConsistent()) {
        throw std::logic_error("Loaded sandbox state is inconsistent");
    }
}

SandboxState Application::sandboxState() const {
    SandboxState state{
        .headId = m_CorrectExample.list.head()
                      ? std::optional<std::uint32_t>{m_CorrectExample.list.head()->value}
                      : std::nullopt,
    };
    state.nodes.reserve(m_CorrectExample.nodes.size());
    for (const view::NodeViewState& visual : m_CorrectExample.nodes) {
        const model::SinglyLinkedList<std::string>::Node& node =
            m_CorrectExample.list.node(visual.id);
        state.nodes.push_back(
            {
                .id = node.id.value,
                .value = node.value,
                .nextId = node.next ? std::optional<std::uint32_t>{node.next->value} : std::nullopt,
                .x = visual.x,
                .y = visual.y,
                .paletteIndex = visual.paletteIndex,
            });
    }
    const auto persistedReferenceId = [this](const std::optional<NodeReference>& reference)
        -> std::optional<std::uint32_t> {
        if (!reference) {
            return std::nullopt;
        }
        if (reference->example != &m_CorrectExample) {
            throw std::logic_error("Cannot save a reference from an unknown sandbox");
        }
        return reference->id.value;
    };
    state.currId = persistedReferenceId(m_CurrNode);
    state.tempId = persistedReferenceId(m_TempNode);
    state.decks.reserve(m_Decks.size());
    for (const DeckState& deck : m_Decks) {
        if (deck.lead.example != &m_CorrectExample) {
            throw std::logic_error("Cannot save a deck from an unknown sandbox");
        }
        SandboxDeckState persistedDeck{
            .leadId = deck.lead.id.value,
        };
        for (const DeckMember& member : deck.members) {
            persistedDeck.members.push_back(
                {
                    .id = member.id.value,
                    .originalX = member.originalX,
                    .originalY = member.originalY,
                });
        }
        state.decks.push_back(std::move(persistedDeck));
    }
    return state;
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
