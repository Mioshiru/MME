#ifndef RME_SVG_ICONS_H
#define RME_SVG_ICONS_H

namespace RME::UI::SVG {
    // Minimalistische RPG-Icons als SVG-Pfad-Daten
    
    // Selektions-Tool (Cursor)
    inline const char* ICON_SELECT = R"(<svg viewBox="0 0 24 24"><path fill="#DAA520" d="M7 2l12 11h-6l2 7-3 1-2-7-3 3z"/></svg>)";
    
    // Pinsel-Tool (Pencil)
    inline const char* ICON_PENCIL = R"(<svg viewBox="0 0 24 24"><path fill="#DAA520" d="M3 17.25V21h3.75L17.81 9.94l-3.75-3.75L3 17.25zM20.71 7.04c.39-.39.39-1.02 0-1.41l-2.34-2.34c-.39-.39-1.02-.39-1.41 0l-1.83 1.83 3.75 3.75 1.83-1.83z"/></svg>)";
    
    // Radiergummi (Eraser)
    inline const char* ICON_ERASER = R"(<svg viewBox="0 0 24 24"><path fill="#DAA520" d="M16.24 3.56l4.95 4.94c.78.79.78 2.05 0 2.84L12 20.53a4.008 4.008 0 0 1-5.66 0L2.81 17c-.78-.79-.78-2.05 0-2.84l10.59-10.6c.79-.78 2.05-.78 2.84 0zM7.07 18.28l5.17-5.17-2.83-2.83-5.17 5.17 2.83 2.83z"/></svg>)";
    
    // Füll-Eimer (Bucket)
    inline const char* ICON_BUCKET = R"(<svg viewBox="0 0 24 24"><path fill="#DAA520" d="M19 13c.02 0 .04 0 .06.01.01.01.03.02.04.03l.03.03c.83.84.83 2.2 0 3.04l-3.23 3.26c-.84.83-2.2.83-3.04 0L12 18.5l.89-.89c.14-.14.22-.33.22-.53 0-.41-.34-.75-.75-.75-.2 0-.39.08-.53.22L11 17.44l-5.66-5.66c-.83-.84-.83-2.2 0-3.04l5.66-5.65c.84-.84 2.2-.84 3.04 0l5.65 5.65c.16.16.27.35.31.56z"/></svg>)";

    // Sichtbarkeit (Eye)
    inline const char* ICON_EYE    = R"(<svg viewBox="0 0 24 24"><path fill="#DAA520" d="M12 4.5C7 4.5 2.73 7.61 1 12c1.73 4.39 6 7.5 11 7.5s9.27-3.11 11-7.5c-1.73-4.39-6-7.5-11-7.5zM12 17c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.66 0-3 1.34-3 3s1.34 3 3 3 3-1.34 3-3-1.34-3-3-3z"/></svg>)";

    // Etage hoch (Arrow Up)
    inline const char* ICON_UP     = R"(<svg viewBox="0 0 24 24"><path fill="#DAA520" d="M7 14l5-5 5 5z"/></svg>)";
    
    // Etage runter (Arrow Down)
    inline const char* ICON_DOWN   = R"(<svg viewBox="0 0 24 24"><path fill="#DAA520" d="M7 10l5 5 5-5z"/></svg>)";

    // Waypoint (Marker)
    inline const char* ICON_WAYPOINT = R"(<svg viewBox="0 0 24 24"><path fill="#DAA520" d="M12 2C8.13 2 5 5.13 5 9c0 5.25 7 13 7 13s7-7.75 7-13c0-3.87-3.13-7-7-7zm0 9.5a2.5 2.5 0 0 1 0-5 2.5 2.5 0 0 1 0 5z"/></svg>)";

    // Contextual Brushing (Magic Wand / Smart Tool)
    inline const char* ICON_CONTEXTUAL = R"(<svg viewBox="0 0 24 24"><path fill="#FFD700" d="M7.5,5.6L5,7L6.4,4.5L5,3L7.5,4.4L10,3L8.6,4.5L10,6L7.5,5.6M19.5,15.4L22,14L20.6,16.5L22,18L19.5,16.6L17,18L18.4,16.5L17,15L19.5,15.4M22,2L20.6,4.5L22,7L19.5,5.6L17,7L18.4,4.5L17,2L19.5,3.4L22,2M13.3,12.8L4.4,21.7L2.3,19.6L11.2,10.7L13.3,12.8Z"/></svg>)";

    // RPG Gold Style Icons für Prefabs (Flaticon Style)
    inline const char* ICON_HAMMER   = R"(<svg viewBox="0 0 64 64"><path fill="#FFD700" d="M54.5,43.5L34.1,23.1c1.5-3.3,1.1-7.3-1.4-10.2c-2.8-3.3-7.2-4.4-11.2-3.1l6.9,6.9L24.1,21l-6.9-6.9c-1.3,4,0.2,8.4,3.1,11.2c2.9,2.5,6.9,2.9,10.2,1.4l20.4,20.4L54.5,43.5z"/></svg>)";
    inline const char* ICON_BED      = R"(<svg viewBox="0 0 64 64"><path fill="#FFD700" d="M12,24h40v20H12V24z M48,16H16c-2.2,0-4,1.8-4,4v4h40v-4C52,17.8,50.2,16,48,16z M8,32h4v24H8V32z M52,32h4v24h-4V32z"/></svg>)";
    inline const char* ICON_BACKPACK = R"(<svg viewBox="0 0 64 64"><path fill="#FFD700" d="M32,8c-8.8,0-16,7.2-16,16v24h32V24C48,15.2,40.8,8,32,8z M32,16c4.4,0,8,3.6,8,8s-3.6,8-8,8s-8-3.6-8-8S27.6,16,32,16z"/></svg>)";
    inline const char* ICON_WAND     = R"(<svg viewBox="0 0 64 64"><path fill="#FFD700" d="M50.5,13.5c-2-2-5.2-2-7.2,0L13.5,43.3c-2,2-2,5.2,0,7.2l0,0c2,2,5.2,2,7.2,0l29.8-29.8C52.5,18.7,52.5,15.5,50.5,13.5L50.5,13.5z M44,18l2,2l-4,4l-2-2L44,18z"/></svg>)";
}

#endif
