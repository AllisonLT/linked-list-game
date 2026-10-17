#pragma once

#include <algorithm>

namespace game::render {

inline bool isPointInsidePointerHitArea(
    float sourceX,
    float sourceY,
    float destinationX,
    float destinationY,
    float x,
    float y,
    float radius) {
    const float directionX = destinationX - sourceX;
    const float directionY = destinationY - sourceY;
    const float lengthSquared = directionX * directionX + directionY * directionY;
    if (lengthSquared == 0.0f) {
        const float offsetX = x - sourceX;
        const float offsetY = y - sourceY;
        return offsetX * offsetX + offsetY * offsetY <= radius * radius;
    }

    const float projection = std::clamp(
        ((x - sourceX) * directionX + (y - sourceY) * directionY) / lengthSquared,
        0.0f,
        1.0f);
    const float closestX = sourceX + projection * directionX;
    const float closestY = sourceY + projection * directionY;
    const float offsetX = x - closestX;
    const float offsetY = y - closestY;
    return offsetX * offsetX + offsetY * offsetY <= radius * radius;
}

} // namespace game::render
