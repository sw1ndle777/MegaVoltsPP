if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND UNIX AND NOT WIN32)
    message(STATUS "Using custom Clang flags for Linux")

    set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -fvisibility=hidden -march=native -fPIC")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -fvisibility=hidden -fvisibility-inlines-hidden -march=native -fPIC -stdlib=libstdc++")

    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-fuse-ld=lld -flto=full")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")

    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
endif()
