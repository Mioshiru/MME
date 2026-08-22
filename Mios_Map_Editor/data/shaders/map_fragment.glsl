#version 330 compatibility
uniform sampler2D uTexture;
in vec2 vTexCoord;
in vec4 vColor;

void main() {
    gl_FragColor = texture(uTexture, vTexCoord) * vColor;
}