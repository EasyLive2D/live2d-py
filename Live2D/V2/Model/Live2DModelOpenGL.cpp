#include "Live2DModelOpenGL.hpp"
#include "ModelContext.hpp"
#include "../Graphics/DrawParamOpenGL.hpp"
namespace live2d {
Live2DModelOpenGL::Live2DModelOpenGL() {
    mDrawParam = new DrawParamOpenGL();
}
Live2DModelOpenGL::~Live2DModelOpenGL() {
    delete mDrawParam;
}
void Live2DModelOpenGL::draw() {
    mModelContext->draw(mDrawParam);
}
void Live2DModelOpenGL::resize(int w, int h) { (void)w; (void)h; }
Live2DModelOpenGL* Live2DModelOpenGL::loadModel(const std::vector<uint8_t>& data) {
    (void)data; return new Live2DModelOpenGL();
}
} // namespace live2d
