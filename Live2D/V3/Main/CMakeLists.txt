add_subdirectory(src)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/fine-grained FINE_GRAINED_SRCS)

target_sources(V3 PRIVATE ${FINE_GRAINED_SRCS})

# 在配置阶段立即执行文件修改脚本
include(${CMAKE_CURRENT_SOURCE_DIR}/patch_1.cmake)