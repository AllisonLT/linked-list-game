#version 330 core

out vec4 fragmentColor;

uniform vec4 u_Color;

void main() {
    fragmentColor = u_Color;
}
