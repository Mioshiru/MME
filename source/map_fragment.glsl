#version 130
uniform sampler2D uTexture;

in vec2 vTexCoord;
in vec4 vColor;

void main() {
    vec4 texel = texture(uTexture, vTexCoord);
    if (texel.a < 0.01) discard;
    gl_FragColor = texel * vColor;
}