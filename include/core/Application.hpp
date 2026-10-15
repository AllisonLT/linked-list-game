#pragma once

#include "core/ShaderProgram.hpp"
#include "core/TextRenderer.hpp"
#include "core/Window.hpp"
#include "render/NodeLayout.hpp"
#include <memory>
#include <string>

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
        render::NodeLayout layout;
        std::string value;
        std::string nextLabel;
    };

    void processInput();
    void update(float deltaTime);
    void render();
    void createRectangleRenderer();
    void drawNodeCard(const NodeVisual& node) const;
    void drawNodeText(const NodeVisual& node) const;
    void drawDirectionalPointer(float sourceX, float sourceY, float destinationX, float destinationY) const;
    void drawNullEndpoint(const NodeVisual& node) const;
    void drawCircle(float centerX, float centerY, float radius, const Color& color) const;
    void drawRectangle(const render::Rectangle& rectangle, const Color& color) const;

    std::unique_ptr<Window> m_Window;
    std::unique_ptr<ShaderProgram> m_ColorShader;
    std::unique_ptr<TextRenderer> m_RobotoMonoValueTextRenderer;
    std::unique_ptr<TextRenderer> m_RobotoMonoPointerTextRenderer;
    std::unique_ptr<TextRenderer> m_RobotoRegularTextRenderer;
    NodeVisual m_SourceNode;
    NodeVisual m_DestinationNode;
    unsigned int m_RectangleVertexArray = 0;
    unsigned int m_RectangleVertexBuffer = 0;
};

} // namespace game::core
