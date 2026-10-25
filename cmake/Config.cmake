# cmake/Config.cmake — 线上默认配置（会提交到仓库）
#
# 本地覆盖：新建 cmake/Config.local.cmake（已被 .gitignore 忽略，不会推送），
# 其中用普通 set() 写覆盖项即可，例如：
#   set(LIVE2D_CPP_TEST on)
# 优先级：Config.local.cmake > -D 命令行 > 本文件默认值
# （local 文件用非 CACHE 的 set，在 include 之后对整个 configure 生效）

# backend - Vulkan | OpenGL
set(GR_BACKEND OpenGL CACHE STRING "Graphics backend: Vulkan | OpenGL | Dual")

set(VIEWER off CACHE BOOL "enable viewer")

set(WRAPPER on CACHE BOOL "enable wrapper")

set(DEBUG_ENABLE_CALLSTACK on CACHE BOOL "print backtrace when crashing")

set(FORMAT_UTIL off CACHE BOOL "format source code by batch")

set(DEBUG_ENABLE_ASAN off CACHE BOOL "detect memory corruption")

# Vulkan 校验层（VK_LAYER_KHRONOS_validation）：开启后带上 VK_EXT_debug_utils，
# 所有违反 spec 的调用（信号量复用、present 未挂信号量等）打印到 stdout。
# 压测勿开启，否则会拖慢性能
set(LIVE2D_VK_VALIDATION off CACHE BOOL "enable Vulkan validation layers")

# build the C++ test app (tests/cpp, needs the glfw submodule)
set(LIVE2D_CPP_TEST off CACHE BOOL "build Live2DTest in tests/cpp")

# Tracy 可视化分析（可选）：记录 V2/V3 Draw 与 Vulkan 各阶段的 zone。
# 源码来自 submodule tracy/（git submodule update --init tracy）；需用 Tracy GUI 连接。
set(LIVE2D_TRACY off CACHE BOOL "enable Tracy profiler instrumentation")