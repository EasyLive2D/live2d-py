#pragma once

#include "../Core/ModelImpl.hpp"

namespace live2d {

class ModelContext;
class DrawParamOpenGL;

class ALive2DModel {
public:
    ALive2DModel();
    virtual ~ALive2DModel() = default;

    void setModelImpl(ModelImpl* impl) { mModelImpl = impl; }
    ModelImpl* getModelImpl();
    ModelContext* getModelContext() const { return mModelContext; }
    virtual DrawParamOpenGL* getDrawParam() = 0;
    virtual void draw() = 0;

    int getCanvasWidth() const;
    int getCanvasHeight() const;
    float getParamFloat(int index) const;
    void setParamFloat(int index, float value, float weight = 1.0f);

protected:
    ModelImpl* mModelImpl = nullptr;
    ModelContext* mModelContext = nullptr;
};

} // namespace live2d
