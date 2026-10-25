/**
 * live2d-py C++ unified test entry point — 只负责参数与环境准备，然后分发：
 *   - --smoke [all | model] : 无窗口 API 回归（SmokeTest）
 *   - 其余                  : GLFW 交互渲染（AppRun）
 *
 * 模块划分：
 *   Cli.*         命令行解析
 *   ModelCatalog.* 短名表与路径解析
 *   SmokeTest.*   无窗口回归
 *   AppRun.*      窗口 / 后端初始化 / 主循环
 *
 * Build:
 *   cmake -DLIVE2D_CPP_TEST=ON -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -S . -B build
 *   cmake --build build --config Release --target Live2DTest -j 24
 *
 * Run (from repo root):
 *   .\build\tests\cpp\Live2DTest.exe [--smoke] [--opengl|--vulkan] [all | model-name | model.json path]
 *
 * Default model: Resources/v3/llny/llny.model3.json
 */

#include <cstdio>
#include <string>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#endif

#include <Init.hpp>

#include "AppRun.hpp"
#include "Cli.hpp"
#include "ModelCatalog.hpp"
#include "SmokeTest.hpp"

#ifndef SHADER_DIRECTORY
#define SHADER_DIRECTORY ""
#endif

// Windows 的 main() argv 是 ANSI(GBK) 编码，而库的 readFile 按 UTF-8 解释路径
// （std::filesystem::u8path）。中文路径（如 托尔）的 GBK 字节不是合法 UTF-8，
// 直接传给 u8path 会抛异常导致 terminate。重新按宽字符取命令行并转 UTF-8。
#ifdef _WIN32
static void ConvertArgsToUtf8(int& argc,
                              char**& argv,
                              std::vector<std::string>& storage,
                              std::vector<char*>& out) {
    int wargc = 0;
    LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
    if (!wargv)
        return;
    for (int i = 0; i < wargc; i++) {
        int len = WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, nullptr, 0, nullptr, nullptr);
        std::string s(len > 0 ? len - 1 : 0, '\0');
        if (len > 1)
            WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, s.data(), len, nullptr, nullptr);
        storage.push_back(std::move(s));
    }
    LocalFree(wargv);
    for (auto& s : storage)
        out.push_back(s.data());
    argc = (int)out.size();
    argv = out.data();
}
#endif

int main(int argc, char* argv[]) {
    // 窗口模式下 stdout 全缓冲会吞掉崩溃前的日志——强制无缓冲便于调试
    setvbuf(stdout, nullptr, _IONBF, 0);
#ifdef _WIN32
    std::vector<std::string> u8argStorage;
    std::vector<char*> u8argv;
    ConvertArgsToUtf8(argc, argv, u8argStorage, u8argv);
    SetConsoleOutputCP(65001);
#endif

    AppOptions opt;
    if (!ParseOptions(argc, argv, opt))
        return 1;

    Live2D::Init(SHADER_DIRECTORY);
    Live2D::Common::Backend::SetBackend(opt.backend);
    printf("Backend: %s\n", Live2D::Common::Backend::IsVulkan() ? "Vulkan" : "OpenGL");
    if (opt.smokeAll)
        printf("Smoke all: %d model entries\n", (int)ModelTableSize());
    else
        printf("Model path: %s\n", opt.modelPath.c_str());

    if (opt.smoke)
        return opt.smokeAll ? RunSmokeAll() : RunSmokeModel(opt.modelPath);
    return RunInteractive(opt.modelPath);
}
