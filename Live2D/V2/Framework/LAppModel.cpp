#include "LAppModel.hpp"
#include "../Model/ModelContext.hpp"
#include "L2DEyeBlink.hpp"
#include "L2DPhysics.hpp"
#include "L2DPose.hpp"
#include "L2DPartsParam.hpp"
#include "L2DMotionManager.hpp"
#include "L2DExpressionMotion.hpp"
#include "../Motion/Live2DMotion.hpp"
#include "../Model/Live2DModelOpenGL.hpp"
#include "../Core/ModelImpl.hpp"
#include "../Core/PartsData.hpp"
#include "../Core/PartsDataContext.hpp"
#include "../Draw/MeshContext.hpp"
#include "../Draw/Mesh.hpp"
#include "../Graphics/DrawParamOpenGL.hpp"
#include "../Core/Id.hpp"
#include "../Util/UtSystem.hpp"
#include <stb_image.h>
#include "Log.hpp"
#include <fstream>
#include <vector>
#include <sstream>
#include <cmath>
#include <GL/glew.h>
#include <filesystem>

namespace live2d {

// Helper: read entire file using std::filesystem::u8path for Unicode path support
static std::vector<uint8_t> readFile(const std::string& path) {
    std::filesystem::path fp = std::filesystem::u8path(path);
    std::ifstream f(fp, std::ios::binary | std::ios::ate);
    if (!f) return {};
    auto sz = f.tellg(); f.seekg(0);
    std::vector<uint8_t> data((size_t)sz);
    f.read((char*)data.data(), sz);
    return data;
}

// Simple JSON texture path extractor
static std::vector<std::string> parseTexturePaths(const std::string& json) {
    std::vector<std::string> paths;
    auto pos = json.find("\"textures\"");
    if (pos == std::string::npos) return paths;
    pos = json.find('[', pos);
    if (pos == std::string::npos) return paths;
    auto end = json.find(']', pos);
    if (end == std::string::npos) return paths;
    std::string arr = json.substr(pos + 1, end - pos - 1);
    size_t start = 0;
    while (start < arr.size()) {
        auto q1 = arr.find('"', start);
        if (q1 == std::string::npos) break;
        auto q2 = arr.find('"', q1 + 1);
        if (q2 == std::string::npos) break;
        paths.push_back(arr.substr(q1 + 1, q2 - q1 - 1));
        start = q2 + 1;
    }
    return paths;
}

LAppModel::LAppModel() = default;
LAppModel::~LAppModel() = default;

bool LAppModel::loadModelJson(const std::string& path) {
    // Read JSON (use filesystem for Unicode path support)
    auto jdata = readFile(path);
    if (jdata.empty()) return false;
    std::string json((char*)jdata.data(), jdata.size());

    // Get base directory and load .moc from JSON model field
    std::string baseDir = path.substr(0, path.find_last_of("/\\") + 1);
    std::string mocPath;
    auto modelPos = json.find("\"model\"");
    if (modelPos != std::string::npos) {
        auto q1 = json.find('"', modelPos + 7);
        auto q2 = q1 != std::string::npos ? json.find('"', q1 + 1) : std::string::npos;
        if (q2 != std::string::npos)
            mocPath = baseDir + json.substr(q1 + 1, q2 - q1 - 1);
    }
    if (mocPath.empty()) {
        mocPath = path;
        size_t pos = mocPath.find(".model.json");
        if (pos != std::string::npos) mocPath = mocPath.substr(0, pos) + ".moc";
    }
    auto mocData = readFile(mocPath);
    if (mocData.empty()) return false;
    loadModelData(mocData, 0);
    mModelMatrix.mWidth = (float)mLive2DModel->getCanvasWidth();
    mModelMatrix.mHeight = (float)mLive2DModel->getCanvasHeight();
    Info("Load model: %s", mocPath.c_str());

    // Load textures
    auto texPaths = parseTexturePaths(json);
    for (size_t i = 0; i < texPaths.size(); i++) {
        std::string texPath = baseDir + texPaths[i];
        int w, h, n;
        auto texData = readFile(texPath);
        unsigned char* pixels = texData.empty() ? nullptr :
            stbi_load_from_memory(texData.data(), (int)texData.size(), &w, &h, &n, 4);
        if (!pixels) continue;
        Info("Load texture: %s", texPaths[i].c_str());

        // Only upload textures if GL is ready (glad loaded via glInit)
        extern bool gGLReady;
        if (gGLReady) {
            GLuint texId;
            glGenTextures(1, &texId);
            glBindTexture(GL_TEXTURE_2D, texId);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glGenerateMipmap(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, 0);
            setTexture((int)i, (int)texId);
        } else {
            setTexture((int)i, 0); // Will be loaded when GL is ready
        }
        stbi_image_free(pixels);
    }

    // Apply init parts visibility from JSON
    auto pvPos = json.find("\"init_parts_visible\"");
    if (pvPos != std::string::npos) {
        auto arrStart = json.find('[', pvPos);
        auto arrEnd = json.find(']', arrStart);
        if (arrStart != std::string::npos && arrEnd != std::string::npos) {
            std::string arr = json.substr(arrStart + 1, arrEnd - arrStart - 1);
            size_t p = 0;
            while (p < arr.size()) {
                auto idStart = arr.find("\"id\"", p);
                if (idStart == std::string::npos) break;
                auto v1 = arr.find('"', idStart + 5);
                if (v1 == std::string::npos) break;
                auto v2 = arr.find('"', v1 + 1);
                if (v2 == std::string::npos) break;
                std::string partId = arr.substr(v1 + 1, v2 - v1 - 1);
                auto valStart = arr.find("\"value\"", v2);
                if (valStart == std::string::npos) break;
                auto v3 = arr.find(':', valStart);
                if (v3 == std::string::npos) break;
                float val = (float)std::atof(arr.c_str() + v3 + 1);
                int idx = mLive2DModel->getModelContext()->getPartsDataIndex(&Id::getID(partId));
                if (idx >= 0) setPartOpacity(idx, val);
                p = valStart + 10;
            }
        }
    }

    // Apply init params from JSON
    auto ipPos = json.find("\"init_params\"");
    if (ipPos != std::string::npos) {
        auto arrStart = json.find('[', ipPos);
        auto arrEnd = json.find(']', arrStart);
        if (arrStart != std::string::npos && arrEnd != std::string::npos) {
            std::string arr = json.substr(arrStart + 1, arrEnd - arrStart - 1);
            size_t p = 0;
            while (p < arr.size()) {
                auto idStart = arr.find("\"id\"", p);
                if (idStart == std::string::npos) break;
                auto v1 = arr.find('"', idStart + 5);
                if (v1 == std::string::npos) break;
                auto v2 = arr.find('"', v1 + 1);
                if (v2 == std::string::npos) break;
                std::string paramId = arr.substr(v1 + 1, v2 - v1 - 1);
                auto valStart = arr.find("\"value\"", v2);
                if (valStart == std::string::npos) break;
                auto v3 = arr.find(':', valStart);
                if (v3 == std::string::npos) break;
                float val = (float)std::atof(arr.c_str() + v3 + 1);
                int idx = mLive2DModel->getModelContext()->getParamIndex(&Id::getID(paramId));
                if (idx >= 0) mLive2DModel->setParamFloat(idx, val);
                p = valStart + 10;
            }
        }
    }

    // Load pose file
    auto poseKeyPos = json.find("\"pose\"");
    if (poseKeyPos != std::string::npos) {
        auto q1 = json.find('"', poseKeyPos + 6);
        auto q2 = q1 != std::string::npos ? json.find('"', q1 + 1) : std::string::npos;
        if (q2 != std::string::npos) {
            std::string poseFile = json.substr(q1 + 1, q2 - q1 - 1);
            std::string posePath = baseDir + poseFile;
            auto poseData = readFile(posePath);
            if (!poseData.empty()) {
                Info("Load pose: %s", poseFile.c_str());
                delete mPose;
                mPose = L2DPose::load(poseData);
                // Initialize part indices from model
                for (auto& g : mPose->mMGroups) {
                    for (auto& p : g.parts) {
                        if (!p.id.empty()) {
                            p.partsIndex = mLive2DModel->getModelContext()
                                ->getPartsDataIndex(&Id::getID(p.id));
                            p.paramIndex = mLive2DModel->getModelContext()
                                ->getParamIndex(&Id::getID(p.id));
                        }
                    }
                }
                mPose->updateParam(mLive2DModel);
            }
        }
    }

    // Load motion files
    auto motPos = json.find("\"motions\"");
    if (motPos != std::string::npos) {
        size_t searchPos = json.find('{', motPos);
        if (searchPos != std::string::npos) searchPos++; // enter motions object
        while (searchPos < json.size()) {
            auto gs = json.find('"', searchPos);
            if (gs == std::string::npos) break;
            auto ge = json.find('"', gs + 1);
            if (ge == std::string::npos) break;
            std::string groupName = json.substr(gs + 1, ge - gs - 1);
            // Skip non-group-name strings (e.g. "motions", "file", file names)
            if (groupName == "motions" || groupName == "expressions" ||
                groupName.find('/') != std::string::npos || groupName.find('.') != std::string::npos) {
                searchPos = ge + 1; continue;
            }
            // Find the array start for this group
            auto arrStart = json.find('[', ge);
            if (arrStart == std::string::npos) { searchPos = ge + 1; continue; }
            // Find matching ] to get the group's array
            int depth = 0;
            size_t arrEnd = arrStart;
            for (size_t k = arrStart; k < json.size(); k++) {
                if (json[k] == '[') depth++;
                else if (json[k] == ']') { depth--; if (depth == 0) { arrEnd = k; break; } }
            }
            // Find ALL "file" entries within this group's array
            size_t fileSearch = ge;
            auto& motVec = mMotions[groupName];
            while (true) {
                auto filePos = json.find("\"file\"", fileSearch);
                if (filePos == std::string::npos || filePos >= arrEnd) break;
                auto fv1 = json.find('"', filePos + 7);
                auto fv2 = json.find('"', fv1 + 1);
                if (fv1 == std::string::npos || fv2 == std::string::npos || fv2 >= arrEnd) break;
                std::string motFile = json.substr(fv1 + 1, fv2 - fv1 - 1);
                std::string motPath = baseDir + motFile;
                auto motData = readFile(motPath);
                if (!motData.empty()) {
                    auto* motion = Live2DMotion::load(motData);
                    Info("Load motion: %s", motFile.c_str());
                    motVec.push_back(motion);
                }
                fileSearch = fv2 + 1;
            }
            if (motVec.empty()) mMotions.erase(groupName);
            searchPos = arrEnd + 1;
        }
    }

    // Load expression files
    auto expPos = json.find("\"expressions\"");
    if (expPos != std::string::npos) {
        auto arrStart = json.find('[', expPos);
        if (arrStart != std::string::npos) {
            int depth = 0;
            size_t arrEnd = arrStart;
            for (size_t k = arrStart; k < json.size(); k++) {
                if (json[k] == '[') depth++;
                else if (json[k] == ']') { depth--; if (depth == 0) { arrEnd = k; break; } }
            }
            std::string arr = json.substr(arrStart + 1, arrEnd - arrStart - 1);
            size_t pos = 0;
            while (pos < arr.size()) {
                auto namePos = arr.find("\"name\"", pos);
                auto filePos = arr.find("\"file\"", pos);
                if (namePos == std::string::npos || filePos == std::string::npos) break;

                // Extract name
                auto nq1 = arr.find('"', namePos + 7);
                auto nq2 = arr.find('"', nq1 + 1);
                // Extract file
                auto fq1 = arr.find('"', filePos + 7);
                auto fq2 = arr.find('"', fq1 + 1);

                if (nq1 == std::string::npos || nq2 == std::string::npos ||
                    fq1 == std::string::npos || fq2 == std::string::npos) break;

                std::string expName = arr.substr(nq1 + 1, nq2 - nq1 - 1);
                std::string expFile = arr.substr(fq1 + 1, fq2 - fq1 - 1);
                std::string expPath = baseDir + expFile;
                auto expData = readFile(expPath);
                if (!expData.empty()) {
                    auto* expr = L2DExpressionMotion::load(expData);
                    Info("Load expression: %s", expName.c_str());
                    mExpressions[expName] = expr;
                }
                pos = std::max(nq2, fq2) + 1;
            }
        }
    }

    return true;
}
void LAppModel::resize(int w, int h) {
    mMatrixManager.onResize(w, h);
    mLive2DModel->resize(w, h);
}
void LAppModel::drag(float x, float y) {
    // Convert screen coords to scene coords (match Python MatrixManager.screenToScene)
    float w = (float)mMatrixManager.getWidth();
    float h = (float)mMatrixManager.getHeight();
    float sx = (x - w * 0.5f) * 2.0f / h;
    float sy = (y - h * 0.5f) * -2.0f / h;
    mDragMgr.set(sx, sy);
}
bool LAppModel::isMotionFinished() const { return mMainMotionMgr->isFinished(); }
void LAppModel::setOffset(float dx, float dy) { mMatrixManager.setOffset(dx, dy); }
void LAppModel::setScale(float s) { mMatrixManager.setScale(s); }
void LAppModel::setParameterValue(const std::string& id, float val, float w) {
    int idx = mLive2DModel->getModelContext()->getParamIndex(&Id::getID(id));
    mLive2DModel->setParamFloat(idx, val, w);
    // Match Python: also update savedParamValues so loadParam() doesn't revert
    auto* mc = mLive2DModel->getModelContext();
    if (idx >= 0 && idx < (int)mc->mSavedParamValues.size())
        mc->mSavedParamValues[idx] = mc->mSavedParamValues[idx] * (1.0f - w) + val * w;
}
void LAppModel::addParameterValue(const std::string& id, float val, float w) {
    int idx = mLive2DModel->getModelContext()->getParamIndex(&Id::getID(id));
    auto* mc = mLive2DModel->getModelContext();
    mc->setParamFloat(idx, mc->getParamFloat(idx) + val * w);
}
int LAppModel::getParameterCount() const {
    return (int)mLive2DModel->getModelContext()->mParamValues.size();
}
int LAppModel::getPartCount() const {
    return (int)mLive2DModel->getModelContext()->mPartsDataList.size();
}
std::string LAppModel::getPartId(int index) const {
    auto& parts = mLive2DModel->getModelContext()->mPartsDataList;
    if (index >= 0 && index < (int)parts.size() && parts[index]->getId())
        return parts[index]->getId()->str();
    return "";
}
void LAppModel::setPartOpacity(int index, float val) {
    mLive2DModel->getModelContext()->setPartsOpacity(index, val);
}
void LAppModel::update() {
    mDragMgr.update(0.016f);
    setDrag(mDragMgr.getX(), mDragMgr.getY());

    // Match v2 Python update() flow
    bool updated = false;
    if (mClearFlag) {
        mMainMotionMgr->stopAllMotions();
        if (mPose) {
            for (auto& g : mPose->mMGroups)
                for (auto& p : g.parts) p.initIndex(mLive2DModel);
        }
        mClearFlag = false;
    } else {
        mLive2DModel->getModelContext()->loadParam();
        updated = mMainMotionMgr->updateParam(mLive2DModel);
    }
    mLive2DModel->getModelContext()->saveParam();

    // Python suppresses eye-blink while a main motion is active
    if (!updated && mAutoBlink && mEyeBlink)
        mEyeBlink->updateParam(mLive2DModel);

    // Python skips expression update when no expressions exist
    if (!mExpressions.empty())
        mExpressionMgr->updateParam(mLive2DModel);

    // Drag-based parameter updates (match v2 Python)
    auto* mc = mLive2DModel->getModelContext();
    auto addParam = [&](const char* id, float value, float weight) {
        int idx = mc->getParamIndex(&Id::getID(id));
        if (idx >= 0) {
            float cur = mc->getParamFloat(idx);
            mc->setParamFloat(idx, cur + value * weight);
        }
    };
    addParam("PARAM_ANGLE_X", mDragX * 30, 1);
    addParam("PARAM_ANGLE_Y", mDragY * 30, 1);
    addParam("PARAM_ANGLE_Z", mDragX * mDragY * -30, 1);
    addParam("PARAM_BODY_ANGLE_X", mDragX * 10, 1);
    addParam("PARAM_EYE_BALL_X", mDragX, 1);
    addParam("PARAM_EYE_BALL_Y", mDragY, 1);

    // Auto-breath animation (wall clock time, match v2 Python periods)
    if (mAutoBreath) {
        float t = (float)UtSystem::getUserTimeMSec() / 1000.0f;
        addParam("PARAM_ANGLE_X", 15.0f * sinf(t / 6.5345f), 0.5f);
        addParam("PARAM_ANGLE_Y", 8.0f * sinf(t / 3.5345f), 0.5f);
        addParam("PARAM_ANGLE_Z", 10.0f * sinf(t / 5.5345f), 0.5f);
        addParam("PARAM_BODY_ANGLE_X", 4.0f * sinf(t / 15.5345f), 0.5f);
        int breathIdx = mc->getParamIndex(&Id::getID("PARAM_BREATH"));
        if (breathIdx >= 0)
            mc->setParamFloat(breathIdx, 0.5f + 0.5f * sinf(t / 3.2345f));
    }

    if (mPhysics) mPhysics->updateParam(mLive2DModel);
    if (mPose) mPose->updateParam(mLive2DModel);

}
void LAppModel::draw() {
    // Match v2 Python: process deformer chain in draw(), not update()
    // This allows SetParameterValue between Update() and Draw() to take effect
    mLive2DModel->getModelContext()->update();

    auto* dp = mLive2DModel->getDrawParam();
    auto mvp = mMatrixManager.getMvp(&mModelMatrix);
    dp->setMatrix(mvp.data());
    mLive2DModel->getModelContext()->preDraw(dp);
    mLive2DModel->draw();
}
bool LAppModel::hitTest(const std::string& area, float x, float y) {
    // Convert screen pixels → scene coords (match Python screenToScene)
    float w = (float)mMatrixManager.getWidth();
    float h = (float)mMatrixManager.getHeight();
    float sx = (x - w * 0.5f) * 2.0f / h;
    float sy = (y - h * 0.5f) * -2.0f / h;
    return hitTestSimple(area, sx, sy);
}
void LAppModel::setExpression(const std::string& name) {
    auto it = mExpressions.find(name);
    if (it != mExpressions.end()) {
        Info("Start expression: %s", name.c_str());
        mExpressionMgr->startMotion(it->second, false);
    }
}
void LAppModel::setRandomExpression() {
    if (!mExpressions.empty()) {
        auto it = mExpressions.begin();
        std::advance(it, rand() % mExpressions.size());
        Info("Start random expression: %s", it->first.c_str());
        mExpressionMgr->startMotion(it->second, false);
    }
}
void LAppModel::startMotion(const std::string& group, int no, int priority) {
    auto it = mMotions.find(group);
    if (it != mMotions.end() && !it->second.empty()) {
        if (no < 0 || no >= (int)it->second.size()) no = 0;
        Info("Start motion: group=%s no=%d priority=%d", group.c_str(), no, priority);
        mMainMotionMgr->startMotionPrio(it->second[no], priority);
    }
}
void LAppModel::startRandomMotion(const std::string& group, int priority) {
    if (group.empty()) {
        // Pick a random group from all available motions
        if (mMotions.empty()) return;
        int idx = rand() % (int)mMotions.size();
        auto it = mMotions.begin();
        std::advance(it, idx);
        // Also pick a random motion within that group (match v2 Python)
        int count = (int)it->second.size();
        int no = (count > 1) ? (rand() % count) : 0;
        startMotion(it->first, no, priority);
    } else {
        auto it = mMotions.find(group);
        int count = (it != mMotions.end()) ? (int)it->second.size() : 1;
        int no = (count > 1) ? (rand() % count) : 0;
        startMotion(group, no, priority);
    }
}
void LAppModel::clearMotions() { mClearFlag = true; }
void LAppModel::resetExpression() { mExpressionMgr->stopAllMotions(); }
void LAppModel::resetPose() { if (mPose) { for (auto& g : mPose->mMGroups) for (auto& p : g.parts) p.initIndex(mLive2DModel); } }
void LAppModel::rotate(float deg) { mMatrixManager.rotate(deg); }
float LAppModel::getParameterValue(int index) const {
    return mLive2DModel->getModelContext()->getParamFloat(index);
}
float LAppModel::getParameterMin(int index) const {
    return mLive2DModel->getModelContext()->getParamMin(index);
}
float LAppModel::getParameterMax(int index) const {
    return mLive2DModel->getModelContext()->getParamMax(index);
}
float LAppModel::getParameterDefault(int index) const {
    return mLive2DModel->getModelContext()->getParamDefault(index);
}
std::string LAppModel::getParameterId(int index) const {
    auto& ids = mLive2DModel->getModelContext()->mParamIdList;
    if (index >= 0 && index < (int)ids.size() && ids[index])
        return ids[index]->str();
    return "";
}
void LAppModel::setPartScreenColor(int index, float r, float g, float b, float a) {
    mLive2DModel->getModelContext()->setPartScreenColor(index, r, g, b, a);
}
void LAppModel::setPartMultiplyColor(int index, float r, float g, float b, float a) {
    mLive2DModel->getModelContext()->setPartMultiplyColor(index, r, g, b, a);
}
std::vector<float> LAppModel::getPartScreenColor(int index) const {
    auto* ctx = mLive2DModel->getModelContext()->getPartsContext(index);
    return {ctx->mScreenColor[0], ctx->mScreenColor[1], ctx->mScreenColor[2], ctx->mScreenColor[3]};
}
std::vector<float> LAppModel::getPartMultiplyColor(int index) const {
    auto* ctx = mLive2DModel->getModelContext()->getPartsContext(index);
    return {ctx->mMultiplyColor[0], ctx->mMultiplyColor[1], ctx->mMultiplyColor[2], ctx->mMultiplyColor[3]};
}
static bool isInTriangle(float px, float py,
                          float ax, float ay, float bx, float by, float cx, float cy) {
    float v0x = cx - ax, v0y = cy - ay;
    float v1x = bx - ax, v1y = by - ay;
    float v2x = px - ax, v2y = py - ay;
    float dot00 = v0x * v0x + v0y * v0y;
    float dot01 = v0x * v1x + v0y * v1y;
    float dot02 = v0x * v2x + v0y * v2y;
    float dot11 = v1x * v1x + v1y * v1y;
    float dot12 = v1x * v2x + v1y * v2y;
    float inv = dot00 * dot11 - dot01 * dot01;
    if (inv == 0.0f) return false;
    float u = (dot11 * dot02 - dot01 * dot12) / inv;
    float v = (dot00 * dot12 - dot01 * dot02) / inv;
    return (u >= 0) && (v >= 0) && (u + v <= 1);
}

std::vector<std::string> LAppModel::hitPart(float x, float y, bool topOnly) {
    // Step 1: screen pixels → scene coords (match Python MatrixManager.screenToScene)
    float w = (float)mMatrixManager.getWidth();
    float h = (float)mMatrixManager.getHeight();
    float sx = (x - w * 0.5f) * 2.0f / h;
    float sy = (y - h * 0.5f) * -2.0f / h;

    // Step 2: scene → canvas via MVP inverse (match Python MatrixManager.invertTransform)
    auto mvp = mMatrixManager.getMvp(&mModelMatrix);
    float mx = (sx - mvp[12]) / mvp[0];
    float my = (sy - mvp[13]) / mvp[5];

    auto* mc = mLive2DModel->getModelContext();
    auto& drawCtxs = mc->mDrawContextList;
    auto& nextList = mc->mNextListDrawIndex;
    auto& firstList = mc->mOrderListFirstDrawIndex;
    int range = (int)firstList.size();

    std::vector<std::string> result;

    // Iterate draw orders in reverse
    for (int order = range - 1; order >= 0; order--) {
        if ((int)firstList.size() <= order) continue;
        int idx = firstList[order];
        if (idx < 0 || idx >= (int)drawCtxs.size()) continue;
        while (true) {
            auto* dctx = drawCtxs[idx];
            if (!dctx || !dctx->mAvailable) {
                if (nextList.size() > (size_t)idx) idx = nextList[idx];
                else break;
                if (idx < 0 || idx == 65535) break;
                continue;
            }

            // Check part visibility
            auto* pctx = mc->getPartsContext(dctx->mPartsIndex);
            if (pctx && pctx->mPartsData && !pctx->mPartsData->isVisible()) {
                if (nextList.size() > (size_t)idx) idx = nextList[idx];
                else break;
                if (idx < 0 || idx == 65535) break;
                continue;
            }
            if (pctx && pctx->getPartsOpacity() < 0.1f) {
                if (nextList.size() > (size_t)idx) idx = nextList[idx];
                else break;
                if (idx < 0 || idx == 65535) break;
                continue;
            }

            // Get part ID
            std::string partId;
            if (pctx && pctx->mPartsData && pctx->mPartsData->getId())
                partId = pctx->mPartsData->getId()->str();

            // Skip duplicate parts
            bool dup = false;
            for (auto& r : result) if (r == partId) { dup = true; break; }
            if (dup) {
                if (nextList.size() > (size_t)idx) idx = nextList[idx];
                else break;
                if (idx < 0 || idx == 65535) break;
                continue;
            }

            // Triangle hit test against drawable vertices
            auto& verts = !dctx->mTransformedPoints.empty()
                              ? dctx->mTransformedPoints
                              : dctx->mInterpolatedPoints;
            auto* dd = static_cast<Mesh*>(mc->getDrawData(idx));
            if (dd && !verts.empty()) {
                auto& indices = dd->getIndexArray();
                for (size_t i = 0; i + 2 < indices.size(); i += 3) {
                    int i0 = indices[i] * 2, i1 = indices[i + 1] * 2, i2 = indices[i + 2] * 2;
                    if (i0 < 0 || i1 < 0 || i2 < 0) continue;
                    if ((size_t)i0 + 1 >= verts.size() || (size_t)i1 + 1 >= verts.size() || (size_t)i2 + 1 >= verts.size()) continue;
                    if (isInTriangle(mx, my,
                                     verts[i0], verts[i0 + 1],
                                     verts[i1], verts[i1 + 1],
                                     verts[i2], verts[i2 + 1])) {
                        result.push_back(partId);
                        if (topOnly) return result;
                        break;
                    }
                }
            }

            if (nextList.size() > (size_t)idx) idx = nextList[idx];
            else break;
            if (idx < 0 || idx == 65535) break;
        }
    }
    return result;
}
void LAppModel::setTexture(int no, int texId) {
    auto* dp = mLive2DModel->getDrawParam();
    if (dp) dp->setTexture(no, static_cast<GLuint>(texId));
}
} // namespace live2d
