add_library(${V3_TARGET} STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/Model.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/CubismUserModelProxy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/LAppAllocator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/LAppPal.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/LAppTextureManager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/MatrixManagerV3.cpp
)
