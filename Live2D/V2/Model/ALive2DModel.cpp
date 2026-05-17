#include "ALive2DModel.hpp"
#include "ModelContext.hpp"

namespace live2d {

ALive2DModel::ALive2DModel() {
    mModelContext = new ModelContext(this);
}

ModelImpl* ALive2DModel::getModelImpl() {
    if (!mModelImpl) {
        mModelImpl = new ModelImpl();
    }
    return mModelImpl;
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
