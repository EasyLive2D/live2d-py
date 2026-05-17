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
  add_compile_options("/utf-8" "/wd4018" "/wd4244" "/wd4996")
  add_link_options("/NODEFAULTLIB:LIBCMT")
  if(CMAKE_CL_64)
    add_link_options("/BASE:0x800000000")
  endif()
  execute_process(COMMAND chcp 65001)
endif()

# ---- Shared: Glad (OpenGL loader, used by both v2 and v3) ----
if(NOT CMAKE_SYSTEM_NAME MATCHES "Android")
  add_subdirectory(Live2D/Glad ${CMAKE_CURRENT_BINARY_DIR}/Live2D/Glad)
  add_library(Live2D::Glad ALIAS glad)
endif()

# ---- Shared Wrapper config (Python3, output naming) ----
include(cmake/Wrapper.cmake)

# ---- V3: Cubism Native SDK ----
include(Live2D/V3/cmake/Live2D.cmake)

# set(VIEWER 1)
# ---- V2: Live2D Cubism 2.x C++ port ----
option(BUILD_V2CPP "Build v2cpp C++ port" ON)

if(DEFINED VIEWER)
  message("Build for Viewer")
  include(cmake/Live2DViewer.cmake)
else()
  message("Build for Wrapper")
  add_subdirectory(Wrapper/V3 ${CMAKE_CURRENT_BINARY_DIR}/Wrapper/V3)
  if(BUILD_V2CPP)
    add_subdirectory(Live2D/V2 ${CMAKE_CURRENT_BINARY_DIR}/Live2D/V2)
    add_subdirectory(Wrapper/V2 ${CMAKE_CURRENT_BINARY_DIR}/Wrapper/V2)
  endif()
endif()

