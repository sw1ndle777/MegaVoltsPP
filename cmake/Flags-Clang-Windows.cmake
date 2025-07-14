if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND WIN32)
    message(STATUS "Using custom Clang flags for Windows")

    set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -fvisibility=hidden -finput-charset=utf-8 -fexec-charset=utf-8 -Wno-unused-variable")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -fvisibility=hidden -fvisibility-inlines-hidden -finput-charset=utf-8 -fexec-charset=utf-8 -Wno-unused-variable")

    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-fuse-ld=lld -flto=full -Wl,/OPT:REF -Wl,/OPT:ICF")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")

    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

    unset(CMAKE_MSVC_RUNTIME_LIBRARY CACHE)
endif()
