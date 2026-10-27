#pragma once
#include <memory>
#include <unordered_map>

#include "Core/Id.hpp"
#include "ISerializable.hpp"
#include <array>
#include <cstdint>
#include <vector>

namespace Live2D {
namespace V2 {

class ALive2DModel;
class Deformer;
class Drawable;
class Parameter;
class Part;


/**
 * Moc — V2 模型数据层，对标 V3 CubismModel。
 *
 * 集 moc 数据宿主（原 ModelImpl：parts/paramDefSet/画布尺寸）与
 * 运行时上下文（参数值、deformer/drawable context）于一体：
 *  1. Read()：解析 moc 二进制，持有解析数据的所有权
 *  2. Init()：基于 moc 数据构建 deformer/drawable/parts context
 *  3. Update()：参数传播、deformer 链与 drawable 插值/变换
 *  4. Resolve()：update 完成后执行一次，把渲染所需的派生状态
 *     （最终不透明度、可见性、渲染序列）解析进内部缓冲
 *
 * getDrawable* 系列为只读零拷贝视图（语义对齐 CubismModel 同名 getter），
 * 返回指针的有效窗口为：本次 update/resolve 完成 → 下次 update。
 */
class Moc : public ISerializable {
private:
public:
    static constexpr int NOT_USED_ORDER = -1;
    static constexpr int NO_NEXT = -1;

    explicit Moc(Id::IdPool& idPool);
    ~Moc() override;

    void Read(BinaryReader& br) override;
    void Init();
    void Update();
    void Resolve();

    int GetParamIndex(const Id* paramId);
    int GetParamIndex(const std::string& paramIdStr) {
        return GetParamIndex(mIdPool.Resolve(paramIdStr));
    }
    int GetDeformerIndex(const Id* id);

    void SetParamValue(int index, float value);
    void SetParamValue(int index, float value, float weight);
    float GetParamValue(int index) const;
    float GetParamMax(int index) const;
    float GetParamMin(int index) const;
    float GetParamDefault(int index) const;

    // VISIBLE:xxx side-channel：motion 的 VISIBLE 曲线与 pose 子系统的通信
    // 媒介，key 为 part Id*（Id 池保证指针恒等），不物化为参数表条目
    void SetVisibleFlag(const Id* partId, float v);
    float GetVisibleFlag(const Id* partId) const;

    void LoadParam();
    void SaveParam();


    Deformer* GetDeformer(int index) const { return mDeformers[index]; }
    Drawable* GetDrawable(int index) const;
    Part* GetPart(int index) const {
        return (index >= 0 && index < static_cast<int>(mParts.size())) ? mParts[index].get()
                                                                       : nullptr;
    }
    int GetDrawableIndex(const Id* drawableId) const;
    int GetDrawableIndex(const std::string& idStr) const {
        return GetDrawableIndex(mIdPool.Resolve(idStr));
    }
    int GetPartIndex(const Id* id) const;
    int GetPartIndex(const std::string& idStr) const {
        return GetPartIndex(const_cast<const Id*>(mIdPool.Resolve(idStr)));
    }

    void SetPartsOpacity(int index, float opacity);
    float GetPartsOpacity(int index) const;

    void SetPartMultiplyColor(int index, float r, float g, float b, float a);
    void SetPartScreenColor(int index, float r, float g, float b, float a);

    // Temp arrays used by pivot interpolation
    std::vector<int16_t>& GetTempPivotTableIndices() { return mTmpPivotTableIndices; }
    std::vector<float>& GetTempT() { return mTempTArray; }

    int GetCanvasWidth() const { return mCanvasWidth; }
    int GetCanvasHeight() const { return mCanvasHeight; }

    //========================================================
    //  Drawable 只读数据面（对标 CubismModel getter）
    //========================================================

    /// drawable 数量
    int GetDrawableCount() const { return static_cast<int>(mDrawables.size()); }

    /// drawable 顶点数（xy 交错存储，坐标浮点数为 vertexCount*2）
    int GetDrawableVertexCount(int index) const;
    /// drawable 顶点坐标指针（零拷贝，长度为 vertexCount*2）
    const float* GetDrawableVertices(int index) const;
    /// drawable 顶点 UV 指针（零拷贝，长度为 vertexCount*2）
    const float* GetDrawableVertexUvs(int index) const;
    /// drawable 三角形索引数
    int GetDrawableVertexIndexCount(int index) const;
    /// drawable 三角形索引指针（内部为 int16_t 存储，值域非负，按 uint16_t 视图返回）
    const uint16_t* GetDrawableVertexIndices(int index) const;

