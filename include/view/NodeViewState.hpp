#pragma once

#include "model/NodeId.hpp"

namespace game::view {

struct NodeViewState {
    model::NodeId id;
    float x;
    float y;
};

} // namespace game::view
