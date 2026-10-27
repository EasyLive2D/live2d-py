#include "ModelCatalog.hpp"

#include <cstdio>
#include <filesystem>
#include <string>

namespace {

const std::pair<const char*, const char*> kModelTable[] = {
    {"kasumi2", "Resources/v2/kasumi2/kasumi2.model.json"},
    {"haru2", "Resources/v2/haru/haru.model.json"},
    {"hibiki", "Resources/v2/hibiki/hibiki.model.json"},
    {"kana", "Resources/v2/kana/Kobayaxi.model.json"},
    {"shizuku", "Resources/v2/shizuku/shizuku.model.json"},
    {"epsilon", "Resources/v2/Epsilon/Epsilon.model.json"},
    {"test", "Resources/v2/model/test.model.json"},
    {"tuoer", "Resources/v2/托尔/model0.json"},
    {"haru3", "Resources/v3/Haru/Haru.model3.json"},
    {"llny", "Resources/v3/llny/llny.model3.json"},
    {"mao", "Resources/v3/Mao/Mao.model3.json"},
    {"nn", "Resources/v3/绫地宁宁/nn.model3.json"},
    {"acbase", "Resources/v3/ac_base_rem01/ac_base_rem01.model3.json"},
};

}   // namespace

const std::pair<const char*, const char*>* ModelTable() {
    return kModelTable;
}

size_t ModelTableSize() {
    return sizeof(kModelTable) / sizeof(kModelTable[0]);
}

std::string U8ToString(const std::u8string& u8) {
    return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
}

std::string FindUpward(const char* relPath) {
    namespace fs = std::filesystem;
    // relPath 为 UTF-8 字面量，必须经 u8path 转换，否则中文路径按 ACP 误解码
    const fs::path rel = fs::u8path(relPath);
    fs::path cwd = fs::current_path();
    while (true) {
        fs::path candidate = cwd / rel;
        if (fs::exists(candidate))
            // 保持 UTF-8（.string() 在 Windows 会转 ACP 弄坏中文）
            return U8ToString(candidate.u8string());
        if (!cwd.has_parent_path() || cwd == cwd.parent_path())
            return "";
        cwd = cwd.parent_path();
    }
}

std::string ResolveModelPath(const std::string& arg) {
    if (arg.find(".json") != std::string::npos)
        return U8ToString(std::filesystem::absolute(std::filesystem::u8path(arg)).u8string());
    for (size_t i = 0; i < ModelTableSize(); i++) {
        if (arg == ModelTable()[i].first)
            return FindUpward(ModelTable()[i].second);
    }
    fprintf(stderr, "unknown model name: %s\navailable names:", arg.c_str());
    for (size_t i = 0; i < ModelTableSize(); i++)
        fprintf(stderr, " %s", ModelTable()[i].first);
    fprintf(stderr, "\n");
    return "";
}

std::string DefaultModelPath() {
    return FindUpward("Resources/v3/llny/llny.model3.json");
}
