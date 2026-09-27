# live2d-py

## Build Commands

Configure (MSVC):
```
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE --no-warn-unused-cli -S . -B build -G "Visual Studio 18 2026" -T host=x64 -A x64
```

Build V2 static lib:
```
cmake --build build --config Release --target V2 -j 24
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
Live2D/                    # Live2D SDK (git submodule, see Live2D/README.md)
  CMakeLists.txt           # Top-level entry: Common, Glad, V2, V3 orchestration
  Common/                  # Shared: Log.hpp/cpp, Debug.hpp/cpp, IModel.hpp (统一模型接口)
  Glad/                    # Shared: OpenGL loader (glad)
  V2/
    cmake/V2.cmake         # V2 target: includes, links, alias (Live2D::V2)
    include/V2/            # headers: Model.hpp (implements Live2D::IModel), Core/, Framework/
    src/                   # v2cpp SDK sources (ported from Python v2)
      CMakeLists.txt
      Model.cpp            # High-level model (loading, update, draw, hit test)
      Core/                # BinaryReader, Id, ParamDef, PivotManager
      Model/               # Live2DModelOpenGL, ModelContext, ALive2DModel
      Draw/                # Mesh, IDrawData
      Deformer/            # RotationDeformer, WarpDeformer, AffineEnt
      Graphics/            # GLRenderer, ClippingManagerOpenGL
      Motion/              # Live2DMotion, AMotion
      Framework/           # L2DBaseModel, L2DModelMatrix, MatrixManager, L2DPose, L2DEyeBlink
      Util/                # UtMath, UtInterpolate, stb_impl
  V3/                      # v3 SDK (Cubism Native)
    cmake/                 # V3.cmake, Core.cmake, Framework.cmake, Main.cmake
    Core/                  # Cubism Core (prebuilt libs)
    Framework/             # Cubism Framework
    Main/                  # LAppModel, MatrixManager, LAppPal
      auto_patch.cmake     # Auto-patches Framework sources during configure

Wrapper/
  V2/                      # v2cpp CPython bindings
    Init.cpp               # Module init, glInit, clearBuffer
    PyModel.cpp/hpp        # LAppModel Python wrapper (holds Live2D::IModel*)
  V3/                      # v3 CPython bindings
    Init.cpp               # Module init, CubismFramework StartUp
    PyModel.cpp/hpp        # Model Python wrapper (holds Live2D::IModel*)

package/live2d/
  v2cpp/                   # v2cpp Python package
    __init__.py            # Re-exports from _v2cpp
  v3/                      # v3 Python package

cmake/
  Wrapper.cmake            # Shared: Python3 config, set_wrapper_output()
```

### Live2D Target Aliases

| Alias | Description |
|---|---|
| `Live2D::Common` | Shared logging (Log.hpp) |
| `Live2D::V2` | Cubism 2.x C++ port (static lib) |
| `Live2D::V3Core` | Cubism Native Core (prebuilt import) |
| `Live2D::V3Framework` | Cubism Native Framework |
| `Live2D::V3` | V3 top-level (Model, LAppPal, ...) |

## Key Technical Decisions

