#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace game::core {

struct SandboxNodeState {
    std::uint32_t id = 0;
    std::string value;
    std::optional<std::uint32_t> nextId;
    float x = 0.0f;
    float y = 0.0f;
    std::size_t paletteIndex = 0;
};

struct SandboxDeckMemberState {
    std::uint32_t id = 0;
    float originalX = 0.0f;
    float originalY = 0.0f;
};

struct SandboxDeckState {
    std::uint32_t leadId = 0;
    std::vector<SandboxDeckMemberState> members;
};

struct SandboxState {
    std::vector<SandboxNodeState> nodes;
    std::vector<SandboxDeckState> decks;
    std::optional<std::uint32_t> headId;
    std::optional<std::uint32_t> currId;
    std::optional<std::uint32_t> tempId;
};

class SandboxStateStore {
public:
    [[nodiscard]] static SandboxState load(const std::filesystem::path& path);
    static void save(const std::filesystem::path& path, const SandboxState& state);
};

} // namespace game::core
