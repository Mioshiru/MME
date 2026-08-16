const fs = require('fs');

// Let's trace doBorders logic in ground_brush.cpp!
// Suppose tile (x, y) is Grass (z=3200).
// Its neighbor (x, y+1) (to the South) is Mountain (z=9900).
// What does GroundBrush::doBorders do on the Grass tile?
//
// In doBorders for Grass tile:
// borderBrush = grass (z=3200).
// neighbor (South) = mountain (z=9900).
// other = mountain.
// In loop (i=0..7):
// other->hasOuterBorder() is true (mountain has border 10).
// getBrushTo(grass, mountain):
//   first = grass (3200)
//   second = mountain (9900)
//   first->getZ() (3200) < second->getZ() (9900) && second->hasOuterBorder()
//   matches: second's outer border! -> border id 10!
//
// So on the Grass tile (to the North of the mountain):
// Neighbor to the South is Mountain (TILE_SOUTH).
// getBrushTo(grass, mountain) returns border 10.
// tiledata has TILE_SOUTH.
// border_types[TILE_SOUTH] is SOUTH_HORIZONTAL (or NORTH_HORIZONTAL?)
//
// Wait! Let's check which border item is placed on the Grass tile!
// And WHERE does that Grass tile draw its item?
// The Grass tile is at (x, y). The mountain is at (x, y+1).
// The Grass tile gets borderitem edge="s" -> item 4458 (mountain wall south face? or north face?).
// BUT WAIT! In Tibia:
// Item 4456 is North face, 4457 is East face, 4458 is South face, 4459 is West face!
// The border item is drawn AT (x, y) (the grass tile position)!
//
// WAIT! Is item 4456/4457/4458/4459 a border item that has an offset / size?
// Let's check the size/offset or drawing of item 4456..4467!
