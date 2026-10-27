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

        ${CMAKE_SOURCE_DIR}/Live2D/V3/src/*.cpp
        ${CMAKE_SOURCE_DIR}/Live2D/V3/src/*.hpp
        ${CMAKE_SOURCE_DIR}/Live2D/V3/src/*.h
        ${CMAKE_SOURCE_DIR}/Live2D/V3/include/*.cpp
        ${CMAKE_SOURCE_DIR}/Live2D/V3/include/*.hpp
        ${CMAKE_SOURCE_DIR}/Live2D/V3/include/*.h

        ${CMAKE_SOURCE_DIR}/Live2D/Common/*.cpp
        ${CMAKE_SOURCE_DIR}/Live2D/Common/*.hpp

        ${CMAKE_SOURCE_DIR}/Wrapper/*.cpp
        ${CMAKE_SOURCE_DIR}/Wrapper/*.hpp
        ${CMAKE_SOURCE_DIR}/Wrapper/*.h

        ${CMAKE_SOURCE_DIR}/tests/cpp/*.cpp
        ${CMAKE_SOURCE_DIR}/tests/cpp/*.hpp
    )
    list(FILTER ALL_SOURCES EXCLUDE REGEX ".*/backward/.*")
    list(FILTER ALL_SOURCES EXCLUDE REGEX ".*/nlohmann/.*")
    list(FILTER ALL_SOURCES EXCLUDE REGEX ".*/glfw/.*")

    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXECUTABLE} -i ${ALL_SOURCES}
        COMMENT "Formatting ${CMAKE_SOURCE_DIR} with clang-format"
        VERBATIM
    )
else()
    message(WARNING "clang-format not found, 'format' target disabled")
endif()