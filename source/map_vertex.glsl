#version 130
in vec2  aPos;
in vec2  aTexCoord;
in vec4  aColor;
in float aShaderData;

uniform float uTime;

out vec2 vTexCoord;
out vec4 vColor;

void main() {
    vColor = aColor;
    vec2 pos = aPos;

    // GPU-side water wave animation (flag 1.0)
    if (aShaderData == 1.0) {
        pos.x += sin(uTime * 2.5 + aPos.y * 0.05) * 3.0;
        pos.y += cos(uTime * 1.8 + aPos.x * 0.05) * 1.5;
        vColor.a *= 0.85;
    }

    // Dynamic objects (flag 2.0) - no vertical displacement
    if (aShaderData == 2.0) {
        // Reserved for sprite animation flags without vertex displacement
    }

    vTexCoord   = aTexCoord;
    gl_Position = gl_ModelViewProjectionMatrix * vec4(pos, 0.0, 1.0);
}