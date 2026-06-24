#version 120
uniform sampler2D uTexture;
uniform float uTime;
uniform vec2 uResolution;
varying vec2 vTexCoord;

vec2 curve(vec2 uv) {
    uv = (uv - 0.5) * 2.0;
    uv *= 1.1;
    uv.x *= 1.0 + pow((abs(uv.y) / 5.0), 2.0);
    uv.y *= 1.0 + pow((abs(uv.x) / 4.0), 2.0);
    uv = (uv / 2.0) + 0.5;
    uv = uv * 0.92 + 0.05;
    return uv;
}

void main() {
    vec2 uv = curve(vTexCoord);
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    
    float x = sin(0.3 * uTime + uv.y * 21.0) * sin(0.7 * uTime + uv.y * 29.0) * 0.0012;
    vec3 col;
    col.r = texture2D(uTexture, vec2(uv.x + x + 0.001, uv.y)).x + 0.05;
    col.g = texture2D(uTexture, vec2(uv.x + x, uv.y)).y + 0.05;
    col.b = texture2D(uTexture, vec2(uv.x + x - 0.001, uv.y)).z + 0.05;

    col -= sin(uv.y * uResolution.y * 1.8) * 0.08;
    float vig = 16.0 * uv.x * uv.y * (1.0 - uv.x) * (1.0 - uv.y);
    gl_FragColor = vec4(col * vec3(pow(vig, 0.25)), 1.0);
}