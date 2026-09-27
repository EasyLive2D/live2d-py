#include <CubismUserModelProxy.hpp>

#include <LAppPal.hpp>

#include <string>

using namespace Live2D::Cubism::Framework;

namespace Live2D {
namespace V3 {

Csm::ACubismMotion* CubismUserModelProxy::LoadMotion(
    const Csm::csmByte* buffer,
    Csm::csmSizeInt size,
    const Csm::csmChar* name,
    Csm::ACubismMotion::FinishedMotionCallback onFinished,
    Csm::ACubismMotion::BeganMotionCallback onBegan,
    Csm::ICubismModelSetting* modelSetting,
    const Csm::csmChar* group,
    Csm::csmInt32 index,
    csmBool shouldCheckMotionConsistency) {
    std::string fixed(reinterpret_cast<const char*>(buffer), size);
    LAppPal::FixMotionJson(fixed);
    return CubismUserModel::LoadMotion(reinterpret_cast<const Csm::csmByte*>(fixed.data()),
                                       static_cast<Csm::csmSizeInt>(fixed.size()),
                                       name,
                                       onFinished,
                                       onBegan,
                                       modelSetting,
                                       group,
                                       index,
                                       shouldCheckMotionConsistency);
}

bool CubismUserModelProxy::IsHit(CubismIdHandle drawableId, Csm::csmFloat32 pointX,
                                 Csm::csmFloat32 pointY) {
    const Csm::csmInt32 drawIndex = _model->GetDrawableIndex(drawableId);

    if (drawIndex < 0) {
        return false;   // 存在しない場合はfalse
    }

    const Csm::csmInt32 count = _model->GetDrawableVertexCount(drawIndex);
    const Csm::csmFloat32* vertices = _model->GetDrawableVertices(drawIndex);

    Csm::csmFloat32 left = vertices[0];
    Csm::csmFloat32 right = vertices[0];
    Csm::csmFloat32 top = vertices[1];
    Csm::csmFloat32 bottom = vertices[1];

    for (Csm::csmInt32 j = 1; j < count; ++j) {
        Csm::csmFloat32 x = vertices[Constant::VertexOffset + j * Constant::VertexStep];
        Csm::csmFloat32 y = vertices[Constant::VertexOffset + j * Constant::VertexStep + 1];

        if (x < left) {
            left = x;   // Min x
        }

        if (x > right) {
            right = x;   // Max x
        }

        if (y < top) {
            top = y;   // Min y
        }

        if (y > bottom) {
            bottom = y;   // Max y
        }
    }

    return ((left <= pointX) && (pointX <= right) && (top <= pointY) && (pointY <= bottom));
}

}   // namespace V3
}   // namespace Live2D
