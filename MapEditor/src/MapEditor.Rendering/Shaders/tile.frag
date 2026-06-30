#version 330 core

in vec2 vTexCoord;

uniform sampler2D uAtlas;
uniform float uAlpha;  // floor fade (1.0 for active floor, 0.4 for inactive)

out vec4 FragColor;

void main()
{
    vec4 texel = texture(uAtlas, vTexCoord);
    if (texel.a < 0.01) discard;          // transparent sprite pixel
    FragColor = vec4(texel.rgb, texel.a * uAlpha);
}
