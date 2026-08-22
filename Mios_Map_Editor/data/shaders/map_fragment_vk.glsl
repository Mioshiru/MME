#version 450

layout (binding = 1) uniform sampler2D uTexture;

layout (location = 0) in vec2 vTexCoord;
layout (location = 1) in vec4 vColor;

layout (location = 0) out vec4 outColor;

void main() {
    outColor = texture(uTexture, vTexCoord) * vColor;
}
