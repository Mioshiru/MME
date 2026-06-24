#include "otc_export.h"
#include "iomap_otbm.h"
#include "gui.h"
#include <iostream>
#include <wx/filename.h>
#include <wx/dir.h>

namespace RME::Core {
    bool OTCExport::ExportMap(Map* map, const std::string& filepath) {
        if (!map) return false;
        
        std::cout << "Generating Ready-to-Use OTClient Map Package at: " << filepath << std::endl;
        
        wxFileName fn(wxString::FromUTF8(filepath));
        wxString dir = fn.GetPath();
        wxString minimapDir = dir + "/minimap";
        if (!wxDir::Exists(minimapDir)) {
            wxDir::Make(minimapDir, wxDIR_DEFAULT);
        }

        // Export OTBM
        IOMapOTBM iomap(map->getVersion());
        FileName out_fn(fn);
        if (!iomap.saveMap(*map, out_fn)) {
            return false;
        }

        // Export minimap files
        map->exportOTClient(minimapDir.ToStdString());

        return true;
    }
}