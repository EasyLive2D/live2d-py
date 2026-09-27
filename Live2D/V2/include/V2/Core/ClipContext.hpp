#pragma once

#include <array>
#include <string>
#include <vector>

namespace Live2D {
namespace V2 {

class ModelContext;

class ClipContext
{
public:
    ClipContext(ModelContext* mc, const std::vector<std::string>& clipIDs);

    void addClippedDrawData(const std::string& drawId, int drawIdx);

    std::vector<std::string> mClipIDList;
    std::vector<int> mClippedDrawIndexList;
    std::vector<int> mClippingMaskDrawIndexList;
    bool mIsUsing = false;
    int mLayoutChannelNo = 0;
    std::array<float, 4> mLayoutBounds = {0, 0, 1, 1};
    std::array<float, 4> mAllClippedDrawRect = {0, 0, 0, 0};
    std::array<float, 16> mMatrixForMask{};
    std::array<float, 16> mMatrixForDraw{};
};

}   // namespace V2
}   // namespace Live2D