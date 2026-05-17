#pragma once

#include "IDrawData.hpp"
#include <vector>

namespace live2d {

class PivotManager;
class MeshContext;
class DrawParamOpenGL;

class Mesh final : public IDrawData {
public:
    static constexpr int MASK_COLOR_COMPOSITION = 30;
    static constexpr int COLOR_COMPOSITION_NORMAL = 0;
    static constexpr int COLOR_COMPOSITION_SCREEN = 1;
    static constexpr int COLOR_COMPOSITION_MULTIPLY = 2;

    Mesh();

    int getType() const override { return TYPE_MESH; }
    void read(class BinaryReader& br) override;
    void setupTransform(ModelContext* mc, IDrawContext* dc = nullptr) override;

    MeshContext* init(ModelContext* modelContext);
    void setupInterpolate(ModelContext* modelContext, MeshContext* meshContext);
    void draw(DrawParamOpenGL* dp, ModelContext* mctx, MeshContext* dctx);

    void setTextureNo(int texNo) { mTextureNo = texNo; }
    int getTextureNo() const { return mTextureNo; }
    int getColorCompositionType() const { return mColorCompositionType; }
    const std::vector<float>& getUvs() const { return mUvs; }
    int getNumPoints() const { return mPointCount; }
    const std::vector<int16_t>& getIndexArray() const { return mIndexArray; }

private:
    int mTextureNo = -1;
    int mPointCount = 0;
    int mPolygonCount = 0;
    int mOptionFlag = 0;
    std::vector<int16_t> mIndexArray;
    std::vector<std::vector<float>> mPivotPoints;
    std::vector<float> mUvs;
    int mColorCompositionType = COLOR_COMPOSITION_NORMAL;
    bool mCulling = true;
    int mInstanceNo = 0;
};

} // namespace live2d
