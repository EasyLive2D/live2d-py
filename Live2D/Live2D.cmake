set(LIVE2D_ROOT ${CMAKE_CURRENT_LIST_DIR})

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/Common)

if(CMAKE_SYSTEM_NAME MATCHES "Android")
    set(OPENGL_LIBRARIES GLESv2)
else() # Windows, Linux, MacOS
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/Glad)
    find_package(OpenGL REQUIRED)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  # Error on delete of pointer to incomplete type
  add_compile_options(-Werror=delete-incomplete)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/V3/cmake/V3.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/V2/cmake/V2.cmake)