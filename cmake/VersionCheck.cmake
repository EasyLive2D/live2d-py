# 读 _version.py
file(READ "${CMAKE_SOURCE_DIR}/package/live2d/__init__.py" _ver_content)

# 提取 __version__
string(REGEX MATCH "__version__ *= *[\"']([^\"']+)[\"']" _ "${_ver_content}")
set(LIVE2D_PY_VERSION "${CMAKE_MATCH_1}")

# 提取 __csm_version__（注意：正则要匹配带 __ 后缀的名字）
string(REGEX MATCH "__csm_version__ *= *[\"']([^\"']+)[\"']" _ "${_ver_content}")
set(CUBISM_SDK_VERSION "${CMAKE_MATCH_1}")
add_compile_definitions(CUBISM_NATIVE_SDK_VERSION="${CUBISM_SDK_VERSION}")
# 校验
if(NOT LIVE2D_PY_VERSION)
    message(FATAL_ERROR "无法从 _version.py 提取 __version__")
endif()
if(NOT CUBISM_SDK_VERSION)
    message(FATAL_ERROR "无法从 _version.py 提取 __csm_version__")
endif()

message("[live2d-py]: ${LIVE2D_PY_VERSION}")
message("[CubismNativeSDK]: ${CUBISM_SDK_VERSION}")