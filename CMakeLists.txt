#
# CMakeLists.txt
#
# cmake required (http://www.cmake.org/)
#

cmake_minimum_required( VERSION 2.8 )
if( POLICY CMP0017 )
  cmake_policy( SET CMP0017 NEW )
endif()

include( CMakeDependentOption )

# This line has to appear before 'PROJECT' in order to be able to disable incremental linking
set( MSVC_INCREMENTAL_DEFAULT ON )

set( CMAKE_BUILD_TYPE Release CACHE STRING "Build configuration (Debug, Release, RelWithDebInfo, MinSizeRel)" )

# convenience option for source distros
option( MINIMAL_FLAGS "try not to mess up system flags" 0)

project( Daemon C CXX ASM )

set( X86 0 )
if( WIN32 OR MSVC OR CMAKE_SYSTEM_PROCESSOR MATCHES "i.86" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )
  set( X86 1 )
endif()

set( MOUNT_DIR ${CMAKE_SOURCE_DIR}/src )
set( OBJ_DIR ${CMAKE_BINARY_DIR}/CMakeFiles )

# Check if this is first run and if so set some default flags
if( NOT DEFINED SUBSEQUENT_RUN )
  set( SUBSEQUENT_RUN 1 CACHE INTERNAL "True if not first run" )

  if( MSVC )
    set( CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /arch:SSE /fp:fast /Oy" CACHE STRING "Flags used by the compiler during release builds" FORCE )
	set( CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} /arch:SSE /fp:fast /d2Zi+" CACHE STRING "Flags used by the compiler during release builds with debug info" FORCE )
  else()
    if(NOT MINIMAL_FLAGS)
      # "gcc -msse2" fails on non-x86
      if( X86 )
        set( CMAKE_USE_SSE2 "-msse2" )
      else()
        set( CMAKE_USE_SSE2 "" )
      endif()

      macro( try_compiler_option COMPILER_ID COMPILER_VERSION PROP FLAG_TRUE FLAG_FALSE )
        if( COMPILER_ID STREQUAL "" OR ( CMAKE_C_COMPILER_ID STREQUAL "${COMPILER_ID}" AND NOT ( CMAKE_C_COMPILER_VERSION VERSION_LESS "${COMPILER_VERSION}" ) ) )
          set( SAVED_FLAGS "${CMAKE_C_FLAGS}" )
          set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAG_TRUE}" )
          try_compile( ${PROP}_TEST ${MOUNT_DIR}/tests ${MOUNT_DIR}/tests/test.c
                       CMAKE_FLAGS "-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}" )
          set( CMAKE_C_FLAGS "${SAVED_FLAGS}" )
          if( ${PROP}_TEST )
            message( STATUS "Using flag ${FLAG_TRUE}" )
            set( ${PROP} ${FLAG_TRUE} )
          else()
            set( ${PROP} ${FLAG_FALSE} )
          endif()
        else()
          set( ${PROP} ${FLAG_FALSE} )
        endif()
      endmacro()

      try_compiler_option( GNU   0.0 DEBUG_SYMBOLS_LEVEL -g3 -g  ) # gcc: macro expansion etc. in gdb
      try_compiler_option( GNU   4.8 DEBUG_OPTIMISE      -Og ""  ) # gcc 4.8: optimisation level for debug builds

      set( CMAKE_C_FLAGS "${CMAKE_USE_SSE2} -pipe" CACHE STRING "Flags used by the compiler during all build types" FORCE )
      set( CMAKE_C_FLAGS_DEBUG "${DEBUG_OPTIMISE} ${DEBUG_SYMBOLS_LEVEL}" CACHE STRING "Flags used by the compiler during debug builds." FORCE )
      set( CMAKE_C_FLAGS_MINSIZEREL "-DNDEBUG -Os" CACHE STRING "Flags used by the compiler during release minsize builds." FORCE )
      set( CMAKE_C_FLAGS_RELEASE "-DNDEBUG -O3 -ffast-math -fomit-frame-pointer -fno-strict-aliasing" CACHE STRING "Flags used by the compiler during release builds" FORCE )
      set( CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELEASE} -g" CACHE STRING "Flags used by the compiler during Release with Debug Info builds." FORCE )
    else(NOT MINIMAL_FLAGS)
      set( CMAKE_C_FLAGS_DEBUG "" )
      set( CMAKE_C_FLAGS_RELEASE "-DNDEBUG" )
    endif(NOT MINIMAL_FLAGS)
  endif()

  set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "Flags used by the compiler during all build types" FORCE )
  set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}" CACHE STRING "Flags used by the compiler during debug builds." FORCE )
  set( CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL}" CACHE STRING "Flags used by the compiler during release minsize builds." FORCE )
  set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}" CACHE STRING "Flags used by the compiler during release builds" FORCE )
  set( CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}" CACHE STRING "Flags used by the compiler during Release with Debug Info builds." FORCE )
