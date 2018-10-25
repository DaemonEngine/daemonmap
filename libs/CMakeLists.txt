add_subdirectory(detour)
add_subdirectory(fastlz)
add_subdirectory(filematch)
add_subdirectory(l_net)
add_subdirectory(mathlib)
add_subdirectory(recast)

add_library(libs
        _.cpp
        bytebool.h
        globaldefs.h
        )

target_include_directories(libs PRIVATE ${GLIB_INCLUDE_DIRS})
target_link_libraries(libs PRIVATE ${GLIB_LIBRARIES})
