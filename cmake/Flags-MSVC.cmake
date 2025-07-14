if (MSVC)
    message(STATUS "Using custom MSVC flags")

    # Compile-time optimizations
    add_compile_options(
        /O2     # Full optimization
        /Ot     # Favor fast code
        /Oi     # Enable intrinsic functions
        /GL     # Whole program optimization (for LTCG)
        /MP     # Multi-processor build
        /bigobj # Support large object files
        /wd4661 # Suppress C4661 warning
        /utf-8  # Source code is UTF-8
    )

    # Link-time optimizations
    add_link_options(
        /LTCG                          # Link Time Code Generation
        $<$<CONFIG:Release>:/OPT:REF> # Eliminate unreferenced functions/data
        $<$<CONFIG:Release>:/OPT:ICF> # Fold identical COMDATs
        $<$<CONFIG:Release>:/INCREMENTAL:NO> # Disable incremental linking
        $<$<CONFIG:Release>:/DEBUG:FULL>     # Generate PDBs in Release
    )
endif()
