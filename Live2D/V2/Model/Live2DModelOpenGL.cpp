#include "Live2DModelOpenGL.hpp"
#include "../Graphics/DrawParamOpenGL.hpp"
#include "ModelContext.hpp"

namespace live2d {

Live2DModelOpenGL::Live2DModelOpenGL()
    : mDrawParam(std::make_unique<DrawParamOpenGL>()) {}
Live2DModelOpenGL::~Live2DModelOpenGL() = default;
void Live2DModelOpenGL::draw() {
    mModelContext->draw(mDrawParam.get());
}
void Live2DModelOpenGL::resize(int w, int h) { (void)w; (void)h; }
Live2DModelOpenGL* Live2DModelOpenGL::loadModel(const std::vector<uint8_t>& data) {
    (void)data; return new Live2DModelOpenGL();
}
} // namespace live2d
