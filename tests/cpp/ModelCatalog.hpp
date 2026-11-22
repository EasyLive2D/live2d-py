#pragma once

#include <cstddef>
#include <string>
#include <utility>

/**
 * 模型短名 → 仓库内相对路径。
 */
const std::pair<const char*, const char*>* ModelTable();
size_t ModelTableSize();

/**
 * u8string()/generic_u8string() 在 C++17 返回 std::string，C++20（char8_t）返回 std::u8string；
 * 两者内容都是 UTF-8 字节，统一按字节转回窄字符串。
 */
template <typename U8Str>
std::string U8ToString(const U8Str& u8) {
    return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
}

/** 自 cwd 向上查找 relPath 存在的目录，返回绝对路径；找不到返回空串。 */
std::string FindUpward(const char* relPath);

/**
 * 参数 → 绝对模型路径：含 ".json" 视为路径，否则查短名表；
 * 失败时向 stderr 打印可用短名并返回空串。
 */
std::string ResolveModelPath(const std::string& arg);

/** 默认模型：Resources/v3/llny/llny.model3.json */
std::string DefaultModelPath();
