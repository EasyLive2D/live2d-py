add_library(Live2DCubismCore STATIC IMPORTED)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(CSM_TARGET CSM_TARGET_LINUX_GL)
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    message("Target: linux arm64")
    set(CORE_LIB_NAME experimental/linux/arm64/libLive2DCubismCore.a)  # ARM64 库
  else()
    message("Target: linux x86_64")
    set(CORE_LIB_NAME linux/x86_64/libLive2DCubismCore.a) # x86_64 库
  endif()
  add_compile_options(-fPIC)
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
  set(CSM_TARGET CSM_TARGET_WIN_GL) # on windows, use msvc only
  if (CMAKE_CL_64)
    message("Target: win x64")
    set(CORE_LIB_NAME windows/x86_64/143/Live2DCubismCore_MT.lib)
  else()
    message("Target: win x86")
    set(CORE_LIB_NAME windows/x86/143/Live2DCubismCore_MT.lib)
  endif()
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(CSM_TARGET CSM_TARGET_MAC_GL)
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    message("Target: Darwin arm64")
    set(CORE_LIB_NAME macos/arm64/libLive2DCubismCore.a)
  else()
    message("Target: Darwin x86_64")
    set(CORE_LIB_NAME macos/x86_64/libLive2DCubismCore.a)
  endif()
elseif(CMAKE_SYSTEM_NAME MATCHES "Android")
  set(CSM_TARGET CSM_TARGET_ANDROID_ES2)
  message("Target: Darwin x86_64")
  set(CORE_LIB_NAME android/${CMAKE_ANDROID_ARCH_ABI}/libLive2DCubismCore.a)
endif()

message("Live2D Core: ${CORE_LIB_NAME}")

set_target_properties(Live2DCubismCore
  PROPERTIES
    IMPORTED_LOCATION
    ${LIVE2D_ROOT}/Core/lib/${CORE_LIB_NAME}
    INTERFACE_INCLUDE_DIRECTORIES
      ${LIVE2D_ROOT}/Core/include
)
target_compile_definitions(Live2DCubismCore
  INTERFACE
  ${CSM_TARGET}
)