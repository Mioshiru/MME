get_filename_component(VCPKG_IMPORT_PREFIX "${CMAKE_CURRENT_LIST_DIR}/../../" ABSOLUTE)
#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "unofficial::tinyexr::tinyexr" for configuration "Debug"
set_property(TARGET unofficial::tinyexr::tinyexr APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(unofficial::tinyexr::tinyexr PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${VCPKG_IMPORT_PREFIX}/debug/lib/tinyexr.lib"
  )

list(APPEND _cmake_import_check_targets unofficial::tinyexr::tinyexr )
list(APPEND _cmake_import_check_files_for_unofficial::tinyexr::tinyexr "${VCPKG_IMPORT_PREFIX}/debug/lib/tinyexr.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
