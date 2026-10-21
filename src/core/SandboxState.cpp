#include "core/SandboxState.hpp"

#include <nlohmann/json.hpp>

#include <cmath>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_set>

namespace game::core {
namespace {

using json = nlohmann::json;

std::optional<std::uint32_t> readOptionalId(const json& value, const char* description) {
    if (value.is_null()) {
        return std::nullopt;
    }
    if (!value.is_number_unsigned()) {
        throw std::runtime_error(std::string(description) + " must be an unsigned integer or null");
    }
    const std::uint32_t id = value.get<std::uint32_t>();
    if (id == 0) {
        throw std::runtime_error(std::string(description) + " must not be zero");
    }
    return id;
}

float readFiniteFloat(const json& value, const char* description) {
    if (!value.is_number()) {
        throw std::runtime_error(std::string(description) + " must be a number");
    }
    const float result = value.get<float>();
    if (!std::isfinite(result)) {
        throw std::runtime_error(std::string(description) + " must be finite");
    }
    return result;
}

void validateReferences(const SandboxState& state) {
    std::unordered_set<std::uint32_t> nodeIds;
    for (const SandboxNodeState& node : state.nodes) {
        if (node.id == 0 || !nodeIds.insert(node.id).second) {
            throw std::runtime_error("Sandbox nodes must have unique, non-zero IDs");
        }
    }

    const auto validate = [&nodeIds](std::optional<std::uint32_t> id, const char* description) {
        if (id && !nodeIds.contains(*id)) {
            throw std::runtime_error(std::string(description) + " references a missing node");
        }
    };
    validate(state.headId, "Sandbox head");
    validate(state.currId, "Sandbox curr");
    validate(state.tempId, "Sandbox temp");
    for (const SandboxNodeState& node : state.nodes) {
        validate(node.nextId, "Sandbox next");
    }
}

} // namespace

SandboxState SandboxStateStore::load(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Failed to open sandbox state: " + path.string());
    }

    try {
        const json document = json::parse(file);
        const json& nodes = document.at("nodes");
        if (!nodes.is_array()) {
            throw std::runtime_error("Sandbox nodes must be an array");
        }

        SandboxState state{
            .headId = readOptionalId(document.at("head"), "Sandbox head"),
            .currId = readOptionalId(document.at("clamps").at("curr"), "Sandbox curr"),
            .tempId = readOptionalId(document.at("clamps").at("temp"), "Sandbox temp"),
        };
        state.nodes.reserve(nodes.size());
        for (std::size_t index = 0; index < nodes.size(); ++index) {
            const json& node = nodes.at(index);
            if (!node.at("id").is_number_unsigned() || !node.at("paletteIndex").is_number_unsigned()) {
                throw std::runtime_error("Sandbox node IDs and palette indices must be unsigned integers");
            }
            if (!node.at("value").is_string()) {
                throw std::runtime_error("Sandbox node values must be strings");
            }
            SandboxNodeState parsed{
                .id = node.at("id").get<std::uint32_t>(),
                .value = node.at("value").get<std::string>(),
                .nextId = readOptionalId(node.at("next"), "Sandbox next"),
                .x = readFiniteFloat(node.at("x"), "Sandbox node x"),
                .y = readFiniteFloat(node.at("y"), "Sandbox node y"),
                .paletteIndex = node.at("paletteIndex").get<std::size_t>(),
            };
            state.nodes.push_back(std::move(parsed));
        }
        validateReferences(state);
        return state;
    } catch (const json::exception& error) {
        throw std::runtime_error("Failed to parse sandbox state " + path.string() + ": " + error.what());
    }
}

void SandboxStateStore::save(const std::filesystem::path& path, const SandboxState& state) {
    validateReferences(state);
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) {
        throw std::runtime_error("Failed to create sandbox save directory: " + error.message());
    }

    json document{
        {"head", state.headId ? json(*state.headId) : json(nullptr)},
        {
            "clamps",
            {
                {"curr", state.currId ? json(*state.currId) : json(nullptr)},
                {"temp", state.tempId ? json(*state.tempId) : json(nullptr)},
            },
        },
        {"nodes", json::array()},
    };
    for (const SandboxNodeState& node : state.nodes) {
        document["nodes"].push_back(
            {
                {"id", node.id},
                {"value", node.value},
                {"next", node.nextId ? json(*node.nextId) : json(nullptr)},
                {"x", node.x},
                {"y", node.y},
                {"paletteIndex", node.paletteIndex},
            });
    }

    std::ofstream file(path, std::ios::trunc);
    if (!file) {
        throw std::runtime_error("Failed to open sandbox save file: " + path.string());
    }
    file << document.dump(2) << '\n';
    if (!file) {
        throw std::runtime_error("Failed to write sandbox state: " + path.string());
    }
}

} // namespace game::core
