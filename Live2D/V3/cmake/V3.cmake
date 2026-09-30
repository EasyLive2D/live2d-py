set(V3_TARGET V3)

include(${CMAKE_CURRENT_LIST_DIR}/Core.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/Framework.cmake)
# 在配置阶段立即执行文件修改脚本
include(${CMAKE_CURRENT_LIST_DIR}/PatchCubismShader_OpenGLES2.cmake)

add_subdirectory(${LIVE2D_ROOT}/V3/src)

target_compile_definitions(${V3_TARGET} PRIVATE MODULE_LOG_TAG="v3")

set_property(TARGET ${V3_TARGET} PROPERTY CXX_STANDARD 17)
set_property(TARGET ${V3_TARGET} PROPERTY CXX_STANDARD_REQUIRED ON)

target_include_directories(${V3_TARGET} 
  PUBLIC ${LIVE2D_ROOT}/V3/include/V3
  PUBLIC ${LIVE2D_ROOT}/V3/include
)

if(APPLE)
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

  message(${CMAKE_OSX_ARCHITECTURES})
  find_library(COCOA_LIBRARY Cocoa REQUIRED)
  find_library(IOKIT_LIBRARY IOKit REQUIRED)
  find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)
endif()

target_link_libraries(${V3_TARGET} PUBLIC
  Framework
  Common
  ${OPENGL_LIBRARIES}
)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(${V3_TARGET} PUBLIC stdc++fs)
endif()

if(APPLE)
  target_link_libraries(${V3_TARGET} PUBLIC
    ${COCOA_LIBRARY}
    ${IOKIT_LIBRARY}
    ${COREVIDEO_LIBRARY}
  )
endif()


add_library(Live2D::V3Core ALIAS Live2DCubismCore)
add_library(Live2D::V3Framework ALIAS Framework)
add_library(Live2D::V3 ALIAS ${V3_TARGET})