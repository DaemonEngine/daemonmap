set(CPACK_PACKAGE_NAME "NetRadiant")
set(CPACK_PACKAGE_VERSION_MAJOR "${NetRadiant_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${NetRadiant_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${NetRadiant_VERSION_PATCH}")

# binary: --target package
set(CPACK_GENERATOR "ZIP")
set(CPACK_STRIP_FILES 1)

# source: --target package_source
set(CPACK_SOURCE_GENERATOR "ZIP")
set(CPACK_SOURCE_IGNORE_FILES "/\\\\.git/;/build/;/install/")

# configure
include(InstallRequiredSystemLibraries)
include(CPack)
