# ---- V2: Live2D Cubism 2.x C++ port ----
set(V2_TARGET V2)

add_subdirectory(${LIVE2D_ROOT}/V2/src)

target_include_directories(${V2_TARGET}
    PUBLIC  ${LIVE2D_ROOT}/V2/src
)

if (NOT CMAKE_SYSTEM_NAME MATCHES "Android")
    target_link_libraries(${V2_TARGET} glad)
endif()

target_link_libraries(${V2_TARGET} Common)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(${V2_TARGET} PRIVATE stdc++fs)
endif()
target_compile_features(${V2_TARGET} PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(${V2_TARGET} PRIVATE "/utf-8" "/wd4018" "/wd4244" "/wd4996")
endif()

# Alias for external projects: target_link_libraries(foo Live2D::V2)
add_library(Live2D::V2 ALIAS ${V2_TARGET})
