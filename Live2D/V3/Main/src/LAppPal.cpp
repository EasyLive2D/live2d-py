/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "LAppPal.hpp"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <Model/CubismMoc.hpp>
#include <Utils/CubismJson.hpp>

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
    SHADER_DIR = path;
    SHADER_DIR += std::filesystem::path::preferred_separator;
    Info("[Pal] Init Shader Dir: %s", SHADER_DIR.c_str());
}

void LAppPal::FixMotionJson(std::string& jsonStr)
{
    using namespace Live2D::Cubism::Framework::Utils;
    CubismJson* json = CubismJson::Create(
        reinterpret_cast<const csmByte*>(jsonStr.data()), static_cast<csmSizeInt>(jsonStr.size()));
    if (!json) return;

    Value& root = json->GetRoot();
    auto* curvesVec = root["Curves"].GetVector();
    if (!curvesVec) { CubismJson::Delete(json); return; }

    const csmInt32 actualCurveCount = static_cast<csmInt32>(curvesVec->GetSize());
    csmInt32 actualSegmentCount = 0;
    csmInt32 actualPointCount = 0;

    enum { Linear = 0, Bezier = 1, Stepped = 2, InverseStepped = 3 };

    Value& curves = root["Curves"];
    for (csmInt32 ci = 0; ci < actualCurveCount; ci++) {
        auto* segVec = curves[ci]["Segments"].GetVector();
        if (!segVec) continue;
        const csmInt32 segLen = static_cast<csmInt32>(segVec->GetSize());
        if (segLen < 2) continue;
        actualPointCount++;
        csmInt32 v = 2;
        while (v < segLen) {
            csmInt32 ident = static_cast<csmInt32>((*segVec)[v]->ToInt());
            if (ident == Bezier) {
                actualPointCount += 3; v += 7;
            } else if (ident == Linear || ident == Stepped || ident == InverseStepped) {
                actualPointCount += 1; v += 3;
            } else break;
            actualSegmentCount++;
        }
    }

    int metaCurveCount = root["Meta"]["CurveCount"].ToInt(-1);
    int metaSegCount   = root["Meta"]["TotalSegmentCount"].ToInt(-1);
    int metaPointCount = root["Meta"]["TotalPointCount"].ToInt(-1);

    CubismJson::Delete(json);

    if (metaCurveCount == actualCurveCount &&
        metaSegCount   == actualSegmentCount &&
        metaPointCount == actualPointCount)
        return;

    auto fixInt = [&](const char* key, int newVal) {
        size_t kp = jsonStr.find(key);
        if (kp == std::string::npos) return;
        size_t colon = jsonStr.find(':', kp);
        if (colon == std::string::npos) return;
        size_t vs = colon + 1;
        while (vs < jsonStr.size() && (jsonStr[vs] == ' ' || jsonStr[vs] == '\t')) vs++;
        size_t ve = vs;
        while (ve < jsonStr.size() && jsonStr[ve] >= '0' && jsonStr[ve] <= '9') ve++;
        if (ve > vs) jsonStr.replace(vs, ve - vs, std::to_string(newVal));
    };

    fixInt("\"CurveCount\"", actualCurveCount);
    fixInt("\"TotalSegmentCount\"", actualSegmentCount);
    fixInt("\"TotalPointCount\"", actualPointCount);
}

void LAppPal::interceptShaderLoading(std::string &filePath)
{
    if (filePath.substr(0, 17) == "FrameworkShaders/") 
    {
        Debug("[Pal] intercept for shader: %s", filePath.c_str());
        filePath = SHADER_DIR + (char)std::filesystem::path::preferred_separator + filePath;
    }
}
