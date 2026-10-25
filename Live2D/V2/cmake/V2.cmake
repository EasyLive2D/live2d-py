# ---- V2: Live2D Cubism 2.x C++ port ----
set(V2_TARGET V2)

add_subdirectory(${LIVE2D_ROOT}/V2/src)

target_include_directories(${V2_TARGET}
    PUBLIC ${LIVE2D_ROOT}/V2/include
    PUBLIC ${LIVE2D_ROOT}/V2/include/V2
    PUBLIC ${LIVE2D_ROOT}/V2/include/V2/Core
    PUBLIC ${LIVE2D_ROOT}/V2/include/V2/Framework
    PUBLIC ${LIVE2D_ROOT}/V2/include/V2/Renderer
)

target_link_libraries(${V2_TARGET} PUBLIC Common)
if (${GR_BACKEND} MATCHES "Vulkan" OR ${GR_BACKEND} MATCHES "Dual")
    # Vulkan 渲染后端（VkRenderer 依赖 VulkanManager / VkUtil）
    target_link_libraries(${V2_TARGET} PUBLIC Live2D::Vulkan)
    include(${CMAKE_CURRENT_LIST_DIR}/V2Shaders.cmake)
    add_dependencies(${V2_TARGET} V2Shaders)
endif()
if (${GR_BACKEND} MATCHES "OpenGL" OR ${GR_BACKEND} MATCHES "Dual")
    # V2 OpenGL shader 源文件目录（.vert/.frag，构建后拷到 package/live2d/FrameworkShaders）
    set_target_properties(${V2_TARGET} PROPERTIES
        LIVE2D_GL_SHADER_DIR "${LIVE2D_ROOT}/V2/shaders/opengl")
    if (NOT CMAKE_SYSTEM_NAME MATCHES "Android")
        target_link_libraries(${V2_TARGET} PUBLIC glad)
    endif()
endif()
target_compile_definitions(${V2_TARGET} PRIVATE MODULE_LOG_TAG="v2")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(${V2_TARGET} PRIVATE stdc++fs)
endif()
target_compile_features(${V2_TARGET} PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(${V2_TARGET} PRIVATE "/utf-8" "/wd4018" "/wd4244" "/wd4996")
endif()

# Alias for external projects: target_link_libraries(foo Live2D::V2)
add_library(Live2D::V2 ALIAS ${V2_TARGET})
