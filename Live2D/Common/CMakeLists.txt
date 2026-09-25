add_library(Common STATIC 
    ${LIVE2D_ROOT}/Common/Log.cpp
    ${LIVE2D_ROOT}/Common/Debug.cpp
)

set_target_properties(Common PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(Common PUBLIC ${CMAKE_CURRENT_LIST_DIR})

add_library(Live2D::Common ALIAS Common)