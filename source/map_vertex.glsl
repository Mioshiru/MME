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

    // Atmospheric Vertex Displacements: Foliage & Tree Wind Sway (Oberflaeche: Floor <= 7)
    if (uAmbientEffects == 1 && aShaderData == 4.0 && uFloor <= 7) {
        vec2 objAnchor = floor(aPos / 64.0) * 64.0;
        float wind = sin(uTime * 1.8 + objAnchor.x * 0.025 + objAnchor.y * 0.02) * 1.8;
        pos.x += wind;
    }

    vTexCoord   = aTexCoord;
    gl_Position = gl_ModelViewProjectionMatrix * vec4(pos, 0.0, 1.0);
}