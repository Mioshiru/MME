#include "main.h"
#include "iomap_otbm.h"
#include "item.h"
#include "complexitem.h"
#include "house.h"

void IOMapOTBM::readItem(FileReadHandle& file, Map& map, Container* parent) {
    uint16_t id = file.getU16();
    Item* item = Item::Create(id);
    
    while (true) {
        uint8_t attr = file.getU8();
        if (attr == OTBM_ATTR_END) break;

        switch (attr) {
            case OTBM_ATTR_COUNT:
            case OTBM_ATTR_RUNE_CHARGES:
                item->setSubtype(file.getU8()); break;
            case OTBM_ATTR_CHARGES:
                item->setSubtype(file.getU16()); break;
            case OTBM_ATTR_TEXT: item->setText(file.getString()); break;
            case OTBM_ATTR_DESC: item->setDescription(file.getString()); break;
            case OTBM_ATTR_ACTION_ID: item->setActionID(file.getU16()); break;
            case OTBM_ATTR_UNIQUE_ID: item->setUniqueID(file.getU16()); break;
            case OTBM_ATTR_TELE_DEST: {
                uint16_t x = file.getU16(), y = file.getU16();
                uint8_t z = file.getU8();
                if (auto* tele = dynamic_cast<Teleport*>(item)) tele->setDestination(Position(x, y, z));
                break;
            }
            case OTBM_ATTR_HOUSEDOORID:
                if (auto* door = dynamic_cast<Door*>(item)) door->setDoorID(file.getU8()); break;
            case OTBM_ATTR_DEPOT_ID:
                if (auto* depot = dynamic_cast<Depot*>(item)) depot->setDepotID(file.getU16()); break;
            case OTBM_ATTR_TIER:
                item->setTier(file.getU8()); break;
            case OTBM_ATTR_ATTRIBUTE_MAP:
                item->unserializeAttributeMap(*this, file); break;
            default:
                file.skip(file.getU16()); break; // Skip unknown
        }
    }

    if (parent) parent->addItem(item);
}

void IOMapOTBM::writeItem(FileWriteHandle& file, Item* item) {
    file.addU16(item->getID());
    if (item->hasSubtype()) { file.addU8(OTBM_ATTR_COUNT); file.addU8(item->getSubtype()); }
    if (item->getActionID()) { file.addU8(OTBM_ATTR_ACTION_ID); file.addU16(item->getActionID()); }
    if (item->getUniqueID()) { file.addU8(OTBM_ATTR_UNIQUE_ID); file.addU16(item->getUniqueID()); }
    if (!item->getText().empty()) { file.addU8(OTBM_ATTR_TEXT); file.addString(item->getText()); }
    if (item->getTier()) { file.addU8(OTBM_ATTR_TIER); file.addU8(item->getTier()); }
    
    if (item->isComplex() && version.otbm >= MAP_OTBM_4) {
        file.addU8(OTBM_ATTR_ATTRIBUTE_MAP);
        item->serializeAttributeMap(*this, file);
    }
}