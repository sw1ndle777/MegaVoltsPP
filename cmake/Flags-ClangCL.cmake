# Detect MSVC Clang (clang-cl.exe)
if (MSVC AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Applying flags for clang-cl (MSVC's Clang)")

    # Static CRT runtime (/MT or /MTd)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    # Keep Debug compatible with runtime checks and sanitizer-like debug flows.
    # Avoid injecting /O2 into Debug, which conflicts with /RTC1.
    set(MVPP_NON_DEBUG "$<NOT:$<CONFIG:Debug>>")

    # Compile options: same as MSVC, accepted by clang-cl
    add_compile_options(
        "$<${MVPP_NON_DEBUG}:/O2>"      # Optimize for speed (non-Debug)
        "$<${MVPP_NON_DEBUG}:/Ot>"      # Favor fast code (non-Debug)
        "$<${MVPP_NON_DEBUG}:/Oi>"      # Enable intrinsics (non-Debug)
        "$<${MVPP_NON_DEBUG}:/GL>"      # Whole program optimization (non-Debug)
        /MP       # Multi-processor compilation
        /bigobj   # Large object support
        /utf-8    # Source is UTF-8
        /wd4661   # Suppress template warning
    )

    # Linker options
    add_link_options(
        "$<${MVPP_NON_DEBUG}:/LTCG>"           # Link Time Code Generation
        "$<${MVPP_NON_DEBUG}:/OPT:REF>"        # Remove unused functions/data
        "$<${MVPP_NON_DEBUG}:/OPT:ICF>"        # Fold identical COMDATs
        "$<${MVPP_NON_DEBUG}:/INCREMENTAL:NO>"
        "$<${MVPP_NON_DEBUG}:/DEBUG:FULL>"     # PDB generation in non-Debug configs
    )

    # Interprocedural Optimization (enables /GL + /LTCG correctly)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL ON)
endif()
