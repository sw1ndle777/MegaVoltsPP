cmake_minimum_required(VERSION 3.16)
project(front_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Detect platform and set vcpkg paths
if (WIN32)
    set(VCPKG_DIR "${CMAKE_SOURCE_DIR}/../../deps/libs/windows")
else()
    set(VCPKG_DIR "${CMAKE_SOURCE_DIR}/../../deps/libs/linux")
endif()

# Use vcpkg toolchain
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake")

# Create the executable
add_executable(front_server
    CFrontServer.cpp
    main.cpp
)

# Include directories
target_include_directories(front_server PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/header
)

# 🔹 FIND PACKAGES CROSS-PLATFORM 🔹
find_package(fmt CONFIG REQUIRED
    PATHS "${VCPKG_DIR}/installed/x64-windows" "${VCPKG_DIR}/installed/x64-linux"
)

# Link `_common` and fmt
target_link_libraries(front_server PRIVATE common fmt::fmt)
