#pragma once

#include "model/NodeId.hpp"

#include <cstddef>

namespace game::view {

struct NodeViewState {
    model::NodeId id;
    float x;
    float y;
    std::size_t paletteIndex = 0;
};

} // namespace game::view
