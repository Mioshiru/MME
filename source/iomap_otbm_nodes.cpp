#include "main.h"
#include "iomap_otbm.h"
#include "filehandle.h"

bool IOMapOTBM::loadMap(Map& map, const wxString& filename) {
    FileReadHandle file(nstr(filename));
    if (!file.isOk()) return false;

    // OTBM Header Check
    uint32_t identifier;
    file.getU32(identifier);
    if (identifier != 0x4D42544F) return false; // "OTBM"

    return parseNode(file, map);
}

bool IOMapOTBM::parseNode(FileReadHandle& file, Map& map) {
    uint8_t type = file.getU8();
    if (type == OTBM_TILE_AREA) {
        // Handle Area
    } else if (type == OTBM_TILE) {
        readTile(file, map);
    }
    
    // Rekursives Suchen nach Child-Nodes
    return true;
}