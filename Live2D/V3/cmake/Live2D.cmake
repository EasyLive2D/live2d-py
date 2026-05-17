get_filename_component(LIVE2D_ROOT ${CMAKE_CURRENT_LIST_DIR} DIRECTORY)
set(LIVE2D_COMMON ${LIVE2D_ROOT}/../Common)

message("Live2D Python Root: ${LIVE2D_ROOT}")

include(${CMAKE_CURRENT_LIST_DIR}/Core.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/Framework.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/Main.cmake)

add_library(Live2D::Core ALIAS Live2DCubismCore)
add_library(Live2D::Framework ALIAS Framework)
add_library(Live2D::Main ALIAS V3)