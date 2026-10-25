# Add Cubism Native Framework.
if(${GR_BACKEND} MATCHES "Vulkan")
  set(FRAMEWORK_SOURCE Vulkan)
elseif(${GR_BACKEND} MATCHES "OpenGL")
  set(FRAMEWORK_SOURCE OpenGL)
elseif(${GR_BACKEND} MATCHES "Dual")
  set(FRAMEWORK_SOURCE OpenGL)
endif()

add_subdirectory(${LIVE2D_ROOT}/V3/Framework)

if(${GR_BACKEND} MATCHES "Dual")
  aux_source_directory(${LIVE2D_ROOT}/V3/Framework/src/Rendering/Vulkan VulkanSources)
  target_sources(Framework PRIVATE ${VulkanSources})
endif()

# If current compiler is mingw, add some options
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
  # extra compile options for mingw on Windows
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(Framework PRIVATE -fpermissive -Wconversion-null)
  endif()  
endif()
# Link libraries to framework.

# include for HackProperties
target_include_directories(Framework PUBLIC ${LIVE2D_ROOT}/V3/include/V3)
target_link_libraries(Framework PUBLIC Live2DCubismCore)

if(${GR_BACKEND} MATCHES "OpenGL" OR ${GR_BACKEND} MATCHES "Dual")
  if (NOT CMAKE_SYSTEM_NAME MATCHES "Android")
    target_link_libraries(Framework PUBLIC glad)
  endif()
  # Expose GL shader source directory via target property for wrapper post-build copy
  # (Dual 下与 LIVE2D_VK_SHADER_DIR 并存，两个目录都会拷到 package/live2d/FrameworkShaders)
  set_target_properties(Framework PROPERTIES
    LIVE2D_GL_SHADER_DIR "${LIVE2D_ROOT}/V3/Framework/src/Rendering/OpenGL/Shaders/Standard"
  )
endif()

if(${GR_BACKEND} MATCHES "Vulkan" OR ${GR_BACKEND} MATCHES "Dual")
  if (NOT CMAKE_SYSTEM_NAME MATCHES "Android")
    target_link_libraries(Framework PUBLIC Live2D::Vulkan)
  endif()
  # Compile Vulkan shaders (.spv) using the SDK's own Shaders/CMakeLists.txt
  # (Framework 源码树不修改，仅通过 add_subdirectory 复用其编译规则).
  # Exposes the output dir via $<TARGET_PROPERTY:Framework,LIVE2D_VK_SHADER_DIR>.
  add_subdirectory(${LIVE2D_ROOT}/V3/Framework/src/Rendering/Vulkan/Shaders
                  ${CMAKE_CURRENT_BINARY_DIR}/FrameworkVulkanShaders)
  set_target_properties(FrameworkShaders PROPERTIES
    LIVE2D_VK_SHADER_DIR "${CMAKE_CURRENT_BINARY_DIR}/FrameworkVulkanShaders/compiledShaders"
  )
  set_target_properties(Framework PROPERTIES
    LIVE2D_VK_SHADER_DIR "${CMAKE_CURRENT_BINARY_DIR}/FrameworkVulkanShaders/compiledShaders"
  )
endif()