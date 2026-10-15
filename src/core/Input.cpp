#include "core/Input.hpp"
#include "core/Window.hpp"
#include <algorithm>

namespace game::core {

void Input::init(Window* window) {
    s_Window = window;
    GLFWwindow* nativeWin = window->getNativeWindow();

    glfwSetCursorPosCallback(nativeWin, cursorPosCallback);
    glfwSetMouseButtonCallback(nativeWin, mouseButtonCallback);
    glfwSetKeyCallback(nativeWin, keyCallback);
}

void Input::update() {
    s_MouseButtonsPrevious = s_MouseButtonsCurrent;
    s_KeysPrevious = s_KeysCurrent;
}

glm::vec2 Input::getMousePosition() {
    return s_VirtualMousePos;
}

bool Input::isMouseButtonDown(int button) {
    if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) return false;
    return s_MouseButtonsCurrent[button];
}

bool Input::isMouseButtonJustPressed(int button) {
    if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) return false;
    return s_MouseButtonsCurrent[button] && !s_MouseButtonsPrevious[button];
}

bool Input::isMouseButtonJustReleased(int button) {
    if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) return false;
    return !s_MouseButtonsCurrent[button] && s_MouseButtonsPrevious[button];
}

bool Input::isKeyDown(int key) {
    if (key < 0 || key > GLFW_KEY_LAST) return false;
    return s_KeysCurrent[key];
}

bool Input::isKeyJustPressed(int key) {
    if (key < 0 || key > GLFW_KEY_LAST) return false;
    return s_KeysCurrent[key] && !s_KeysPrevious[key];
}

bool Input::isMouseInsideCanvas() {
    return s_MouseInCanvas;
}

void Input::cursorPosCallback(GLFWwindow* /*window*/, double xpos, double ypos) {
    s_RawMousePos = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));

    if (!s_Window) return;

    const auto& vp = s_Window->getViewport();
    int windowWidth = 0;
    int windowHeight = 0;
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetWindowSize(s_Window->getNativeWindow(), &windowWidth, &windowHeight);
    glfwGetFramebufferSize(s_Window->getNativeWindow(), &framebufferWidth, &framebufferHeight);

    if (windowWidth == 0 || windowHeight == 0 || framebufferWidth == 0 || framebufferHeight == 0 ||
        vp.width == 0 || vp.height == 0) {
        return;
    }

    const float framebufferX = s_RawMousePos.x * static_cast<float>(framebufferWidth) /
                               static_cast<float>(windowWidth);
    const float framebufferY = s_RawMousePos.y * static_cast<float>(framebufferHeight) /
                               static_cast<float>(windowHeight);

    const float canvasX = framebufferX - static_cast<float>(vp.x);
    const float canvasY = framebufferY - static_cast<float>(vp.y);

    s_MouseInCanvas = (canvasX >= 0.0f && canvasX <= static_cast<float>(vp.width) &&
                       canvasY >= 0.0f && canvasY <= static_cast<float>(vp.height));

    s_VirtualMousePos.x = (canvasX / static_cast<float>(vp.width)) * Window::TARGET_WIDTH;
    s_VirtualMousePos.y = (canvasY / static_cast<float>(vp.height)) * Window::TARGET_HEIGHT;
}

void Input::mouseButtonCallback(GLFWwindow* /*window*/, int button, int action, int /*mods*/) {
    if (button >= 0 && button <= GLFW_MOUSE_BUTTON_LAST) {
        s_MouseButtonsCurrent[button] = (action != GLFW_RELEASE);
    }
}

void Input::keyCallback(GLFWwindow* /*window*/, int key, int /*scancode*/, int action, int /*mods*/) {
    if (key >= 0 && key <= GLFW_KEY_LAST) {
        s_KeysCurrent[key] = (action != GLFW_RELEASE);
    }
}

} // namespace game::core
