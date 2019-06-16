cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
project(NetRadiant C CXX)

option(BUILD_RADIANT "Build the GUI" ON)
option(BUILD_TOOLS "Build the tools" ON)
option(BUILD_CRUNCH "Build Crunch image support" OFF)
option(DOWNLOAD_GAMEPACKS "Download game packs" ON)
option(USE_WERROR "Build with -Werror -pedantic-errors" OFF)
option(FHS_INSTALL "Install according to Filesystem Hierarchy Standard" OFF)

set(BUILD_BINARIES OFF CACHE INTERNAL "...")
if (BUILD_RADIANT OR BUILD_TOOLS)
    set(BUILD_BINARIES ON)
endif ()

#-----------------------------------------------------------------------
# Paths
#-----------------------------------------------------------------------

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/install" CACHE PATH "..." FORCE)
endif ()

set(RADIANT_NAME "NetRadiant" CACHE INTERNAL "...")
add_definitions(-DRADIANT_NAME="${RADIANT_NAME}")
set(RADIANT_BASENAME "netradiant" CACHE INTERNAL "...")
add_definitions(-DRADIANT_BASENAME="${RADIANT_BASENAME}")

set(RADIANT_LIB_DIR ${CMAKE_INSTALL_PREFIX} CACHE INTERNAL "...")
set(RADIANT_BIN_DIR ${CMAKE_INSTALL_PREFIX} CACHE INTERNAL "...")
set(RADIANT_DATA_DIR ${CMAKE_INSTALL_PREFIX} CACHE INTERNAL "...")

if (FHS_INSTALL)
    set(RADIANT_LIB_DIR ${CMAKE_INSTALL_PREFIX}/lib/${RADIANT_BASENAME})
    set(RADIANT_BIN_DIR ${CMAKE_INSTALL_PREFIX}/bin)
    set(RADIANT_DATA_DIR ${CMAKE_INSTALL_PREFIX}/share/${RADIANT_BASENAME})
endif ()

#-----------------------------------------------------------------------
# Version
#-----------------------------------------------------------------------

# CMake 3.0+ would allow this in project()
set(RADIANT_VERSION_MAJOR 1)
set(RADIANT_VERSION_MINOR 5)
set(RADIANT_VERSION_PATCH 0)
set(RADIANT_VERSION "${RADIANT_VERSION_MAJOR}.${RADIANT_VERSION_MINOR}.${RADIANT_VERSION_PATCH}")

set(RADIANT_ABOUTMSG "Custom build" CACHE STRING "About message")

find_package(Git REQUIRED)

execute_process(
     COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
     WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
     OUTPUT_VARIABLE GIT_VERSION
     OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(RADIANT_VERSION_STRING "${RADIANT_VERSION}n")

if (GIT_VERSION)
    set(RADIANT_VERSION_STRING "${RADIANT_VERSION_STRING}-git-${GIT_VERSION}")
endif ()

message(STATUS "Building ${PROJECT_NAME} ${RADIANT_VERSION_STRING} ${RADIANT_ABOUTMSG}")

#-----------------------------------------------------------------------
# Language standard
#-----------------------------------------------------------------------

if (BUILD_BINARIES)
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)

    if (CMAKE_VERSION VERSION_LESS "3.1")
        if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR CMAKE_COMPILER_IS_GNUCXX)
            include(CheckCXXCompilerFlag)
            check_cxx_compiler_flag(--std=c++${CMAKE_CXX_STANDARD} STD_CXX)
            if (STD_CXX)
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++${CMAKE_CXX_STANDARD}")
            else ()
                message(SEND_ERROR "Requires C++${CMAKE_CXX_STANDARD} or better")
            endif ()
        else ()
            message(WARNING "Unrecognized compiler: ${CMAKE_CXX_COMPILER_ID}, make sure it supports C++${CMAKE_CXX_STANDARD}")
        endif ()
    endif ()
endif ()

#-----------------------------------------------------------------------
# Flags
#-----------------------------------------------------------------------

if (BUILD_BINARIES)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")

    macro (addflags_c args)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${args}")
    endmacro ()

    macro (addflags_cxx args)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${args}")
    endmacro ()

    macro (addflags args)
        addflags_c("${args}")
        addflags_cxx("${args}")
    endmacro ()

    addflags("-fno-strict-aliasing")
    if (NOT WIN32)
        addflags("-fvisibility=hidden")
    endif ()

    if (USE_WERROR)
        addflags("-Werror")
        addflags("-pedantic-errors")
    endif ()

    addflags("-Wall")
    addflags("-Wextra")
    addflags("-pedantic")

    addflags_c("-Wno-deprecated-declarations") # vfs.c: g_strdown

    addflags("-Wno-unused-function")
    addflags("-Wno-unused-variable")
    addflags("-Wno-unused-parameter")

    set(CMAKE_POSITION_INDEPENDENT_CODE 1)
