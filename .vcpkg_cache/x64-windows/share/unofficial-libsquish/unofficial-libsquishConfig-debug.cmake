#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "unofficial::libsquish::squish" for configuration "Debug"
set_property(TARGET unofficial::libsquish::squish APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(unofficial::libsquish::squish PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/debug/lib/squishd.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/bin/squishd.dll"
  )

list(APPEND _cmake_import_check_targets unofficial::libsquish::squish )
list(APPEND _cmake_import_check_files_for_unofficial::libsquish::squish "${_IMPORT_PREFIX}/debug/lib/squishd.lib" "${_IMPORT_PREFIX}/debug/bin/squishd.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
