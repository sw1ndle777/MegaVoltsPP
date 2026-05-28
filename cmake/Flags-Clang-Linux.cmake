if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND UNIX AND NOT WIN32)
    message(STATUS "Using custom Clang flags for Linux")

    # Keep release performance while preserving crash-symbolization quality.
    # -g                     : emit DWARF debug symbols on Linux (no PDB on Linux)
    # -fno-omit-frame-pointer: improves stack unwinding reliability in crash dumps
    set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -g -fno-omit-frame-pointer -fvisibility=hidden ${MVPP_MARCH_FLAG} -fPIC")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -g -fno-omit-frame-pointer -fvisibility=hidden -fvisibility-inlines-hidden ${MVPP_MARCH_FLAG} -fPIC -stdlib=libstdc++ -fdeclspec")

    # Build-ID helps symbol server / postmortem tooling map dumps to exact binaries.
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-fuse-ld=lld -flto=full -Wl,--build-id")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")

    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
endif()
