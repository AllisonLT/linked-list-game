#pragma once

#include "core/ShaderProgram.hpp"
#include "core/Window.hpp"
#include "render/NodeLayout.hpp"
#include <memory>

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

    void processInput();
    void update(float deltaTime);
    void render();
    void createRectangleRenderer();
    void drawNodeCard() const;
    void drawCircle(float centerX, float centerY, float radius, const Color& color) const;
    void drawRectangle(const render::Rectangle& rectangle, const Color& color) const;

    std::unique_ptr<Window> m_Window;
    std::unique_ptr<ShaderProgram> m_ColorShader;
    unsigned int m_RectangleVertexArray = 0;
    unsigned int m_RectangleVertexBuffer = 0;
};

} // namespace game::core
