/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "LAppPal.hpp"
#include <cstdio>
#include <iostream>
#include <fstream>
#include <Model/CubismMoc.hpp>

#include <Log.hpp>

#include <filesystem>

using namespace Csm;

std::string LAppPal::SHADER_DIR = "";

csmByte* LAppPal::LoadFileAsBytes(const std::string filePath, csmSizeInt* outSize)
{
    //filePath;//
    std::string pathStr = filePath;
    interceptShaderLoading(pathStr);
    std::filesystem::path path = std::filesystem::u8path(pathStr);

    size_t size = 0;
    if (std::filesystem::exists(path))
    {
        size = std::filesystem::file_size(path);
        if (size == 0)
        {
            Info("Stat succeeded but file size is zero. path:%s", pathStr.c_str());
            return NULL;
        }
    }
    else
    {
        Info("Stat failed. errno:%d path:%s", errno, pathStr.c_str());
        return NULL;
    }

    std::fstream file;
    file.open(path, std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        Info("File open failed. path:%s", pathStr.c_str());
        return NULL;
    }

    char* buf = new char[size];
    file.read(buf, size);
    file.close();

    if (outSize)
    {
        *outSize = static_cast<unsigned int>(size);
    }

    return reinterpret_cast<csmByte*>(buf);
}

void LAppPal::ReleaseBytes(csmByte* byteData)
{
    delete[] byteData;
}

void LAppPal::PrintLn(const Csm::csmChar *message)
{
    Info(message);
}

double LAppPal::GetCurrentTimePoint()
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

void LAppPal::InitShaderDir(const std::string& path)
{
    SHADER_DIR = std::move(path);
    SHADER_DIR += std::filesystem::path::preferred_separator;
    Info("[Pal] Init Shader Dir: %s", SHADER_DIR.c_str());
}

void LAppPal::interceptShaderLoading(std::string &filePath)
{
    if (filePath.substr(0, 17) == "FrameworkShaders/") 
    {
        Debug("[Pal] intercept for shader: %s", filePath.c_str());
        filePath = SHADER_DIR + (char)std::filesystem::path::preferred_separator + filePath;
    }
}
