#pragma once

#include <algorithm>

namespace game::render {

struct Rectangle {
    float x;
    float y;
    float width;
    float height;
};

struct Circle {
    float centerX;
    float centerY;
    float radius;
};

struct NodeLayout {
    Rectangle card;
    Rectangle value;
    Rectangle nextLabel;
    Circle referenceDock;
    Circle outputSocket;
};

inline constexpr float NODE_CARD_HEIGHT = 200.0f;
inline constexpr float NODE_CARD_WIDTH = NODE_CARD_HEIGHT;

inline NodeLayout makeNodeLayout(float x, float y) {
    constexpr float padding = 24.0f;
    constexpr float valueY = 48.0f;
    constexpr float valueHeight = 80.0f;
    constexpr float labelY = 140.0f;
    constexpr float labelWidth = 116.0f;
    constexpr float referenceDockRadius = 32.0f;
    constexpr float socketOffsetX = 10.0f;
    constexpr float socketY = NODE_CARD_HEIGHT / 2.0f;
    constexpr float socketRadius = 40.0f;
    constexpr float referenceDockOffsetY =
        socketOffsetX * referenceDockRadius / socketRadius;

    return {
        .card = {x, y, NODE_CARD_WIDTH, NODE_CARD_HEIGHT},
        .value = {x + padding, y + valueY, NODE_CARD_WIDTH - (2.0f * padding), valueHeight},
        .nextLabel = {x + padding, y + labelY, labelWidth, 32.0f},
        .referenceDock = {
            x + NODE_CARD_WIDTH / 2.0f,
            y + referenceDockOffsetY,
            referenceDockRadius,
        },
        .outputSocket = {x + NODE_CARD_WIDTH + socketOffsetX, y + socketY, socketRadius},
    };
}

inline Rectangle clampCardToCanvas(
    float x,
    float y,
    float canvasWidth,
    float canvasHeight) {
    return {
        .x = std::clamp(
            x,
            -NODE_CARD_WIDTH / 2.0f,
            canvasWidth - (NODE_CARD_WIDTH / 2.0f)),
        .y = std::clamp(
            y,
            -NODE_CARD_HEIGHT / 2.0f,
            canvasHeight - (NODE_CARD_HEIGHT / 2.0f)),
        .width = NODE_CARD_WIDTH,
        .height = NODE_CARD_HEIGHT,
    };
}

} // namespace game::render
