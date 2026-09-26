if(FORMAT_UTIL)
    message("[Format] on")
else()
    return()
endif()

find_program(CLANG_FORMAT_EXECUTABLE PATHS "tools" NAMES clang-format)

if(CLANG_FORMAT_EXECUTABLE)
    # 递归收集源文件
    file(GLOB_RECURSE ALL_SOURCES
        ${CMAKE_SOURCE_DIR}/Live2D/V2/*.cpp
        ${CMAKE_SOURCE_DIR}/Live2D/V2/*.hpp
        ${CMAKE_SOURCE_DIR}/Live2D/V2/*.h

        ${CMAKE_SOURCE_DIR}/Live2D/V3/Main/*.cpp
        ${CMAKE_SOURCE_DIR}/Live2D/V3/Main/*.hpp
        ${CMAKE_SOURCE_DIR}/Live2D/V3/Main/*.h

        ${CMAKE_SOURCE_DIR}/Live2D/Common/*.cpp
        ${CMAKE_SOURCE_DIR}/Live2D/Common/*.hpp
        ${CMAKE_SOURCE_DIR}/Live2D/Common/*.h

        ${CMAKE_SOURCE_DIR}/Wrapper/*.cpp
        ${CMAKE_SOURCE_DIR}/Wrapper/*.hpp
        ${CMAKE_SOURCE_DIR}/tests/v2/main.cpp
    )

    # 过滤掉第三方目录
    list(FILTER ALL_SOURCES EXCLUDE REGEX ".*/backward/.*")
    list(FILTER ALL_SOURCES EXCLUDE REGEX ".*/nlohmann/.*")
    list(FILTER ALL_SOURCES EXCLUDE REGEX ".*/stb_image.h")


    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXECUTABLE} -i ${ALL_SOURCES}
        COMMENT "Formatting ${CMAKE_SOURCE_DIR} with clang-format"
        VERBATIM
    )
else()
    message(WARNING "clang-format not found, 'format' target disabled")
endif()