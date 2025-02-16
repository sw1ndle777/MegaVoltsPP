cmake_minimum_required(VERSION 3.30.5)
project(MegaVoltsPP LANGUAGES CXX)


# Enable latest preview standard (C++26)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add the common (shared) library
add_subdirectory(common)

# Add the Front server
add_subdirectory(src/Front)

# Add the Cast server
add_subdirectory(src/Cast)

# Add the Main server
add_subdirectory(src/Main)
