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

// RGB to HSL
vec3 rgb2hsl(vec3 c) {
    float maxC = max(c.r, max(c.g, c.b));
    float minC = min(c.r, min(c.g, c.b));
    float delta = maxC - minC;
    float l = (maxC + minC) * 0.5;
    float s = 0.0;
    float h = 0.0;
    if (delta > 0.00001) {
        s = l > 0.5 ? delta / (2.0 - maxC - minC) : delta / (maxC + minC);
        if (maxC == c.r) {
            h = (c.g - c.b) / delta + (c.g < c.b ? 6.0 : 0.0);
        } else if (maxC == c.g) {
            h = (c.b - c.r) / delta + 2.0;
        } else {
            h = (c.r - c.g) / delta + 4.0;
        }
        h /= 6.0;
    }
    return vec3(h, s, l);
}

float hue2rgb_f(float p, float q, float t) {
    if (t < 0.0) t += 1.0;
    if (t > 1.0) t -= 1.0;
    if (t < 1.0/6.0) return p + (q - p) * 6.0 * t;
    if (t < 1.0/2.0) return q;
    if (t < 2.0/3.0) return p + (q - p) * (2.0/3.0 - t) * 6.0;
    return p;
}

vec3 hsl2rgb(vec3 hsl) {
    if (hsl.y <= 0.00001) return vec3(hsl.z);
    float q = hsl.z < 0.5 ? hsl.z * (1.0 + hsl.y) : hsl.z + hsl.y - hsl.z * hsl.y;
    float p = 2.0 * hsl.z - q;
    return vec3(
        hue2rgb_f(p, q, hsl.x + 1.0/3.0),
        hue2rgb_f(p, q, hsl.x),
        hue2rgb_f(p, q, hsl.x - 1.0/3.0)
    );
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

    vec3 hslCheck = rgb2hsl(center.rgb);
    bool isWater = (vShaderData == 1.0) || (hslCheck.x >= 0.50 && hslCheck.x <= 0.72);
    bool isGrass = (hslCheck.x >= 0.20 && hslCheck.x <= 0.44 && hslCheck.y > 0.15);

    vec4 texel = center;

    // ── 1. Optimiertes Wasser: Lebendige, weiche Wasserströmung & Tiefenblau ──
    if (isWater) {
        vec2 waveDisp = vec2(
            sin(uTime * 1.6 + vWorldPos.x * 0.15 + vWorldPos.y * 0.10) * 0.003,
            cos(uTime * 1.3 - vWorldPos.x * 0.10 + vWorldPos.y * 0.12) * 0.003
        );
        vec4 refracTex = texture2D(uTexture, vTexCoord + waveDisp);
        texel.rgb = mix(texel.rgb, refracTex.rgb, 0.45);

        // Satte, kristallklare Azur-Tönung
        vec3 deepAqua = vec3(0.04, 0.28, 0.60);
        texel.rgb = mix(deepAqua, texel.rgb, 0.88);
    }

    // ── 2. Optimiertes Gras & Flora: Saftiges, natürliches Smaragdgrün ──
    if (isGrass) {
        vec3 hslGras = rgb2hsl(texel.rgb);
        // Frisches, sattes RPG-Grün ohne Gelbstich oder Übersättigung
        hslGras.x = mix(hslGras.x, 0.30, 0.22);
        hslGras.y = clamp(hslGras.y * 1.15, 0.0, 0.95);
        hslGras.z = clamp(hslGras.z * 1.03, 0.0, 1.0);
        texel.rgb = hsl2rgb(hslGras);
    }

    gl_FragColor = texel * vColor;
}