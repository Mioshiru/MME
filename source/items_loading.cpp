#include "main.h"
#include "items.h"
#include "filehandle.h"

bool Items::loadFromOtb(const wxString& filename, wxString& error, wxArrayString& warnings) {
    FileReadHandle file(nstr(filename));
    if (!file.isOk()) return false;

    uint32_t signature;
    file.getU32(signature); // OTB Signature
    
    // Node-basiertes Laden der Items
    while (!file.isEOF()) {
        uint8_t node_type;
        file.getU8(node_type);
        if (node_type == 0xFE) { // Start Node
            // Item Definition Parsing...
        }
    }
    return true;
}

bool Items::loadFromGameXml(const wxString& filename, wxString& error, wxArrayString& warnings) {
    pugi::xml_document doc;
    if (!doc.load_file(filename.fn_str())) return false;

    for (pugi::xml_node itemNode = doc.child("items").child("item"); itemNode; itemNode = itemNode.next_sibling("item")) {
        uint16_t id = itemNode.attribute("id").as_int();
        if (id == 0) id = itemNode.attribute("fromid").as_int();
        
        ItemType& it = types[id];
        it.name = itemNode.attribute("name").as_string();
        
        for (pugi::xml_node attrNode = itemNode.child("attribute"); attrNode; attrNode = attrNode.next_sibling("attribute")) {
            std::string key = attrNode.attribute("key").as_string();
            if (key == "containerSize") it.isContainer = true;
            else if (key == "stackable") it.stackable = true;
        }
    }
    return true;
}