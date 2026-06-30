# Performance-Optimierungen für schnelles Kompilieren
if(MSVC)
    # Unity Build aktivieren (fasst Dateien zusammen)
    set(CMAKE_UNITY_BUILD OFF)
    # set(CMAKE_UNITY_BUILD_BATCH_SIZE 20)
    
    # Multicore-Kompilierung auf Dateiebene erzwingen
    add_compile_options(/MP)
    # Schnellere Verknüpfung und weniger Debug-Overhead im Release
    add_compile_options(/Zc:inline)
endif()

file(GLOB_RECURSE rme_H CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.h" "${CMAKE_CURRENT_LIST_DIR}/*.hpp")
file(GLOB_RECURSE rme_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/*.c")

set(EXCLUDED_SOURCE_NAMES
    editor_actions.cpp
    editor_io.cpp
    editor_selection.cpp
    canvas_minimap.cpp
    drawer_core.cpp
    drawer_blit.cpp
    drawer_overlay.cpp
    map_drawer_brush.cpp
    map_drawer_ui.cpp
    materials_tilesets.cpp
    materials_xml.cpp
    dat_debug_view.cpp
    rme_net.cpp
    updater.cpp
)

# Keep the build focused on the active OTBM implementation path.
# This avoids pulling in legacy split implementations, optional network/debug helpers,
# and other non-core pieces that are not required for the editor workflow.
foreach(_excluded_name IN LISTS EXCLUDED_SOURCE_NAMES)
    list(FILTER rme_SRC EXCLUDE REGEX "[/\\]${_excluded_name}$")
endforeach()
list(FILTER rme_SRC EXCLUDE REGEX "[/\\].*\\.cpp\\.stub$")

if(WIN32)
    list(APPEND rme_SRC "${CMAKE_CURRENT_LIST_DIR}/rme.rc")
endif()