    /// 最终不透明度（interpolated * parts * base 三重合成，resolve 阶段计算）
    float GetDrawableOpacity(int index) const { return mResolvedOpacity[index]; }
    /// 可见性（!paramOutOfRange && available，resolve 阶段计算）
    bool IsDrawableVisible(int index) const { return mResolvedVisible[index]; }
    /// deformer 变换可用性（裁剪边界计算跳过条件，resolve 阶段快照）
    bool IsDrawableAvailable(int index) const { return mResolvedAvailable[index]; }
    /// 参数越界标记（mask 源渲染跳过条件，resolve 阶段快照）
    bool IsDrawableParamOutside(int index) const { return mResolvedParamOutOfRange[index]; }
    /// 插值后的绘制顺序值
    int GetDrawableRenderOrder(int index) const;
    /// 渲染序列（按渲染顺序排列的可见 drawable 索引，resolve 阶段摊平）
    const int* GetRenderSequence(int& count) const {
        count = static_cast<int>(mRenderSequence.size());
        return mRenderSequence.data();
    }

    /// 纹理编号（texNo < 0 时按旧路径语义兜底为 1）
    int GetDrawableTextureIndex(int index) const;
    /// 颜色合成方式（0 normal / 1 screen / 2 multiply，对应 V3 blendMode 语义）
    int GetDrawableColorComposition(int index) const;
    /// 是否启用背面剔除
    bool GetDrawableCulling(int index) const;
    /// 所属 parts 的 multiply color（RGBA）
    const std::array<float, 4>& GetDrawableMultiplyColor(int index) const;
    /// 所属 parts 的 screen color（RGBA）
    const std::array<float, 4>& GetDrawableScreenColor(int index) const;
    /// 裁剪遮罩 mask drawable 索引列表（init 阶段由 clipID 解析，静态不变）
    const int* GetDrawableMasks(int index, int& count) const {
        count = static_cast<int>(mDrawableMasks[index].size());
        return mDrawableMasks[index].data();
    }

    // Data lists (populated from moc data in Init())
    std::vector<const Id*> mParamIds;
    std::vector<float> mParamValues;
    std::vector<float> mParamMinValues;
    std::vector<float> mParamMaxValues;
    std::vector<float> mParamDefaultValues;
    std::vector<float> mSavedParamValues;
    std::vector<Deformer*> mDeformers;
    std::vector<Drawable*> mDrawables;
    std::vector<std::unique_ptr<Part>> mParts;


private:
    void Release();

public:
    int mCanvasWidth = 0;
    int mCanvasHeight = 0;

    std::vector<int16_t> mOrderListFirstDrawIndex;
    std::vector<int16_t> mOrderListLastDrawIndex;
    std::vector<int16_t> mNextListDrawIndex;
    std::vector<int16_t> mTmpPivotTableIndices;
    std::vector<float> mTempTArray;

    // Id* -> index caches (Ids are interned, so pointer keys are stable;
    // lists only grow/append, so cached indices never go stale).
    std::unordered_map<const Id*, int> mParamIndexCache;
    mutable std::unordered_map<const Id*, int> mDrawableIndexCache;
    mutable std::unordered_map<const Id*, int> mPartIndexCache;

private:
    // Init() 阶段解析的静态值域：全部 drawable 的 pivot draw order 键位
    // [min, max]（order 插值输出为键位间的凸组合，必落此闭区间内，
    // Update() 的 order 桶数组以此为界）
    int mMinDrawOrder = 0;
    int mMaxDrawOrder = 0;

private:
    // Resolve() 解析缓冲（有效窗口：resolve 完成 → 下次 update）
    std::vector<float> mResolvedOpacity;
    std::vector<bool> mResolvedVisible;
    std::vector<bool> mResolvedAvailable;
    std::vector<bool> mResolvedParamOutOfRange;
    // drawable 最终颜色（color override 有效时替换所属 part 颜色）
    std::vector<std::array<float, 4>> mResolvedMultiplyColors;
    std::vector<std::array<float, 4>> mResolvedScreenColors;
    std::vector<int> mRenderSequence;
    // Init() 阶段解析的静态 mask 索引（clipID -> drawable index）
    std::vector<std::vector<int>> mDrawableMasks;

    // Motion→Pose 的 VISIBLE 通信 side-channel（缺失视为 0，等价旧幻影参数初值）
    std::unordered_map<const Id*, float> mVisibleFlags;

    // moc 解析数据所有权（Read() 填充，生命周期 = Moc）
    std::vector<std::unique_ptr<Parameter>> mParameters;
    Id::IdPool& mIdPool;
};

}   // namespace V2
}   // namespace Live2D
