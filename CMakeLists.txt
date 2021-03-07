cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(DaemonMap C CXX)

#-----------------------------------------------------------------------
# CMake modules
#-----------------------------------------------------------------------

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

#-----------------------------------------------------------------------
# Build options
#-----------------------------------------------------------------------

option(BUILD_TOOLS "Build the tools" ON)
option(USE_WERROR "Build with -Werror -pedantic-errors" OFF)
option(FHS_INSTALL "Install according to Filesystem Hierarchy Standard" OFF)

set(BUILD_BINARIES OFF CACHE INTERNAL "...")
if (BUILD_RADIANT OR BUILD_TOOLS)
    set(BUILD_BINARIES ON)
endif ()

if (BUILD_BINARIES)
    if (WIN32 OR APPLE
        OR "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux"
        OR "${CMAKE_SYSTEM_NAME}" STREQUAL "FreeBSD" )
        set(BUNDLING_SUPPORTED ON)
    endif()

    if (BUNDLING_SUPPORTED)
        if(WIN32 AND NOT CMAKE_CROSSCOMPILING)
            set(BUNDLE_LIBRARIES_DEFAULT ON)
        else ()
            set(BUNDLE_LIBRARIES_DEFAULT OFF)
        endif ()

        option(BUNDLE_LIBRARIES "Bundle libraries" ${BUNDLE_LIBRARIES_DEFAULT})
    endif ()

    if (BUNDLE_LIBRARIES AND BUNDLING_SUPPORTED)
        set(FHS_INSTALL OFF CACHE PATH "Disabled because of BUNDLE_LIBRARIES" FORCE)
    endif ()
endif ()

#-----------------------------------------------------------------------
# Paths
#-----------------------------------------------------------------------

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/install" CACHE PATH "..." FORCE)
endif ()

set(FINAL_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})

set(RADIANT_NAME "NetRadiant" CACHE INTERNAL "...")
add_definitions(-DRADIANT_NAME="${RADIANT_NAME}")


set(RADIANT_BIN_DIR ${FINAL_INSTALL_PREFIX} CACHE INTERNAL "...")
set(RADIANT_LIB_DIR ${FINAL_INSTALL_PREFIX}/lib CACHE INTERNAL "...")

if (FHS_INSTALL AND NOT BUNDLE_LIBRARIES)
    set(RADIANT_BIN_DIR ${FINAL_INSTALL_PREFIX}/bin)
    set(RADIANT_LIB_DIR ${FINAL_INSTALL_PREFIX}/lib)
endif ()

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

    if (APPLE)
        add_definitions(-DGL_SILENCE_DEPRECATION=1)
    endif()

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
    add_custom_target(binaries ALL)

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

    macro (copy_dlls target)
        if (BUNDLE_LIBRARIES AND BUNDLING_SUPPORTED)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND "${PROJECT_SOURCE_DIR}/library-bundler"
                "register" "${PROJECT_BINARY_DIR}" "$<TARGET_FILE:${target}>"
                VERBATIM
            )
            add_dependencies(bundle ${target})
        endif ()
    endmacro ()
endif ()

#-----------------------------------------------------------------------
# Bundle
#-----------------------------------------------------------------------

if (BUILD_BINARIES)
    if (BUNDLE_LIBRARIES AND BUNDLING_SUPPORTED)
        add_subdirectory(bundle)
    endif ()
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
    macro (radiant_tool name)
        add_dependencies(binaries ${name})

        if (WIN32)
            add_executable(${name} ${ARGN} ${PROJECT_SOURCE_DIR}/include/lib.rc)
        else ()
            add_executable(${name} ${ARGN})
        endif()

        copy_dlls(${name})

        install(
            TARGETS ${name}
            RUNTIME DESTINATION ${RADIANT_BIN_DIR}
        )
    endmacro ()
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
# Install
#-----------------------------------------------------------------------

install(
        FILES
        DESTINATION .
    )

include(cmake/scripts/package.cmake)
