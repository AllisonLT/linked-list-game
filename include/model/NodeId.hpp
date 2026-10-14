#pragma once

#include <cstdint>
#include <functional>

namespace game::model {

struct NodeId {
    std::uint32_t value = 0;

    constexpr bool operator==(const NodeId&) const = default;
};

struct NodeIdHash {
    std::size_t operator()(NodeId id) const noexcept {
        return std::hash<std::uint32_t>{}(id.value);
    }
};

} // namespace game::model
