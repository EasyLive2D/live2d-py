#pragma once

namespace Live2D {
namespace V2 {

class BinaryReader;

class ISerializable
{
public:
    virtual ~ISerializable() = default;
    virtual void read(BinaryReader& br) = 0;
};

}   // namespace V2
}   // namespace Live2D