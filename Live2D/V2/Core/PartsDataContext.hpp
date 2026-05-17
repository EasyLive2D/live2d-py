#pragma once

#include <array>

namespace live2d {

class PartsData;

class PartsDataContext {
public:
    explicit PartsDataContext(PartsData* parts) : mPartsData(parts) {}

    float getPartsOpacity() const { return mPartsOpacity; }
    void setPartsOpacity(float value) { mPartsOpacity = value; }

    void setPartScreenColor(float r, float g, float b, float a);
    void setPartMultiplyColor(float r, float g, float b, float a);

    PartsData* mPartsData = nullptr;
    std::array<float, 4> mScreenColor = {0.0f, 0.0f, 0.0f, 0.0f};
    std::array<float, 4> mMultiplyColor = {1.0f, 1.0f, 1.0f, 0.0f};

private:
    float mPartsOpacity = 1.0f;
};

} // namespace live2d
