#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <array>

namespace game::core {

class Window;

class Input {
public:
    static void init(Window* window);
    static void update();

    // Virtual canvas coordinates (0..1920, 0..1080)
    static glm::vec2 getMousePosition();
    static bool isMouseButtonDown(int button);
    static bool isMouseButtonJustPressed(int button);
    static bool isMouseButtonJustReleased(int button);
    static bool isKeyDown(int key);
    static bool isKeyJustPressed(int key);
    static bool isMouseInsideCanvas();

private:
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    static inline Window* s_Window = nullptr;
    static inline glm::vec2 s_RawMousePos{0.0f};
    static inline glm::vec2 s_VirtualMousePos{0.0f};
    static inline bool s_MouseInCanvas = true;

    static inline std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> s_MouseButtonsCurrent{};
    static inline std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> s_MouseButtonsPrevious{};

    static inline std::array<bool, GLFW_KEY_LAST + 1> s_KeysCurrent{};
    static inline std::array<bool, GLFW_KEY_LAST + 1> s_KeysPrevious{};
};

} // namespace game::core
