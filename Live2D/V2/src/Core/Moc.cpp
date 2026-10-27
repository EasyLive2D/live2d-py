#include "Moc.hpp"
#include "DEF.hpp"
#include "BinaryReader.hpp"
#include "Id.hpp"
#include "Parameter.hpp"
#include "Part.hpp"
#include "Deformer.hpp"
#include "Drawable.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace Live2D {
namespace V2 {

Moc::Moc(Id::IdPool& idPool): mIdPool(idPool) {
    mTmpPivotTableIndices.resize(PIVOT_TABLE_SIZE);
    mTempTArray.resize(MAX_INTERPOLATION);
}

Moc::~Moc() {
    Release();
}

void Moc::Release() {
    mDeformerList.clear();
    mDrawableList.clear();
    mPartsList.clear();
}

void Moc::Read(BinaryReader& br) {
    // moc 流中参数表为 class 137 的包装对象（V2 遗留的 ParamDefSet，纯数组容器，
    // V3 无对应概念）：跳过包装头直接读取参数数组；并补偿一个对象表槽位
    // （原包装对象 read 完成后会被 push 一次），保持 OBJECT_REF 索引对齐。
    int paramTableType = br.ReadType();
    if (paramTableType == 0) {
        br.AlignObjectTable(nullptr);
    } else {
        if (paramTableType != 137) {
            LOGE("Unexpected param table class id: %d", paramTableType);
            std::abort();
        }
        auto rawParams = br.ReadObject<std::vector<Parameter*>>();
        mParameters.reserve(rawParams.size());
        for (auto* p : rawParams)
            // assert that params in mParameters cannot be nullptr
            mParameters.emplace_back(p);
        br.AlignObjectTable(nullptr);
    }
    auto rawParts = br.ReadObject<std::vector<Part*>>();
    mParts.reserve(rawParts.size());
    for (auto* p : rawParts)
        mParts.emplace_back(p);
    mCanvasWidth = br.ReadInt32();
    mCanvasHeight = br.ReadInt32();
}

void Moc::Init() {
    mInitVersion++;
    if (!mPartsList.empty())
        Release();

    std::vector<Deformer*> allDefs;

    for (auto& part : mParts) {
        mPartsList.push_back(part.get());
        part->InitRuntimeState();

        auto& baseDefs = part->GetDeformers();
        for (auto& d : baseDefs) {
            d->InitRuntimeState();
            d->mPartsIndex = static_cast<int>(mPartsList.size()) - 1;
            allDefs.push_back(d.get());
        }

        auto& drawables = part->GetDrawables();
        for (auto& dd : drawables) {
            dd->InitRuntimeState();
            dd->mPartsIndex = static_cast<int>(mPartsList.size()) - 1;
            mDrawableList.push_back(dd.get());
        }
    }

    // DAG sort deformers
    size_t nDef = allDefs.size();
    while (true) {
        bool progress = false;
        for (size_t i = 0; i < nDef; i++) {
            auto* d = allDefs[i];
            if (!d)
                continue;
            auto* target = d->GetTargetId();
            if (!target || target == Id::DST_BASE_ID() || GetDeformerIndex(target) >= 0) {
                mDeformerList.push_back(d);
                allDefs[i] = nullptr;
                progress = true;
            }
        }
        if (!progress)
            break;
    }

    // 参数表一次性构建：预分配后直接填充（加载后不可变，无运行期扩展）
    const size_t nParam = mParameters.size();
    mParamIdList.resize(nParam);
    mParamValues.resize(nParam);
    mParamMinValues.resize(nParam);
    mParamMaxValues.resize(nParam);
    mParamDefaultValues.resize(nParam);
    mSavedParamValues.resize(nParam);
    for (size_t i = 0; i < mParameters.size(); i++) {
        auto* p = mParameters[i].get();
        mParamIdList[i] = p->GetId();
        mParamValues[i] = p->GetDefaultValue();
        mParamMinValues[i] = p->GetMinValue();
        mParamMaxValues[i] = p->GetMaxValue();
        mParamDefaultValues[i] = p->GetDefaultValue();
        mSavedParamValues[i] = p->GetDefaultValue();
    }

    // 静态 mask 索引解析：clipID 列表 -> mask drawable 索引（moc 数据，init 后不变）
    mDrawableMasks.clear();
    mDrawableMasks.resize(mDrawableList.size());
    for (size_t i = 0; i < mDrawableList.size(); i++) {
        for (auto& cid : mDrawableList[i]->GetClipIDList()) {
            int maskIdx = GetDrawableIndex(cid);
            if (maskIdx >= 0)
                mDrawableMasks[i].push_back(maskIdx);
        }
    }

    // 全部 drawable 的 pivot draw order 键位值域（等价旧 Drawable 静态累积，
    // 收敛为 per-model 实例状态，消除跨模型残留）
    mMinDrawOrder = mMaxDrawOrder = Drawable::DEFAULT_ORDER;
    for (auto* d : mDrawableList) {
        for (int order : d->GetPivotDrawOrders()) {
            if (order < mMinDrawOrder)
                mMinDrawOrder = order;
            if (order > mMaxDrawOrder)
                mMaxDrawOrder = order;
        }
    }
}

void Moc::Update() {
    int nDef = static_cast<int>(mDeformerList.size());
    int nDraw = static_cast<int>(mDrawableList.size());
    int minOrder = mMinDrawOrder;
    int maxOrder = mMaxDrawOrder;
    int range = maxOrder - minOrder + 1;

    if (static_cast<int>(mOrderListFirstDrawIndex.size()) < range) {
        mOrderListFirstDrawIndex.resize(range);
        mOrderListLastDrawIndex.resize(range);
    }
    for (int i = 0; i < range; i++) {
        mOrderListFirstDrawIndex[i] = NOT_USED_ORDER;
        mOrderListLastDrawIndex[i] = NOT_USED_ORDER;
    }
    if (static_cast<int>(mNextListDrawIndex.size()) < nDraw)
        mNextListDrawIndex.resize(nDraw);
    for (int i = 0; i < nDraw; i++)
        mNextListDrawIndex[i] = NO_NEXT;

    for (int i = 0; i < nDef; i++) {
        mDeformerList[i]->SetupInterpolate(this);
        mDeformerList[i]->SetupTransform(this);
    }

    for (int i = 0; i < nDraw; i++) {
        auto* dd = mDrawableList[i];
        dd->SetupInterpolate(this);
        if (dd->mParamOutside)
            continue;

        dd->SetupTransform(this);
        int order = static_cast<int>(std::floor(dd->GetDrawOrder() - minOrder));
        int lastIdx = mOrderListLastDrawIndex[order];
        if (lastIdx == NOT_USED_ORDER)
            mOrderListFirstDrawIndex[order] = i;
        else
            mNextListDrawIndex[lastIdx] = i;
        mOrderListLastDrawIndex[order] = i;
    }
}

void Moc::Resolve() {
    int nDraw = static_cast<int>(mDrawableList.size());
    if (static_cast<int>(mResolvedOpacity.size()) != nDraw) {
        mResolvedOpacity.resize(nDraw);
        mResolvedVisible.resize(nDraw);
        mResolvedAvailable.resize(nDraw);
        mResolvedParamOutside.resize(nDraw);
        mResolvedMultiplyColors.resize(nDraw);
        mResolvedScreenColors.resize(nDraw);
    }

    for (int i = 0; i < nDraw; i++) {
        auto* dd = mDrawableList[i];
        mResolvedVisible[i] = !dd->mParamOutside && dd->mAvailable;
        mResolvedAvailable[i] = dd->mAvailable;
        mResolvedParamOutside[i] = dd->mParamOutside;
        // drawable 级颜色覆盖优先（对齐 V3 per-drawable override 语义），
        // 未覆盖时回落所属 part 颜色
        auto* part = mPartsList[dd->mPartsIndex];
        mResolvedMultiplyColors[i] = dd->HasColorOverride() ? dd->mOverrideMultiplyColor
                                                            : part->mMultiplyColor;
        mResolvedScreenColors[i] = dd->HasColorOverride() ? dd->mOverrideScreenColor
                                                          : part->mScreenColor;
        // 与旧路径 Mesh::draw 相同的三重合成与运算顺序（保证逐位一致）
        mResolvedOpacity[i] = dd->mInterpolatedOpacity *
                              part->GetPartsOpacity() * dd->mBaseOpacity;
    }

    // 摊平渲染序列：复刻 renderer 旧遍历逻辑（order 链 + available 过滤）
    mRenderSequence.clear();
    for (size_t oi = 0; oi < mOrderListFirstDrawIndex.size(); oi++) {
        int idx = mOrderListFirstDrawIndex[oi];
        if (idx == NOT_USED_ORDER)
            continue;
        while (true) {
            if (mResolvedVisible[idx])
                mRenderSequence.push_back(idx);
            int next = mNextListDrawIndex[idx];
            if (next <= idx || next == NO_NEXT)
                break;
            idx = next;
        }
    }
}

int Moc::GetDrawableVertexCount(int index) const {
    return mDrawableList[index]->GetNumPoints();
}
const float* Moc::GetDrawableVertices(int index) const {
    auto* dd = mDrawableList[index];
    return !dd->mTransformedPoints.empty() ? dd->mTransformedPoints.data()
                                           : dd->mInterpolatedPoints.data();
}
const float* Moc::GetDrawableVertexUvs(int index) const {
    return mDrawableList[index]->GetUvs().data();
}
int Moc::GetDrawableVertexIndexCount(int index) const {
    return static_cast<int>(mDrawableList[index]->GetIndexArray().size());
}
const uint16_t* Moc::GetDrawableVertexIndices(int index) const {
    return reinterpret_cast<const uint16_t*>(mDrawableList[index]->GetIndexArray().data());
}
int Moc::GetDrawableRenderOrder(int index) const {
    return mDrawableList[index]->GetDrawOrder();
}
int Moc::GetDrawableTextureIndex(int index) const {
    int texNo = mDrawableList[index]->GetTextureNo();
    return texNo < 0 ? 1 : texNo;
}
int Moc::GetDrawableColorComposition(int index) const {
    return mDrawableList[index]->GetColorCompositionType();
}
bool Moc::GetDrawableCulling(int index) const {
    return mDrawableList[index]->GetCulling();
}
const std::array<float, 4>& Moc::GetDrawableMultiplyColor(int index) const {
    return mResolvedMultiplyColors[index];
}
const std::array<float, 4>& Moc::GetDrawableScreenColor(int index) const {
    return mResolvedScreenColors[index];
}

int Moc::GetParamIndex(const Id* paramId) {
    auto it = mParamIndexCache.find(paramId);
    if (it != mParamIndexCache.end())
        return it->second;
    for (size_t i = 0; i < mParamIdList.size(); i++)
        if (mParamIdList[i] == paramId) {
            mParamIndexCache.emplace(paramId, static_cast<int>(i));
            return static_cast<int>(i);
        }
    // 参数表装载后不可变：未知 id 返回 -1（写端跳过、读端取默认值），
    // 不再物化幻影参数。-1 结果同样进缓存（表不会再增长，缓存不会失效）。
    mParamIndexCache.emplace(paramId, -1);
    return -1;
}

int Moc::GetDeformerIndex(const Id* id) {
    for (int i = static_cast<int>(mDeformerList.size()) - 1; i >= 0; i--)
        if (mDeformerList[i] && mDeformerList[i]->GetId() == id)
            return i;
    return -1;
}

void Moc::SetParamValue(int index, float value) {
    if (index < 0)
        return;
    if (value < mParamMinValues[index])
        value = mParamMinValues[index];
    if (value > mParamMaxValues[index])
        value = mParamMaxValues[index];
    mParamValues[index] = value;
}

void Moc::SetParamValue(int index, float value, float weight) {
    if (index < 0)
        return;
    if (weight <= 0)
        return;
    if (weight >= 1) {
        SetParamValue(index, value);
        return;
    }
    float v = mParamValues[index] * (1.0f - weight) + value * weight;
    SetParamValue(index, v);
}

float Moc::GetParamValue(int index) const {
    return (index >= 0 && index < (int)mParamValues.size()) ? mParamValues[index] : 0;
}

// VISIBLE side-channel：Motion 写、Pose 读，缺失视为 0（等价旧幻影参数初值）
void Moc::SetVisibleFlag(const Id* partId, float v) {
    mVisibleFlags[partId] = v;
}

float Moc::GetVisibleFlag(const Id* partId) const {
    auto it = mVisibleFlags.find(partId);
    return it != mVisibleFlags.end() ? it->second : 0.0f;
}

float Moc::GetParamMax(int index) const {
    return mParamMaxValues[index];
}
float Moc::GetParamMin(int index) const {
    return mParamMinValues[index];
}
float Moc::GetParamDefault(int index) const {
    return mParamDefaultValues[index];
}

void Moc::LoadParam() {
    memcpy(mParamValues.data(), mSavedParamValues.data(), mSavedParamValues.size());
}
void Moc::SaveParam() {
    for (size_t i = 0; i < mParamValues.size(); i++)
        mSavedParamValues[i] = mParamValues[i];
    memcpy(mSavedParamValues.data(), mParamValues.data(), mParamValues.size());
}

Drawable* Moc::GetDrawable(int index) const {
    return (index >= 0 && index < (int)mDrawableList.size()) ? mDrawableList[index] : nullptr;
}
int Moc::GetDrawableIndex(const Id* drawableId) const {
    auto it = mDrawableIndexCache.find(drawableId);
    if (it != mDrawableIndexCache.end())
        return it->second;
    for (int i = (int)mDrawableList.size() - 1; i >= 0; i--)
        if (mDrawableList[i] && mDrawableList[i]->GetId() == drawableId) {
            mDrawableIndexCache.emplace(drawableId, i);
            return i;
        }
    mDrawableIndexCache.emplace(drawableId, -1);
    return -1;
}
int Moc::GetPartIndex(const Id* id) const {
    auto it = mPartIndexCache.find(id);
    if (it != mPartIndexCache.end())
        return it->second;
    for (int i = (int)mPartsList.size() - 1; i >= 0; i--)
        if (mPartsList[i] && mPartsList[i]->GetId() == id) {
            mPartIndexCache.emplace(id, i);
            return i;
        }
    mPartIndexCache.emplace(id, -1);
    return -1;
}
void Moc::SetPartsOpacity(int i, float op) {
    mPartsList[i]->SetPartsOpacity(op);
}
float Moc::GetPartsOpacity(int i) const {
    return mPartsList[i]->GetPartsOpacity();
}
void Moc::SetPartMultiplyColor(int i, float r, float g, float b, float a) {
    if (i < 0 || i >= static_cast<int>(mPartsList.size()))
        return;
    mPartsList[i]->SetPartMultiplyColor(r, g, b, a);
    // 后写覆盖先写：清除该 part 下 drawable 的颜色覆盖，回落新 part 颜色
    for (auto* dd : mDrawableList)
        if (dd->mPartsIndex == i)
            dd->ClearColorOverride();
}
void Moc::SetPartScreenColor(int i, float r, float g, float b, float a) {
    if (i < 0 || i >= static_cast<int>(mPartsList.size()))
        return;
    mPartsList[i]->SetPartScreenColor(r, g, b, a);
    for (auto* dd : mDrawableList)
        if (dd->mPartsIndex == i)
            dd->ClearColorOverride();
}

}   // namespace V2
}   // namespace Live2D