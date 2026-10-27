#pragma once
#include "AMotion.hpp"
#include "ExpressionParam.hpp"
#include "Moc.hpp"
#include <cstdint>
#include <vector>

namespace Live2D {
namespace V2 {
class ExpressionMotion : public AMotion {
public:
    ExpressionMotion();
    void UpdateParam(Moc* model, float timeSec, float weight) override;
    float GetDurationSec() const override { return 1.0f; }
    bool IsLoop() const override { return false; }
    static ExpressionMotion* Load(const std::vector<uint8_t>& data, Id::IdPool& idPool);
    std::vector<ExpressionParam> mParams;
};
}   // namespace V2
}   // namespace Live2D
