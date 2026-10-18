#include "core/Theme.hpp"

#include <nlohmann/json.hpp>

#include <charconv>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace game::core {
namespace {

using json = nlohmann::json;

render::Color parseHexColor(std::string_view value, std::string_view description) {
    if (value.size() != 7 || value.front() != '#') {
        throw std::runtime_error(
            "Theme color " + std::string(description) + " must use #RRGGBB format");
    }

    const auto parseComponent = [&value, description](std::size_t offset) {
        unsigned int component = 0;
        const char* begin = value.data() + offset;
        const char* end = begin + 2;
        const auto [pointer, error] = std::from_chars(begin, end, component, 16);
        if (error != std::errc{} || pointer != end) {
            throw std::runtime_error("Theme color " + std::string(description) + " is not valid hexadecimal");
        }
        return static_cast<float>(component) / 255.0f;
    };

    return {
        .red = parseComponent(1),
        .green = parseComponent(3),
        .blue = parseComponent(5),
    };
}

render::Color parseColor(const json& value, std::string_view description) {
    if (value.is_string()) {
        return parseHexColor(value.get<std::string>(), description);
    }

    if (!value.is_array() || (value.size() != 3 && value.size() != 4)) {
        throw std::runtime_error(
            "Theme color " + std::string(description) +
            " must be #RRGGBB or an RGB/RGBA array");
    }

    render::Color color{};
    const auto readComponent = [&value, description](std::size_t index) {
        if (!value.at(index).is_number()) {
            throw std::runtime_error("Theme color " + std::string(description) + " must contain numbers");
        }
        const float component = value.at(index).get<float>();
        if (component < 0.0f || component > 1.0f) {
            throw std::runtime_error(
                "Theme color " + std::string(description) + " components must be between 0 and 1");
        }
        return component;
    };

    color.red = readComponent(0);
    color.green = readComponent(1);
    color.blue = readComponent(2);
    color.alpha = value.size() == 4 ? readComponent(3) : 1.0f;
    return color;
}

render::Color colorAt(const json& object, std::string_view key, std::string_view description) {
    return parseColor(object.at(std::string(key)), description);
}

} // namespace

Theme Theme::loadFromFile(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Failed to open theme file: " + path.string());
    }

    try {
        const json document = json::parse(file);
        const json& canvas = document.at("canvas");
        const json& nodeCard = document.at("nodeCard");
        const json& pointer = document.at("pointer");
        const json& workbench = document.at("workbench");
        const json& toolbox = workbench.at("toolbox");
        const json& clamp = workbench.at("clamp");
        const json& templateScroll = workbench.at("templateScroll");
        const json& rubbishBin = workbench.at("rubbishBin");
        const json& drawers = workbench.at("drawers");

        Theme theme{
            .canvas = {
                .letterbox = colorAt(canvas, "letterbox", "canvas.letterbox"),
                .background = colorAt(canvas, "background", "canvas.background"),
            },
            .nodeCard = {
                .defaultFill = colorAt(nodeCard, "defaultFill", "nodeCard.defaultFill"),
                .text = colorAt(nodeCard, "text", "nodeCard.text"),
                .hoverGlow = colorAt(nodeCard, "hoverGlow", "nodeCard.hoverGlow"),
                .selectedGlow = colorAt(nodeCard, "selectedGlow", "nodeCard.selectedGlow"),
                .selectedFill = colorAt(nodeCard, "selectedFill", "nodeCard.selectedFill"),
                .tapeSourceGlow = colorAt(nodeCard, "tapeSourceGlow", "nodeCard.tapeSourceGlow"),
                .tapeTargetGlow = colorAt(nodeCard, "tapeTargetGlow", "nodeCard.tapeTargetGlow"),
            },
            .pointer = {
                .normal = colorAt(pointer, "normal", "pointer.normal"),
                .label = colorAt(pointer, "label", "pointer.label"),
                .scissorsGlow = colorAt(pointer, "scissorsGlow", "pointer.scissorsGlow"),
                .scissorsHighlight = colorAt(
                    pointer,
                    "scissorsHighlight",
                    "pointer.scissorsHighlight"),
            },
            .workbench = {
                .border = colorAt(workbench, "border", "workbench.border"),
                .surface = colorAt(workbench, "surface", "workbench.surface"),
                .title = colorAt(workbench, "title", "workbench.title"),
                .toolbox = {
                    .border = colorAt(toolbox, "border", "workbench.toolbox.border"),
                    .surface = colorAt(toolbox, "surface", "workbench.toolbox.surface"),
                    .button = colorAt(toolbox, "button", "workbench.toolbox.button"),
                    .label = colorAt(toolbox, "label", "workbench.toolbox.label"),
                    .inactiveLabel = colorAt(
                        toolbox,
                        "inactiveLabel",
                        "workbench.toolbox.inactiveLabel"),
                    .selected = colorAt(toolbox, "selected", "workbench.toolbox.selected"),
                    .error = colorAt(toolbox, "error", "workbench.toolbox.error"),
                    .errorGlow = colorAt(toolbox, "errorGlow", "workbench.toolbox.errorGlow"),
                    .heldTool = colorAt(toolbox, "heldTool", "workbench.toolbox.heldTool"),
                },
                .clamp = {
                    .body = colorAt(clamp, "body", "workbench.clamp.body"),
                    .inset = colorAt(clamp, "inset", "workbench.clamp.inset"),
                    .label = colorAt(clamp, "label", "workbench.clamp.label"),
                    .slotOutline = colorAt(clamp, "slotOutline", "workbench.clamp.slotOutline"),
                },
                .templateScroll = {
                    .shadow = colorAt(templateScroll, "shadow", "workbench.templateScroll.shadow"),
                    .paper = colorAt(templateScroll, "paper", "workbench.templateScroll.paper"),
                    .roll = colorAt(templateScroll, "roll", "workbench.templateScroll.roll"),
                },
                .rubbishBin = {
                    .lid = colorAt(rubbishBin, "lid", "workbench.rubbishBin.lid"),
                    .body = colorAt(rubbishBin, "body", "workbench.rubbishBin.body"),
                    .detail = colorAt(rubbishBin, "detail", "workbench.rubbishBin.detail"),
                    .label = colorAt(rubbishBin, "label", "workbench.rubbishBin.label"),
                },
                .drawers = {
                    .frame = colorAt(drawers, "frame", "workbench.drawers.frame"),
                    .face = colorAt(drawers, "face", "workbench.drawers.face"),
                    .handle = colorAt(drawers, "handle", "workbench.drawers.handle"),
                    .label = colorAt(drawers, "label", "workbench.drawers.label"),
                },
            },
        };

        const json& palette = nodeCard.at("palette");
        if (!palette.is_array()) {
            throw std::runtime_error("Theme nodeCard.palette must be an array");
        }
        theme.nodeCard.palette.reserve(palette.size());
        for (std::size_t index = 0; index < palette.size(); ++index) {
            theme.nodeCard.palette.push_back(
                parseColor(palette.at(index), "nodeCard.palette[" + std::to_string(index) + "]"));
        }
        return theme;
    } catch (const json::exception& error) {
        throw std::runtime_error("Failed to parse theme file " + path.string() + ": " + error.what());
    }
}

} // namespace game::core
