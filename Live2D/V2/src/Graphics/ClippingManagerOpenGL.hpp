#pragma once
#include <memory>

#include <vector>
#include <array>
#include "ClipContext.hpp"

namespace live2d {

class ModelContext;
class GLRenderer;
class IDrawData;
class MeshContext;

class ALive2DModel;
class ClippingManagerOpenGL {
public:
    explicit ClippingManagerOpenGL(GLRenderer& renderer);
    ~ClippingManagerOpenGL();

    void init(ModelContext* mc, const std::vector<IDrawData*>& drawDataList,
              const std::vector<MeshContext*>& drawContextList);
    void setupClip(ModelContext* mc);
    void calcClippedDrawTotalBounds(ModelContext* mc, ClipContext* clip);
    void setupLayoutBounds(int count);

private:
    GLRenderer& mRenderer;
    std::vector<std::unique_ptr<ClipContext>> mClipContextList;
    std::vector<std::array<float, 4>> mChannelColors;
};

} // namespace live2d
