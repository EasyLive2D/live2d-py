
add_subdirectory(${LIVE2D_ROOT}/V3/Main)

set_property(TARGET ${V3_TARGET} PROPERTY CXX_STANDARD 17)
set_property(TARGET ${V3_TARGET} PROPERTY CXX_STANDARD_REQUIRED ON)

target_include_directories(${V3_TARGET} PUBLIC ${LIVE2D_ROOT}/V3/Main/src)

if(APPLE)
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

  message(${CMAKE_OSX_ARCHITECTURES})
  find_library(COCOA_LIBRARY Cocoa REQUIRED)
  find_library(IOKIT_LIBRARY IOKit REQUIRED)
  find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)
endif()

target_link_libraries(V3
  Framework
  Common
  ${OPENGL_LIBRARIES}
)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(V3 stdc++fs)
endif()

if(APPLE)
  target_link_libraries(V3
    ${COCOA_LIBRARY}
    ${IOKIT_LIBRARY}
    ${COREVIDEO_LIBRARY}
  )
endif()
