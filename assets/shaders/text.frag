#version 330 core

in vec2 texCoord;
out vec4 fragmentColor;

uniform sampler2D u_Text;
uniform vec4 u_Color;

void main() {
    fragmentColor = vec4(u_Color.rgb, u_Color.a * texture(u_Text, texCoord).r);
}
