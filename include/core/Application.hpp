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

    struct ListExample {
        model::SinglyLinkedList<std::string> list;
        std::vector<view::NodeViewState> nodes;
        std::string description;
        float descriptionBaselineY = 0.0f;
    };

    struct NodeReference {
        ListExample* example;
        model::NodeId id;
    };

    struct DragState {
        NodeReference node;
        float offsetX;
        float offsetY;
    };

    void processInput();
    void update(float deltaTime);
    void render();
    void updateInteraction();
    void updateHoveredNode();
    void createRectangleRenderer();
    void drawNodeCard(const ListExample& example, const view::NodeViewState& node) const;
    void drawNodeText(const ListExample& example, const view::NodeViewState& node) const;
    void drawExampleConnections(const ListExample& example) const;
    void drawListPointers(const ListExample& example) const;
    void drawHeadIndicator(const ListExample& example) const;
    void drawNullEndpoints(const ListExample& example) const;
    void drawDirectionalPointer(float sourceX, float sourceY, float destinationX, float destinationY) const;
    void drawNullEndpoint(const view::NodeViewState& node) const;
    void drawCircle(float centerX, float centerY, float radius, const Color& color) const;
    void drawReferenceDockOutline(
        const render::Circle& dock,
        float outlineWidth,
        const Color& color) const;
    void drawOutputSocketOutline(
        const render::Rectangle& card,
        const render::Circle& socket,
        float outlineWidth,
        const Color& color) const;
    void drawRectangle(const render::Rectangle& rectangle, const Color& color) const;
    void bringToFront(const NodeReference& node);
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
    ListExample m_IncorrectExample;
    std::vector<NodeReference> m_DrawOrder;
    std::optional<NodeReference> m_HoveredNode;
    std::optional<NodeReference> m_SelectedNode;
    std::optional<DragState> m_DragState;
    unsigned int m_RectangleVertexArray = 0;
    unsigned int m_RectangleVertexBuffer = 0;
};

} // namespace game::core
