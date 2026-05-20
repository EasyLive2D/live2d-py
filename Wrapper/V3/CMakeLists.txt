# ---- v3 Wrapper ----
set(V3_TARGET Live2DWrapper)

add_library(${V3_TARGET} SHARED
    Init.cpp
    PyModel.cpp
)

target_link_libraries(${V3_TARGET} PRIVATE Live2D::Main Python3::SABIModule)

set_wrapper_output(${V3_TARGET} "_v3cpp")

add_custom_command(
    TARGET ${V3_TARGET} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/package/live2d/v3
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${V3_TARGET}> ${CMAKE_SOURCE_DIR}/package/live2d/v3/
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Live2D/V3/Framework/src/Rendering/OpenGL/Shaders/Standard ${CMAKE_SOURCE_DIR}/package/live2d/v3/FrameworkShaders
)
