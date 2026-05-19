# ---- Shared Wrapper configuration ----

set(Python3_FIND_REGISTRY "NEVER")

if(DEFINED PYTHON_INSTALLATION_PATH)
    message("Found PYTHON_INSTALLATION_PATH in environment variables")
    set(CMAKE_PREFIX_PATH ${PYTHON_INSTALLATION_PATH})
else()
    message("Not found PYTHON_INSTALLATION_PATH in environment variables. \nUse default path.")
    set(CMAKE_PREFIX_PATH D:/Dev/Python/x64/3.14.5)
endif()

find_package(Python3 REQUIRED COMPONENTS Development.SABIModule)

# Helper: set output name (.pyd on Windows, .so elsewhere) and OUTPUT_NAME
function(set_wrapper_output TARGET baseName)
    if(CMAKE_SYSTEM_NAME MATCHES "Windows")
        set_target_properties(${TARGET} PROPERTIES
            SUFFIX ".pyd" PREFIX "" OUTPUT_NAME "${baseName}")
    elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
        set_target_properties(${TARGET} PROPERTIES
            SUFFIX ".so" PREFIX "lib" OUTPUT_NAME "${baseName}")
    else()
        set_target_properties(${TARGET} PROPERTIES
            SUFFIX ".so" PREFIX "lib" OUTPUT_NAME "${baseName}")
    endif()
endfunction()
