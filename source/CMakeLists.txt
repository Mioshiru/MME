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

if(WIN32)
    list(APPEND rme_SRC "${CMAKE_CURRENT_LIST_DIR}/rme.rc")
endif()