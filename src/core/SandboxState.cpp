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

    std::unordered_set<std::uint32_t> deckNodeIds;
    for (const SandboxDeckState& deck : state.decks) {
        if (deck.leadId == 0 || !nodeIds.contains(deck.leadId) || deck.members.empty()) {
            throw std::runtime_error("Sandbox decks must have a valid lead and at least one member");
        }
        if (!deckNodeIds.insert(deck.leadId).second) {
            throw std::runtime_error("Sandbox deck nodes cannot belong to multiple decks");
        }
        for (const SandboxDeckMemberState& member : deck.members) {
            if (
                member.id == 0 ||
                member.id == deck.leadId ||
                !nodeIds.contains(member.id) ||
                !std::isfinite(member.originalX) ||
                !std::isfinite(member.originalY) ||
                !deckNodeIds.insert(member.id).second) {
                throw std::runtime_error("Sandbox deck members must be unique valid nodes");
            }
        }
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
        const json decks = document.value("decks", json::array());
        if (!decks.is_array()) {
            throw std::runtime_error("Sandbox decks must be an array");
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
        state.decks.reserve(decks.size());
        for (const json& deck : decks) {
            if (!deck.at("lead").is_number_unsigned() || !deck.at("members").is_array()) {
                throw std::runtime_error("Sandbox decks must contain an unsigned lead and members array");
            }
            SandboxDeckState parsed{
                .leadId = deck.at("lead").get<std::uint32_t>(),
            };
            for (const json& member : deck.at("members")) {
                if (!member.at("id").is_number_unsigned()) {
                    throw std::runtime_error("Sandbox deck member IDs must be unsigned integers");
                }
                parsed.members.push_back(
                    {
                        .id = member.at("id").get<std::uint32_t>(),
                        .originalX = readFiniteFloat(member.at("originalX"), "Sandbox deck member x"),
                        .originalY = readFiniteFloat(member.at("originalY"), "Sandbox deck member y"),
                    });
            }
            state.decks.push_back(std::move(parsed));
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
        {"decks", json::array()},
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
    for (const SandboxDeckState& deck : state.decks) {
        json serializedDeck{
            {"lead", deck.leadId},
            {"members", json::array()},
        };
        for (const SandboxDeckMemberState& member : deck.members) {
            serializedDeck["members"].push_back(
                {
                    {"id", member.id},
                    {"originalX", member.originalX},
                    {"originalY", member.originalY},
                });
        }
        document["decks"].push_back(std::move(serializedDeck));
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
