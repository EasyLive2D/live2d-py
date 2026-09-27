#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace Live2D {
namespace V2 {
class ModelContext;
struct PartData {
    int partsIndex = -1;
    int paramIndex = -1;
    std::string id;
    std::vector<PartData> link;
    void initIndex(ModelContext* context);
};
struct PosePartGroup {
    std::vector<PartData> parts;
};
class L2DPose {
public:
    L2DPose();
    // 时间由外部传入（时钟只由 Model 管理）: dtSec 为本次时间步长（秒）
    void updateParam(ModelContext* context, float dtSec);
    void initParam(ModelContext* context);
    static L2DPose* load(const std::vector<uint8_t>& data);

public:
    std::vector<PosePartGroup> mMGroups;

private:

};
}   // namespace V2
}   // namespace Live2D