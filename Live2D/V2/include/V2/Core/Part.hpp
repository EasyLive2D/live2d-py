#pragma once
#include <memory>

#include "Deformer.hpp"
#include "Drawable.hpp"
#include "ISerializable.hpp"
#include "Id.hpp"
#include <array>
#include <vector>

namespace Live2D {
namespace V2 {

class Part final : public ISerializable {
public:
    Part() = default;
    ~Part() override;

    void Read(class BinaryReader& br) override;

    /// 初始化运行时状态（Moc::init 阶段调用一次）
    void InitRuntimeState() { SetPartsOpacity(IsVisible() ? 1.0f : 0.0f); }

    bool IsVisible() const { return mVisible; }
    bool IsLocked() const { return mLocked; }
    void SetVisible(bool v) { mVisible = v; }
    void SetLocked(bool v) { mLocked = v; }

    std::vector<std::unique_ptr<Deformer>>& GetDeformers() { return mDeformers; }
    std::vector<std::unique_ptr<Drawable>>& GetDrawables() { return mDrawables; }
    const Id* GetId() const { return mId; }
    void SetId(const Id* idVal) { mId = idVal; }

    // ---- 运行时状态（原 PartsDataContext）----
    float GetPartsOpacity() const { return mPartsOpacity; }
    void SetPartsOpacity(float value) { mPartsOpacity = value; }

    void SetPartScreenColor(float r, float g, float b, float a) { mScreenColor = {r, g, b, a}; }
    void SetPartMultiplyColor(float r, float g, float b, float a) { mMultiplyColor = {r, g, b, a}; }

    std::array<float, 4> mScreenColor = {0.0f, 0.0f, 0.0f, 0.0f};
    std::array<float, 4> mMultiplyColor = {1.0f, 1.0f, 1.0f, 0.0f};

private:
    bool mVisible = true;
    bool mLocked = false;
    const Id* mId = nullptr;
    std::vector<std::unique_ptr<Deformer>> mDeformers;
    std::vector<std::unique_ptr<Drawable>> mDrawables;

    float mPartsOpacity = 1.0f;
};
}   // namespace V2
}   // namespace Live2D
