#version 330 core

// Per-vertex attributes (static tile quad geometry)
layout(location = 0) in vec2 aPos;       // local quad position [0,1]
layout(location = 1) in vec2 aTexCoord;  // local UV coords [0,1]

// Per-instance attributes (one per tile drawn in this batch)
layout(location = 2) in vec2  iGridPos;    // tile position in grid coordinates (x, y)
layout(location = 3) in vec4  iUV;         // atlas UV rect bounds (u0, v0, u1, v1)
layout(location = 4) in float iSliceIndex; // texture array sheet slice index
layout(location = 5) in float iDepth;      // z-order depth (for proper layering/culling)

uniform mat4 uProjection;   // orthographic camera view projection matrix
uniform vec2 uTileSize;     // width and height of a single tile in pixels (normally 32x32)

out vec3 vTexCoord; // passed to fragment shader (u, v, sliceIndex)

void main()
{
    // Scale local quad position by TileSize and offset by grid location
    vec2 worldPos = iGridPos * uTileSize + aPos * uTileSize;

    // Output clip space position
    gl_Position = uProjection * vec4(worldPos, iDepth, 1.0);

    // Map local quad [0,1] UV to the mapped sprite coordinates inside the atlas
    float u = mix(iUV.x, iUV.z, aTexCoord.x);
    float v = mix(iUV.y, iUV.w, aTexCoord.y);

    vTexCoord = vec3(u, v, iSliceIndex);
}
