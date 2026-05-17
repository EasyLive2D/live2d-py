#pragma once
#include "ALive2DModel.hpp"
namespace live2d {
class DrawParamOpenGL;
class Live2DModelOpenGL : public ALive2DModel {
public:
    Live2DModelOpenGL();
    ~Live2DModelOpenGL() override;
    DrawParamOpenGL* getDrawParam() override { return mDrawParam; }
    void draw() override;
    void resize(int w, int h);
    static Live2DModelOpenGL* loadModel(const std::vector<uint8_t>& data);
    DrawParamOpenGL* mDrawParam = nullptr;
};
} // namespace live2d
