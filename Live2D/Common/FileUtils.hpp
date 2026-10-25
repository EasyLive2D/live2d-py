#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Live2D {
namespace Common {

/**
 * @brief 平台无关的文件加载 / shader 路径重定向工具。
 *
 * 不依赖 Cubism Framework，V2 / V3 共用。
 * shader 路径（"FrameworkShaders/..."）会被重定向到 InitShaderDir 设置的目录。
 */
class FileUtils {
    static std::string SHADER_DIR;

    static void interceptShaderLoading(std::string& filePath);

public:
    /**
     * @brief 以字节数据读取文件。
     *
     * @param[in]   filePath  文件路径
     * @param[out]  outSize   文件大小
     * @return                字节数据（调用方负责用 ReleaseBytes 释放）
     */
    static unsigned char* LoadFileAsBytes(const std::string filePath, unsigned int* outSize);

    static std::vector<uint8_t> LoadFileAsBytes(const std::string& filePath);

    /**
     * @brief 释放 LoadFileAsBytes 返回的字节数据。
     */
    static void ReleaseBytes(unsigned char* byteData);

    /**
     * @brief Cubism Framework 的日志回调。
     */
    static void PrintLn(const char* message);

    /**
     * @brief 设置 shader 重定向目录（末尾会自动补分隔符）。
     */
    static void InitShaderDir(const std::string& path);
};
}   // namespace Common
}   // namespace Live2D
