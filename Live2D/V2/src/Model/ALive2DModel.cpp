#include "ALive2DModel.hpp"
#include "ModelContext.hpp"

namespace live2d {

ALive2DModel::ALive2DModel()
    : mModelContext(std::make_unique<ModelContext>(this)) {}

ALive2DModel::~ALive2DModel() = default;

ModelImpl* ALive2DModel::getModelImpl() {
    if (!mModelImpl) {
        mModelImpl = std::make_unique<ModelImpl>();
    }
    return mModelImpl.get();
}

int ALive2DModel::getCanvasWidth() const {
    return mModelImpl ? mModelImpl->getCanvasWidth() : 0;
}

int ALive2DModel::getCanvasHeight() const {
    return mModelImpl ? mModelImpl->getCanvasHeight() : 0;
}

float ALive2DModel::getParamFloat(int index) const {
    return mModelContext ? mModelContext->getParamFloat(index) : 0.0f;
}

void ALive2DModel::setParamFloat(int index, float value, float weight) {
    if (mModelContext) {
        mModelContext->setParamFloat(index,
            mModelContext->getParamFloat(index) * (1.0f - weight) + value * weight);
    }
}

} // namespace live2d
