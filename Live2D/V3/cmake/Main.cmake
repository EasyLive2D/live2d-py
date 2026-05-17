
add_subdirectory(${LIVE2D_ROOT}/Main)

set_property(TARGET V3 PROPERTY CXX_STANDARD 17)
set_property(TARGET V3 PROPERTY CXX_STANDARD_REQUIRED ON)

target_include_directories(V3 PUBLIC ${LIVE2D_ROOT}/Main/src ${LIVE2D_COMMON})

if(CMAKE_SYSTEM_NAME MATCHES "Android")
  set(OPENGL_LIBRARIES GLESv2)
else() # Windows, Linux, MacOS
  find_package(OpenGL REQUIRED)
endif()

if(APPLE)
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

  # set(CMAKE_OSX_ARCHITECTURES "arm64")
  message(${CMAKE_OSX_ARCHITECTURES})
  find_library(COCOA_LIBRARY Cocoa REQUIRED)
  find_library(IOKIT_LIBRARY IOKit REQUIRED)
  find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)
endif()

target_link_libraries(V3
  Framework
  ${OPENGL_LIBRARIES}
)

if(APPLE)
  target_link_libraries(V3
    ${COCOA_LIBRARY}
    ${IOKIT_LIBRARY}
    ${COREVIDEO_LIBRARY}
  )
endif()
