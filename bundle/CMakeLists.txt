if (WIN32)
    set(BUNDLE_OS_NAME "windows")
	set(BUNDLE_SHARE ON)
elseif ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    set(BUNDLE_OS_NAME "linux")
else ()
    set(BUNDLE_OS_NAME "unsupported")
endif ()

add_custom_target(bundle ALL
    COMMAND
    export LD_LIBRARY_PATH=$ENV{LD_LIBRARY_PATH} &&
    "${PROJECT_SOURCE_DIR}/library-bundler"
    "${BUNDLE_OS_NAME}" "${PROJECT_BINARY_DIR}"
    VERBATIM
    COMMENT "Bundling libraries"
)

install(DIRECTORY
    ${PROJECT_BINARY_DIR}/lib
    DESTINATION ${CMAKE_INSTALL_PREFIX}
)

if (BUNDLE_SHARE)
    install(DIRECTORY
        ${PROJECT_BINARY_DIR}/share
        DESTINATION ${CMAKE_INSTALL_PREFIX}
    )
endif ()
