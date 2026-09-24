#pragma once

namespace live2d {

class BinaryReader;

class ISerializable {
public:
    virtual ~ISerializable() = default;
    virtual void read(BinaryReader& br) = 0;
};

} // namespace live2d
