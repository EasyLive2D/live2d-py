add_library(glad
    ${CMAKE_CURRENT_SOURCE_DIR}/src/glad.c
)

set_target_properties(glad PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(glad
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/
)