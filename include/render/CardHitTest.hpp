#pragma once

#include "render/NodeLayout.hpp"
#include "view/NodeViewState.hpp"

namespace game::render {

inline bool isPointInsideCard(const view::NodeViewState& node, float x, float y) {
    const NodeLayout layout = makeNodeLayout(node.x, node.y);
    const bool insideCard =
        x >= layout.card.x && x <= layout.card.x + layout.card.width &&
        y >= layout.card.y && y <= layout.card.y + layout.card.height;
    const float dockX = x - layout.referenceDock.centerX;
    const float dockY = y - layout.referenceDock.centerY;
    const bool insideReferenceDock =
        dockX * dockX + dockY * dockY < layout.referenceDock.radius * layout.referenceDock.radius;
    return insideCard && !insideReferenceDock;
}

inline bool isPointInsideNextOutput(const view::NodeViewState& node, float x, float y) {
    const NodeLayout layout = makeNodeLayout(node.x, node.y);
    const float offsetX = x - layout.outputSocket.centerX;
    const float offsetY = y - layout.outputSocket.centerY;
    return offsetX * offsetX + offsetY * offsetY <=
           layout.outputSocket.radius * layout.outputSocket.radius;
}

inline bool isPointInsideNextInput(const view::NodeViewState& node, float x, float y) {
    return isPointInsideCard(node, x, y);
}

} // namespace game::render