- **CPython limited API** (not pybind11) — matches v3 architecture
- **glad** for OpenGL — removes PyOpenGL dependency
- **STL containers** replace Python Array types
- **`std::filesystem::u8path`** required for Chinese/Unicode file paths
- **V2CPP_DEBUG macro** (`Debug.hpp`) wraps all debug fprintf, enabled via CMake option
- **统一模型接口** `Live2D::IModel` (`Common/IModel.hpp`): 以 V3 暴露给 Python 的 API 为基准（PascalCase、const char* 字符串、collector 回调、std::function 动作回调）。含版本接口：`Version()` 纯虚（V2 返回 2、V3 返回 3），`IsV2()`/`IsV3()` 由 Version() 内联推导。两个 Wrapper 持有 `IModel*`。**`Update(float deltaSecs = -1.0f)` 哨兵**：<0（不传）自适配（V2 墙钟 Python 1:1 / V3 内部自计时 clamp 0.1），>=0 全 delta 驱动（确定性测试/变速/暂停）。表情 fadeout 状态机在 C++ Model 内：`SetExpression(id, fadeoutMs=-1)`（>=0 临时、到期恢复 `mLastExpression`）；`SetOffsetX/Y`、`GetMotionSound(group,no)` 同属 IModel
- **三个模型类统一命名 `Model`**：`live2d.v2cpp.Model`（C++，v2 模型）与 `live2d.v3.Model`（C++，v3 模型）方法面完全一致（92=92）；纯 Python `live2d.v2.Model` 已废弃（实例化时打印一次 deprecated 信息，未来移除），其方法名已对齐统一命名（缺少的接口未补）。旧名 `LAppModel` 全部删除
- **两个 wrapper 的 Python API 已统一**（v2cpp.Model 与 v3.Model 方法面一致）：参数族统一 `Param` 缩写 + `ByIndex`/`ById` 成对（`SetParamByIndex/ById`、`AddParamByIndex/ById`、`SetSaveParamByIndex/ById`、`AddSaveParamByIndex/ById`、`GetParamValue/Max/Min/DefaultByIndex` + `*ById`——ById 读取版为 wrapper 层查下标实现）；`GetParamCount/GetParamIds`。删除冗余别名（ClearMotions/HitTest/ReleaseRenderer/SetAuto*Enable/GetParameter/GetCanvasWidth-Height/autoBreath 属性等）；`GetMotionGroups` 为 wrapper 级便利（由 GetMotions 推导）。纯 Python `live2d.v3.LAppModel` shim 已删除（老项目迁移靠报错型改名指引，无兼容层）；纯 Python `live2d.v2` 保留但未来废弃（不补齐 fine-grained API）
- **V2 计时架构**：时钟只由 Model 读（`UtSystem` 唯一读取点）。子系统全部收 dt（毫秒/秒），内部累计 elapsed：动作（MotionQueueEntry elapsed 状态机 + 相对 fade-out 调度）、眨眼（mCurrentTime 累计）、物理（mElapsedMs 累计）、姿态（直接 dt）、拖拽（L2DTargetPoint 为 Python 物理公式的 delta_time_weight = deltaSec*FRAME_RATE）。Model 墙钟路径每帧算一次 dt（首帧 dt=0，与 Python 一致），granular Update* 各自直接传 dt。**不要给子系统加回墙钟读取**继承链完全单继承：`V2::Model → L2DBaseModel → IModel`；`V3::Model → IModel`，SDK 不修改——`CubismUserModel` 作为组合成员（`CubismUserModelProxy.hpp` 独立文件：用 `using` 导出无 getter 的 protected 成员，`IsHit`/`LoadMotion` 虚函数钩子移入其中；有 getter 的成员如 `_model`/`_modelMatrix`/`_opacity` 直接走 `GetModel()`/`GetModelMatrix()`/`GetOpacity()`；`operator->` 直通 `GetModel()`）。**不要内联 CubismUserModel 的胶水代码**（LoadModel/ctor/dtor 等）——SDK 升级时其内部逻辑会自动跟随，内联副本会静默错过变化。V2 侧 IModel 全部接口已实现：细化 Update*（动作/呼吸走墙钟，忽略 deltaSecs）、参数保存/恢复、drawable 访问（顶点/索引缓存到成员）、动作组枚举（`mMotionInfos` 记录 file/sound）、表情枚举与额外加载；`AddAndSaveParameterValue` 用逐参数保存（非全量 saveParam）；`SetDrawableMultiColor/ScreenColor` 作用于所属 part；`HasMocConsistencyFromFile` 恒返回 false（v2 .moc 无此概念）
- **陷阱：wrapper 目标没有头文件依赖追踪**（ninja 规则为 unscanned，无 .d depfile）——改了 `Model.hpp` 等头文件后必须手动删 `build/Wrapper/*/CMakeFiles/*.dir/*.obj` 强制重编译，否则 `new Model()` 按旧类尺寸分配导致堆损坏（0xc0000374）

## v2 vs v2cpp Comparison

v2cpp port must match v2 Python behavior 1:1. Key areas requiring careful alignment:

### Parameter flow
- Python `LAppModel.Update()` does NOT call `modelContext.update()` — it only sets params/motion/physics/pose
- Python `LAppModel.Draw()` calls `live2DModel.update()` → `modelContext.update()`
- v2cpp `Model::Update(deltaSecs)` (unified IModel, default 0.016f) sets params/motion/pose; `Model::Draw()` calls `mModelContext->update()` — matches Python flow
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
