#version 330 core

// Per-vertex attributes
layout(location = 0) in vec2 aPos;       // local tile-quad position [0,1]
layout(location = 1) in vec2 aTexCoord;  // local UV [0,1]

// Per-instance attributes (one per tile)
layout(location = 2) in vec2  iGridPos;  // tile position in world grid (x, y)
layout(location = 3) in vec4  iUV;       // atlas UV rect (u0, v0, u1, v1)
layout(location = 4) in float iLayer;    // z-order within floor

uniform mat4 uProjection;   // orthographic projection
uniform vec2 uTileSize;     // size of one tile in world units (32, 32)

out vec2 vTexCoord;

void main()
{
    // Scale unit quad to tile size
    vec2 worldPos = iGridPos * uTileSize + aPos * uTileSize;

    gl_Position = uProjection * vec4(worldPos, iLayer, 1.0);

    // Map local [0,1] UV to atlas sub-rect
    vTexCoord = vec2(
        mix(iUV.x, iUV.z, aTexCoord.x),
        mix(iUV.y, iUV.w, aTexCoord.y)
    );
}
