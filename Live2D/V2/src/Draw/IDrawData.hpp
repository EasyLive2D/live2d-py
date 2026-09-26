#pragma once
#include <memory>

#include "../Core/ISerializable.hpp"
#include "../Core/Id.hpp"
#include <string>
#include <vector>

namespace Live2D {
namespace V2 {

class PivotManager;
class MeshContext;
class ModelContext;
class IDrawContext;

class IDrawData : public ISerializable
{
public:
    static constexpr int DEFORMER_INDEX_NOT_INIT = -2;
    static constexpr int DEFAULT_ORDER = 500;
    static constexpr int TYPE_MESH = 2;
    static int sTotalMinOrder;
    static int sTotalMaxOrder;

    IDrawData() = default;
    ~IDrawData() override;

    void read(class BinaryReader& br) override;

    void setupInterpolate(ModelContext* mc, MeshContext* ctx);
    virtual void setupTransform(ModelContext* mc, IDrawContext* dc = nullptr) = 0;

    const Id* getId() const { return mId; }
    void setId(const Id* value) { mId = value; }
    const Id* getTargetId() const { return mTargetId; }
    void setTargetId(const Id* value) { mTargetId = value; }
    bool needTransform() const { return mTargetId != nullptr && *mTargetId != Id::DST_BASE_ID(); }

    static float getOpacity(MeshContext* ctx);
    static int getDrawOrder(MeshContext* ctx);
    virtual int getType() const = 0;

    static void setDrawOrders(const std::vector<int>& orders);
    static int getTotalMinOrder() { return sTotalMinOrder; }
    static int getTotalMaxOrder() { return sTotalMaxOrder; }

    const std::vector<std::string>& getClipIDList() const { return mClipIDList; }


protected:
    const Id* mId = nullptr;
    const Id* mTargetId = nullptr;
    std::unique_ptr<PivotManager> mPivotMgr;
    int mAverageDrawOrder = 0;
    std::vector<int> mPivotDrawOrders;
    std::vector<float> mPivotOpacities;
    std::vector<std::string> mClipIDList;
};

}   // namespace V2
}   // namespace Live2D