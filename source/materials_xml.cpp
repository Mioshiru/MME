#include "main.h"
#include "materials.h"

bool Materials::loadMaterials(const FileName &filename, wxString &error,
                              wxArrayString &warnings) {
  pugi::xml_document doc;
  if (!doc.load_file(filename.GetFullPath().mb_str()))
    return false;

  for (pugi::xml_node brushNode = doc.child("materials").first_child();
       brushNode; brushNode = brushNode.next_sibling()) {
    std::string type = brushNode.name();
    if (type == "brush") {
      // Brush Parsing Logik...
    }
  }
  return true;
}

bool Materials::loadExtensions(FileName directory, wxString &error,
                               wxArrayString &warnings) {
  // Scans directory for .xml extension files and calls loadMaterials
  return true;
}