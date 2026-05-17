# live2d-py

## Build Commands

Configure (MSVC):
```
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE --no-warn-unused-cli -S . -B build -G "Visual Studio 18 2026" -T host=x64 -A x64
```

Build SDK2 (static lib):
```
cmake --build build --config Release --target SDK2 -j 24
```

Build v2cpp .pyd:
```
cmake --build build --config Release --target Live2DV2Wrapper -j 24
```

Debug build (enables V2CPP_DEBUG macro):
```
cmake -DV2CPP_DEBUG=ON -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE --no-warn-unused-cli -S . -B build -G "Visual Studio 18 2026" -T host=x64 -A x64
cmake --build build --config Release --target Live2DV2Wrapper -j 24
```

## Directory Structure

```
Live2D/
  Common/                # Shared: Log.hpp/cpp, stb_image.h
  Glad/                  # Shared: OpenGL loader
  V2/                    # v2cpp SDK (ported from Python v2)
    Core/                # BinaryReader, Id, ISerializable, PivotManager
    Model/               # Live2DModelOpenGL, ModelContext, ALive2DModel
    Draw/                # Mesh, IDrawData
    Deformer/            # RotationDeformer, WarpDeformer, AffineEnt
    Graphics/            # DrawParamOpenGL, ClippingManagerOpenGL, shaders
    Motion/              # Live2DMotion, AMotion, MotionQueueManager
    Physics/             # PhysicsHair
    Framework/           # LAppModel, L2DBaseModel, L2DModelMatrix, MatrixManager, L2DPose, L2DEyeBlink
    Util/                # UtMath, UtInterpolate
  V3/                    # v3 SDK (Cubism Native)
    cmake/               # Live2D.cmake, Core.cmake, Framework.cmake, Glad.cmake, Main.cmake
    Core/                # Cubism Core (prebuilt libs)
    Framework/           # Cubism Framework
    Main/                # LAppModel, MatrixManager, LAppPal

V2Wrapper/               # CPython limited API bindings
  Init.cpp               # Module init, glInit, clearBuffer
  PyLAppModel.cpp        # LAppModel Python wrapper

Wrapper/                 # v3 CPython bindings

package/live2d/
  v2cpp/                 # v2cpp Python package
    __init__.py          # Re-exports from _v2cpp
  v3/                    # v3 Python package

cmake/
  SDK2.cmake             # v2cpp static lib build
  V2Wrapper.cmake        # _v2cpp.pyd build
  Wrapper.cmake          # v3 live2d.pyd build
```

## Key Technical Decisions

- **CPython limited API** (not pybind11) — matches v3 architecture
- **glad** for OpenGL — removes PyOpenGL dependency
- **STL containers** replace Python Array types
- **`std::filesystem::u8path`** required for Chinese/Unicode file paths
- **V2CPP_DEBUG macro** (`Debug.hpp`) wraps all debug fprintf, enabled via CMake option

## v2 vs v2cpp Comparison

v2cpp port must match v2 Python behavior 1:1. Key areas requiring careful alignment:

### Parameter flow
- Python `LAppModel.Update()` does NOT call `modelContext.update()` — it only sets params/motion/physics/pose
- Python `LAppModel.Draw()` calls `live2DModel.update()` → `modelContext.update()`
- v2cpp `LAppModel::update()` calls `modelContext->update()` directly (different architecture)
- Both eventually call `modelContext.update()` before drawing

### Deformer chain
- Python `getAngleNotAbs(v1, v2)` = `getAngleDiff(atan2(v1), atan2(v2))` = **q1 - q2** (normalized to [-π, π])
- C++ must match: `q1 - q2` with wrap-around, NOT `q2 - q1`
- Type 1 = RotationDeformer, Type 2 = WarpDeformer

### Model matrix
- Python `L2DModelMatrix.scale()` is **assignment** (`tr[0]=sx, tr[5]=sy`), not multiply
- Python `L2DModelMatrix.translate()` is **absolute set** (`tr[12]=x, tr[13]=y`), not relative
- `MatrixManager` defaults: `mWidth=600, mHeight=600` (Python) — must match

### Model loading
- `L2DBaseModel.loadModelData()` must call `mModelMatrix.setWidth(2)` and `mModelMatrix.setCenterPosition(0,0)` after `init()`

### Python binding properties
- `autoBreath`/`autoBlink` need `PyGetSetDef` entries to work as Python attributes
- Setting `model.autoBreath = False` directly on C++ object requires getter/setter in the type spec

## Debug Techniques

### XForm dump (deformer chain comparison)
```bash
cd package && V2CPP_XFORM_DUMP=1 python gen_v2cpp_screenshot.py
# Writes xform_dump_v2cpp.txt with all params, deformer affine values, draw vertices
```

### Per-draw vertex debug (v2cpp C++)
```bash
# Set env var in C++ model_context.cpp init() — dumps all pre-deformation mesh vertices
```

### Python v2 deformer debug
```python
# Add to roation_deformer.py setupTransform:
print(f"[DEFORM_PY] {name} parent={pname} ox={:.1f}->{:.1f} ... angle={:.4f} dir=... tDir=...", file=sys.stderr)
```

### Quick screenshot comparison
```python
# Run both v2 and v2cpp headlessly, capture first frame, compare
# Use gen_v2_screenshot.py / gen_v2cpp_screenshot.py pattern
```

## Coding Conventions (v2cpp / SDK2)

- Member variables: `mXxxx`
- Static variables: `sXxxx`
- Constants: `ABC_CCCC`
- Use modern C++: smart pointers, `constexpr`, templates, `override`
- `.hpp` / `.cpp` file extensions
- Namespace: `live2d`

## Known Issues / Common Pitfalls

1. **`getAngleNotAbs` C++ must match Python**: `atan2(v1) - atan2(v2)` normalized, NOT `atan2(cross,dot)`
2. **WarpDeformer extrapolation**: Full ~170 line Python logic with 9 quadrant cases; simplified 2-line version produces wrong vertex positions
3. **Blend modes**: `glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha)` order must match Python
4. **Clip matrix**: `forMask=true` includes NDC mapping `T(-1,-1)*S(2,2)`; `forMask=false` maps to [0,1]
5. **Clip rect**: Python `Float32Array` trailing zeros → `min()=0`; set `minX=0, minY=0` in C++
6. **Motion $fps**: MTN file format uses `'$fps` (with leading quote) — must strip it
7. **Motion JSON**: Use bracket counting for nested arrays, not simple `find(']')`
8. **u_baseColor**: RGB must be multiplied by opacity, not left as (1,1,1)
