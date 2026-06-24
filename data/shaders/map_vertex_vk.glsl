#version 450

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aColor;
layout (location = 3) in float aAnimFrames;
layout (location = 4) in float aAnimSpeed;

layout (binding = 0) uniform UniformBufferObject {
    float uTime;
    mat4 uProjection;
} ubo;

layout (location = 0) out vec2 vTexCoord;
layout (location = 1) out vec4 vColor;

void main() {
    vColor = aColor;
    vec2 uv = aTexCoord;
    float frame = (aAnimFrames > 1.0) ? floor(mod(ubo.uTime * (aAnimSpeed > 0.0 ? aAnimSpeed : 2.0), aAnimFrames)) : 0.0;
    uv.x += (aAnimFrames > 1.0) ? frame * (1.0 / aAnimFrames) : 0.0;
    vTexCoord = uv;
    gl_Position = ubo.uProjection * vec4(aPos, 0.0, 1.0);
}
