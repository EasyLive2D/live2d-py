# BY TONGYILINGMA

# 源文件路径
set(INPUT_FILE ${LIVE2D_ROOT}/V3/Framework/src/Rendering/OpenGL/CubismShader_OpenGLES2.cpp)
# 备份文件路径
set(BACKUP_FILE ${INPUT_FILE}.bak)

if(NOT EXISTS "${BACKUP_FILE}")
  # 一次性读入整个文件，保留所有格式
  file(READ "${INPUT_FILE}" FILE_CONTENTS)

  # 原始行（12 个空格缩进，和文件里完全一致）
  set(ORIGINAL_LINE "            glDeleteProgram(_shaderSets[i]->ShaderProgram);")

  # 替换后的内容：带 glIsProgram 检查，防止多个 ShaderSet 共享同一 program 时被重复删除
  set(PATCHED_LINES "            if (glIsProgram(_shaderSets[i]->ShaderProgram))\n            {\n                glDeleteProgram(_shaderSets[i]->ShaderProgram);\n            }")

  # 检查目标行是否存在
  string(FIND "${FILE_CONTENTS}" "${ORIGINAL_LINE}" POS)
  if(POS EQUAL -1)
    message(WARNING "Patch not applied: pattern not found in ${INPUT_FILE}")
  else()
    # 原地替换，其余内容一个字节都不动
    string(REPLACE "${ORIGINAL_LINE}" "${PATCHED_LINES}" FILE_CONTENTS "${FILE_CONTENTS}")

    # 备份原文件
    file(COPY_FILE "${INPUT_FILE}" "${BACKUP_FILE}")

    # 写回
    file(WRITE "${INPUT_FILE}" "${FILE_CONTENTS}")
    message(STATUS "Successfully patched ${INPUT_FILE}")
  endif()
endif()