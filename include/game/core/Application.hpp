#pragma once

#include "game/core/Window.hpp"
#include <memory>

namespace game::core {

class Application {
public:
    Application();
    ~Application() = default;

    int run();

private:
    void processInput();
    void update(float deltaTime);
    void render();

    std::unique_ptr<Window> m_Window;
};

} // namespace game::core
