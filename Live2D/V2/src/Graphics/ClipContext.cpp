#include "ClipContext.hpp"

namespace Live2D {
namespace V2 {

ClipContext::ClipContext(ModelContext* mc, const std::vector<std::string>& clipIDs)
    : mClipIDList(clipIDs)
{
    (void)mc;
}

void ClipContext::addClippedDrawData(const std::string& drawId, int drawIdx)
{
    (void)drawId;
    mClippedDrawIndexList.push_back(drawIdx);
}

}   // namespace V2
}   // namespace Live2D
