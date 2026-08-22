#version 330 core
uniform sampler2D uTexture;
uniform vec2 uResolution;
out vec4 FragColor;
in vec2 vTexCoord;

void main() {
    vec2 texelSize = 1.0 / uResolution;
    vec3 result = vec3(0.0);
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            result += texture(uTexture, vTexCoord + vec2(x, y) * texelSize).rgb;
        }
    }
    FragColor = vec4(result / 25.0, 1.0);
}