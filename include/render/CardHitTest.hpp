#pragma once

#include "render/NodeLayout.hpp"
#include "view/NodeViewState.hpp"

namespace game::render {

inline bool isPointInsideCard(const view::NodeViewState& node, float x, float y) {
    const Rectangle& card = makeNodeLayout(node.x, node.y).card;
    return x >= card.x && x <= card.x + card.width && y >= card.y && y <= card.y + card.height;
}

} // namespace game::render
