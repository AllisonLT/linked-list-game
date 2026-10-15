#pragma once

#include "core/ShaderProgram.hpp"
#include "core/TextRenderer.hpp"
#include "core/Window.hpp"
#include "model/SinglyLinkedList.hpp"
#include "render/NodeLayout.hpp"
#include <memory>
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

    struct NodeVisual {
        model::NodeId id;
        render::NodeLayout layout;
    };

    void processInput();
    void update(float deltaTime);
    void render();
    void createRectangleRenderer();
    void drawNodeCard(const NodeVisual& node) const;
    void drawNodeText(const NodeVisual& node) const;
    void drawListPointers() const;
    void drawHeadIndicator() const;
    void drawNullEndpoints() const;
    void drawDirectionalPointer(float sourceX, float sourceY, float destinationX, float destinationY) const;
    void drawNullEndpoint(const NodeVisual& node) const;
    void drawCircle(float centerX, float centerY, float radius, const Color& color) const;
    void drawRectangle(const render::Rectangle& rectangle, const Color& color) const;
    [[nodiscard]] const NodeVisual& findNodeVisual(model::NodeId id) const;

    std::unique_ptr<Window> m_Window;
    std::unique_ptr<ShaderProgram> m_ColorShader;
    std::unique_ptr<TextRenderer> m_RobotoMonoValueTextRenderer;
    std::unique_ptr<TextRenderer> m_RobotoMonoPointerTextRenderer;
    std::unique_ptr<TextRenderer> m_RobotoRegularTextRenderer;
    model::SinglyLinkedList<std::string> m_List;
    std::vector<NodeVisual> m_NodeVisuals;
    unsigned int m_RectangleVertexArray = 0;
    unsigned int m_RectangleVertexBuffer = 0;
};

} // namespace game::core
