#version 120
attribute vec2  aPos;
attribute vec2  aTexCoord;
attribute vec4  aColor;
attribute float aShaderData;

uniform float uTime;
uniform int   uAmbientEffects;
uniform int   uFloor;

varying vec2  vTexCoord;
varying vec4  vColor;
varying vec2  vWorldPos;
varying float vShaderData;

void main() {
    vColor      = aColor;
    vShaderData = aShaderData;
    vWorldPos   = aPos;
    vec2 pos    = aPos;

    // 4.0: Foliage Wind Sway - nur Oberflaeche (floor 0-7), kein Untergrund
    if (uAmbientEffects == 1 && aShaderData == 4.0 && uFloor <= 7) {
        // Multi-tile synchronization: quantize coordinates to grid cells
        // so all tiles of a 2x1, 1x2, 2x2, 3x3 object share the exact same sway without tearing!
        vec2 objAnchor = floor(aPos / 64.0) * 64.0;
        float wind = sin(uTime * 1.6 + objAnchor.x * 0.02 + objAnchor.y * 0.015) * 2.0;
        pos.x += wind;
    }

    vTexCoord   = aTexCoord;
    gl_Position = gl_ModelViewProjectionMatrix * vec4(pos, 0.0, 1.0);
}