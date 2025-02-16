cmake_minimum_required(VERSION 3.30.5)
project(MegaVoltsPP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(common)
add_subdirectory(src/Front)
add_subdirectory(src/Cast)
add_subdirectory(src/Main)
