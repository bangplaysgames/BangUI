# Building and Running BangUI

## Prerequisites

- Windows 10/11
- Visual Studio 2022 (or LLVM/Clang/MinGW with C++17 support)
- CMake 3.16+

All runtime dependencies (renderer, image decoding, font rasterizer, window loop)
ship in this repository—no SDL/Diligent DLLs are required anymore.

## Configure & Build

```powershell
cmake -S . -B build
cmake --build build --config Release
```

Artifacts:

- `build/src/<cfg>/BangUI_lib.lib`
- `build/src/<cfg>/BangUI_native_demo.exe`

## Running the Standalone Demo

```powershell
build\src\Release\BangUI_native_demo.exe
```

You should see two draggable windows rendered via the software backend. The demo
uses the built-in `NativeWindow` to present its own Win32 window, so no external
toolkits are needed.

### Controls

- Left click + drag on the title bar to move windows
- Drag the bottom-right corner to resize
- Mouse wheel scrolls panel content
- Close button removes a window
- Closing the OS window or pressing Alt+F4 exits

## Debug vs Release

`--config Debug` builds with assertions and diagnostic logging. `Release`
enables optimizations for smoother rendering. Switch between configs with
the `--config` flag when running `cmake --build`.

## Troubleshooting

| Issue | Fix |
| --- | --- |
| Blank window | Ensure `window.present(renderer)` is called each frame; see `src/lib/native_demo.cpp`. |
| Incorrect size after resize | Call `SDL_RendererResize(renderer, newWidth, newHeight)` inside your resize event handler and update `UIManager::setApplicationWindow`. |
| Missing fonts/textures | Files are loaded relative to `std::filesystem::current_path()`. Launch your app from the repo root or provide absolute paths. |

## Integrating BangUI

1. Create or embed a platform window. On Windows you can reuse `Standalone::NativeWindow`.
2. Instantiate the software renderer with `SDL_CreateSoftwareRenderer(width, height)`.
3. Feed `BangUI::API::UIEvent` instances to `UIManager::handleEvent`.
4. Call `UIManager::Update(dt)` and `UIManager::Render(renderer)`.
5. Present the renderer’s pixel buffer via your swap chain (for `NativeWindow`
   call `present(renderer)`).

With this flow BangUI runs completely standalone, enabling native composition
without SDL or Diligent Engine.

