add_subdirectory(src)

# 在配置阶段立即执行文件修改脚本
include(${CMAKE_CURRENT_SOURCE_DIR}/patch_1.cmake)