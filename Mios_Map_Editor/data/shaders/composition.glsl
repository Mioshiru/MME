#version 120
uniform sampler2D uMapTex;
uniform sampler2D uLightTex;
varying vec2 vTexCoord;

void main() {
    vec4 mapColor = texture2D(uMapTex, vTexCoord);
    vec4 lightColor = texture2D(uLightTex, vTexCoord);
    // Multiplikatives Blending: Dunkelt die Map basierend auf der Lightmap ab
    gl_FragColor = vec4(mapColor.rgb * lightColor.rgb, mapColor.a);
}