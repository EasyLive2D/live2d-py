#pragma once
#include <cstdint>
#include <vector>
#include <string>
namespace live2d {
class ModelContext;
struct PartData {
    int partsIndex = -1; int paramIndex = -1;
    std::string id;
    std::vector<PartData> link;
    void initIndex(ModelContext* context);
};
struct PosePartGroup { std::vector<PartData> parts; };
class L2DPose {
public:
    L2DPose();
    void updateParam(ModelContext* context);
    void initParam(ModelContext* context);
    static L2DPose* load(const std::vector<uint8_t>& data);
public:
    std::vector<PosePartGroup> mMGroups;
private:
    float mLastTime = 0;
};
} // namespace live2d
