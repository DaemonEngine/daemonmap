include_directories(BEFORE common)

set(Q3MAP_VERSION 2.5.17n)
find_package(Git REQUIRED)
execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
if (GIT_VERSION)
    set(Q3MAP_VERSION "${Q3MAP_VERSION}-git-${GIT_VERSION}")
endif ()
add_definitions(-DQ3MAP_VERSION="${Q3MAP_VERSION}")

find_package(GLIB REQUIRED)
include_directories(${GLIB_INCLUDE_DIRS})

find_package(JPEG REQUIRED)
include_directories(${JPEG_INCLUDE_DIR})

find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIR})

find_package(LibXml2 REQUIRED)
include_directories(${LIBXML2_INCLUDE_DIR})

find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})

find_package(Minizip REQUIRED)
include_directories(${Minizip_INCLUDE_DIRS})

include_directories(
        ../../libs/botlib
        ../../libs/recastnavigation/Recast/Include
        ../../libs/recastnavigation/RecastDemo/Include
        ../../libs/recastnavigation/Detour/Include
        ../../libs/recastnavigation/DetourTileCache/Include
        )

set(q3map2_games
        q3map2/game_quake3.h
        q3map2/game_unvanquished.h
        q3map2/game_xonotic.h
        )

radiant_tool(daemonmap
        common/cmdlib.c common/cmdlib.h
        common/inout.c common/inout.h
        common/mutex.c common/mutex.h
        common/polylib.c common/polylib.h
        common/qfiles.h
        common/qthreads.h
        common/scriplib.c common/scriplib.h
        common/threads.c
        common/vfs.c common/vfs.h

        q3map2/brush.c
        q3map2/bspfile_abstract.c
        q3map2/bspfile_ibsp.c
        ${q3map2_games} q3map2/game__null.h
        q3map2/help.c
        q3map2/main.c
        q3map2/mesh.c
        q3map2/nav.cpp
        q3map2/path_init.c
        q3map2/q3map2.h
        q3map2/shaders.c
        )

target_link_libraries(daemonmap
        ${GLIB_LIBRARIES}
        ${LIBXML2_LIBRARIES}
        ${Minizip_LIBRARIES}
        ${ZLIB_LIBRARIES}
        detour
        fastlz
        filematch
        l_net
        mathlib
        recast
        )

add_custom_target(quake3)
add_dependencies(quake3 daemonmap)

if (UNIX)
    target_link_libraries(daemonmap pthread m)
endif ()
