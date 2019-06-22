cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
project(DaemonMap C CXX)

option(BUILD_TOOLS "Build the tools" ON)
option(USE_WERROR "Build with -Werror -pedantic-errors" OFF)
option(FHS_INSTALL "Install according to Filesystem Hierarchy Standard" OFF)

set(BUILD_BINARIES OFF CACHE INTERNAL "...")
if (BUILD_RADIANT OR BUILD_TOOLS)
    set(BUILD_BINARIES ON)
endif ()

#-----------------------------------------------------------------------
# Paths
#-----------------------------------------------------------------------

set(RADIANT_NAME "NetRadiant" CACHE INTERNAL "...")
add_definitions(-DRADIANT_NAME="${RADIANT_NAME}")

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/install" CACHE PATH "..." FORCE)
endif ()

set(RADIANT_LIB_DIR ${CMAKE_INSTALL_PREFIX} CACHE INTERNAL "...")
set(RADIANT_BIN_DIR ${CMAKE_INSTALL_PREFIX} CACHE INTERNAL "...")

if (FHS_INSTALL)
    set(RADIANT_LIB_DIR ${CMAKE_INSTALL_PREFIX}/lib/${RADIANT_BASENAME})
    set(RADIANT_BIN_DIR ${CMAKE_INSTALL_PREFIX}/bin)
endif ()

set(GAMEPACKS_DOWNLOAD_DIR ${PROJECT_BINARY_DIR}/download CACHE PATH "Where to store downloaded game packs")

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
        file(GLOB DLL_FILES ${PROJECT_BINARY_DIR}/*.dll)

        install(FILES
            ${DLL_FILES}
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

		bundle_stuff(${name})

        install(
            TARGETS ${name}
            RUNTIME DESTINATION ${RADIANT_BIN_DIR}
        )
    endmacro()
endif ()

#-----------------------------------------------------------------------
# Tools
#-----------------------------------------------------------------------

if (BUILD_TOOLS)
    add_subdirectory(tools)
endif ()

#-----------------------------------------------------------------------
# Install
#-----------------------------------------------------------------------

install(
        FILES
        DESTINATION .
)

include(cmake/scripts/package.cmake)
