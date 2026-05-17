#pragma once

#include "ISerializable.hpp"
#include "Id.hpp"
#include <vector>

namespace live2d {

class PartsDataContext;
class Deformer;
class IDrawData;

class PartsData final : public ISerializable {
public:
    PartsData() = default;

    void read(class BinaryReader& br) override;

    PartsDataContext* init();

    bool isVisible() const { return mVisible; }
    bool isLocked() const { return mLocked; }
    void setVisible(bool v) { mVisible = v; }
    void setLocked(bool v) { mLocked = v; }

    void setDeformer(std::vector<Deformer*>& list) { mDeformerList = list; }
    void setDrawData(std::vector<IDrawData*>& list) { mDrawDataList = list; }

    std::vector<Deformer*>& getDeformer() { return mDeformerList; }
    std::vector<IDrawData*>& getDrawData() { return mDrawDataList; }
    const Id* getId() const { return mId; }
    void setId(const Id* idVal) { mId = idVal; }

private:
    bool mVisible = true;
    bool mLocked = false;
    const Id* mId = nullptr;
    std::vector<Deformer*> mDeformerList;
    std::vector<IDrawData*> mDrawDataList;
};

} // namespace live2d
