# Fallback toolchain loader for vcpkg.
# This lets CMake configure even when the vcpkg install is not in the default
# USERPROFILE location or when the environment variable is not set.

set(_vcpkg_toolchain_candidates
    "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
    "$ENV{VCPKG_ROOT_DIR}/scripts/buildsystems/vcpkg.cmake"
    "$ENV{USERPROFILE}/vcpkg/scripts/buildsystems/vcpkg.cmake"
    "C:/src/vcpkg/scripts/buildsystems/vcpkg.cmake"
    "D:/src/vcpkg/scripts/buildsystems/vcpkg.cmake"
    "c:/vcpkg/scripts/buildsystems/vcpkg.cmake"
    "d:/vcpkg/scripts/buildsystems/vcpkg.cmake"
)

foreach(_candidate IN LISTS _vcpkg_toolchain_candidates)
    if(EXISTS "${_candidate}")
        set(VCPKG_TOOLCHAIN_FILE "${_candidate}" CACHE FILEPATH "Path to vcpkg toolchain" FORCE)
        break()
    endif()
endforeach()

if(DEFINED VCPKG_TOOLCHAIN_FILE AND EXISTS "${VCPKG_TOOLCHAIN_FILE}")
    message(STATUS "Using vcpkg toolchain: ${VCPKG_TOOLCHAIN_FILE}")
    include("${VCPKG_TOOLCHAIN_FILE}")
else()
    message(WARNING "vcpkg toolchain not found; continuing without it")
endif()
