#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "bgfx::bx" for configuration "Debug"
set_property(TARGET bgfx::bx APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(bgfx::bx PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/lib/bx.lib"
  )

list(APPEND _cmake_import_check_targets bgfx::bx )
list(APPEND _cmake_import_check_files_for_bgfx::bx "${_IMPORT_PREFIX}/debug/lib/bx.lib" )

# Import target "bgfx::bimg" for configuration "Debug"
set_property(TARGET bgfx::bimg APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(bgfx::bimg PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/lib/bimg.lib"
  )

list(APPEND _cmake_import_check_targets bgfx::bimg )
list(APPEND _cmake_import_check_files_for_bgfx::bimg "${_IMPORT_PREFIX}/debug/lib/bimg.lib" )

# Import target "bgfx::bimg_decode" for configuration "Debug"
set_property(TARGET bgfx::bimg_decode APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(bgfx::bimg_decode PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/lib/bimg_decode.lib"
  )

list(APPEND _cmake_import_check_targets bgfx::bimg_decode )
list(APPEND _cmake_import_check_files_for_bgfx::bimg_decode "${_IMPORT_PREFIX}/debug/lib/bimg_decode.lib" )

# Import target "bgfx::bimg_encode" for configuration "Debug"
set_property(TARGET bgfx::bimg_encode APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(bgfx::bimg_encode PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C;CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/lib/bimg_encode.lib"
  )

list(APPEND _cmake_import_check_targets bgfx::bimg_encode )
list(APPEND _cmake_import_check_files_for_bgfx::bimg_encode "${_IMPORT_PREFIX}/debug/lib/bimg_encode.lib" )

# Import target "bgfx::bgfx" for configuration "Debug"
set_property(TARGET bgfx::bgfx APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(bgfx::bgfx PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/lib/bgfx.lib"
  )

list(APPEND _cmake_import_check_targets bgfx::bgfx )
list(APPEND _cmake_import_check_files_for_bgfx::bgfx "${_IMPORT_PREFIX}/debug/lib/bgfx.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
