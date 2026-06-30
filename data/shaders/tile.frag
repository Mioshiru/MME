#version 330 core

in vec3 vTexCoord; // received from vertex shader (u, v, sliceIndex)

uniform sampler2DArray uAtlasArray; // texture array holding all atlas sheets
uniform float uAlpha;               // alpha overlay value (e.g. 1.0 active floor, 0.4 faded floors)

out vec4 FragColor;

void main()
{
    // Sample texture array using 3D texture coordinate
    vec4 texel = texture(uAtlasArray, vTexCoord);
    
    // Discard transparent background pixels
    if (texel.a < 0.01) 
    {
        discard;
    }
    
    FragColor = vec4(texel.rgb, texel.a * uAlpha);
}
