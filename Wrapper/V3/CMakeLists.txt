# ---- v3 Wrapper ----
set(V3_WRAPPER Live2DWrapper)

add_library(${V3_WRAPPER} SHARED
    Init.cpp
    PyModel.cpp
)

target_link_libraries(${V3_WRAPPER} PRIVATE Live2D::V3 Python3::SABIModule)

set_wrapper_output(${V3_WRAPPER} "_v3cpp")

add_custom_command(
    TARGET ${V3_WRAPPER} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/package/live2d/v3
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${V3_WRAPPER}> ${CMAKE_SOURCE_DIR}/package/live2d/v3/
    COMMAND ${CMAKE_COMMAND} -E copy_directory $<TARGET_PROPERTY:Framework,LIVE2D_SHADER_DIR> ${CMAKE_SOURCE_DIR}/package/live2d/v3/FrameworkShaders
)
