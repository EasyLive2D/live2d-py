#pragma once
#include <memory>
#include <unordered_map>

#include "../Core/Id.hpp"
#include <vector>

namespace Live2D {
namespace V2 {

class ALive2DModel;
class ModelImpl;
class Deformer;
class DeformerContext;
class IDrawData;
class IDrawContext;
class MeshContext;
class PartsData;
class PartsDataContext;
class PivotManager;
class ParamPivots;


class ModelContext
{
public:
    static constexpr int NOT_USED_ORDER = -1;
    static constexpr int NO_NEXT = -1;
    static constexpr bool PARAM_UPDATED = true;
    static constexpr float PARAM_FLOAT_MIN = -1000000.0f;
    static constexpr float PARAM_FLOAT_MAX = 1000000.0f;

    explicit ModelContext();
    ~ModelContext();

    void init(ModelImpl* modelImpl);
    void update();

    int getParamIndex(const Id* paramId);
    int getDeformerIndex(const Id* id);
    int extendAndAddParam(const Id* paramId, float defaultVal, float maxVal, float minVal);

    void setParamFloat(int index, float value);
    void setParamFloat(int index, float value, float weight);
    float getParamFloat(int index) const;
    float getParamMax(int index) const;
    float getParamMin(int index) const;
    float getParamDefault(int index) const;
    bool isParamUpdated(int index) const;

    void loadParam();
    void saveParam();

    int getInitVersion() const { return mInitVersion; }
    bool requireSetup() const { return mNeedSetup; }

    Deformer* getDeformer(int index) const { return mDeformerList[index]; }
    DeformerContext* getDeformerContext(int index) const
    {
        return mDeformerContextList[index].get();
    }
    IDrawData* getDrawData(int index) const;
    MeshContext* getDrawContext(int index) const { return mDrawContextList[index].get(); }
    PartsDataContext* getPartsContext(int index) const { return mPartsContextList[index].get(); }
    int getDrawDataIndex(const Id* drawDataId) const;
    int getPartsDataIndex(const Id* id) const;

    void setPartsOpacity(int index, float opacity);
    float getPartsOpacity(int index) const;

    void setPartMultiplyColor(int index, float r, float g, float b, float a);
    void setPartScreenColor(int index, float r, float g, float b, float a);

    // Temp arrays used by pivot interpolation
    std::vector<int16_t>& getTempPivotTableIndices() { return mTmpPivotTableIndices; }
    std::vector<float>& getTempT() { return mTempTArray; }

    int getCanvasWidth() const { return mCanvasWidth; }
    int getCanvasHeight() const { return mCanvasHeight; }

    // Data lists (populated from ModelImpl)
    std::vector<const Id*> mParamIdList;
    std::vector<float> mParamValues;
    std::vector<float> mLastParamValues;
    std::vector<float> mParamMinValues;
    std::vector<float> mParamMaxValues;
    std::vector<float> mParamDefaultValues;
    std::vector<float> mSavedParamValues;
    std::vector<bool> mUpdatedParamFlags;
    std::vector<Deformer*> mDeformerList;
    std::vector<IDrawData*> mDrawDataList;
    std::vector<PartsData*> mPartsDataList;
    std::vector<std::unique_ptr<DeformerContext>> mDeformerContextList;
    std::vector<std::unique_ptr<MeshContext>> mDrawContextList;
    std::vector<std::unique_ptr<PartsDataContext>> mPartsContextList;

private:
    void release();

public:
    bool mNeedSetup = true;
    int mInitVersion = -1;
    int mNextParamPos = 0;

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
    mutable std::unordered_map<const Id*, int> mDrawDataIndexCache;
    mutable std::unordered_map<const Id*, int> mPartsDataIndexCache;
};

}   // namespace V2
}   // namespace Live2D
