cmake_minimum_required(VERSION 3.30.5)
project(common)

add_library(common STATIC
	${CMAKE_SOURCE_DIR}/deps/StackWalker/StackWalker.cpp
    BaseLib/CCrashHandler.cpp
    BaseLib/CDatabase.cpp
    BaseLib/CDBM.cpp
    BaseLib/CLog.cpp
    BaseLib/CSettings.cpp
    BaseLib/Utility.cpp
    NetEngine/CMessage.cpp
    NetEngine/CServer.cpp
    NetEngine/CSession.cpp
)

# Define RELEASE_1_0_3 for this target
target_compile_definitions(common PRIVATE RELEASE_1_0_3)

target_include_directories(common PUBLIC
    "${CMAKE_SOURCE_DIR}/deps/mariadb/include"
    "${CMAKE_SOURCE_DIR}/deps/fmt/include"
    "${CMAKE_SOURCE_DIR}/deps/openssl/include"
    "${CMAKE_SOURCE_DIR}/deps/asio/include"
    "${CMAKE_SOURCE_DIR}/deps/StackWalker"
    "${CMAKE_SOURCE_DIR}/deps/unordered"
	"${CMAKE_SOURCE_DIR}/deps"
	"${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/BaseLib"
    "${CMAKE_CURRENT_SOURCE_DIR}/NetEngine"
)

set_target_properties(common PROPERTIES
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_options(common PRIVATE --target=x86_64-linux-gnu)
    target_link_libraries(common PRIVATE
        "${CMAKE_SOURCE_DIR}/deps/mariadb/lib/libmariadbcpp.a"
        "${CMAKE_SOURCE_DIR}/deps/fmt/lib/libfmt.a"
        "${CMAKE_SOURCE_DIR}/deps/openssl/lib/libcrypto.a"
    )
else()
    target_link_libraries(common PRIVATE
        "${CMAKE_SOURCE_DIR}/deps/mariadb/lib/mariadbcpp.lib"
        "${CMAKE_SOURCE_DIR}/deps/fmt/lib/fmt.lib"
        "${CMAKE_SOURCE_DIR}/deps/openssl/lib/libcrypto.lib"
    )
endif()

target_compile_options(common PRIVATE
    -Wall -Wextra -Wpedantic -O2 -march=native -std=c++2c -finput-charset=UTF-8 -fexec-charset=UTF-8
)
