# ---- Live2D Test: loads a v2/v3 model (auto-detected) with GLFW ----
#
# 开关在 cmake/Config.cmake（LIVE2D_CPP_TEST），本地可用 cmake/Config.local.cmake 覆盖

if(NOT LIVE2D_CPP_TEST)
  return()
endif()
message("LIVE2D.CPP.TEST Enabled")


# ---- GLFW (git submodule) --------------------------------------------------
set(GLFW_BUILD_DOCS     OFF CACHE INTERNAL "")
set(GLFW_BUILD_TESTS    OFF CACHE INTERNAL "")
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "")
add_subdirectory(glfw ${CMAKE_CURRENT_BINARY_DIR}/glfw)

# ---- Live2DTest executable ---------------------------------------------------
add_executable(Live2DTest
  main.cpp
  AppRun.cpp
  Cli.cpp
  ModelCatalog.cpp
  SmokeTest.cpp
)

target_link_libraries(Live2DTest PRIVATE
  Live2D::Live2D
  glfw
)

if(MSVC)
  target_compile_options(Live2DTest PRIVATE "/utf-8" "/wd4018" "/wd4244" "/wd4996")
endif()

# Output next to build dir for easy access to Resources/ (relative path)
set_target_properties(Live2DTest PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/cpp
)

target_compile_definitions(Live2DTest PRIVATE SHADER_DIRECTORY="${CMAKE_SOURCE_DIR}/package/live2d")

# ---- shader files next to the executable ------------------------------------
# GL / Vulkan 渲染器都在创建时按 CWD 读取 "FrameworkShaders/*"；
# 把每个已启用后端的 shader 目录拷到 exe 旁（Live2DTest 启动时把 CWD 切到 exe 目录）。
#   GL     : V3 Framework 源文件（*.vert/*.frag）+ V2 源文件（V2*GL.*）
#   Vulkan : V3 / V2 编译产物（*.spv）
set(TEST_SHADER_OUT $<TARGET_FILE_DIR:Live2DTest>/FrameworkShaders)

add_custom_command(TARGET Live2DTest POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ${TEST_SHADER_OUT}
  COMMENT "Prepare FrameworkShaders next to Live2DTest"
)

foreach(shaderDir ${LIVE2D_SHADER_DIRS})
  add_custom_command(TARGET Live2DTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${shaderDir} ${TEST_SHADER_OUT}
    COMMENT "Copy shaders: ${shaderDir}"
  )
endforeach()

# Wire into ctest for local use: `ctest --test-dir build` (opens a GLFW window;
# not run in CI since runners are headless)
include(CTest)
if(BUILD_TESTING)
  add_test(NAME Live2DTest COMMAND Live2DTest)
endif()
