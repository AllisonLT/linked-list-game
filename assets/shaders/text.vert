#version 330 core

layout (location = 0) in vec4 a_Vertex;

out vec2 texCoord;

void main() {
    gl_Position = vec4(a_Vertex.xy, 0.0, 1.0);
    texCoord = a_Vertex.zw;
}
