set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")

add_subdirectory(quake3)

add_custom_target(tools)
add_dependencies(tools quake3)
