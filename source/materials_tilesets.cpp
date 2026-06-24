#include "main.h"
#include "brush.h"
#include "materials.h"
#include "tileset.h"

void Materials::createOtherTileset() {
  Tileset *other = new Tileset(g_brushes, "Other");
  tilesets["Other"] = other;
}

bool Materials::isInTileset(Item *item, std::string tilesetName) const {
  auto it = tilesets.find(tilesetName);
  if (it == tilesets.end())
    return false;
  return it->second->containsBrush(item->getBrush());
}

void Materials::clear() {
  for (auto &[name, ts] : tilesets)
    delete ts;
  tilesets.clear();
}