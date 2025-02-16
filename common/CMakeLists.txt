cmake_minimum_required(VERSION 3.30.5)
project(CommonLib LANGUAGES CXX)

# Enable latest preview standard (C++26)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)



# Include directories for internal headers
include_directories(PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/common/BaseLib"
    "${CMAKE_CURRENT_SOURCE_DIR}/common/NetEngine"
    "${CMAKE_CURRENT_SOURCE_DIR}/common/NetEngine/Packets"
    "${CMAKE_CURRENT_SOURCE_DIR}/common/NetEngine/Protocols"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/mariadb/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/fmt/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/openssl/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/asio/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/rapidjson/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/StackWalker/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/boost/include"
)


file(GLOB_RECURSE COMMON_SRC
    "${CMAKE_SOURCE_DIR}/common/BaseLib/*.cpp"
    "${CMAKE_SOURCE_DIR}/common/NetEngine/*.cpp"
)


target_sources(PRIVATE ${COMMON_SRC})

# Use the same `deps/` directory for Windows & Linux
set(DEPS_DIR "${CMAKE_SOURCE_DIR}/deps")

# Add include directories
include_directories(PUBLIC
    "${DEPS_DIR}/mariadb/include"
    "${DEPS_DIR}/fmt/include"
    "${DEPS_DIR}/openssl/include"
)

# Add library directories
target_link_directories(PUBLIC
    "${DEPS_DIR}/mariadb/lib"
    "${DEPS_DIR}/fmt/lib"
    "${DEPS_DIR}/openssl/lib"
)

# Link against static libraries explicitly
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_options(common PRIVATE --target=x86_64-linux-gnu)

    target_link_libraries(common PUBLIC
        "${DEPS_DIR}/mariadb/lib/libmariadbcpp.a"
        "${DEPS_DIR}/fmt/lib/libfmt.a"
        "${DEPS_DIR}/openssl/lib/libcrypto.a"
    )
else()
    # Compiling for Windows
    target_link_libraries(common PUBLIC
        "${DEPS_DIR}/mariadb/lib/mariadbcpp.lib"
        "${DEPS_DIR}/fmt/lib/fmt.lib"
        "${DEPS_DIR}/openssl/lib/libcrypto.lib"
    )
endif()

# Enable warnings and optimizations
target_compile_options(common PRIVATE
    -Wall -Wextra -Wpedantic -O2 -march=native -std=c++2c
)
