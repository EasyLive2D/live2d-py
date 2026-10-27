#include "FileUtils.hpp"

#include <Log.hpp>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <filesystem>

using namespace Live2D::Common::Log;

namespace Live2D {
namespace Common {

std::string FileUtils::SHADER_DIR = "";

unsigned char* FileUtils::LoadFileAsBytes(const std::string filePath, unsigned int* outSize) {
    std::string pathStr = filePath;
    interceptShaderLoading(pathStr);
    std::filesystem::path path = std::filesystem::u8path(pathStr);

    std::ifstream file;
    file.open(path, std::ios::binary | std::ios::ate);
    if (!file) {
        LOGI("File open failed. path:%s", pathStr.c_str());
        return nullptr;
    }

    size_t size = file.tellg();
    file.seekg(0);
    char* buf = new char[size];
    file.read(buf, size);

    if (outSize) {
        *outSize = static_cast<unsigned int>(size);
    }

    return reinterpret_cast<unsigned char*>(buf);
}

std::vector<uint8_t> FileUtils::LoadFileAsBytes(const std::string& filePath) {
    std::string pathStr = filePath;
    interceptShaderLoading(pathStr);
    std::filesystem::path path = std::filesystem::u8path(pathStr);

    std::ifstream file;
    file.open(path, std::ios::binary | std::ios::ate);
    if (!file) {
        LOGI("File open failed. path:%s", pathStr.c_str());
        return {};
    }

    size_t size = file.tellg();
    file.seekg(0);
    std::vector<uint8_t> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);

    return data;
}

void FileUtils::ReleaseBytes(unsigned char* byteData) {
    delete[] byteData;
}

void FileUtils::PrintLn(const char* message) {
    if (IsLive2DLogEnabled()) {
        printf("%s\n", message);
    }
}

void FileUtils::InitShaderDir(const std::string& path) {
    SHADER_DIR = path;
    SHADER_DIR += std::filesystem::path::preferred_separator;
    LOGI("[FileUtils] Init Shader Dir: %s", SHADER_DIR.c_str());
}

void FileUtils::interceptShaderLoading(std::string& filePath) {
    if (filePath.substr(0, 17) == "FrameworkShaders/") {
        LOGD("[FileUtils] intercept for shader: %s", filePath.c_str());
        filePath = SHADER_DIR + (char)std::filesystem::path::preferred_separator + filePath;
    }
}

}   // namespace Common
}   // namespace Live2D
