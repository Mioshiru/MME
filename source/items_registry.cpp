#include "main.h"
#include "items.h"

const ItemType& Items::getItemType(uint16_t id) const {
    auto it = types.find(id);
    if (it != types.end()) return it->second;
    return types.at(0); // Fallback to Unknown
}

uint16_t Items::findItemTypeByName(const std::string& name) const {
    for (const auto& [id, type] : types) {
        if (type.name == name) return id;
    }
    return 0;
}

void Items::clear() {
    types.clear();
    tile_items.clear();
}

bool Items::typeExists(uint16_t id) const {
    return types.find(id) != types.end();
}