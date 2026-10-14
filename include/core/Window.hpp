#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>

namespace game::core {

struct ViewportRect {
    int x = 0;
    int y = 0;
    int width = 1280;
    int height = 720;
};

class Window {
public:
    static constexpr float TARGET_WIDTH = 1920.0f;
    static constexpr float TARGET_HEIGHT = 1080.0f;
    static constexpr float TARGET_ASPECT = TARGET_WIDTH / TARGET_HEIGHT;

    Window(int width, int height, const std::string& title);
    ~Window();

    bool shouldClose() const;
    void setTitle(const std::string& title) const;
    void swapBuffers();
    void pollEvents();

    void beginRender();
    void endRender();

    GLFWwindow* getNativeWindow() const { return m_Window; }
    const ViewportRect& getViewport() const { return m_Viewport; }

private:
    void updateViewport(int windowWidth, int windowHeight);
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);

    GLFWwindow* m_Window = nullptr;
    ViewportRect m_Viewport;
};

} // namespace game::core
