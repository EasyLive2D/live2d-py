# BY TONGYILINGMA

# 源文件路径
set(INPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../Framework/src/Motion/ACubismMotion.hpp)
# 备份文件路径
set(BACKUP_FILE ${INPUT_FILE}.bak)

if(NOT EXISTS "${BACKUP_FILE}")
  # 设置要插入的代码
  set(INSERT_INCLUDE "#include <HackProperties.h>\n")
  set(INSERT_ADDITIONAL_PROPERTIES "    __ADDITIONAL_PROPERTIES__\n")

  # 读取整个文件到变量
  file(STRINGS ${INPUT_FILE} FILE_CONTENTS)

  # 初始化输出内容为空字符串
  set(OUTPUT_CONTENTS "")
  set(HAS_INSERTED_INCLUDE FALSE)
  set(HAS_INSERTED_PROPERTIES FALSE)

  # 遍历每一行，检查是否需要插入代码
  foreach(line IN LISTS FILE_CONTENTS)
    # 插入 include 语句
    if("${line}" MATCHES "^#include \"Type/csmVector.hpp\"$" AND NOT HAS_INSERTED_INCLUDE)
      set(OUTPUT_CONTENTS "${OUTPUT_CONTENTS}${line}\n")
      set(OUTPUT_CONTENTS "${OUTPUT_CONTENTS}${INSERT_INCLUDE}")
      set(HAS_INSERTED_INCLUDE TRUE)
    elseif("${line}" MATCHES "^public:$" AND NOT HAS_INSERTED_PROPERTIES)
      set(OUTPUT_CONTENTS "${OUTPUT_CONTENTS}${line}\n")
      set(OUTPUT_CONTENTS "${OUTPUT_CONTENTS}${INSERT_ADDITIONAL_PROPERTIES}")
      set(HAS_INSERTED_PROPERTIES TRUE)
    else()
      set(OUTPUT_CONTENTS "${OUTPUT_CONTENTS}${line}\n")
    endif()
  endforeach()

  # 将原始文件复制为备份
  file(COPY_FILE ${INPUT_FILE} ${BACKUP_FILE})

  # 将最终的内容写回原文件
  file(WRITE ${INPUT_FILE} "${OUTPUT_CONTENTS}")
endif()
