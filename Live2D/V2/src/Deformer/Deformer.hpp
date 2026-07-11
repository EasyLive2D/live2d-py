#pragma once

#include "../Core/ISerializable.hpp"
#include "../Core/Id.hpp"
#include <vector>

namespace live2d {

class DeformerContext;
class ModelContext;
class PivotManager;

class Deformer : public ISerializable {
public:
    static constexpr int DEFORMER_INDEX_NOT_INIT = -2;
    static constexpr int TYPE_ROTATION = 1;
    static constexpr int TYPE_WARP = 2;

    Deformer() = default;

    void read(class BinaryReader& br) override;
    void readOpacity(class BinaryReader& br);

    virtual DeformerContext* init(ModelContext* mc) = 0;
    virtual void setupInterpolate(ModelContext* modelContext, DeformerContext* deformerContext) = 0;
    virtual void interpolateOpacity(ModelContext* mdc, PivotManager* pivotMgr, DeformerContext* bctx, bool& ret);
    virtual bool setupTransform(ModelContext* mc, DeformerContext* dc) = 0;
    virtual void transformPoints(ModelContext* mc, DeformerContext* dc,
                                 const std::vector<float>& srcPoints, std::vector<float>& dstPoints,
                                 int numPoint, int ptOffset, int ptStep) = 0;
    virtual int getType() const = 0;

    void setTargetId(const Id* targetId) { mTargetId = targetId; }
    void setId(const Id* idVal) { mId = idVal; }
    const Id* getTargetId() const { return mTargetId; }
    const Id* getId() const { return mId; }
    bool needTransform() const;

protected:
    const Id* mId = nullptr;
    const Id* mTargetId = nullptr;
    bool mDirty = true;
    std::vector<float> mPivotOpacities;
};

} // namespace live2d
