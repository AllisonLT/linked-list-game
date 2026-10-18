#version 330 core

in vec2 texCoord;
out vec4 fragmentColor;

uniform sampler2D u_Texture;

void main() {
    fragmentColor = texture(u_Texture, texCoord);
}
