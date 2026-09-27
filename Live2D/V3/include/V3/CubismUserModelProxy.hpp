/**
 * @brief CubismUserModelProxy.hpp
 * @author Arkueid
 * @date 2026/09/27
 * @note SDK 侧模型聚合 CubismUserModel 的代理（SDK 原样，不修改）
 *
 * - SDK 升级时 CubismUserModel 的构造/析构/加载逻辑自动跟随，不需同步
 * - 用 using 导出无 getter 的 protected 成员；IsHit/LoadMotion 两个虚函数钩子
 *   签名由编译器对照 SDK 检查，SDK 变化时立即报错
 */

#pragma once

#include <Model/CubismUserModel.hpp>
#include <Motion/ACubismMotion.hpp>

using namespace Csm;

namespace Live2D {
namespace V3 {

class CubismUserModelProxy : public Csm::CubismUserModel {
public:
    // -> 直通 SDK 模型: mProxy->Xxx() == GetModel()->Xxx()
    // 注意: 与 SDK GetModel() const 一致，const 版本也返回非 const 指针（SDK 方法大多非 const）
    CubismModel* operator->() { return GetModel(); }
    CubismModel* operator->() const { return GetModel(); }

    using Csm::CubismUserModel::_motionManager;
    using Csm::CubismUserModel::_expressionManager;
    using Csm::CubismUserModel::_eyeBlink;
    using Csm::CubismUserModel::_breath;
    using Csm::CubismUserModel::_physics;
    using Csm::CubismUserModel::_pose;
    using Csm::CubismUserModel::_dragManager;
    using Csm::CubismUserModel::_mocConsistency;

    // Hook motion loading to auto-fix meta counts
    Csm::ACubismMotion* LoadMotion(const Csm::csmByte* buffer,
                                   Csm::csmSizeInt size,
                                   const Csm::csmChar* name,
                                   Csm::ACubismMotion::FinishedMotionCallback onFinished = NULL,
                                   Csm::ACubismMotion::BeganMotionCallback onBegan = NULL,
                                   Csm::ICubismModelSetting* modelSetting = NULL,
                                   const Csm::csmChar* group = NULL,
                                   Csm::csmInt32 index = -1,
                                   csmBool shouldCheckMotionConsistency = false) override;

    bool IsHit(CubismIdHandle drawableId, Csm::csmFloat32 pointX,
               Csm::csmFloat32 pointY) override;
};

}   // namespace V3
}   // namespace Live2D
