#version 330 compatibility
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aColor;
layout (location = 3) in float aAnimFrames;
layout (location = 4) in float aAnimSpeed;

uniform float uTime;
uniform mat4 uProjection;
out vec2 vTexCoord;
out vec4 vColor;

void main() {
    vColor = aColor;
    vec2 uv = aTexCoord;
    float frame = (aAnimFrames > 1.0) ? floor(mod(uTime * (aAnimSpeed > 0.0 ? aAnimSpeed : 2.0), aAnimFrames)) : 0.0;
    uv.x += (aAnimFrames > 1.0) ? frame * (1.0 / aAnimFrames) : 0.0;
    vTexCoord = uv;
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
}