endif ()

if (BUILD_RADIANT)
    set(GTK_TARGET 2 CACHE STRING "GTK target")
    add_definitions(-DGTK_TARGET=${GTK_TARGET})
endif ()

#-----------------------------------------------------------------------
# Defs
#-----------------------------------------------------------------------

if (BUILD_BINARIES)
    add_definitions(-DRADIANT_VERSION="${RADIANT_VERSION}")
    add_definitions(-DRADIANT_VERSION_STRING="${RADIANT_VERSION_STRING}")

    if (NOT CMAKE_BUILD_TYPE MATCHES Release)
        add_definitions(-D_DEBUG=1)
    endif ()
endif ()

if (BUILD_RADIANT)
    add_definitions(-DRADIANT_ABOUTMSG="${RADIANT_ABOUTMSG}")

    macro (disable_deprecated name gtk2only)
        add_definitions(-D${name}_DISABLE_SINGLE_INCLUDES)
        if ((${gtk2only} EQUAL 0) OR (GTK_TARGET EQUAL 2))
            add_definitions(-D${name}_DISABLE_DEPRECATED)
        endif ()
    endmacro ()

    disable_deprecated(ATK 0)
    disable_deprecated(G 0)
    disable_deprecated(GDK 0)
    disable_deprecated(GDK_PIXBUF 0)
    disable_deprecated(GTK 1)
    disable_deprecated(PANGO 0)
endif ()

