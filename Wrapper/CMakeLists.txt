# ---- v3 Wrapper ----
set(WRAPPER Live2DWrapper)

add_library(${WRAPPER} SHARED
    Init.cpp
    PyModel.cpp
)

target_compile_definitions(${WRAPPER} PRIVATE MODULE_LOG_TAG="pyi")

target_link_libraries(${WRAPPER} PRIVATE Live2D::Live2D Python3::SABIModule)

set_wrapper_output(${WRAPPER} "_live2d")

add_custom_command(
    TARGET ${WRAPPER} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/package/live2d/
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${WRAPPER}> ${CMAKE_SOURCE_DIR}/package/live2d/
    COMMAND ${CMAKE_COMMAND} -E copy_directory $<TARGET_PROPERTY:Framework,LIVE2D_SHADER_DIR> ${CMAKE_SOURCE_DIR}/package/live2d/FrameworkShaders
)