cmake_minimum_required(VERSION 3.30.5)
project(MainServer LANGUAGES CXX)

# Enable latest preview standard (C++26)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define `MainServer` executable
add_executable(MainServer)

# Define RELEASE_1_0_3 for this target
target_compile_definitions(MainServer PRIVATE RELEASE_1_0_3)

# Include directories
target_include_directories(MainServer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/handlers
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/common/BaseLib
    ${CMAKE_SOURCE_DIR}/common/NetEngine
)

# Collect all source files
file(GLOB_RECURSE FRONT_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/handlers/*.cpp"
)

target_sources(MainServer PRIVATE ${FRONT_SRC})

# If cross-compiling for Linux, add target
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_options(MainServer PRIVATE --target=x86_64-linux-gnu)
endif()

# Link against `_common` (which already includes fmt, OpenSSL, and MariaDB)
target_link_libraries(MainServer PRIVATE common)

# Enable warnings and optimizations
target_compile_options(MainServer PRIVATE
    -Wall -Wextra -Wpedantic -O2 -march=native -std=c++2c -finput-charset=UTF-8 -fexec-charset=UTF-8
)
