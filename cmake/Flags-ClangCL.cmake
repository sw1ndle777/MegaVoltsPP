# Detect MSVC Clang (clang-cl.exe)
if (MSVC AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Applying flags for clang-cl (MSVC's Clang)")

    # Static CRT runtime (/MT or /MTd)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    # Compile options: same as MSVC, accepted by clang-cl
    add_compile_options(
        /O2       # Optimize for speed
        /Ot       # Favor fast code
        /Oi       # Enable intrinsics
        /GL       # Whole program optimization (for LTCG)
        /MP       # Multi-processor compilation
        /bigobj   # Large object support
        /utf-8    # Source is UTF-8
        /wd4661   # Suppress template warning
    )

    # Linker options
    add_link_options(
        /LTCG                          # Link Time Code Generation
        $<$<CONFIG:Release>:/OPT:REF> # Remove unused functions/data
        $<$<CONFIG:Release>:/OPT:ICF> # Fold identical COMDATs
        $<$<CONFIG:Release>:/INCREMENTAL:NO>
        $<$<CONFIG:Release>:/DEBUG:FULL> # PDB generation in release
    )

    # Interprocedural Optimization (enables /GL + /LTCG correctly)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
endif()
