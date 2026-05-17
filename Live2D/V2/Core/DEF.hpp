#pragma once

namespace live2d {

// Vertex format constants
constexpr int VERTEX_TYPE_OFFSET0_STEP2 = 1;
constexpr int VERTEX_TYPE_OFFSET2_STEP5 = 2;
constexpr int VERTEX_OFFSET = 0;
constexpr int VERTEX_STEP = 2;
constexpr int VERTEX_TYPE = VERTEX_TYPE_OFFSET0_STEP2;
constexpr bool REVERSE_TEXTURE_T = true;

// Interpolation and pivot constants
constexpr int MAX_INTERPOLATION = 5;
constexpr int PIVOT_TABLE_SIZE = 65;
constexpr float GOSA = 0.0001f;

// Format version constants
constexpr int LIVE2D_FORMAT_VERSION_V2_8_TEX_OPTION = 8;
constexpr int LIVE2D_FORMAT_VERSION_V2_10_SDK2 = 10;
constexpr int LIVE2D_FORMAT_VERSION_V2_11_SDK2_1 = 11;
constexpr int LIVE2D_FORMAT_VERSION_AVAILABLE = LIVE2D_FORMAT_VERSION_V2_11_SDK2_1;

// Binary format constants
constexpr int OBJECT_REF = 33;

} // namespace live2d
