#pragma once

#include "core/ShaderProgram.hpp"
#include "core/TextRenderer.hpp"
#include "core/Theme.hpp"
#include "core/TextureRenderer.hpp"
#include "core/Window.hpp"
#include "model/SinglyLinkedList.hpp"
#include "render/CardHitTest.hpp"
#include "render/NodeLayout.hpp"
#include "view/NodeViewState.hpp"
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace game::core {

class Application {
public:
    Application();
    ~Application();

    int run();

private:
    using Color = render::Color;

    enum class HeldTool {
        None,
        Tape,
        Scissors,
    };

    enum class AdvanceFeedback {
        None,
        Activated,
        Error,
    };

    struct ListExample {
        model::SinglyLinkedList<std::string> list;
        std::vector<view::NodeViewState> nodes;
    };

    struct NodeReference {
        ListExample* example;
        model::NodeId id;
    };

    struct DraggedNode {
        model::NodeId id;
        float initialX;
        float initialY;
    };

    struct DragState {
        NodeReference node;
        float offsetX;
        float offsetY;
        float initialX;
        float initialY;
        std::vector<DraggedNode> downstreamNodes;
    };

    struct TapeDragState {
        NodeReference source;
        float pressX;
        float pressY;
        bool isDragging = false;
    };

    void processInput();
    void update(float deltaTime);
    void render();
    void updateInteraction();
    void updateHoveredNode();
    void createRectangleRenderer();
    void drawWorkbench() const;
    void drawToolbox() const;
    void drawClampSlot() const;
    void drawDashedNodeSilhouette(
        const render::NodeLayout& layout,
        const Color& color) const;
    void drawTemplateScroll() const;
    void drawRubbishBin() const;
    void drawDrawers() const;
    void drawCurrClamp() const;
    void drawHeldTool() const;
    void drawTapePreview() const;
    void triggerAdvanceFeedback();
    void drawNodeCard(const ListExample& example, const view::NodeViewState& node) const;
    void drawNodeText(const ListExample& example, const view::NodeViewState& node) const;
    void drawExampleConnections(const ListExample& example) const;
    void drawListPointers(const ListExample& example) const;
    void drawHeadIndicator(const ListExample& example) const;
    void drawNullEndpoints(const ListExample& example) const;
    void drawDirectionalPointer(
        float sourceX,
        float sourceY,
        float destinationX,
        float destinationY,
        const Color& color,
        float sizeScale) const;
    void drawNullEndpoint(const view::NodeViewState& node) const;
    void drawCircle(float centerX, float centerY, float radius, const Color& color) const;
    void drawNodeSilhouette(
        const render::NodeLayout& layout,
        float glowExpansion,
        const Color& fillColor) const;
    void drawRectangleWithCircularCutout(
        const render::Rectangle& rectangle,
        const render::Circle& cutout,
        const Color& color) const;
    void drawRectangle(const render::Rectangle& rectangle, const Color& color) const;
    void bringToFront(const NodeReference& node);
    [[nodiscard]] render::Rectangle randomClampEjectionPosition();
    [[nodiscard]] std::optional<HeldTool> toolAt(float x, float y) const;
    [[nodiscard]] bool isAdvanceToolAt(float x, float y) const;
    [[nodiscard]] std::optional<NodeReference> findScissorsTarget(float x, float y) const;
    [[nodiscard]] std::optional<NodeReference> findTapeOutput(float x, float y) const;
    [[nodiscard]] bool isScissorsTarget(const ListExample& example, model::NodeId id) const;
    [[nodiscard]] bool isTapeSource(const ListExample& example, model::NodeId id) const;
    [[nodiscard]] bool isTapeTarget(const ListExample& example, model::NodeId id) const;
    void updateHoveredScissorsTarget();
    void updateHoveredTapeSource();
    void updateHoveredTapeTarget();
    [[nodiscard]] std::optional<NodeReference> findTopmostCard(float x, float y) const;
    [[nodiscard]] bool isNodeHovered(const ListExample& example, model::NodeId id) const;
    [[nodiscard]] bool isNodeSelected(const ListExample& example, model::NodeId id) const;
    [[nodiscard]] static render::NodeLayout layoutFor(const view::NodeViewState& node);
    [[nodiscard]] view::NodeViewState& findNodeVisual(
        std::vector<view::NodeViewState>& nodes,
        model::NodeId id);
    [[nodiscard]] const view::NodeViewState& findNodeVisual(
        const std::vector<view::NodeViewState>& nodes,
        model::NodeId id) const;

    std::unique_ptr<Theme> m_Theme;
    std::unique_ptr<Window> m_Window;
    std::unique_ptr<ShaderProgram> m_ColorShader;
    std::unique_ptr<TextRenderer> m_RobotoMonoValueTextRenderer;
    std::unique_ptr<TextRenderer> m_RobotoMonoPointerTextRenderer;
    std::unique_ptr<TextRenderer> m_RobotoRegularTextRenderer;
    std::unique_ptr<TextureRenderer> m_TapeInactiveTexture;
    std::unique_ptr<TextureRenderer> m_TapeActiveTexture;
    std::unique_ptr<TextureRenderer> m_ScissorsInactiveTexture;
    std::unique_ptr<TextureRenderer> m_ScissorsActiveTexture;
    ListExample m_CorrectExample;
    std::vector<NodeReference> m_DrawOrder;
    std::optional<NodeReference> m_HoveredNode;
    std::optional<NodeReference> m_SelectedNode;
    std::optional<NodeReference> m_CurrNode;
    std::optional<DragState> m_DragState;
    std::optional<NodeReference> m_HoveredScissorsTarget;
    std::optional<TapeDragState> m_TapeDrag;
    std::optional<NodeReference> m_HoveredTapeSource;
    std::optional<NodeReference> m_HoveredTapeTarget;
    HeldTool m_HeldTool = HeldTool::None;
    AdvanceFeedback m_AdvanceFeedback = AdvanceFeedback::None;
    float m_AdvanceFeedbackRemaining = 0.0f;
    std::mt19937 m_RandomEngine{std::random_device{}()};
    unsigned int m_RectangleVertexArray = 0;
    unsigned int m_RectangleVertexBuffer = 0;
};

} // namespace game::core