endif()

set( CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER} )
set( CMAKE_ASM_COMPILE_OBJECT ${CMAKE_C_COMPILE_OBJECT} )
set( CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "" ) # Need this to get rid of -rdynamic linker flag set by default by cmake
set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" )


# Options
option( ENABLE_W_ALL         "Use -Wall"                                   0 )
option( ENABLE_WARNINGS      "Enable security & code-checking warning options" 1 )
option( ENABLE_HARDENING     "Enable security hardening options"           1 )

option( USE_SDL2             "Enable SDL 2.0 support (incompatible with SDL 1.2)"    1 )

option( USE_INTERNAL_JPEG    "Use JPEG files included with Unvanquished"             0 )

cmake_dependent_option( ENABLE_W_EXTRA       "Use -Wextra"                                 0 ENABLE_W_ALL     0 )
cmake_dependent_option( ENABLE_FSTACKPROT    "Enable stack protection if available"        1 ENABLE_HARDENING 0 )
cmake_dependent_option( ENABLE_RELRO         "Enable relro & bindnow if available"         1 ENABLE_HARDENING 0 )

mark_as_advanced( ENABLE_FSTACKPROT ENABLE_RELRO )

set( LIB_DIR ${MOUNT_DIR}/libs ) # LIB_DIR is also defined in Visual_Studio32/64.cmake

if( USE_SDL2 )
	set( SDL_DIR ${LIB_DIR}/libsdl2 )
else()
	set( SDL_DIR ${LIB_DIR}/libsdl1.2 )
endif()

set( JPEG_DIR ${LIB_DIR}/jpeg )

if( WIN32 )
  add_definitions( -D_CRT_SECURE_NO_WARNINGS )
  set( CMAKE_SHARED_LIBRARY_PREFIX "" )
endif()

if( MSVC )
  add_definitions( -D_WINDOWS -DWIN32 )
endif()

# Determine platform
if( WIN32 )
  set( PLATFORM "win32" )
  set( CONSOLE "win32" )
