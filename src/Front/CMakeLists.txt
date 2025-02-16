cmake_minimum_required(VERSION 3.30.5)
project(FrontServer LANGUAGES CXX)

# Enable latest preview standard (C++26)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define `FrontServer` executable
add_executable(FrontServer)

# Include directories
target_include_directories(FrontServer PUBLIC
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

target_sources(FrontServer PRIVATE ${FRONT_SRC})

# If cross-compiling for Linux, add target
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_options(FrontServer PRIVATE --target=x86_64-linux-gnu)
endif()

# Link against `_common` (which already includes fmt, OpenSSL, and MariaDB)
target_link_libraries(FrontServer PRIVATE common)

# Enable warnings and optimizations
target_compile_options(FrontServer PRIVATE
    -Wall -Wextra -Wpedantic -O2 -march=native -std=c++2c
)
