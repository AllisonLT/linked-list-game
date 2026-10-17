#pragma once

#include "core/ShaderProgram.hpp"
#include "core/TextRenderer.hpp"
#include "core/Window.hpp"
#include "model/SinglyLinkedList.hpp"
#include "render/CardHitTest.hpp"
#include "render/NodeLayout.hpp"
#include "view/NodeViewState.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace game::core {

class Application {
public:
    Application();
    ~Application();

    int run();

private:
    struct Color {
        float red;
        float green;
        float blue;
        float alpha = 1.0f;
    };

    enum class HeldTool {
        None,
        Tape,
        Scissors,
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

    void processInput();
    void update(float deltaTime);
    void render();
    void updateInteraction();
    void updateHoveredNode();
    void createRectangleRenderer();
    void drawWorkbench() const;
    void drawToolbox() const;
    void drawCurrClamp() const;
    void drawHeldTool() const;
    void drawNodeCard(const ListExample& example, const view::NodeViewState& node) const;
    void drawNodeText(const ListExample& example, const view::NodeViewState& node) const;
    void drawExampleConnections(const ListExample& example) const;
    void drawListPointers(const ListExample& example) const;
    void drawHeadIndicator(const ListExample& example) const;
    void drawNullEndpoints(const ListExample& example) const;
    void drawDirectionalPointer(float sourceX, float sourceY, float destinationX, float destinationY) const;
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
    [[nodiscard]] std::optional<HeldTool> toolAt(float x, float y) const;
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

    std::unique_ptr<Window> m_Window;
    std::unique_ptr<ShaderProgram> m_ColorShader;
    std::unique_ptr<TextRenderer> m_RobotoMonoValueTextRenderer;
    std::unique_ptr<TextRenderer> m_RobotoMonoPointerTextRenderer;
    std::unique_ptr<TextRenderer> m_RobotoRegularTextRenderer;
    ListExample m_CorrectExample;
    std::vector<NodeReference> m_DrawOrder;
    std::optional<NodeReference> m_HoveredNode;
    std::optional<NodeReference> m_SelectedNode;
    std::optional<NodeReference> m_CurrNode;
    std::optional<DragState> m_DragState;
    HeldTool m_HeldTool = HeldTool::None;
    unsigned int m_RectangleVertexArray = 0;
    unsigned int m_RectangleVertexBuffer = 0;
};

} // namespace game::core
