# Implementation Plan

Tech Stack: C++17, standalone software renderer, CMake

Architecture:
- src/: library implementation
- src/lib/: core runtime (LayoutManager, UIElement, etc)
- src/models/: data models (Window, Panel)
- tests/: integration/demo-based tests (sdl_diligent_demo.cpp)

Goals:
- Provide a layout manager that computes positions using docking and size modes
- Support manual positioning (dragging) and ensure children move with parent
- Deterministic behavior for Auto/Stretch sizing

Files to change:
- src/lib/LayoutManager.h (layout algorithm)
- src/models/UIElement.h (ensure properties exist)
- tests/unit/layout_tests.cpp (add tests)

