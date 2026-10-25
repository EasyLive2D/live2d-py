# V2 Vulkan shader 编译：Live2D/V2/shaders/*.vert|frag → .spv
# 输出目录追加到 LIVE2D_SHADER_DIRS，由调用方（Wrapper/tests/Viewer）统一拷贝

set(V2_SHADER_SRC_DIR ${LIVE2D_ROOT}/V2/shaders)
set(V2_SHADER_OUT_DIR ${CMAKE_BINARY_DIR}/V2Shaders)
file(MAKE_DIRECTORY ${V2_SHADER_OUT_DIR})

# 定位 glslc（优先 CMake 提供的 Vulkan::glslc）
if(TARGET Vulkan::glslc)
    set(V2_GLSLC_CMD glslc)
    set(V2_GLSLC_DEP Vulkan::glslc)
else()
    find_program(V2_GLSLC_EXE
        NAMES glslc glslc.exe
        HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/Bin32" "$ENV{VK_SDK_PATH}/Bin"
        REQUIRED)
    set(V2_GLSLC_CMD ${V2_GLSLC_EXE})
    set(V2_GLSLC_DEP "")
endif()

set(V2_SHADERS
    V2Normal.vert
    V2Normal.frag
    V2Mask.vert
    V2Mask.frag
)

set(V2_SPV_OUTPUTS)
foreach(shader ${V2_SHADERS})
    set(in ${V2_SHADER_SRC_DIR}/${shader})
    set(out ${V2_SHADER_OUT_DIR}/${shader}.spv)
    add_custom_command(
        OUTPUT ${out}
        COMMAND ${V2_GLSLC_CMD} -O ${in} -o ${out}
        DEPENDS ${in}
        COMMENT "glslc ${shader} -> ${shader}.spv"
        VERBATIM)
    list(APPEND V2_SPV_OUTPUTS ${out})
endforeach()

add_custom_target(V2Shaders ALL DEPENDS ${V2_SPV_OUTPUTS})
if(V2_GLSLC_DEP)
    add_dependencies(V2Shaders ${V2_GLSLC_DEP})
endif()

list(APPEND LIVE2D_SHADER_DIRS
    ${V2_SHADER_OUT_DIR}
)
