#include "core/Window.hpp"
#include <iostream>
#include <stdexcept>

namespace game::core {

Window::Window(int width, int height, const std::string& title) {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_Window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_Window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(m_Window);
    glfwSetWindowUserPointer(m_Window, this);
    glfwSetFramebufferSizeCallback(m_Window, framebufferSizeCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
        glfwTerminate();
        throw std::runtime_error("Failed to initialize GLAD");
    }

    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(m_Window, &framebufferWidth, &framebufferHeight);
    updateViewport(framebufferWidth, framebufferHeight);
}

Window::~Window() {
    if (m_Window) {
        glfwDestroyWindow(m_Window);
    }
    glfwTerminate();
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(m_Window);
}

void Window::setTitle(const std::string& title) const {
    glfwSetWindowTitle(m_Window, title.c_str());
}

void Window::swapBuffers() {
    glfwSwapBuffers(m_Window);
}

void Window::pollEvents() {
    glfwPollEvents();
}

void Window::beginRender() {
    // Clear whole physical window to dark black (letterbox bars)
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(m_Window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);
    glClearColor(0.06f, 0.08f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Scissor test to only clear the 16:9 canvas region
    glViewport(m_Viewport.x, m_Viewport.y, m_Viewport.width, m_Viewport.height);
    glEnable(GL_SCISSOR_TEST);
    glScissor(m_Viewport.x, m_Viewport.y, m_Viewport.width, m_Viewport.height);
    glClearColor(0.10f, 0.13f, 0.19f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
}

void Window::endRender() {
    swapBuffers();
}

void Window::updateViewport(int windowWidth, int windowHeight) {
    float windowAspect = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);

    if (windowAspect >= TARGET_ASPECT) {
        m_Viewport.height = windowHeight;
        m_Viewport.width = static_cast<int>(windowHeight * TARGET_ASPECT);
        m_Viewport.x = (windowWidth - m_Viewport.width) / 2;
        m_Viewport.y = 0;
    } else {
        m_Viewport.width = windowWidth;
        m_Viewport.height = static_cast<int>(windowWidth / TARGET_ASPECT);
        m_Viewport.x = 0;
        m_Viewport.y = (windowHeight - m_Viewport.height) / 2;
    }
}

void Window::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (self) {
        self->updateViewport(width, height);
    }
}

} // namespace game::core