elseif( UNIX )
  if( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
    add_definitions( -DMACOS_X -DMACOS_X_ACCELERATION_HACK -DMACOS_X_CURSOR_HACK )
  elseif( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
    add_definitions( -D_FILE_OFFSET_BITS=64 )
  endif()
  set( PLATFORM "unix" )
endif()

# Determine arch.
if( CMAKE_SYSTEM_NAME STREQUAL "Darwin" )
  # CMAKE_SYSTEM_PROCESSOR always returns i386 for x86 and x86_64 on Mac
  if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    set( BUILD_ARCH "x86_64" )
  elseif( CMAKE_SIZEOF_VOID_P EQUAL 4 AND CMAKE_SYSTEM_PROCESSOR STREQUAL "i386" )
    set( BUILD_ARCH "i386" )
  elseif( CMAKE_SIZEOF_VOID_P EQUAL 4 AND CMAKE_SYSTEM_PROCESSOR STREQUAL "ppc" )
    set( BUILD_ARCH "ppc" )
  else()
    set( BUILD_ARCH "Unknown" )
  endif()
elseif( WIN32 )
  # CMAKE_SYSTEM_PROCESSOR is not reliable under windows
  if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    set( BUILD_ARCH "x86_64" )
  elseif( CMAKE_SIZEOF_VOID_P EQUAL 4 )
    set( BUILD_ARCH "i386" )
  endif()
else()
  # CMAKE_SYSTEM_PROCESSOR is reliable for *nix
  if( CMAKE_SYSTEM_PROCESSOR MATCHES "i.86" )
    set( BUILD_ARCH "i386" )
  elseif( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    set( BUILD_ARCH "x86_64" )
  elseif( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" AND CMAKE_SIZEOF_VOID_P EQUAL 4 )
    set( BUILD_ARCH "x32" )
  else()
    set( BUILD_ARCH ${CMAKE_SYSTEM_PROCESSOR} )
  endif()
endif()

if( EXISTS "${OBJ_DIR}/revision.h" )
  file( READ "${OBJ_DIR}/revision.h" ACTUAL_REVISION_H_CONTENTS )
  if( NOT "${ACTUAL_REVISION_H_CONTENTS}" STREQUAL "${DESIRED_REVISION_H_CONTENTS}" )
    file( WRITE "${OBJ_DIR}/revision.h" "${DESIRED_REVISION_H_CONTENTS}" )
  endif()
else()
  file( WRITE "${OBJ_DIR}/revision.h" "${DESIRED_REVISION_H_CONTENTS}" )
endif()

include_directories( "${OBJ_DIR}" )

# Import CMakeLists.txt for internal libraries
if( USE_INTERNAL_SDL )
	if( USE_SDL2 )
	  set( SDL_STATIC OFF CACHE BOOL "Build SDL static library" )
	  add_subdirectory( ${SDL_DIR} ${OBJ_DIR}/libsdl2 )


	  if( DIRECTX )
		  if( BUILD_ARCH STREQUAL "i386" )
			link_directories($ENV{DXSDK_DIR}\\lib\\x86)
		  elseif( BUILD_ARCH STREQUAL "x86-64" )
			link_directories($ENV{DXSDK_DIR}\\lib\\x64)
		  endif()
	  endif()

	  set( SDL2_LIBRARIES SDL2 SDL2main CACHE STRING "SDL library" FORCE )
	  set( SDL2_INCLUDE_DIRS ${SDL_DIR}/include CACHE STRING "SDL include directory" FORCE )
	  set( SDL2_FOUND "1" CACHE INTERNAL "" )
  else()
	  add_subdirectory( ${SDL_DIR} ${OBJ_DIR}/libsdl )
	  set( SDL_LIBRARY SDL1.2 SDL1.2main CACHE STRING "SDL library" FORCE )
	  set( SDL_INCLUDE_DIR ${SDL_DIR}/include CACHE STRING "SDL include directory" FORCE )
	  set( SDL_FOUND "1" CACHE INTERNAL "" )
  endif()
endif()

if( USE_INTERNAL_JPEG )
  add_subdirectory( ${JPEG_DIR} ${OBJ_DIR}/libjpeg )
  set( JPEG_LIBRARY INTERNAL_JPEG CACHE STRING "JPEG library" FORCE )
  set( JPEG_INCLUDE_DIR ${JPEG_DIR} CACHE STRING "JPEG include directory" FORCE )
endif()

if(NOT MINIMAL_FLAGS)
  #################
  # Compile flags #
  #################

  macro( try_compiler_flag_internal PROP FLAG CC SUFFIX LABEL )
    set( SAVED_FLAGS "${CMAKE_${CC}_FLAGS}" )
    set( CMAKE_${CC}_FLAGS "${CMAKE_${CC}_FLAGS} ${FLAG}" )
    # test it on an executable
    if( NOT ${PROP}_${CC}_CACHED )
      try_compile( ${PROP}_${CC} ${MOUNT_DIR}/tests ${MOUNT_DIR}/tests/test.${SUFFIX}
                   CMAKE_FLAGS "-DCMAKE_${CC}_FLAGS=${CMAKE_${CC}_FLAGS}" )
    endif()
    if( ${PROP}_${CC} )
      if( NOT ${PROP}_${CC}_CACHED )
        message( STATUS "Using ${LABEL} flag ${FLAG}" )
      endif()
    else()
      if( NOT ${PROP}_${CC}_CACHED )
        message( STATUS "${LABEL} flag ${FLAG} is not available" )
      endif()
      set( CMAKE_${CC}_FLAGS "${SAVED_FLAGS}" )
    endif()
    set( ${PROP}_${CC}_CACHED TRUE CACHE INTERNAL "Set if ${LABEL} option ${FLAG} has been checked for." )
  endmacro()

  macro( try_c_flag PROP FLAG )
    try_compiler_flag_internal( ${PROP} ${FLAG} C c C )
  endmacro()

  macro( try_cxx_flag PROP FLAG )
    try_compiler_flag_internal( ${PROP} ${FLAG} CXX cxx C++ )
  endmacro()

  macro( try_c_cxx_flag PROP FLAG )
    try_c_flag( ${PROP} ${FLAG} )
    try_cxx_flag( ${PROP} ${FLAG} )
  endmacro()

  # Various warnings
  if( ENABLE_W_ALL )
    try_c_cxx_flag( HAVE_W_ALL              -Wall )
  endif()
  if( ENABLE_W_EXTRA )
    try_c_cxx_flag( HAVE_W_EXTRA            -Wextra )
  endif()
  if( ENABLE_WARNINGS )
    try_c_cxx_flag( HAVE_W_FORMAT2          -Wformat=2 )
    if( NOT HAVE_W_FORMAT2_C )
      try_c_cxx_flag( HAVE_W_FORMAT         -Wformat )
    endif()
    try_c_flag    ( HAVE_W_0LEN_FMT         -Wno-format-zero-length )
    try_c_cxx_flag( HAVE_W_FORMAT_SECURITY  -Wformat-security )
    try_c_cxx_flag( HAVE_W_STRICT_ALIAS_2   -Wstrict-aliasing=2 )
    try_c_cxx_flag( HAVE_W_MISSING_FMT_ATTR -Wmissing-format-attribute )
    try_c_cxx_flag( HAVE_W_MISSING_NORETURN -Wmissing-noreturn )
    try_c_flag    ( HAVE_W_IMPLICIT_FN      -Wimplicit-function-declaration )
    if( HAVE_W_IMPLICIT_FN_C )
      try_c_cxx_flag( HAVE_W_ERR_IMPLICIT_FN -Werror=implicit-function-declaration )
    endif()
    try_c_cxx_flag( HAVE_W_FMT_SECURITY     -Wformat-security )
    if( HAVE_W_FMT_SECURITY_C )
      try_c_cxx_flag( HAVE_W_ERR_FMT_SECURITY -Werror=format-security )
    endif()
    try_c_flag( HAVE_W_DECL_AFT_STMT        -Wdeclaration-after-statement )
    if( HAVE_W_DECL_AFT_STMT_C )
      try_c_flag( HAVE_W_ERR_DECL_AFT_STMT  -Werror=declaration-after-statement )
    endif()
  endif()

  # Hardening
  if( ENABLE_HARDENING )
    try_c_cxx_flag( HAVE_F_STACK_PROTECTOR -fstack-protector )
  endif()

  ##############
  # Link flags #
  ##############

  macro( try_linker_flag PROP FLAG )
    set( SAVED_FLAGS "${CMAKE_EXE_LINKER_FLAGS}" )
    set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${FLAG}" )
    # test it on an executable
    if( NOT ${PROP}_CACHED )
      try_compile( ${PROP} ${MOUNT_DIR}/tests ${MOUNT_DIR}/tests/test.c
                   CMAKE_FLAGS -DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS} )
    endif()
    if( ${PROP} )
      if( NOT ${PROP}_CACHED )
        message( STATUS "Using linker flag ${FLAG}" )
      endif()
      # assume that it works for shared & static libs too
      set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${FLAG}" )
      set( CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${FLAG}" )
    else()
      if( NOT ${PROP}_CACHED )
        message( STATUS "Linker flag ${FLAG} is not available" )
      endif()
      set( CMAKE_EXE_LINKER_FLAGS "${SAVED_FLAGS}" )
    endif()
    set( ${PROP}_CACHED TRUE CACHE INTERNAL "Set if linker option ${FLAG} has been checked for." )
  endmacro()

  if( NOT MSVC )
    # Can we get rid of unneeded external libraries?
    try_linker_flag( HAVE_AS_NEEDED -Wl,--as-needed )

    # If we can, fail if there are unresolved symbols
    try_linker_flag( HAVE_Z_DEFS -Wl,-z,defs )
  endif()

  if( ENABLE_FSTACKPROT )
    try_linker_flag( HAVE_FSTACKPROT "-fstack-protector --param=ssp-buffer-size=4" )
  endif()
  if( ENABLE_RELRO )
    try_linker_flag( HAVE_Z_RELRO_Z_NOW -Wl,-z,relro,-z,now )
  endif()
endif(NOT MINIMAL_FLAGS)

######################
# Define Build Files #
######################

set( SDLLIST
  ${MOUNT_DIR}/engine/sys/sdl2_compat.c
)

set( DAEMONMAPLIST
  ${MOUNT_DIR}/engine/sys/sdl2_compat.c
  ${MOUNT_DIR}/tools/common/cmdlib.c
  ${MOUNT_DIR}/tools/common/imagelib.c
  ${MOUNT_DIR}/tools/common/inout.c
  ${MOUNT_DIR}/tools/common/mathlib.c
  ${MOUNT_DIR}/tools/common/md4.c
  ${MOUNT_DIR}/tools/common/mutex.c
  ${MOUNT_DIR}/tools/common/polylib.c
  ${MOUNT_DIR}/tools/common/scriplib.c
  ${MOUNT_DIR}/tools/common/threads.c
  ${MOUNT_DIR}/tools/common/unzip.c
  ${MOUNT_DIR}/tools/common/vfs.c
  ${MOUNT_DIR}/tools/common/ddslib.c
  ${MOUNT_DIR}/libs/picomodel/picointernal.c
  ${MOUNT_DIR}/libs/picomodel/picomodel.c
  ${MOUNT_DIR}/libs/picomodel/picomodules.c
  ${MOUNT_DIR}/libs/picomodel/pm_3ds.c
  ${MOUNT_DIR}/libs/picomodel/pm_ase.c
  ${MOUNT_DIR}/libs/picomodel/pm_fm.c
  ${MOUNT_DIR}/libs/picomodel/pm_lwo.c
  ${MOUNT_DIR}/libs/picomodel/pm_md2.c
  ${MOUNT_DIR}/libs/picomodel/pm_md3.c
  ${MOUNT_DIR}/libs/picomodel/pm_mdc.c
  ${MOUNT_DIR}/libs/picomodel/pm_ms3d.c
  ${MOUNT_DIR}/libs/picomodel/pm_obj.c
  ${MOUNT_DIR}/libs/picomodel/pm_terrain.c
  ${MOUNT_DIR}/libs/picomodel/lwo/clip.c
  ${MOUNT_DIR}/libs/picomodel/lwo/envelope.c
  ${MOUNT_DIR}/libs/picomodel/lwo/list.c
  ${MOUNT_DIR}/libs/picomodel/lwo/lwio.c
  ${MOUNT_DIR}/libs/picomodel/lwo/lwo2.c
  ${MOUNT_DIR}/libs/picomodel/lwo/lwob.c
  ${MOUNT_DIR}/libs/picomodel/lwo/pntspols.c
  ${MOUNT_DIR}/libs/picomodel/lwo/surface.c
  ${MOUNT_DIR}/libs/picomodel/lwo/vecmath.c
  ${MOUNT_DIR}/libs/picomodel/lwo/vmap.c
  ${MOUNT_DIR}/tools/owmap/brush.c
  ${MOUNT_DIR}/tools/owmap/fog.c
  ${MOUNT_DIR}/tools/owmap/brush_primit.c
  ${MOUNT_DIR}/tools/owmap/bsp.c
  ${MOUNT_DIR}/tools/owmap/bspfile_abstract.c
  ${MOUNT_DIR}/tools/owmap/bspfile_ibsp.c
  ${MOUNT_DIR}/tools/owmap/bspfile_rbsp.c
  ${MOUNT_DIR}/tools/owmap/bspfile_xbsp.c
  ${MOUNT_DIR}/tools/owmap/convert_ase.c
  ${MOUNT_DIR}/tools/owmap/convert_map.c
  ${MOUNT_DIR}/tools/owmap/decals.c
  ${MOUNT_DIR}/tools/owmap/facebsp.c
  ${MOUNT_DIR}/tools/owmap/gldraw.c
  ${MOUNT_DIR}/tools/owmap/model.c
  ${MOUNT_DIR}/tools/owmap/image.c
  ${MOUNT_DIR}/tools/owmap/leakfile.c
  ${MOUNT_DIR}/tools/owmap/light.c
  ${MOUNT_DIR}/tools/owmap/light_bounce.c
  ${MOUNT_DIR}/tools/owmap/light_trace.c
  ${MOUNT_DIR}/tools/owmap/light_ydnar.c
  ${MOUNT_DIR}/tools/owmap/lightmaps_ydnar.c
  ${MOUNT_DIR}/tools/owmap/main.c
  ${MOUNT_DIR}/tools/owmap/map.c
  ${MOUNT_DIR}/tools/owmap/mesh.c
  ${MOUNT_DIR}/tools/owmap/patch.c
  ${MOUNT_DIR}/tools/owmap/prtfile.c
  ${MOUNT_DIR}/tools/owmap/path_init.c
  ${MOUNT_DIR}/tools/owmap/portals.c
  ${MOUNT_DIR}/tools/owmap/shaders.c
  ${MOUNT_DIR}/tools/owmap/writebsp.c
  ${MOUNT_DIR}/tools/owmap/surface_abstract.c
  ${MOUNT_DIR}/tools/owmap/surface_extra.c
  ${MOUNT_DIR}/tools/owmap/surface_foliage.c
  ${MOUNT_DIR}/tools/owmap/surface_fur.c
  ${MOUNT_DIR}/tools/owmap/surface_meta.c
  ${MOUNT_DIR}/tools/owmap/tjunction.c
  ${MOUNT_DIR}/tools/owmap/tree.c
  ${MOUNT_DIR}/tools/owmap/vis.c
  ${MOUNT_DIR}/tools/owmap/visflow.c
  ${MOUNT_DIR}/tools/owmap/nav.cpp
  ${MOUNT_DIR}/libs/zlib/adler32.c
  ${MOUNT_DIR}/libs/zlib/compress.c
  ${MOUNT_DIR}/libs/zlib/crc32.c
  ${MOUNT_DIR}/libs/zlib/deflate.c
  ${MOUNT_DIR}/libs/zlib/gzio.c
  ${MOUNT_DIR}/libs/zlib/infback.c
  ${MOUNT_DIR}/libs/zlib/inffast.c
  ${MOUNT_DIR}/libs/zlib/inflate.c
  ${MOUNT_DIR}/libs/zlib/inftrees.c
  ${MOUNT_DIR}/libs/zlib/ioapi.c
  ${MOUNT_DIR}/libs/zlib/trees.c
  ${MOUNT_DIR}/libs/zlib/uncompr.c
  ${MOUNT_DIR}/libs/zlib/zutil.c
)

set( FASTLZLIST
	${MOUNT_DIR}/libs/fastlz/fastlz.c
)

set( DETOURLIST
  ${FASTLZLIST}
  ${MOUNT_DIR}/libs/detour/DetourAlloc.cpp
  ${MOUNT_DIR}/libs/detour/DetourCommon.cpp
  ${MOUNT_DIR}/libs/detour/DetourNavMeshBuilder.cpp
  ${MOUNT_DIR}/libs/detour/DetourNavMesh.cpp
  ${MOUNT_DIR}/libs/detour/DetourNavMeshQuery.cpp
  ${MOUNT_DIR}/libs/detour/DetourNode.cpp
  ${MOUNT_DIR}/libs/detour/DetourPathCorridor.cpp
  ${MOUNT_DIR}/libs/detour/DetourDebugDraw.cpp
  ${MOUNT_DIR}/libs/detour/DebugDraw.cpp
  ${MOUNT_DIR}/libs/detour/DetourTileCache.cpp
  ${MOUNT_DIR}/libs/detour/DetourTileCacheBuilder.cpp
)

set( RECASTLIST
  ${MOUNT_DIR}/libs/recast/RecastAlloc.cpp
  ${MOUNT_DIR}/libs/recast/RecastArea.cpp
  ${MOUNT_DIR}/libs/recast/RecastContour.cpp
  ${MOUNT_DIR}/libs/recast/Recast.cpp
  ${MOUNT_DIR}/libs/recast/RecastFilter.cpp
  ${MOUNT_DIR}/libs/recast/RecastLayers.cpp
  ${MOUNT_DIR}/libs/recast/RecastMesh.cpp
  ${MOUNT_DIR}/libs/recast/RecastMeshDetail.cpp
  ${MOUNT_DIR}/libs/recast/RecastRasterization.cpp
  ${MOUNT_DIR}/libs/recast/RecastRegion.cpp
  ${MOUNT_DIR}/libs/recast/ChunkyTriMesh.cpp
)

####################
# End of file list #
####################

if( WIN32 )
  add_definitions( -DWINVER=0x501 )
  if( MSVC )
    set( OS_LIBRARIES winmm ws2_32 psapi )
    if( ${BUILD_ARCH} STREQUAL "x86_64" )
      add_definitions( -D__WIN64__ )
    endif()
  else()
    set( OS_LIBRARIES m winmm ws2_32 psapi z )
  endif()
elseif( UNIX )
  if( CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" )
    set( OS_LIBRARIES m z )
  elseif( CMAKE_SYSTEM_NAME STREQUAL "OpenBSD" )
    set( OS_LIBRARIES m z )
  elseif( CMAKE_SYSTEM_NAME STREQUAL "Darwin" )
    set( OS_LIBRARIES dl m z )
    # FIXME This looks wrong.
    set( OS_LIBRARIES ${OS_LIBRARIES} "-framework AGL -framework OpenGL -framework Carbon -framework IOKit" )
  else()
    set( OS_LIBRARIES ${CMAKE_DL_LIBS} m z rt )
  endif()
endif()

macro( find_and_use_SDL TARGET )
  if( USE_SDL2 )
    if( NOT SDL2_FOUND AND NOT USE_INTERNAL_SDL )
      find_package( SDL2 )
    endif()
    include_directories( ${SDL2_INCLUDE_DIRS} )
    target_link_libraries( ${TARGET} ${SDL2_LIBRARIES} )
  endif()
  # Fall back on 1.2
  if( NOT USE_SDL2 OR NOT SDL2_FOUND )
    if( NOT SDL_FOUND AND NOT USE_INTERNAL_SDL )
      find_package( SDL REQUIRED )
      set( SDL_FOUND "1" CACHE INTERNAL "" )
    endif()
    include_directories( ${SDL_INCLUDE_DIR} )
    target_link_libraries( ${TARGET} ${SDL_LIBRARY} )
  endif()
  if( NOT SDL2_FOUND AND NOT SDL_LIBRARY)
    # shouldn't get here – should fail above if 1.2 isn't found
    message( FATAL_ERROR "Neither SDL 1.2 nor SDL 2 were found" )
  endif()
endmacro()

###################
# Build DaemonMap #
###################
  # Setup DaemonMap target
  add_executable( daemonMap ${DAEMONMAPLIST} ${DETOURLIST} ${RECASTLIST} ${WIN_RC} )
  include_directories( ${LIB_DIR}/picomodel ${MOUNT_DIR}/tools/common )
  target_link_libraries( daemonMap ${OS_LIBRARIES} )
  set_property( TARGET daemonMap APPEND PROPERTY COMPILE_DEFINITIONS USE_OPENGL )
  set_target_properties( daemonMap PROPERTIES OUTPUT_NAME "daemonmap" PREFIX "" )

  # Setup CPack for target daemonmap
  install( TARGETS daemonMap DESTINATION "Unvanquished" )
  get_target_property( OUTPUT_NAME daemonMap OUTPUT_NAME )
  set( APPS ${APPS} Unvanquished/${OUTPUT_NAME} )

  # Visual Studio specific settings
  if( MSVC )
    set_property( TARGET daemonMap APPEND PROPERTY COMPILE_DEFINITIONS ZLIB_WINAPI )
  endif()

  # This section finds and provides REQUIRED dependencies to DaemonMap target
  find_package( OpenGL REQUIRED )
  find_package( JPEG REQUIRED )
  find_package( PNG REQUIRED )
  find_package( GLib2 REQUIRED )
  find_and_use_SDL( daemonMap )

  if( NOT WIN32 )
    target_link_libraries( daemonMap pthread )
  endif()

  target_link_libraries( daemonMap ${OPENGL_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${GLIB_LIBRARIES} )
  include_directories( ${OPENGL_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${PNG_INCLUDE_DIR} ${GLIB_INCLUDE_DIRS} )
