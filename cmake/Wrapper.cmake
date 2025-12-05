# Create Wrapper 
set(Wrapper Live2DWrapper)
add_library(${Wrapper} SHARED 
  ${PROJECT_SOURCE_DIR}/Wrapper/Python.hpp
  ${PROJECT_SOURCE_DIR}/Wrapper/Live2D.cpp
  ${PROJECT_SOURCE_DIR}/Wrapper/PyLAppModel.hpp
  ${PROJECT_SOURCE_DIR}/Wrapper/PyLAppModel.cpp
  ${PROJECT_SOURCE_DIR}/Wrapper/PyModel.hpp
  ${PROJECT_SOURCE_DIR}/Wrapper/PyModel.cpp
)

set(Python3_FIND_REGISTRY "NEVER")

if(DEFINED PYTHON_INSTALLATION_PATH)
    message("Found PYTHON_INSTALLATION_PATH in environment variables")
    set(CMAKE_PREFIX_PATH ${PYTHON_INSTALLATION_PATH})
else()
    message("Not found PYTHON_INSTALLATION_PATH in environment variables. \nUse default path.")
    set(CMAKE_PREFIX_PATH D:/Python/x64/3.10.0)
endif()

find_package(Python3 REQUIRED COMPONENTS Development.SABIModule)

target_link_libraries(${Wrapper} PRIVATE Live2D::Main Python3::SABIModule)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(MODULE_NAME lib${Wrapper}.so)
  set(OUTPUT_NAME live2d.so)
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
  set(MODULE_NAME ${Wrapper}.dll)
  set(OUTPUT_NAME live2d.pyd)
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(MODULE_NAME lib${Wrapper}.dylib)
  set(OUTPUT_NAME live2d.so)
endif()


add_custom_command(
  TARGET ${Wrapper}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/package/live2d/v3
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${Wrapper}> ${CMAKE_CURRENT_SOURCE_DIR}/package/live2d/v3/${OUTPUT_NAME}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/Live2D/Framework/src/Rendering/OpenGL/Shaders/Standard ${CMAKE_CURRENT_SOURCE_DIR}/package/live2d/v3/FrameworkShaders
)

