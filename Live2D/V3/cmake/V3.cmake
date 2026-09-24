set(LIVE2D_COMMON ${LIVE2D_ROOT}/Common)
set(V3_TARGET V3)

include(${CMAKE_CURRENT_LIST_DIR}/Core.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/Framework.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/Main.cmake)

add_library(Live2D::V3Core ALIAS Live2DCubismCore)
add_library(Live2D::V3Framework ALIAS Framework)
add_library(Live2D::V3 ALIAS V3)