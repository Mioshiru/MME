#ifndef RME_OTC_EXPORT_H_
#define RME_OTC_EXPORT_H_

#include "map.h"
#include <string>

namespace RME::Core {
    class OTCExport {
    public:
        static bool ExportMap(Map* map, const std::string& filepath);
    };
}

#endif // RME_OTC_EXPORT_H_
