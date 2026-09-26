cmake_minimum_required(VERSION 3.26)

project(easylive2d)

if(APPLE)
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    set(CMAKE_OSX_ARCHITECTURES "arm64")
  else()
    set(CMAKE_OSX_ARCHITECTURES "x86_64")
  endif()
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Zi")
  add_compile_options("/utf-8" "/wd4018" "/wd4244" "/wd4996" "/we4150")
  add_link_options("/NODEFAULTLIB:LIBCMT")
  if(CMAKE_CL_64)
    add_link_options("/BASE:0x800000000")
  endif()
endif()

set(LIVE2D_ROOT ${CMAKE_CURRENT_LIST_DIR})

message("[Live2D Root]: ${LIVE2D_ROOT}")

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/Common)

if(CMAKE_SYSTEM_NAME MATCHES "Android")
    set(OPENGL_LIBRARIES GLESv2)
else() # Windows, Linux, MacOS
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/Glad)
    find_package(OpenGL REQUIRED)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  # Error on delete of pointer to incomplete type
  add_compile_options(-Werror=delete-incomplete)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/V3/cmake/V3.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/V2/cmake/V2.cmake)