#pragma once
#include <memory>

#include "ISerializable.hpp"
#include "Id.hpp"
#include <vector>
#include "../Draw/IDrawData.hpp"
#include "../Deformer/Deformer.hpp"

namespace live2d {

class PartsDataContext;

class PartsData final : public ISerializable {
public:
    PartsData() = default;
    ~PartsData() override;

    void read(class BinaryReader& br) override;

    PartsDataContext* init();

    bool isVisible() const { return mVisible; }
    bool isLocked() const { return mLocked; }
    void setVisible(bool v) { mVisible = v; }
    void setLocked(bool v) { mLocked = v; }

    void setDeformer(std::vector<std::unique_ptr<Deformer>>&& list) { mDeformerList = std::move(list); }
    void setDrawData(std::vector<std::unique_ptr<IDrawData>>&& list) { mDrawDataList = std::move(list); }

    std::vector<std::unique_ptr<Deformer>>& getDeformer() { return mDeformerList; }
    std::vector<std::unique_ptr<IDrawData>>& getDrawData() { return mDrawDataList; }
    const Id* getId() const { return mId; }
    void setId(const Id* idVal) { mId = idVal; }

private:
    bool mVisible = true;
    bool mLocked = false;
    const Id* mId = nullptr;
    std::vector<std::unique_ptr<Deformer>> mDeformerList;
    std::vector<std::unique_ptr<IDrawData>> mDrawDataList;
};

} // namespace live2d