if (BUILD_BINARIES)
    if(APPLE OR ${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
        link_directories(
            /usr/local/lib
        )
    endif ()

    if (WIN32)
        add_definitions(
            -DWIN32=1
            -D_WIN32=1
        )
    else ()
        add_definitions(
            -DPOSIX=1
        )
    endif ()
endif ()

if (BUILD_RADIANT AND NOT APPLE AND NOT WIN32)
    set(XWINDOWS ON)
    find_package(X11 REQUIRED)
    include_directories(${X11_INCLUDE_DIR})
    add_definitions(-DXWINDOWS=1)
endif ()

if (BUILD_BINARIES)
    include_directories("${PROJECT_SOURCE_DIR}/include")
    include_directories("${PROJECT_SOURCE_DIR}/libs")

    if (WIN32 AND NOT CMAKE_CROSSCOMPILING)
        set(BUNDLE_LIBRARIES_DEFAULT ON)
    else ()
        set(BUNDLE_LIBRARIES_DEFAULT OFF)
    endif ()

    option(BUNDLE_LIBRARIES "Bundle libraries" ${BUNDLE_LIBRARIES_DEFAULT})

    macro (copy_dlls target)
        if (BUNDLE_LIBRARIES AND WIN32)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND bash
                ARGS -c "ldd '$<TARGET_FILE:${target}>' | egrep -i '\\.dll ' | grep -iv '/c/Windows' | awk '{ print $1 }' | while read dll; do dllbasename=\"$(which \"$dll\")\"; [ -f \"${PROJECT_BINARY_DIR}/$dllbasename\" ] || cp --preserve=timestamps \"$dllbasename\" '${PROJECT_BINARY_DIR}'; done"
                VERBATIM
        )
        endif ()
    endmacro ()

    macro (bundle_stuff target)
        if (BUNDLE_LIBRARIES AND WIN32)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND bash
                ARGS -c "mkdir -p '${PROJECT_BINARY_DIR}/share/themes'; mkdir -p '${PROJECT_BINARY_DIR}/lib'; mingw=\"$(echo '${CMAKE_C_COMPILER}' | sed -e 's|/bin/.*||')\"; [ -d '${PROJECT_BINARY_DIR}/share/themes/MS-Windows' ] || cp -r --preserve=timestamps \"$mingw/share/themes/MS-Windows\" '${PROJECT_BINARY_DIR}/share/themes/'; [ -d '${PROJECT_BINARY_DIR}/lib/gdk-pixbuf-2.0' ] || cp -r --preserve=timestamps \"$mingw/lib/gdk-pixbuf-2.0\" '${PROJECT_BINARY_DIR}/lib/'; [ -d '${PROJECT_BINARY_DIR}/lib/gtk-2.0' ] || cp -r --preserve=timestamps \"$mingw/lib/gtk-2.0\" '${PROJECT_BINARY_DIR}/lib/'"
                VERBATIM
        )

        file(GLOB DLL_FILES ${PROJECT_BINARY_DIR}/*.dll)

        install(FILES
            ${DLL_FILES}
            DESTINATION ${CMAKE_INSTALL_PREFIX}
        )

        install(DIRECTORY
            ${PROJECT_BINARY_DIR}/lib
            ${PROJECT_BINARY_DIR}/share
            DESTINATION ${CMAKE_INSTALL_PREFIX}
        )
        endif ()
    endmacro ()
endif ()

#-----------------------------------------------------------------------
# Libraries
#-----------------------------------------------------------------------

if (BUILD_BINARIES)
    add_subdirectory(libs)
endif ()

if (BUILD_RADIANT)
    add_subdirectory(include)
endif ()

#-----------------------------------------------------------------------
# Plugins
#-----------------------------------------------------------------------

if (BUILD_RADIANT)
    add_subdirectory(contrib)
endif ()

#-----------------------------------------------------------------------
# Modules
#-----------------------------------------------------------------------

if (BUILD_RADIANT)
    add_subdirectory(plugins)
endif ()

#-----------------------------------------------------------------------
# Radiant
#-----------------------------------------------------------------------

if (BUILD_BINARIES)
    if (CMAKE_EXECUTABLE_SUFFIX)
        string(REGEX REPLACE "^[.]" "" RADIANT_EXECUTABLE ${CMAKE_EXECUTABLE_SUFFIX})
    endif ()

    macro (radiant_tool name)
        add_executable(${name} ${ARGN})

        copy_dlls(${name})

        install(
            TARGETS ${name}
            RUNTIME DESTINATION ${RADIANT_BIN_DIR}
        )
    endmacro ()

    if (BUILD_RADIANT)
        add_subdirectory(radiant)
        set_target_properties(${RADIANT_BASENAME} PROPERTIES
        COMPILE_DEFINITIONS RADIANT_EXECUTABLE="${RADIANT_EXECUTABLE}"
    )
    endif ()
endif ()

#-----------------------------------------------------------------------
# Tools
#-----------------------------------------------------------------------

if (BUILD_TOOLS)
    add_subdirectory(tools)

    file(GLOB DATA_FILES "${PROJECT_SOURCE_DIR}/setup/data/tools/*")

    if (NOT (PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR))
        # Copy data files from sources to the build directory
        message(STATUS "Copying data files")
        file(COPY ${DATA_FILES} DESTINATION "${PROJECT_BINARY_DIR}")
    endif ()
endif ()

#-----------------------------------------------------------------------
# Game packs
#-----------------------------------------------------------------------

set(GAMEPACKS_LICENSE_LIST free CACHE STRING "Download game packs by license")
set(GAMEPACKS_NAME_LIST none CACHE STRING "Download game packs by name")

if (DOWNLOAD_GAMEPACKS)
    add_custom_target(game_packs ALL
       COMMAND "${PROJECT_SOURCE_DIR}/gamepack-manager" --license ${GAMEPACKS_LICENSE_LIST} --name ${GAMEPACKS_NAME_LIST} --download-dir "${PROJECT_BINARY_DIR}/download" --install-dir "${PROJECT_BINARY_DIR}" --download --install
       COMMENT "Downloading game packs"
    )
endif ()

#-----------------------------------------------------------------------
# Install
#-----------------------------------------------------------------------

if (BUILD_BINARIES)
    install(DIRECTORY
        setup/data/tools/base
        setup/data/tools/bitmaps
        setup/data/tools/gl
        setup/data/tools/plugins
        docs
        DESTINATION ${RADIANT_DATA_DIR}
    )
endif ()

if (DOWNLOAD_GAMEPACKS)
    if (FHS_INSTALL)
       set(GAME_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/share/netradiant")
    else ()
       set(GAME_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
    endif ()

    file(GLOB GAME_DIRS ${PROJECT_BINARY_DIR}/*.game)

    install(DIRECTORY
        ${PROJECT_BINARY_DIR}/games
        ${GAME_DIRS}
        DESTINATION ${RADIANT_DATA_DIR}
    )
endif ()

if(BUILD_BINARIES AND FHS_INSTALL AND NOT WIN32 AND NOT APPLE)
    install(FILES
        ${PROJECT_SOURCE_DIR}/icons/netradiant.png
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/pixmaps
    )
    install(FILES
        ${PROJECT_SOURCE_DIR}/icons/netradiant.desktop
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications
    )
    install(FILES
        ${PROJECT_SOURCE_DIR}/icons/mime/map.xml
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/mime/model
    )
endif ()

include(cmake/scripts/package.cmake)
