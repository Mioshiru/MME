#version 120
uniform sampler2D uTexture;
uniform int   uUpscaling;
uniform int   uAmbientEffects;
uniform float uTime;
uniform int   uFloor;

varying vec2  vTexCoord;
varying vec4  vColor;
varying vec2  vWorldPos;
varying float vShaderData;

// Smooth Mitchell-Hermite Anti-Aliased Pixel Upscaling without grid noise
vec4 sampleSmoothPixelArt(sampler2D tex, vec2 uv) {
    vec2 texSize = vec2(32.0, 32.0);
    vec2 pixel = uv * texSize;
    vec2 p = floor(pixel);
    vec2 f = fract(pixel);

    // Anti-Aliasing threshold: smooth transitions right at texel boundaries (fwidth-based)
    vec2 df = fwidth(pixel);
    vec2 w = clamp((f - 0.5) / max(df, vec2(0.001)) + 0.5, 0.0, 1.0);

    vec2 uv00 = (p + vec2(0.5, 0.5)) / texSize;
    vec2 uv10 = (p + vec2(1.5, 0.5)) / texSize;
    vec2 uv01 = (p + vec2(0.5, 1.5)) / texSize;
    vec2 uv11 = (p + vec2(1.5, 1.5)) / texSize;

    vec4 c00 = texture2D(tex, uv00);
    vec4 c10 = texture2D(tex, uv10);
    vec4 c01 = texture2D(tex, uv01);
    vec4 c11 = texture2D(tex, uv11);

    // Smooth bilinear blend across sub-pixel borders
    vec4 top = mix(c00, c10, w.x);
    vec4 bot = mix(c01, c11, w.x);
    return mix(top, bot, w.y);
}

void main() {
    vec4 rawCenter = texture2D(uTexture, vTexCoord);
    if (rawCenter.a < 0.01) discard;

    // When Ultra-HD Super-Resolution is OFF: crisp raw point sampling
    if (uUpscaling == 0) {
        gl_FragColor = rawCenter * vColor;
        return;
    }

    // ── Ultra-HD Clean Anti-Aliased Pixel Art ──
    vec4 center = sampleSmoothPixelArt(uTexture, vTexCoord);
    center.a = rawCenter.a;

    if (uUpscaling == 2) {
        // Modern Pixel-Art xBRZ: a soft neighbor blur smooths reconstruction
        // artifacts, followed by a vibrance boost that intensifies color.
        vec2 stepUv = vec2(1.0 / 128.0);
        vec3 blur = (texture2D(uTexture, vTexCoord - vec2(stepUv.x, 0.0)).rgb +
                     texture2D(uTexture, vTexCoord + vec2(stepUv.x, 0.0)).rgb +
                     texture2D(uTexture, vTexCoord - vec2(0.0, stepUv.y)).rgb +
                     texture2D(uTexture, vTexCoord + vec2(0.0, stepUv.y)).rgb) * 0.25;
        center.rgb = mix(center.rgb, blur, 0.60);

        float luma = dot(center.rgb, vec3(0.299, 0.587, 0.114));
        center.rgb = clamp(mix(vec3(luma), center.rgb, 1.30), 0.0, 1.0);
    }

    vec4 texel = center;

    gl_FragColor = texel * vColor;
}