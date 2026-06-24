#version 330 compatibility
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aColor;
layout (location = 3) in float aShaderData; // 0: static, 1: water, 2: animation

uniform float uTime;
uniform mat4 uProjection;
out vec2 vTexCoord;
out vec4 vColor;

void main() {
    vColor = aColor;
    vec2 pos = aPos;
    
    // Wasser-Wellen Logik (Flag 1.0)
    if (aShaderData == 1.0) {
        pos.x += sin(uTime * 2.5 + aPos.y * 0.05) * 3.0;
        pos.y += cos(uTime * 1.8 + aPos.x * 0.05) * 1.5;
        vColor.a *= 0.85; // Wasser ist leicht transparent
    }
    
    // Pulsierende Animation für dynamische Objekte (Flag 2.0)
    if (aShaderData == 2.0) {
        float pulse = sin(uTime * 4.0) * 0.5 + 0.5;
        pos.y -= pulse * 2.0;
    }

    vTexCoord = aTexCoord;
    gl_Position = uProjection * vec4(pos, 0.0, 1.0);
}