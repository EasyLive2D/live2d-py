#pragma once

#include <vector>
#include <array>
#include <string>

namespace live2d {

class ModelContext;
class DrawParamOpenGL;
class IDrawData;
class MeshContext;
class ClipContext;

class ALive2DModel;
class ClippingManagerOpenGL {
public:
    explicit ClippingManagerOpenGL(DrawParamOpenGL* dp);
    ~ClippingManagerOpenGL();

    void init(ModelContext* mc, const std::vector<IDrawData*>& drawDataList,
              const std::vector<MeshContext*>& drawContextList);
    void setupClip(ModelContext* mc, DrawParamOpenGL* dp);
    void calcClippedDrawTotalBounds(ModelContext* mc, ClipContext* clip);
    void setupLayoutBounds(int count);

private:
    DrawParamOpenGL* mDpGL = nullptr;
    std::vector<ClipContext*> mClipContextList;
    std::vector<std::array<float, 4>> mChannelColors;
};

} // namespace live2d
