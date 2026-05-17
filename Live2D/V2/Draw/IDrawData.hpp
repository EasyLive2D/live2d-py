#pragma once

#include "../Core/ISerializable.hpp"
#include "../Core/Id.hpp"
#include <vector>
#include <string>

namespace live2d {

class PivotManager;
class MeshContext;
class ModelContext;
class IDrawContext;

class IDrawData : public ISerializable {
public:
    static constexpr int DEFORMER_INDEX_NOT_INIT = -2;
    static constexpr int DEFAULT_ORDER = 500;
    static constexpr int TYPE_MESH = 2;

    IDrawData() = default;

    void read(class BinaryReader& br) override;

    void setupInterpolate(ModelContext* mc, MeshContext* ctx);
    virtual void setupTransform(ModelContext* mc, IDrawContext* dc = nullptr) = 0;

    const Id* getId() const { return mId; }
    void setId(const Id* value) { mId = value; }
    const Id* getTargetId() const { return mTargetId; }
    void setTargetId(const Id* value) { mTargetId = value; }
    bool needTransform() const {
        return mTargetId != nullptr && *mTargetId != Id::DST_BASE_ID();
    }

    static float getOpacity(MeshContext* ctx);
    static int getDrawOrder(MeshContext* ctx);
    virtual int getType() const = 0;

    std::vector<std::string>* getClipIDList() const { return mClipIDList; }

    static int getTotalMinOrder() { return sTotalMinOrder; }
    static int getTotalMaxOrder() { return sTotalMaxOrder; }

    static int sTotalMinOrder;
    static int sTotalMaxOrder;

protected:

    const Id* mId = nullptr;
    const Id* mTargetId = nullptr;
    PivotManager* mPivotMgr = nullptr;
    int mAverageDrawOrder = 0;
    std::vector<int> mPivotDrawOrders;
    std::vector<float> mPivotOpacities;
    std::vector<std::string>* mClipIDList = nullptr;
};

} // namespace live2d
