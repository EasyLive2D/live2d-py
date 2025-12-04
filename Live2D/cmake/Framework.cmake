set(FRAMEWORK_SOURCE OpenGL)
# Add Cubism Native Framework.
add_subdirectory(${LIVE2D_ROOT}/Framework)
# Add rendering definition to framework.
target_compile_definitions(Framework PUBLIC ${CSM_TARGET})

# If current compiler is mingw, add some options
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
  # extra compile options for mingw on Windows
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(Framework PRIVATE -fpermissive -Wconversion-null)
  endif()  
endif()
# Link libraries to framework.

target_include_directories(Framework PUBLIC ${LIVE2D_ROOT}/Main/src)
target_link_libraries(Framework Live2DCubismCore)

if (CMAKE_SYSTEM_NAME MATCHES "Android")
else()
  target_include_directories(Framework PUBLIC ${LIVE2D_ROOT}/Glad/include)
  target_link_libraries(Framework glad)
endif()