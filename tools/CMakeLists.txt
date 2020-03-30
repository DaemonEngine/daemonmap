set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")

if (NOT TARGET tools)
    add_custom_target(tools)
endif()

if (NOT TARGET unvanquished)
    add_custom_target(unvanquished)
endif()

add_subdirectory(quake3 unvanquished)

add_dependencies(tools unvanquished)
