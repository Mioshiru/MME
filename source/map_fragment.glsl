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

// Perceptual color distance metric (YUV luminance-weighted for accurate sprite edge detection)
float colorDist(vec4 c1, vec4 c2) {
    if (c1.a < 0.05 && c2.a < 0.05) return 0.0;
    if (c1.a < 0.05 || c2.a < 0.05) return 2.0;
    vec3 yuv1 = vec3(dot(c1.rgb, vec3(0.299, 0.587, 0.114)), c1.r - c1.b, c1.g - c1.b);
    vec3 yuv2 = vec3(dot(c2.rgb, vec3(0.299, 0.587, 0.114)), c2.r - c2.b, c2.g - c2.b);
    vec3 diff = abs(yuv1 - yuv2);
    return diff.x * 0.75 + (diff.y + diff.z) * 0.25 + abs(c1.a - c2.a) * 0.5;
}

// Alpha-safe color blending (prevents dark halos around transparent sprite boundaries)
vec4 blendEdge(vec4 c1, vec4 c2) {
    if (c1.a < 0.05) return c2;
    if (c2.a < 0.05) return c1;
    return mix(c1, c2, 0.5);
}

// Advanced 4x/5x Multi-Angle xBRZ & Super-xBR Filter (OTCv8 & Medivia HD Compatible)
vec4 sampleXBRZ(sampler2D tex, vec2 uv) {
    vec2 texSize = vec2(32.0, 32.0);
    vec2 ps = vec2(1.0 / 32.0, 1.0 / 32.0);
    vec2 pos = uv * texSize;
    vec2 f = fract(pos);
    vec2 centerUV = (floor(pos) + 0.5) * ps;

    // 13-Tap Extended Kernel Matrix
    //        [B1]
    //    [A] [B]  [C]
    //[D1][D] [E]  [F] [F1]
    //    [G] [H]  [I]
    //        [H1]
    vec4 E  = texture2D(tex, centerUV);
    vec4 B  = texture2D(tex, centerUV + vec2( 0.0,   -ps.y));
    vec4 D  = texture2D(tex, centerUV + vec2(-ps.x,   0.0));
    vec4 F  = texture2D(tex, centerUV + vec2( ps.x,   0.0));
    vec4 H  = texture2D(tex, centerUV + vec2( 0.0,    ps.y));

    // Fast-path early exit: flat areas (grass/water/dirt) skip all 13-tap calculations
    if (B == D && D == F && F == H && H == E) {
        return E;
    }

    vec4 A  = texture2D(tex, centerUV + vec2(-ps.x,  -ps.y));
    vec4 C  = texture2D(tex, centerUV + vec2( ps.x,  -ps.y));
    vec4 G  = texture2D(tex, centerUV + vec2(-ps.x,   ps.y));
    vec4 I  = texture2D(tex, centerUV + vec2( ps.x,   ps.y));
    vec4 B1 = texture2D(tex, centerUV + vec2( 0.0,   -2.0 * ps.y));
    vec4 D1 = texture2D(tex, centerUV + vec2(-2.0 * ps.x, 0.0));
    vec4 F1 = texture2D(tex, centerUV + vec2( 2.0 * ps.x, 0.0));
    vec4 H1 = texture2D(tex, centerUV + vec2( 0.0,    2.0 * ps.y));

    vec4 color = E;

    // --- Top-Left Quadrant (Multi-Slope Reconstruction) ---
    if (f.x < 0.5 && f.y < 0.5) {
        float d_edge = colorDist(D, B);
        float d_diag = colorDist(A, E);
        if (d_edge < d_diag && (colorDist(E, D) < 0.12 || colorDist(E, B) < 0.12 || d_edge < 0.35)) {
            vec4 edgeCol = blendEdge(D, B);
            float dist45 = (0.5 - f.x) + (0.5 - f.y);
            float distShallow = (0.5 - f.x) * 0.5 + (0.5 - f.y);
            float distSteep   = (0.5 - f.x) + (0.5 - f.y) * 0.5;

            // Shallow slope (2:1 angle) check
            if (colorDist(D1, B) < colorDist(D1, E) && distShallow > 0.30) {
                float blend = smoothstep(0.28, 0.48, distShallow);
                color = mix(color, edgeCol, blend);
            }
            // Steep slope (1:2 angle) check
            else if (colorDist(D, B1) < colorDist(E, B1) && distSteep > 0.30) {
                float blend = smoothstep(0.28, 0.48, distSteep);
                color = mix(color, edgeCol, blend);
            }
            // Standard 45° diagonal
            else if (dist45 > 0.38) {
                float blend = smoothstep(0.35, 0.55, dist45);
                color = mix(color, edgeCol, blend);
            }
        }
    }
    // --- Top-Right Quadrant (Multi-Slope Reconstruction) ---
    else if (f.x >= 0.5 && f.y < 0.5) {
        float d_edge = colorDist(B, F);
        float d_diag = colorDist(C, E);
        if (d_edge < d_diag && (colorDist(E, B) < 0.12 || colorDist(E, F) < 0.12 || d_edge < 0.35)) {
            vec4 edgeCol = blendEdge(B, F);
            float dist45 = (f.x - 0.5) + (0.5 - f.y);
            float distShallow = (f.x - 0.5) * 0.5 + (0.5 - f.y);
            float distSteep   = (f.x - 0.5) + (0.5 - f.y) * 0.5;

            if (colorDist(F1, B) < colorDist(F1, E) && distShallow > 0.30) {
                float blend = smoothstep(0.28, 0.48, distShallow);
                color = mix(color, edgeCol, blend);
            } else if (colorDist(F, B1) < colorDist(E, B1) && distSteep > 0.30) {
                float blend = smoothstep(0.28, 0.48, distSteep);
                color = mix(color, edgeCol, blend);
            } else if (dist45 > 0.38) {
                float blend = smoothstep(0.35, 0.55, dist45);
                color = mix(color, edgeCol, blend);
            }
        }
    }
    // --- Bottom-Left Quadrant (Multi-Slope Reconstruction) ---
    else if (f.x < 0.5 && f.y >= 0.5) {
        float d_edge = colorDist(D, H);
        float d_diag = colorDist(G, E);
        if (d_edge < d_diag && (colorDist(E, D) < 0.12 || colorDist(E, H) < 0.12 || d_edge < 0.35)) {
            vec4 edgeCol = blendEdge(D, H);
            float dist45 = (0.5 - f.x) + (f.y - 0.5);
            float distShallow = (0.5 - f.x) * 0.5 + (f.y - 0.5);
            float distSteep   = (0.5 - f.x) + (f.y - 0.5) * 0.5;

            if (colorDist(D1, H) < colorDist(D1, E) && distShallow > 0.30) {
                float blend = smoothstep(0.28, 0.48, distShallow);
                color = mix(color, edgeCol, blend);
            } else if (colorDist(D, H1) < colorDist(E, H1) && distSteep > 0.30) {
                float blend = smoothstep(0.28, 0.48, distSteep);
                color = mix(color, edgeCol, blend);
            } else if (dist45 > 0.38) {
                float blend = smoothstep(0.35, 0.55, dist45);
                color = mix(color, edgeCol, blend);
            }
        }
    }
    // --- Bottom-Right Quadrant (Multi-Slope Reconstruction) ---
    else if (f.x >= 0.5 && f.y >= 0.5) {
        float d_edge = colorDist(F, H);
        float d_diag = colorDist(I, E);
        if (d_edge < d_diag && (colorDist(E, F) < 0.12 || colorDist(E, H) < 0.12 || d_edge < 0.35)) {
            vec4 edgeCol = blendEdge(F, H);
            float dist45 = (f.x - 0.5) + (f.y - 0.5);
            float distShallow = (f.x - 0.5) * 0.5 + (f.y - 0.5);
            float distSteep   = (f.x - 0.5) + (f.y - 0.5) * 0.5;

            if (colorDist(F1, H) < colorDist(F1, E) && distShallow > 0.30) {
                float blend = smoothstep(0.28, 0.48, distShallow);
                color = mix(color, edgeCol, blend);
            } else if (colorDist(F, H1) < colorDist(E, H1) && distSteep > 0.30) {
                float blend = smoothstep(0.28, 0.48, distSteep);
                color = mix(color, edgeCol, blend);
            } else if (dist45 > 0.38) {
                float blend = smoothstep(0.35, 0.55, dist45);
                color = mix(color, edgeCol, blend);
            }
        }
    }

    // Smooth HD Reconstruction without noisy grain / krissel
    vec4 crossAvg = (B + D + F + H) * 0.25;
    vec4 sharp = color + (color - crossAvg) * 0.05;
    color = mix(color, sharp, 0.50);

    // Subtle HD Color Vibrance Enhancement
    float lum = dot(color.rgb, vec3(0.299, 0.587, 0.114));
    color.rgb = mix(vec3(lum), color.rgb, 1.03);

    return color;
}

void main() {
    vec4 texel;
    if (uUpscaling == 1) {
        texel = sampleXBRZ(uTexture, vTexCoord);
    } else {
        texel = texture2D(uTexture, vTexCoord);
    }

    // Lava / Magma subtle warm pulse (vShaderData == 3.0)
    if (uAmbientEffects == 1 && vShaderData == 3.0) {
        float pulse = 0.93 + 0.15 * sin(uTime * 2.4 + vWorldPos.x * 0.05 + vWorldPos.y * 0.05);
        if (texel.r > 0.35) {
            texel.rgb *= pulse;
            texel.r = min(1.0, texel.r * 1.10);
        }
    }

    if (texel.a < 0.01) discard;
    gl_FragColor = texel * vColor;
}