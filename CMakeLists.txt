# `Python Develop.SABIModule` is introduced in 3.26
cmake_minimum_required(VERSION 3.26)

project(LAppModelWrapper)

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
  execute_process(COMMAND chcp 65001)
endif()

# 统一 C++20：（vendored 目录保留自身设置）
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ---- import config ----
include(cmake/Config.cmake)
# 本地覆盖（不入库，见 .gitignore）：存在时优先级最高
include(cmake/Config.local.cmake OPTIONAL)
include(cmake/VersionCheck.cmake)

# ---- backend target ----
if(${GR_BACKEND} MATCHES "OpenGL" OR ${GR_BACKEND} MATCHES "Dual")
  if(CMAKE_SYSTEM_NAME MATCHES "Linux")
      set(CSM_TARGET CSM_TARGET_LINUX_GL)
  elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
      set(CSM_TARGET CSM_TARGET_WIN_GL) # on windows, use msvc only
  elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
      set(CSM_TARGET CSM_TARGET_MAC_GL)
  elseif(CMAKE_SYSTEM_NAME MATCHES "Android")
      set(CSM_TARGET CSM_TARGET_ANDROID_ES2)
  endif()
  add_compile_definitions(${CSM_TARGET}=1)
endif()
if(${GR_BACKEND} MATCHES "Vulkan" OR ${GR_BACKEND} MATCHES "Dual")
  set(CSM_TARGET CSM_TARGET_VULKAN)
  add_compile_definitions(CSM_TARGET_VULKAN=1)
  if (LIVE2D_VK_VALIDATION)
    message("[Vulkan] validation layers on")
    add_compile_definitions(LIVE2D_VK_VALIDATION=1)
  endif()
endif()

# ---- Cubism Native SDK ----
if (DEBUG_ENABLE_ASAN)
  message("[Asan] on")
  add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address -shared-libasan)
endif()

if (DEBUG_ENABLE_CALLSTACK)
  message("[CallStack] on")
  add_compile_definitions(DEBUG_ENABLE_CALLSTACK)
endif()

# ---- Tracy profiler（可选，源码固定为 submodule: tracy/）----
if (LIVE2D_TRACY)
  set(TRACY_ENABLE ON CACHE BOOL "" FORCE)
  set(TRACY_ON_DEMAND ON CACHE BOOL "" FORCE)
  set(TRACY_STATIC ON CACHE BOOL "" FORCE)
  add_subdirectory(tracy)
  # 消费者 TU 必须看到与 TracyClient 一致的宏：
  # TRACY_ENABLE 才会展开 ZoneScopedN/FrameMark，ON_DEMAND 影响类布局（必须一致）
  add_compile_definitions(TRACY_ENABLE TRACY_ON_DEMAND)
  # 各后端目标（V2/V3/LVulkan）都会编译带 zone 的源码，统一给 include 路径
  include_directories(tracy/public)
endif()

# 所有后端/模块的 shader 目录汇总（由 Live2D/ 填充并 PARENT_SCOPE 导出），
# 供 Wrapper / tests/cpp / Live2DViewer 统一拷贝
set(LIVE2D_SHADER_DIRS)
add_subdirectory(Live2D)

if(VIEWER)
  message("[Viewer]: on")
  include(cmake/Live2DViewer.cmake)
endif()

if(WRAPPER)
  message("[Wrapper]: on")
  # ---- Shared Wrapper config (Python3, output naming) ----
  include(cmake/Wrapper.cmake)

  add_subdirectory(Wrapper ${CMAKE_CURRENT_BINARY_DIR}/Wrapper/)
endif()

# testing
add_subdirectory(tests/cpp ${CMAKE_CURRENT_BINARY_DIR}/tests/cpp)

# --format--
include(cmake/Format.cmake)