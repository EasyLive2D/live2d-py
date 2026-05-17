# ---- v2cpp Wrapper ----
set(V2_TARGET Live2DV2Wrapper)

add_library(${V2_TARGET} SHARED
    Init.cpp
    PyLAppModel.cpp
)

target_include_directories(${V2_TARGET} PRIVATE ${CMAKE_SOURCE_DIR}/Live2D/Glad/include)
target_link_libraries(${V2_TARGET} PRIVATE V2 Python3::SABIModule)

set_wrapper_output(${V2_TARGET} "_v2cpp")

add_custom_command(
    TARGET ${V2_TARGET} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/package/live2d/v2cpp
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${V2_TARGET}> ${CMAKE_SOURCE_DIR}/package/live2d/v2cpp/
)
