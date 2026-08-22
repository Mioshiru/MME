#version 430 core
layout(local_size_x = 16, local_size_y = 16) in;
layout(rgba8, binding = 0) uniform image2D imgOutput;

uniform sampler2D uMapTex;   // Die Map-Daten (Blocking/Eigenschaften)
uniform sampler2D uLightTex; // Die direkten Lichtquellen
uniform sampler2D uEmissiveLUT; // Lookup Table für leuchtende Materialien
uniform float uBounceStrength = 0.45;

void main() {
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    vec2 uv = vec2(pixelCoords) / vec2(imageSize(imgOutput));
    
    vec3 directLight = texture(uLightTex, uv).rgb;
    vec3 indirectLight = vec3(0.0);
    
    // Simple Ray-Marching für Bounce-Light
    int rays = 8;
    for(int i = 0; i < rays; i++) {
        float angle = (float(i) / float(rays)) * 6.28318;
        vec2 dir = vec2(cos(angle), sin(angle)) * 0.02; // Kurze Distanz für Bounces
        
        vec3 sampleLight = texture(uLightTex, uv + dir).rgb;
        vec4 mapData = texture(uMapTex, uv + dir);

        // Suche Eigenleuchten in der LUT basierend auf dem Material-Index (mapData.r)
        vec3 emissiveGlow = texture(uEmissiveLUT, vec2(mapData.r, 0.5)).rgb;
        
        // Kombiniere reflektiertes Licht mit Eigenleuchten des Materials
        indirectLight += (sampleLight + emissiveGlow) * mapData.rgb * uBounceStrength;
    }
    
    imageStore(imgOutput, pixelCoords, vec4(directLight + (indirectLight / rays), 1.0));
}