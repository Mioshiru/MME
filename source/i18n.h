#ifndef RME_I18N_H
#define RME_I18N_H

#include <string_view>

namespace RME::I18N {
    // Tooltip format strings
    inline constexpr std::string_view TOOLTIP_ID          = "id: {}\n";
    inline constexpr std::string_view TOOLTIP_AID         = "aid: {}\n";
    inline constexpr std::string_view TOOLTIP_UID         = "uid: {}\n";
    inline constexpr std::string_view TOOLTIP_DOOR_ID     = "door id: {}\n";
    inline constexpr std::string_view TOOLTIP_TEXT        = "text: {}\n";
    inline constexpr std::string_view TOOLTIP_DESTINATION = "destination: {}, {}, {}\n";
    inline constexpr std::string_view TOOLTIP_WAYPOINT    = "wp: {}\n";
}

#endif
