# Performance-Optimierungen für schnelles Kompilieren
if(MSVC)
    set(CMAKE_UNITY_BUILD OFF)
    add_compile_options(/Zc:inline)
endif()

file(GLOB_RECURSE rme_H CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.h" "${CMAKE_CURRENT_LIST_DIR}/*.hpp")
file(GLOB_RECURSE rme_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/*.c")

list(FILTER rme_SRC EXCLUDE REGEX "[/\\].*\\.cpp\\.stub$")

if(WIN32)
    list(APPEND rme_SRC "${CMAKE_CURRENT_LIST_DIR}/rme.rc")
endif()