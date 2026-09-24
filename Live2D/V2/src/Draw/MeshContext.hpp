#pragma once

#include "IDrawContext.hpp"
#include <vector>

namespace live2d {

class Mesh;

class MeshContext : public IDrawContext {
public:
    explicit MeshContext(Mesh* drawData) : mDrawData(drawData) {}

    Mesh* getDrawData() const { return mDrawData; }

    Mesh* mDrawData;
    std::vector<float> mInterpolatedPoints;
    std::vector<float> mTransformedPoints;
    float mPartsOpacity = 1.0f;
    float mBaseOpacity = 1.0f;
    float mInterpolatedOpacity = 1.0f;
    int mInterpolatedDrawOrder = 0;
    int mPartsIndex = 0;
    int mTmpDeformerIndex = -2;
    bool mAvailable = true;
    bool mParamOutside = false;
    void* mClipBufPre_clipContext = nullptr;
    int mClipChannelNo = -1;
    float mClipMatrixForDraw[16] = {};
};

} // namespace live2d
