#pragma once

namespace live2d {

class Deformer;

class DeformerContext {
public:
    explicit DeformerContext(Deformer* deformer)
        : mDeformer(deformer) {}
    virtual ~DeformerContext() = default;

    Deformer* getDeformer() const { return mDeformer; }

    bool isAvailable() const { return mAvailable; }
    void setAvailable(bool v) { mAvailable = v; }

    bool isOutsideParam() const { return mOutsideParam; }
    void setOutsideParam(bool v) { mOutsideParam = v; }

    float getTotalScale() const { return mTotalScale; }
    void setTotalScale_notForClient(float v) { mTotalScale = v; }

    float getTotalOpacity() const { return mTotalOpacity; }
    void setTotalOpacity(float v) { mTotalOpacity = v; }

    float getInterpolatedOpacity() const { return mInterpolatedOpacity; }
    void setInterpolatedOpacity(float v) { mInterpolatedOpacity = v; }

    int mTmpDeformerIndex = -2;
    int mPartsIndex = 0;

protected:
    Deformer* mDeformer;
    bool mAvailable = true;
    bool mOutsideParam = false;
    float mTotalScale = 1.0f;
    float mTotalOpacity = 1.0f;
    float mInterpolatedOpacity = 1.0f;
};

} // namespace live2d
