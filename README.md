# BangUI

BangUI is a native 2D UI composition library written in modern C++17. The
runtime now includes its own software renderer, windowing glue, and text/texture
pipelines so it no longer depends on external engines such as SDL or Diligent.

## What's Included

- Software rasterizer (`SoftwareRenderer`) that supports rounded rectangles,
  textured elements, alpha blending, render targets, and screenshots.
- Image loader powered by stb_image for textures and 9-slice assets.
- Font rasterizer built on stb_truetype for crisp text rendering without SDL_ttf.
- Win32-native window class (`NativeWindow`) capable of dispatching BangUI's
  cross-platform `UIEvent` structures so the UI manager can compose and control
  its own windows.
- Themeable root window via `BackgroundTheme`, supporting nine-patch textures
  and declarative vector shapes for gradients, frames, and accents.
- `BangUI_native_demo` sample that exercises docking, scrolling, buttons, and
  resize flows entirely through the standalone backend.

## Building

Prerequisites:

- Windows + MSVC or Clang with C++17 support
- CMake 3.16 or newer

Steps:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

This produces:

- `BangUI_lib` – the reusable library
- `BangUI_native_demo` – interactive demo showcasing the standalone renderer

## Running the Demo

```
build\src\Release\BangUI_native_demo.exe
```

Controls:

- Drag windows by their title bar
- Resize using the bottom-right gripper
- Scroll content with the mouse wheel
- Close windows via the title bar button

The demo also demonstrates how to wire BangUI’s `UIManager` to the standalone
window/event loop. Applications can reuse the same pattern or embed the library
inside an existing engine by feeding it `UIEvent` data and presenting the
renderer’s back buffer through their own swap chain.

## Theming the Application Window

Use `BangUI::API::BackgroundTheme` to customize the root window without writing
any rendering boilerplate:

```cpp
BangUI::API::BackgroundTheme theme;
theme.clearColor = BangUI::Color(8, 8, 14);
theme.texture = "textbox.9.png";
theme.textureNinePatch = true;
theme.textureOpacity = 0.4f;

BangUI::API::VectorShape border;
border.type = BangUI::API::VectorShapeType::RoundedRectangle;
border.normalized = true;
border.x = 0.05f;
border.y = 0.05f;
border.width = 0.9f;
border.height = 0.9f;
border.radius = 0.04f;
border.color = BangUI::Color(255, 255, 255, 32);
theme.shapes.push_back(border);

uiManager.setBackgroundTheme(theme);
```

Textures fill the surface before panels/windows render. Vector shapes are drawn
after the texture and can be defined in normalized coordinates (0–1) for
responsive layouts.

## Repository Layout

- `src/impl` – core widgets, layout engine, renderer wrapper
- `src/standalone` – software renderer, event abstraction, image/font loaders
- `src/lib/native_demo.cpp` – reference application
- `specs/` – design documents and checklists

## Upgrading from the SDL build

The public API no longer exposes SDL types. Applications that previously passed
`SDL_Event` instances should switch to BangUI’s lightweight `UIEvent` structure.
Texture, font, and image loading run through the built-in managers, so external
DLL deployment is no longer required.
