#pragma once

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
    Circle outputSocket;
};

inline constexpr float NODE_CARD_WIDTH = 360.0f;
inline constexpr float NODE_CARD_HEIGHT = 200.0f;

inline NodeLayout makeNodeLayout(float x, float y) {
    constexpr float padding = 24.0f;
    constexpr float valueHeight = 88.0f;
    constexpr float labelY = 140.0f;
    constexpr float labelWidth = 116.0f;
    constexpr float socketY = 164.0f;
    constexpr float socketRadius = 14.0f;

    return {
        .card = {x, y, NODE_CARD_WIDTH, NODE_CARD_HEIGHT},
        .value = {x + padding, y + padding, NODE_CARD_WIDTH - (2.0f * padding), valueHeight},
        .nextLabel = {x + padding, y + labelY, labelWidth, 32.0f},
        .outputSocket = {x + NODE_CARD_WIDTH, y + socketY, socketRadius},
    };
}

} // namespace game::render
