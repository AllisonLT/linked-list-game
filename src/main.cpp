#include "game/core/Application.hpp"
#include <exception>
#include <iostream>

int main() {
    try {
        game::core::Application app;
        return app.run();
    } catch (const std::exception& error) {
        std::cerr << "Application startup failed: " << error.what() << '\n';
        return 1;
    }
}
