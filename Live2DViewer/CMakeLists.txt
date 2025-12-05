set(CMAKE_PREFIX_PATH "/home/thyne/Qt/6.9.1/gcc_64/lib/cmake/Qt6/")
set(CMAKE_AUTOMOC ON)   # 自动处理 Q_OBJECT 宏的元对象编译（moc）
set(CMAKE_AUTOUIC ON)   # 自动编译 UI 文件（.ui → .h）
set(CMAKE_AUTORCC ON)   # 自动编译资源文件（.qrc → .cpp）

find_package(Qt6 COMPONENTS Core Widgets Gui OpenGLWidgets LinguistTools REQUIRED)

add_executable(Live2DViewer
    Main.cpp

    scene/Live2DScene.hpp
    scene/Live2DScene.cpp

    MainWindow.ui
    MainWindow.hpp
    MainWindow.cpp

    Live2DView.ui
    Live2DView.hpp
    Live2DView.cpp
)

# Add Windows-specific resource file only on Windows
if(WIN32)
    target_sources(Live2DViewer PRIVATE logo.rc)
endif()

# Platform-specific compiler options
if(MSVC)
    target_compile_options(Live2DViewer PRIVATE "/Zc:__cplusplus" "/permissive-" "/utf-8")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(Live2DViewer PRIVATE "-Wall" "-Wextra")
endif()

# Platform-specific compile definitions
if(WIN32)
    target_compile_definitions(Live2DViewer
        PRIVATE CSM_TARGET_WIN_GL UNICODE _UNICODE
    )
else()
    target_compile_definitions(Live2DViewer
        PRIVATE CSM_TARGET_LINUX_GL
    )
endif()

# Windows-specific linker flags
if(WIN32)
    set_target_properties(Live2DViewer PROPERTIES
        LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
    )
endif()

target_include_directories(Live2DViewer
    PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${QTINC}
)

target_link_libraries(Live2DViewer
    PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::OpenGLWidgets 
    Live2D::Main
)

# Platform-specific output directories
if(WIN32)
    set_target_properties(Live2DViewer PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/x64/Debug
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/x64/Release
    )
endif()

# Copy icon file
file(COPY moeroid.ico DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Windows-specific deployment
if(WIN32)
    set(MARKER $<IF:$<CONFIG:DEBUG>,${PROJECT_SOURCE_DIR}/x64/Debug/.deployed,${PROJECT_SOURCE_DIR}/x64/Release/.deployed>)

    # 获取 windeployqt6 工具的路径
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt6 HINTS "${_qt_bin_dir}")

    add_custom_command(
        TARGET Live2DViewer
        POST_BUILD
        COMMAND
            if not exist ${MARKER} (${WINDEPLOYQT_EXECUTABLE} $<TARGET_FILE:Live2DViewer> && echo done. > ${MARKER}) else (echo deployed)
        COMMAND
        ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/moeroid.ico $<TARGET_FILE_DIR:Live2DViewer>/moeroid.ico
        COMMAND 
        ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/../Live2D/Framework/src/Rendering/OpenGL/Shaders/Standard $<TARGET_FILE_DIR:Live2DViewer>/FrameworkShaders
    )
else()
    # Linux deployment - just copy the icon
    add_custom_command(
        TARGET Live2DViewer
        POST_BUILD
        COMMAND
        ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/moeroid.ico $<TARGET_FILE_DIR:Live2DViewer>/moeroid.ico
        COMMAND 
        ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/../Live2D/Framework/src/Rendering/OpenGL/Shaders/Standard $<TARGET_FILE_DIR:Live2DViewer>/FrameworkShaders
    )
endif()

set(ts_files
    Translations/moe_zh_CN.ts
    Translations/moe_en.ts
    Translations/moe_ja_JP.ts
)

qt6_add_translations(Live2DViewer TS_FILES ${ts_files})
