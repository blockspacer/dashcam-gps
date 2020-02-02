function(bindir TARGET)
    set(__ARCH $<IF:$<EQUAL:8,${CMAKE_SIZEOF_VOID_P}>,x64,x86>)
    set(__DIR "${PROJECT_BINARY_DIR}/bin")
    set_target_properties(${TARGET} PROPERTIES
        DEBUG_POSTFIX "_d"
        RELWITHDEBINFO_POSTFIX "_opt"
        MINSIZEREL_POSTFIX "_min"
        RUNTIME_OUTPUT_DIRECTORY "${__DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${__DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${__DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${__DIR}"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${__DIR}"
    )
endfunction(bindir)

function(plugindir TARGET)
    set(__DIR "${PROJECT_BINARY_DIR}/bin/mgps-plugins")
    set_target_properties(${TARGET} PROPERTIES
        DEBUG_POSTFIX "_d"
        RELWITHDEBINFO_POSTFIX "_opt"
        MINSIZEREL_POSTFIX "_min"
        LIBRARY_OUTPUT_DIRECTORY "${__DIR}"
        LIBRARY_OUTPUT_DIRECTORY_DEBUG "${__DIR}"
        LIBRARY_OUTPUT_DIRECTORY_RELEASE "${__DIR}"
        LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${__DIR}"
        LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${__DIR}"
    )
endfunction(plugindir)
