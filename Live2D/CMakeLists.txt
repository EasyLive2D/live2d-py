cmake_minimum_required(VERSION 3.26)

project(easylive2d)

if(APPLE)
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

set(LIVE2D_TARGET Live2D)

message("[Live2D Root]: ${LIVE2D_ROOT}")


if(${GR_BACKEND} MATCHES "OpenGL" OR ${GR_BACKEND} MATCHES "Dual")
if(CMAKE_SYSTEM_NAME MATCHES "Android")
    set(OPENGL_LIBRARIES GLESv2)
else() # Windows, Linux, MacOS
    add_subdirectory(Glad)
    find_package(OpenGL REQUIRED)
endif()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  # Error on delete of pointer to incomplete type
  add_compile_options(-Werror=delete-incomplete)
endif()

add_subdirectory(Common)

# Vulkan 共通库（VulkanManager / SwapchainManager / VkUtil），V2、V3 共用
if(${GR_BACKEND} MATCHES "Vulkan" OR ${GR_BACKEND} MATCHES "Dual")
  add_subdirectory(Vulkan)
endif()

include(V3/cmake/V3.cmake)
include(V2/cmake/V2.cmake)


# 顶层 Live2D 库：聚合 V2/V3，并承载统一入口（Init 初始化 / CreateModel 工厂）。
# 这些入口依赖 V2+V3，放在此处而非 Common，以保持 Common 为不依赖 SDK 的纯公共层。
add_library(${LIVE2D_TARGET} STATIC
    ${LIVE2D_ROOT}/Init.cpp
    ${LIVE2D_ROOT}/IModel.cpp
)
target_include_directories(${LIVE2D_TARGET} PUBLIC ${LIVE2D_ROOT})
target_link_libraries(${LIVE2D_TARGET} PUBLIC
    Live2D::V2
    Live2D::V3
)
if (LIVE2D_TRACY AND TARGET Tracy::TracyClient)
    target_link_libraries(${LIVE2D_TARGET} PUBLIC Tracy::TracyClient)
endif()
add_library(Live2D::Live2D ALIAS ${LIVE2D_TARGET})