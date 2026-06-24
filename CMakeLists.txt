cmake_minimum_required(VERSION 3.12)

if(POLICY CMP0072)
    cmake_policy(SET CMP0072 NEW) # Prefer GLVND
endif()

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW) # Use BoostConfig
endif()

project(rme)

# Graphics & Core Dependencies
find_package(Vulkan QUIET)
# Falls Vulkan nicht systemweit gefunden wurde, nutze den lokalen Pfad
if(NOT Vulkan_FOUND)
    set(Vulkan_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/deps/vulkan/include")
    # Markiere Vulkan als gefunden, damit der Build weiterläuft
    set(Vulkan_FOUND TRUE)
    # Erstelle ein Dummy-Target für den Linker
    if(NOT TARGET Vulkan::Vulkan)
        add_library(Vulkan::Vulkan INTERFACE IMPORTED)
        set_target_properties(Vulkan::Vulkan PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${Vulkan_INCLUDE_DIR}")
    endif()
endif()

# Jetzt kannst du ganz normal mit deinen Targets weiterarbeiten
include_directories(${Vulkan_INCLUDE_DIR})

find_package(glm CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# Force SOL2 compatibility and definitions globally
# This ensures that the PCH and all source files accept Lua 5.5
add_definitions(-DSOL_USING_CXX_LUA_HPP=1)
add_definitions(-DSOL_LUA_VERSION=504) # Angepasst an vcpkg Lua 5.4
add_definitions(-DLUA_COMPAT_5_3)

add_definitions(-DBOOST_BIND_GLOBAL_PLACEHOLDERS)

if(MSVC)
    string(APPEND CMAKE_CXX_FLAGS " /W3 /EHsc /bigobj /D_CRT_SECURE_NO_WARNINGS")
    string(APPEND CMAKE_CXX_FLAGS_DEBUG " /D__DEBUG__")
else()
    string(APPEND CMAKE_CXX_FLAGS " -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-deprecated-declarations -Wno-overloaded-virtual -Wno-strict-aliasing -Wno-sign-compare -Wno-unused-function -Wunused-result")
    string(APPEND CMAKE_CXX_FLAGS_DEBUG " -D__DEBUG__")
endif()

find_package(OpenGL REQUIRED)

if(APPLE)
    set(CMAKE_PREFIX_PATH /usr/local/opt/libarchive)
endif()
find_package(LibArchive REQUIRED)

if(WIN32)
    set(Boost_THREADAPI win32)
endif()
find_package(Boost COMPONENTS thread system REQUIRED)

if(DEFINED Boost_INCLUDE_DIR)
    if(NOT DEFINED Boost_INCLUDE_DIRS OR "${Boost_INCLUDE_DIRS}" STREQUAL "")
        set(Boost_INCLUDE_DIRS ${Boost_INCLUDE_DIR})
    else()
        list(APPEND Boost_INCLUDE_DIRS ${Boost_INCLUDE_DIR})
    endif()
endif()

set(BOOST_LINK_TARGETS ${Boost_LIBRARIES})
if(TARGET Boost::thread AND TARGET Boost::system)
    set(BOOST_LINK_TARGETS Boost::thread Boost::system)
endif()

# Moderne wxWidgets Einbindung für vcpkg/MSVC
if(MSVC)
    find_package(wxWidgets CONFIG REQUIRED)
    set(WX_LIBS wx::html wx::aui wx::gl wx::adv wx::core wx::net wx::base wx::xml)
else()
    find_package(wxWidgets COMPONENTS html aui gl adv core net base REQUIRED)
    include(${wxWidgets_USE_FILE})
    set(WX_LIBS ${wxWidgets_LIBRARIES})
endif()

find_package(GLUT REQUIRED)
find_package(ZLIB REQUIRED)

# Lua scripting support
if(MSVC)
    # Windows/vcpkg specific configuration
    find_package(lua CONFIG REQUIRED)
    find_package(sol2 CONFIG REQUIRED)
    find_package(cpr CONFIG REQUIRED)
    find_package(nlohmann_json CONFIG REQUIRED)
    find_package(NanoSVG CONFIG REQUIRED)

    # vcpkg provides unofficial::lua::lua in config mode
    set(LUA_LIB_TARGET unofficial::lua::lua)

    find_package(bgfx CONFIG REQUIRED)
    find_package(imgui CONFIG REQUIRED)

    # Add Graphics & Modern Core Libs
    set(LIBS_TO_LINK ${LUA_LIB_TARGET} cpr::cpr nlohmann_json::nlohmann_json sol2::sol2 glm::glm EnTT::EnTT NanoSVG::nanosvg Vulkan::Vulkan bgfx::bgfx imgui::imgui)
    
    # Add Lua include directory from the target properties
    get_target_property(LUA_INC ${LUA_LIB_TARGET} INTERFACE_INCLUDE_DIRECTORIES)
    include_directories(${LUA_INC})
else()
    # Linux / Standard CMake Module configuration
    find_package(Lua REQUIRED)

    include(FetchContent)

    # sol2
    find_package(sol2 CONFIG QUIET)
    if(NOT sol2_FOUND)
        message(STATUS "sol2 not found via find_package, using FetchContent...")
        FetchContent_Declare(
            sol2
            GIT_REPOSITORY https://github.com/ThePhD/sol2.git
            GIT_TAG        v3.3.0
        )
        FetchContent_MakeAvailable(sol2)
        if(NOT TARGET sol2::sol2)
            if(TARGET sol2)
                add_library(sol2::sol2 ALIAS sol2)
            else()
                set(SOL2_INCLUDE_DIRS ${sol2_SOURCE_DIR}/include)
                include_directories(${SOL2_INCLUDE_DIRS})
                add_library(sol2::sol2 INTERFACE IMPORTED)
                set_target_properties(sol2::sol2 PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${SOL2_INCLUDE_DIRS}")
            endif()
        endif()
    endif()

    # cpr
    find_package(cpr CONFIG QUIET)
    if(NOT cpr_FOUND)
        message(STATUS "cpr not found via find_package, using FetchContent...")
        FetchContent_Declare(
            cpr
            GIT_REPOSITORY https://github.com/libcpr/cpr.git
            GIT_TAG        1.10.5
        )
        FetchContent_MakeAvailable(cpr)
    endif()

    # nlohmann_json
    find_package(nlohmann_json CONFIG QUIET)
    if(NOT nlohmann_json_FOUND)
        message(STATUS "nlohmann_json not found via find_package, using FetchContent...")
        FetchContent_Declare(
            json
            GIT_REPOSITORY https://github.com/nlohmann/json.git
            GIT_TAG        v3.11.3
        )
        FetchContent_MakeAvailable(json)
    endif()

    set(LIBS_TO_LINK ${LUA_LIBRARIES} cpr::cpr nlohmann_json::nlohmann_json sol2::sol2)
endif()


include(source/CMakeLists.txt)
if(WIN32)
    add_executable(rme WIN32 ${rme_H} ${rme_SRC})
else()
    add_executable(rme ${rme_H} ${rme_SRC})
endif()
set_target_properties(rme PROPERTIES OUTPUT_NAME "MME")

if(MSVC)
    set_target_properties(rme PROPERTIES 
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        LINKER_LANGUAGE CXX
        LINK_FLAGS "/MAP"
    )
else()
    set_target_properties(rme PROPERTIES 
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        LINKER_LANGUAGE CXX
    )
endif()

if(COMMAND target_precompiled_headers)
    target_precompiled_headers(rme PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/source/main.h")
endif()

include_directories(source)
if(Boost_FOUND)
    if(DEFINED Boost_INCLUDE_DIRS AND NOT "${Boost_INCLUDE_DIRS}" STREQUAL "")
        target_include_directories(rme PRIVATE ${Boost_INCLUDE_DIRS})
    elseif(DEFINED Boost_INCLUDE_DIR AND NOT "${Boost_INCLUDE_DIR}" STREQUAL "")
        target_include_directories(rme PRIVATE ${Boost_INCLUDE_DIR})
    endif()
endif()
target_link_libraries(rme PRIVATE ${WX_LIBS} ${BOOST_LINK_TARGETS} ${LibArchive_LIBRARIES} ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${ZLIB_LIBRARIES} ${LIBS_TO_LINK})

# Compiler options für MSVC
if(MSVC)
    target_compile_options(rme PRIVATE /bigobj)
endif()
