# `Python Develop.SABIModule` is introduced in 3.26
cmake_minimum_required(VERSION 3.26)

project(LAppModelWrapper)

# ---- Platform settings ----
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
  execute_process(COMMAND chcp 65001)
endif()

# ---- Cubism Native SDK ----
include(Live2D/Live2D.cmake)

# set(VIEWER 1)

if(DEFINED VIEWER)
  message("[Target]: Viewer")
  include(cmake/Live2DViewer.cmake)
else()
  message("[Target]: Wrapper")
  # ---- Shared Wrapper config (Python3, output naming) ----
  include(cmake/Wrapper.cmake)

  add_subdirectory(Wrapper/V3 ${CMAKE_CURRENT_BINARY_DIR}/Wrapper/V3)
  add_subdirectory(Wrapper/V2 ${CMAKE_CURRENT_BINARY_DIR}/Wrapper/V2)
endif()

# testing
add_subdirectory(tests/v2 ${CMAKE_CURRENT_BINARY_DIR}/tests/v2)

