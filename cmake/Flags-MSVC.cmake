if (MSVC)
    message(STATUS "Using custom MSVC flags")

    # Keep Debug compatible with MSVC runtime checks (/RTC1).
    # D8016 happens when /O2 (or other optimizations) are injected into Debug.
    # Apply optimization/LTCG only for non-Debug configs.
    set(MVPP_NON_DEBUG "$<NOT:$<CONFIG:Debug>>")

    # Compile-time options
    add_compile_options(
        "$<${MVPP_NON_DEBUG}:/O2>"    # Full optimization (non-Debug)
        "$<${MVPP_NON_DEBUG}:/Ot>"    # Favor fast code (non-Debug)
        "$<${MVPP_NON_DEBUG}:/Oi>"    # Enable intrinsic functions (non-Debug)
        "$<${MVPP_NON_DEBUG}:/GL>"    # Whole program optimization (non-Debug)
        /MP                             # Multi-processor build
        /bigobj                         # Support large object files
        /wd4661                         # Suppress C4661 warning
        /utf-8                          # Source code is UTF-8
    )

    # Link-time options
    add_link_options(
        "$<${MVPP_NON_DEBUG}:/LTCG>"           # Link Time Code Generation
        "$<${MVPP_NON_DEBUG}:/OPT:REF>"        # Eliminate unreferenced functions/data
        "$<${MVPP_NON_DEBUG}:/OPT:ICF>"        # Fold identical COMDATs
        "$<${MVPP_NON_DEBUG}:/INCREMENTAL:NO>" # Disable incremental linking
        "$<${MVPP_NON_DEBUG}:/DEBUG:FULL>"     # Generate PDBs in non-Debug configs
    )

    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL ON)
endif()
