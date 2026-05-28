if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "Using custom GCC flags for Linux")

    set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -g -fno-omit-frame-pointer -fvisibility=hidden ${MVPP_MARCH_FLAG} -fPIC")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -g -fno-omit-frame-pointer -fvisibility=hidden -fvisibility-inlines-hidden ${MVPP_MARCH_FLAG} -fPIC")

    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-flto -Wl,--as-needed -Wl,-O1 -Wl,--build-id")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")

    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
endif()
