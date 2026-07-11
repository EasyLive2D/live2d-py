#pragma once
#include <memory>

#include "ISerializable.hpp"
#include "Id.hpp"
#include <vector>

namespace live2d {

class Deformer;
class IDrawData;
class PartsData;

class Avatar final : public ISerializable {
public:
    Avatar() = default;
    ~Avatar() override;

    void read(class BinaryReader& br) override;

    std::vector<std::unique_ptr<Deformer>>& getDeformer() { return mDeformerList; }
    std::vector<std::unique_ptr<IDrawData>>& getDrawDataList() { return mDrawDataList; }
    void replacePartsData(PartsData* parts);

private:
    const Id* mId = nullptr;
    std::vector<std::unique_ptr<Deformer>> mDeformerList;
    std::vector<std::unique_ptr<IDrawData>> mDrawDataList;
};

} // namespace live2d
