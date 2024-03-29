# Already defined by NetRadiant, when built as submodule
if (NOT TARGET libs)
add_subdirectory(filematch)
add_subdirectory(l_net)
add_subdirectory(mathlib)

add_library(libs
        _.cpp
        bytebool.h
        globaldefs.h
        )

target_include_directories(libs PRIVATE ${GLIB_INCLUDE_DIRS})
target_link_libraries(libs PRIVATE ${GLIB_LIBRARIES})
endif()

add_subdirectory(fastlz)

add_library(cm STATIC
        cm/cm_patch.cpp
        cm/cm_patch.h
        )

add_library(detour STATIC
        recastnavigation/DebugUtils/Source/DebugDraw.cpp
        recastnavigation/DebugUtils/Include/DebugDraw.h

        recastnavigation/Detour/Source/DetourAlloc.cpp
        recastnavigation/Detour/Include/DetourAlloc.h
        recastnavigation/Detour/Source/DetourAssert.cpp
        recastnavigation/Detour/Include/DetourAssert.h
        recastnavigation/Detour/Source/DetourCommon.cpp
        recastnavigation/Detour/Include/DetourCommon.h
        recastnavigation/Detour/Include/DetourMath.h
        recastnavigation/Detour/Source/DetourNavMeshBuilder.cpp
        recastnavigation/Detour/Include/DetourNavMeshBuilder.h
        recastnavigation/Detour/Source/DetourNavMesh.cpp
        recastnavigation/Detour/Include/DetourNavMesh.h
        recastnavigation/Detour/Source/DetourNavMeshQuery.cpp
        recastnavigation/Detour/Include/DetourNavMeshQuery.h
        recastnavigation/Detour/Source/DetourNode.cpp
        recastnavigation/Detour/Include/DetourNode.h
        recastnavigation/Detour/Include/DetourStatus.h

        recastnavigation/DetourCrowd/Source/DetourPathCorridor.cpp
        recastnavigation/DetourCrowd/Include/DetourPathCorridor.h

        recastnavigation/DetourTileCache/Source/DetourTileCacheBuilder.cpp
        recastnavigation/DetourTileCache/Include/DetourTileCacheBuilder.h
        recastnavigation/DetourTileCache/Source/DetourTileCache.cpp
        recastnavigation/DetourTileCache/Include/DetourTileCache.h
        )

include_directories(detour PRIVATE
        recastnavigation/DebugUtils/Include
        recastnavigation/Detour/Include
        recastnavigation/DetourCrowd/Include
        recastnavigation/DetourTileCache/Include
        )

add_library(recast STATIC
        recastnavigation/Recast/Source/RecastAlloc.cpp
        recastnavigation/Recast/Include/RecastAlloc.h
        recastnavigation/Recast/Source/RecastArea.cpp
        recastnavigation/Recast/Source/RecastAssert.cpp
        recastnavigation/Recast/Include/RecastAssert.h
        recastnavigation/Recast/Source/RecastContour.cpp
        recastnavigation/Recast/Source/Recast.cpp
        recastnavigation/Recast/Include/Recast.h
        recastnavigation/Recast/Source/RecastFilter.cpp
        recastnavigation/Recast/Source/RecastLayers.cpp
        recastnavigation/Recast/Source/RecastMesh.cpp
        recastnavigation/Recast/Source/RecastMeshDetail.cpp
        recastnavigation/Recast/Source/RecastRasterization.cpp
        recastnavigation/Recast/Source/RecastRegion.cpp

        recastnavigation/RecastDemo/Source/ChunkyTriMesh.cpp
        recastnavigation/RecastDemo/Include/ChunkyTriMesh.h
        )

target_include_directories(recast PRIVATE
        recastnavigation/Recast/Include
        recastnavigation/RecastDemo/Include
        )
