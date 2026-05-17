#pragma once

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

    void read(class BinaryReader& br) override;

    std::vector<Deformer*>& getDeformer() { return mDeformerList; }
    std::vector<IDrawData*>& getDrawDataList() { return mDrawDataList; }
    void replacePartsData(PartsData* parts);

private:
    const Id* mId = nullptr;
    std::vector<Deformer*> mDeformerList;
    std::vector<IDrawData*> mDrawDataList;
};

} // namespace live2d
