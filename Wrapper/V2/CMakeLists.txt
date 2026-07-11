# ---- v2cpp Wrapper ----
set(V2_WRAPPER Live2DV2Wrapper)

add_library(${V2_WRAPPER} SHARED
    Init.cpp
    PyLAppModel.cpp
)

target_link_libraries(${V2_WRAPPER} PRIVATE Live2D::V2 Python3::SABIModule)

set_wrapper_output(${V2_WRAPPER} "_v2cpp")

add_custom_command(
    TARGET ${V2_WRAPPER} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/package/live2d/v2cpp
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${V2_WRAPPER}> ${CMAKE_SOURCE_DIR}/package/live2d/v2cpp/
)
