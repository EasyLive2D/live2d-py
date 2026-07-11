#pragma once

#include <memory>

namespace live2d {

class ISerializable;

class Live2DObjectFactory {
public:
    static std::unique_ptr<ISerializable> create(int clsNo);
};

} // namespace live2d
