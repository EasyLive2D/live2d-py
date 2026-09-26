set(LIVE2D_COMMON Common)

add_library(${LIVE2D_COMMON} SHARED 
    ${LIVE2D_ROOT}/Common/Log.cpp
    ${LIVE2D_ROOT}/Common/Debug.cpp
)

set_target_properties(${LIVE2D_COMMON} PROPERTIES 
    WINDOWS_EXPORT_ALL_SYMBOLS TRUE
    OUTPUT_NAME "_common"
    PREFIX ""
)

target_include_directories(${LIVE2D_COMMON} PUBLIC ${CMAKE_CURRENT_LIST_DIR})

add_library(Live2D::Common ALIAS ${LIVE2D_COMMON})

if(DEBUG_ENABLE_CALLSTACK)
    add_subdirectory(${LIVE2D_ROOT}/Common/backward)
    target_link_libraries(${LIVE2D_COMMON} PUBLIC Backward::Backward)
endif()

add_custom_command(
    TARGET ${LIVE2D_COMMON} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${LIVE2D_COMMON}> ${CMAKE_SOURCE_DIR}/package/live2d/v2cpp/
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${LIVE2D_COMMON}> ${CMAKE_SOURCE_DIR}/package/live2d/v3/
)