cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
project(NetRadiant C CXX)

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/install" CACHE PATH "..." FORCE)
endif ()

#-----------------------------------------------------------------------
# Version
#-----------------------------------------------------------------------

# CMake 3.0+ would allow this in project()
set(NetRadiant_VERSION_MAJOR 1)
set(NetRadiant_VERSION_MINOR 5)
set(NetRadiant_VERSION_PATCH 0)
set(NetRadiant_VERSION "${NetRadiant_VERSION_MAJOR}.${NetRadiant_VERSION_MINOR}.${NetRadiant_VERSION_PATCH}")

set(RADIANT_ABOUTMSG "Custom build" CACHE STRING "About message")

find_package(Git REQUIRED)
execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(RADIANT_VERSION_STRING "${NetRadiant_VERSION}n")
if (GIT_VERSION)
    set(RADIANT_VERSION_STRING "${RADIANT_VERSION_STRING}-git-${GIT_VERSION}")
endif ()

message(STATUS "Building ${PROJECT_NAME} ${RADIANT_VERSION_STRING} ${RADIANT_ABOUTMSG}")

#-----------------------------------------------------------------------
# Language standard
#-----------------------------------------------------------------------

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

#-----------------------------------------------------------------------
# Flags
#-----------------------------------------------------------------------

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")
macro(addflags_c args)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${args}")
endmacro()
macro(addflags_cxx args)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${args}")
endmacro()
macro(addflags args)
    addflags_c("${args}")
    addflags_cxx("${args}")
endmacro()
addflags("-fno-strict-aliasing")
if (NOT WIN32)
    addflags("-fvisibility=hidden")
endif ()

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
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

#-----------------------------------------------------------------------
# Defs
#-----------------------------------------------------------------------

add_definitions(-DRADIANT_VERSION="${NetRadiant_VERSION}")
add_definitions(-DRADIANT_MAJOR_VERSION="${NetRadiant_VERSION_MAJOR}")
add_definitions(-DRADIANT_MINOR_VERSION="${NetRadiant_VERSION_MINOR}")
add_definitions(-DRADIANT_PATCH_VERSION="${NetRadiant_VERSION_PATCH}")

add_definitions(-DRADIANT_ABOUTMSG="${NetRadiant_ABOUT}")

if (NOT CMAKE_BUILD_TYPE MATCHES Release)
    add_definitions(-D_DEBUG=1)
endif ()

if (APPLE)
    option(XWINDOWS "Build against X11" ON)
    add_definitions(
            -DPOSIX=1
    )
elseif (WIN32)
    add_definitions(
            -DWIN32=1
            -D_WIN32=1
    )
else ()
    set(XWINDOWS ON)
    add_definitions(
            -DPOSIX=1
    )
endif ()

if (XWINDOWS)
    find_package(X11 REQUIRED)
    include_directories(${X11_INCLUDE_DIR})
    add_definitions(-DXWINDOWS=1)
endif ()

include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}/libs")

if (WIN32 AND NOT CMAKE_CROSSCOMPILING)
    set(BUNDLE_LIBRARIES_DEFAULT ON)
else ()
    set(BUNDLE_LIBRARIES_DEFAULT OFF)
endif ()
option(BUNDLE_LIBRARIES "Bundle libraries" ${BUNDLE_LIBRARIES_DEFAULT})

macro(copy_dlls target)
    if (BUNDLE_LIBRARIES)
        add_custom_command(TARGET ${target} POST_BUILD
                COMMAND bash
                ARGS -c "ldd '$<TARGET_FILE:${target}>' | grep -v /c/Windows | awk '{ print $1 }' | while read dll; do cp \"$(which $dll)\" '${PROJECT_BINARY_DIR}'; done"
                VERBATIM
                )
    endif ()
endmacro()

#-----------------------------------------------------------------------
# Libraries
#-----------------------------------------------------------------------

add_subdirectory(libs)

#-----------------------------------------------------------------------
# Radiant
#-----------------------------------------------------------------------

if (CMAKE_EXECUTABLE_SUFFIX)
    string(REGEX REPLACE "^[.]" "" RADIANT_EXECUTABLE ${CMAKE_EXECUTABLE_SUFFIX})
else ()
    execute_process(
            COMMAND uname -m
            OUTPUT_VARIABLE RADIANT_EXECUTABLE
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif ()

macro(radiant_tool name)
    add_executable(${name} ${ARGN})
    install(
            TARGETS ${name}
            RUNTIME DESTINATION .
    )
    if (NOT (CMAKE_EXECUTABLE_SUFFIX STREQUAL ".${RADIANT_EXECUTABLE}"))
        add_custom_command(TARGET ${name} POST_BUILD
                COMMAND ln -f -s "$<TARGET_FILE_NAME:${name}>" "${PROJECT_BINARY_DIR}/${name}.${RADIANT_EXECUTABLE}"
                VERBATIM
                )
        install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
                ${name}${CMAKE_EXECUTABLE_SUFFIX} ${CMAKE_INSTALL_PREFIX}/${name}.${RADIANT_EXECUTABLE})
                ")
    endif ()
endmacro()

#-----------------------------------------------------------------------
# Tools
#-----------------------------------------------------------------------

add_subdirectory(tools)

#-----------------------------------------------------------------------
# Install
#-----------------------------------------------------------------------

install(
        FILES
        "${PROJECT_BINARY_DIR}/RADIANT_MAJOR"
        "${PROJECT_BINARY_DIR}/RADIANT_MINOR"
        "${PROJECT_BINARY_DIR}/RADIANT_PATCH"
        DESTINATION .
)

include(cmake/scripts/package.cmake)
