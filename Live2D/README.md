# Live2D SDK

Live2D Cubism C++ SDK used by live2d-py. Supports both Cubism 2.x (v2cpp, hand-ported from Python) and Cubism 3.x+ (Cubism Native).

## Directory Structure

```
Live2D/
  CMakeLists.txt          # Top-level entry point
  Common/                 # Shared utilities
    CMakeLists.txt
    Log.hpp / Log.cpp     # Logging (used by both V2 and V3)
  Glad/                   # OpenGL loader (glad, used on Windows/Linux/macOS)
  V2/                     # Cubism 2.x C++ port (v2cpp)
    cmake/
      V2.cmake            # V2 target definition (includes, links, alias)
    src/
      CMakeLists.txt      # Source file list for static lib
      LAppModel.cpp/hpp   # High-level model: loading, update, draw
      Core/               # BinaryReader, Id, ParamDef, PivotManager, ...
      Model/              # ModelContext, Live2DModelOpenGL, ALive2DModel
      Deformer/           # RotationDeformer, WarpDeformer, AffineEnt
      Draw/               # Mesh, IDrawData
      Graphics/           # DrawParamOpenGL, ClippingManagerOpenGL
      Motion/             # Live2DMotion, AMotion
      Framework/          # L2DBaseModel, MatrixManager, L2DPose, L2DEyeBlink, ...
      Util/               # Math, interpolation, stb_image loader
  V3/                     # Cubism 3.x+ (Cubism Native SDK)
    cmake/
      V3.cmake            # V3 target orchestration (includes Core/Framework/Main)
      Core.cmake          # Prebuilt Cubism Core import (per-platform .lib/.a)
      Framework.cmake     # Cubism Framework (rendering, math, motion, physics)
      Main.cmake          # V3 top-level target (LAppModel, MatrixManager, ...)
    Core/                 # Prebuilt Cubism Core binaries + headers
    Framework/            # Cubism Framework sources
    Main/                 # Application-layer: Model, LAppPal, TextureManager
      auto_patch.cmake    # Auto-patches Framework sources during configure
```

## Target Aliases

All public targets are accessed through aliases. Never depend on the raw target name.

| Alias | Raw Target | Description |
|---|---|---|
| `Live2D::Common` | `Common` | Shared logging utilities |
| `Live2D::V2` | `V2` | Cubism 2.x C++ port (static lib) |
| `Live2D::V3Core` | `Live2DCubismCore` | Cubism Native Core (prebuilt import) |
| `Live2D::V3Framework` | `Framework` | Cubism Native Framework |
| `Live2D::V3` | `V3` | V3 top-level (Model, LAppPal, ...) |

## Target Properties

| Target | Property | Value |
|---|---|---|
| `Framework` | `LIVE2D_SHADER_DIR` | Path to OpenGL shader sources (`.../Rendering/OpenGL/Shaders/Standard`) |

Use `$<TARGET_PROPERTY:Framework,LIVE2D_SHADER_DIR>` in generator expressions to reference the shader directory without hardcoding internal paths.

## Integration

### As a Git Submodule (recommended)

```bash
git submodule add <repo-url> Live2D
```

```cmake
# In your root CMakeLists.txt
add_subdirectory(Live2D)

# Then link against the aliases:
target_link_libraries(my_app Live2D::V2)   # Cubism 2.x
target_link_libraries(my_app Live2D::V3)   # Cubism 3.x+
```

### As a Plain Directory

```bash
cp -r /path/to/live2d-sdk Live2D
```

```cmake
add_subdirectory(Live2D)
target_link_libraries(my_app Live2D::V3)
```

### Transitive Dependencies

All public headers and link dependencies are propagated through the aliases. For example, linking `Live2D::V2` automatically provides:
- `Live2D::Common` (Log.hpp)
- `glad` (OpenGL loader)
- V2 include directories

No need to manually add include paths for Live2D internals.

## Platform Support

| Platform | V2 | V3 | Notes |
|---|---|---|---|
| Windows (MSVC x64) | ✓ | ✓ | Tested with VS 2026 |
| Linux (x86_64) | ✓ | ✓ | Requires OpenGL, `stdc++fs` |
| Linux (ARM64) | ✓ | ✓ | Experimental Cubism Core |
| macOS (x86_64 / ARM64) | ✓ | ✓ | Requires Cocoa, IOKit, CoreVideo |
| Android | — | ✓ | GLESv2, no glad |

## Build Requirements

- **CMake** ≥ 3.26
- **C++17** (V2, V3)
- **OpenGL** (Windows/Linux/macOS) or **GLESv2** (Android)
- **Python 3** (for wrapper builds, `Development.SABIModule` component)

## Internal Architecture Notes

- `LIVE2D_ROOT` — set in `Live2D/CMakeLists.txt` to `CMAKE_CURRENT_LIST_DIR`. All internal path references use this variable, never `CMAKE_SOURCE_DIR`. This keeps the SDK relocatable.
- V2 sources live under `V2/src/`; cmake configuration lives under `V2/cmake/V2.cmake`. This separates build logic from sources.
- V3 follows a similar pattern: `V3/cmake/` for build logic, with sources under `V3/Framework/` and `V3/Main/`.
- `Common/` is a standalone static library shared by both V2 and V3.
