set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_BUILD_TYPE release)

if(PORT STREQUAL "mariadb-connector-cpp")
    set(VCPKG_LIBRARY_LINKAGE dynamic)
    set(VCPKG_CRT_LINKAGE     dynamic)
endif()