if (WIN32)
    set(BUNDLE_OS_NAME "windows")
elseif ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    set(BUNDLE_OS_NAME "linux")
else ()
    set(BUNDLE_OS_NAME "unsupported")
endif ()

add_custom_target(bundle ALL
    COMMAND "${PROJECT_SOURCE_DIR}/library-bundler"
    "${BUNDLE_OS_NAME}" "${PROJECT_BINARY_DIR}"
    VERBATIM
    COMMENT "Bundling libraries"
)

install(DIRECTORY
    ${PROJECT_BINARY_DIR}/lib
    ${PROJECT_BINARY_DIR}/share
    DESTINATION ${CMAKE_INSTALL_PREFIX}
)
