# ---- Shared Wrapper configuration ----

# Registry is consulted last: an explicit PYTHON_INSTALLATION_PATH (passed by
# setup.py / set by the developer) wins; otherwise standard python.org
# installs are discovered via the Windows registry, then PATH.
set(Python3_FIND_REGISTRY "LAST")

if(DEFINED PYTHON_INSTALLATION_PATH)
    message("Found PYTHON_INSTALLATION_PATH in environment variables")
    set(CMAKE_PREFIX_PATH ${PYTHON_INSTALLATION_PATH})
endif()

find_package(Python3 3.11 REQUIRED COMPONENTS Development.SABIModule)

# Helper: set output name (.pyd on Windows, .so elsewhere) and OUTPUT_NAME
function(set_wrapper_output TARGET baseName)
    if(CMAKE_SYSTEM_NAME MATCHES "Windows")
        set_target_properties(${TARGET} PROPERTIES
            SUFFIX ".pyd" PREFIX "" OUTPUT_NAME "${baseName}")
    elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
        set_target_properties(${TARGET} PROPERTIES
            SUFFIX ".so" PREFIX "" OUTPUT_NAME "${baseName}")
    else()
        set_target_properties(${TARGET} PROPERTIES
            SUFFIX ".so" PREFIX "" OUTPUT_NAME "${baseName}")
    endif()
endfunction()
