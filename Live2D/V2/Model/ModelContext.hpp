#pragma once

#include <vector>
#include "../Core/Id.hpp"

namespace live2d {

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
class DrawParamOpenGL;
class ClippingManagerOpenGL;

class ModelContext {
public:
    static constexpr int NOT_USED_ORDER = -1;
    static constexpr int NO_NEXT = -1;
    static constexpr bool PARAM_UPDATED = true;
    static constexpr float PARAM_FLOAT_MIN = -1000000.0f;
    static constexpr float PARAM_FLOAT_MAX = 1000000.0f;

    explicit ModelContext(ALive2DModel* model);
    ~ModelContext();

    void init();
    void update();
    void preDraw(DrawParamOpenGL* dp);
    void draw(DrawParamOpenGL* dp);

    int getParamIndex(const Id* paramId);
    int getDeformerIndex(const Id* id);
    int extendAndAddParam(const Id* paramId, float defaultVal, float maxVal, float minVal);

    void setParamFloat(int index, float value);
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
    DeformerContext* getDeformerContext(int index) const { return mDeformerContextList[index]; }
    IDrawData* getDrawData(int index) const;
    MeshContext* getDrawContext(int index) const { return mDrawContextList[index]; }
    PartsDataContext* getPartsContext(int index) const { return mPartsContextList[index]; }
    int getDrawDataIndex(const Id* drawDataId) const;
    int getPartsDataIndex(const Id* id) const;

    void setPartsOpacity(int index, float opacity);
    float getPartsOpacity(int index) const;

    void setPartMultiplyColor(int index, float r, float g, float b, float a);
    void setPartScreenColor(int index, float r, float g, float b, float a);

    void setDrawParam(DrawParamOpenGL* dp) { mDpGL = dp; }
    DrawParamOpenGL* getDrawParam() const { return mDpGL; }

    // Temp arrays used by pivot interpolation
    std::vector<int16_t>& getTempPivotTableIndices() { return mTmpPivotTableIndices; }
    std::vector<float>& getTempT() { return mTempTArray; }

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
    std::vector<DeformerContext*> mDeformerContextList;
    std::vector<MeshContext*> mDrawContextList;
    std::vector<PartsDataContext*> mPartsContextList;

private:
    void release();

public:
    ALive2DModel* mModel = nullptr;
    bool mNeedSetup = true;
    int mInitVersion = -1;
    int mNextParamPos = 0;

    DrawParamOpenGL* mDpGL = nullptr;
    ClippingManagerOpenGL* mClipManager = nullptr;

    std::vector<int16_t> mOrderListFirstDrawIndex;
    std::vector<int16_t> mOrderListLastDrawIndex;
    std::vector<int16_t> mNextListDrawIndex;
    std::vector<int16_t> mTmpPivotTableIndices;
    std::vector<float> mTempTArray;

    // Cache for ID lookup
    mutable void* mTmpDrawDataList = nullptr; // TODO: use proper map
};

} // namespace live2d
