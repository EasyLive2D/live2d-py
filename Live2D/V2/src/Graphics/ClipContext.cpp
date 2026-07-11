#include "ClipContext.hpp"

namespace live2d {

ClipContext::ClipContext(ModelContext* mc, const std::vector<std::string>& clipIDs)
    : mClipIDList(clipIDs) {
    (void)mc;
}

void ClipContext::addClippedDrawData(const std::string& drawId, int drawIdx) {
    (void)drawId;
    mClippedDrawIndexList.push_back(drawIdx);
}

} // namespace live2d
