#pragma once

#include "render/Color.hpp"

#include <filesystem>
#include <vector>

namespace game::core {

struct Theme {
    struct Canvas {
        render::Color letterbox;
        render::Color background;
    };

    struct NodeCard {
        render::Color defaultFill;
        render::Color text;
        render::Color hoverGlow;
        render::Color selectedGlow;
        render::Color tapeSourceGlow;
        render::Color tapeTargetGlow;
        std::vector<render::Color> palette;
    };

    struct Pointer {
        render::Color normal;
        render::Color label;
        render::Color scissorsGlow;
        render::Color scissorsHighlight;
    };

    struct Workbench {
        struct Toolbox {
            render::Color border;
            render::Color surface;
            render::Color button;
            render::Color label;
            render::Color inactiveLabel;
            render::Color selected;
            render::Color error;
            render::Color errorGlow;
            render::Color heldTool;
        };

        struct Clamp {
            render::Color body;
            render::Color inset;
            render::Color label;
            render::Color slotOutline;
        };

        struct TemplateScroll {
            render::Color shadow;
            render::Color paper;
            render::Color roll;
            render::Color text;
        };

        struct RubbishBin {
            render::Color lid;
            render::Color body;
            render::Color detail;
            render::Color label;
        };

        struct Drawers {
            render::Color frame;
            render::Color face;
            render::Color handle;
            render::Color label;
        };

        render::Color border;
        render::Color surface;
        render::Color title;
        Toolbox toolbox;
        Clamp clamp;
        TemplateScroll templateScroll;
        RubbishBin rubbishBin;
        Drawers drawers;
    };

    Canvas canvas;
    NodeCard nodeCard;
    Pointer pointer;
    Workbench workbench;

    [[nodiscard]] static Theme loadFromFile(const std::filesystem::path& path);
};

} // namespace game::core
