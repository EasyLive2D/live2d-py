# ---- v3 Wrapper ----
set(WRAPPER Live2DWrapper)

add_library(${WRAPPER} SHARED
    Init.cpp
    PyModel.cpp
)

target_compile_definitions(${WRAPPER} PRIVATE MODULE_LOG_TAG="pyi")

target_link_libraries(${WRAPPER} PRIVATE Live2D::Live2D Python3::SABIModule)

set_wrapper_output(${WRAPPER} "_live2d")

# 运行时 shader 目录：所有后端/模块的 shader 都汇总到 package/live2d/FrameworkShaders，
# 文件名互不冲突（V3: VertShaderSrc*/FragShaderSrc*；V2: V2*.spv / V2*GL.*）
set(WRAPPER_SHADER_OUT ${CMAKE_SOURCE_DIR}/package/live2d/FrameworkShaders)

add_custom_command(
    TARGET ${WRAPPER} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/package/live2d/
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${WRAPPER}> ${CMAKE_SOURCE_DIR}/package/live2d/
    COMMAND ${CMAKE_COMMAND} -E make_directory ${WRAPPER_SHADER_OUT}
)

foreach(shaderDir ${LIVE2D_SHADER_DIRS})
    add_custom_command(
        TARGET ${WRAPPER} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${shaderDir} ${WRAPPER_SHADER_OUT}
    )
endforeach()