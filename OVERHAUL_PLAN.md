# BangUI API Overhaul Plan

Version: 2025-10-28
Author: (automated plan)

Purpose
-------
This document is a complete, step-by-step overhaul plan to convert the current BangUI codebase into a consistent, API-first C++ library following the best practices in the project docs (`Overview.md`, `BangUI_Documentation.md`, `Paradigm.md`, `Operational Paradigm.md`).

Important: I will not change any source files until you explicitly approve this plan. After approval I will proceed file-by-file and report progress frequently.

Goals
-----
- Provide a small, stable public API (headers under `src/api/`) that application developers include.
- Separate implementation from API (implementations under `src/impl/` or `src/lib/impl/`).
- Keep public headers minimal: no heavy includes, no concrete defaults, no non-trivial inline logic.

Single-include policy
----------------------
The project will provide a single public convenience header `src/lib/BangUI.h` that re-exports
the stable public API. All future public elements (types, interfaces, managers, helpers) MUST be
added to `src/api/` and also re-exported from `src/lib/BangUI.h` so applications can include only
one header (`#include "BangUI.h"`) to consume the library. This is the canonical consumer-facing
contract for the library.
- Preserve all existing capabilities and behaviour unless you explicitly request changes.
- Improve performance: layout dirty-tree, batched renderer design, minimal per-frame allocations.
- Make code maintainable and testable; add unit tests for pure logic modules.

High-level design
-----------------
1. API / Interface layer (stable public surface)
   - `src/api/Types.h` - value types and small enums (Color, Rect, SizeMode, HDock/VDock)
   - `src/api/UIElement.h` - minimal interface: getters/setters for properties expressed as plain data; no rendering or heavy behaviour
   - `src/api/IButton.h` - Button interface (SetLabel/GetLabel, SetOnClick callback)
   - `src/api/IWindow.h`, `src/api/IPanel.h`, `src/api/IUIManager.h`, `src/api/IRenderer.h` - small public interfaces

2. Implementation layer (actual behaviour)
   - `src/impl/UIElementImpl.*` - concrete implementation of UIElement
   - `src/impl/ButtonImpl.*` - concrete Button implementation
   - `src/impl/LayoutManager.*` - layout algorithm (measure/arrange)
   - `src/impl/UIManager.*` - input dispatch, event routing, focus, modal handling
      - `src/impl/Renderer/DiligentRenderer.*` - renderer implementation using Diligent (future)
      - `src/impl/RendererImpl.h/.cpp` - temporary SDL-based impl wrapper used during migration
   - `src/impl/BML/*` - parser and stylesheet system

3. Demo and examples
   - `examples/sdl_diligent_demo.cpp` updated to consume `src/api/*` plus a `CreateDefaultImpls()` factory.

4. Tests
   - `tests/unit` for layout, BML, and UIManager logic
   - Integration: demo still used as end-to-end smoke test

Compatibility rules
-------------------
- No public header (under `src/api/`) will include SDL or Diligent headers.
- Public headers will be lightweight and contain only types and abstract interfaces (pure virtual or value classes).
- Implementation headers may include heavy dependencies and be placed under `src/impl/`.
- Existing source behaviour must be preserved: default values and behavioural expectations from `BangUI_Documentation.md` and `Overview.md` will be respected in implementation.

Audit (current code map and capabilities)
----------------------------------------
Key files (non-exhaustive) that must be handled carefully:
- `src/lib/UIElement.h` — currently contains a large concrete structure. This will be split into `src/api/UIElement.h` (minimal) and `src/impl/UIElementImpl.h/.cpp` (concrete fields & helpers).
- `src/lib/Button.h` and `src/lib/Button.cpp` — currently a mix of API and impl; will be migrated to `src/api/IButton.h` and `src/impl/ButtonImpl.*`.
- `src/models/Window.h`, `src/models/Panel.h` — container types; ensure APIs expose `GetChildren()` and basic properties.
- `src/lib/LayoutManager.h` — layout algorithm: keep implementation but export a header that only includes the implementation under `src/impl/` and provide an API entrypoint.
- `src/lib/UIManager.h` — input handling & modal logic; must become `src/api/IUIManager.h` + `src/impl/UIManagerImpl.*`.
- `src/lib/sdl_diligent_demo.cpp` — example that uses libs; update to use API factories.

Specific capabilities to preserve:
- Docking (hDock, vDock), three size modes (Absolute/Auto/Stretch), padding/margins.
- onClick/onHover/onFocus callbacks.
- Modal window handling and hit test restrictions.
- 3D mesh attach-to-bone support (UIElement3D) — API must allow 3D properties to exist even if implementation later uses Diligent.
- BML parsing and styles inheritance (extendable via parser module).

Migration plan — phases & tasks
-------------------------------
PHASE 0 — Preparation (non-destructive)
- Create `src/api/` directory and add API skeleton headers for all public concepts.
  - `Types.h`, `UIElement.h` (minimal), `IButton.h`, `IWindow.h`, `IPanel.h`, `IUIManager.h`, `IRenderer.h`, `Events.h`.
- Create `src/impl/` directory for implementations.
- Add `src/impl/factories.h` to provide convenience factories (e.g., `CreateDefaultUIManager(renderer)`).
- Add `docs/OVERHAUL_PLAN.md` (this file) to root (done).

PHASE 1 — API extraction and shim wrappers
- For each public header currently used across the project, create a corresponding `src/api/` header exposing an interface matching the existing usage surface.
- Add thin wrappers in `src/lib/` or `src/impl/` that implement the API using the current concrete structures (so we don't break behaviour immediately).
- Update code within the repo to start depending on `src/api/*` where appropriate. Keep current files intact but change local includes gradually.

PHASE 2 — Implementation consolidation
- Move heavy fields and helper functions into `src/impl/*` implementations.
- Ensure `UIManager` owns elements using `std::unique_ptr<impl::UIElementImpl>` and exposes `UIElementHandle` or `UIElement*` via API.
- Implement `ButtonImpl` with Press/Release state, capture semantics, and callbacks.

PHASE 3 — Tests & CI
- Add unit tests for LayoutManager correctness and UIManager event semantics.
- Add CI step to run tests and the demo smoke run.

PHASE 4 — Performance tuning & batching
- Introduce batching in renderer for quads/textures and glyph atlas.
- Replace slow hot-paths with batched updates and pre-allocated containers.

PHASE 5 — Cleanup and release
- Remove legacy headers not used by API, or convert them into internal-only impl headers.
- Update README and API docs.
- Tag release.

File-by-file migration mapping (detailed)
----------------------------------------
The following table maps current files to their target replacements and describes the exact edits. The plan is conservative: create new files first and then switch consumers.

1) `src/lib/UIElement.h` (current) ->
   - New: `src/api/UIElement.h` (minimal) — contains value types: ID, position, size modes, getters/setters as plain POD or inline trivial functions; no heavy includes.
   - New: `src/impl/UIElementImpl.h`/`.cpp` — contains the current concrete fields, padding/margins, helper methods, and safe content getters.
   - Update: Replace direct includes of `src/lib/UIElement.h` with `src/api/UIElement.h` in all files. In implementation files include `src/impl/UIElementImpl.h` as needed.

2) `src/lib/Button.h`/`.cpp` ->
   - New: `src/api/IButton.h` — interface with SetLabel/GetLabel, SetOnClick, SetEnabled, GetId.
   - New: `src/impl/ButtonImpl.h`/`.cpp` — contains the concrete state (pressed flag, dimensions) and overrides `Render()` and `OnClick()` semantics. Implementations should call `onClick` safely and set state transitions.
   - Replace direct uses: update demo and any tests to use `CreateButton()` factory from `src/impl/factories.h` or to construct `std::unique_ptr<ButtonImpl>`.

3) `src/models/Window.h`, `src/models/Panel.h` ->
   - New API: `src/api/IWindow.h`, `src/api/IPanel.h` — very small interfaces exposing child iteration and modal getter.
   - Impl: `src/impl/WindowImpl.h/.cpp` and `PanelImpl` with concrete content vector (using `std::vector<std::unique_ptr<UIElementImpl>>`).

4) `src/lib/LayoutManager.h` ->
   - Keep under `src/impl/LayoutManager.h/.cpp` but provide an API entry point `src/api/Layout.h` (or keep only functions internal and expose via `IUILayout` interface).

5) `src/lib/UIManager.h` ->
   - `src/api/IUIManager.h` (interface): methods to add/remove windows/elements, run event loop tick, set modal, hit testing.
   - `src/impl/UIManagerImpl.h/.cpp` implement behaviour and use `UIElementImpl` internals.

6) `src/lib/sdl_diligent_demo.cpp` ->
   - Update to include `src/api/*` and use factory `CreateDefaultUIManager()` to get impl instances. Keep demo logic unchanged otherwise.

7) CMake changes
   - Add new targets `bangui-core` (headers only or small static lib), `bangui-impl` (static lib linking Diligent/SDL), `bangui-demo` (executable linking the impl).
   - Keep `BUILD_TESTS` toggle and add `tests/*` target when enabled.

Verification steps per file change
---------------------------------
- After creating each API header, re-run CMake configure to ensure no missing includes.
- After moving an implementation, run `cmake --build build --config Release` and fix compile errors.
- Run demo and ensure behaviour matches prior run for modal clicks and layout logs.
- Run unit tests for layout and event handling.

Testing & regression strategy
----------------------------
- Add a small suite of unit tests in `tests/unit` for LayoutManager scenarios.
- Keep `sdl_diligent_demo` as a manual/integration test harness.
- When migrating a module, write tests first (where possible) to assert existing behavior.

Rollback plan
-------------
- All new files are added incrementally. If a migration step causes issues, we revert the single commit.
- Keep old headers as `legacy/` wrappers for a short period in case apps depend on them.

Timeline (estimate)
-------------------
- Phase 0 + Phase 1 skeletons: 1–2 days (create API headers & wrappers)
- Phase 2 implementation consolidation: 2–4 days (move impl code, fix compile errors)
- Phase 3 tests & CI: 1–2 days
- Phase 4 performance tuning: ongoing (benchmarks and optimizations as follow-ups)

Deliverables
------------
- `OVERHAUL_PLAN.md` (this file)
- `src/api/*` headers (minimal public API)
- `src/impl/*` implementation files
- Updated CMake targets
- Unit tests in `tests/unit` and CI updates

Next steps (please choose)
-------------------------
- Option A (recommended): Approve this plan. I will then start Phase 0 by creating `src/api/` headers and `src/impl/` directories and place shim wrappers. I will commit changes in small, reviewable patches and run builds after each patch.

- Option B: Request edits to this plan. Tell me which sections to change and I will update `OVERHAUL_PLAN.md` accordingly.

- Option C: Reject and provide a different direction. I will stand by.

When you approve (Option A) I will proceed with Phase 0 and create the first set of API headers and factories. I will not modify implementation code until the skeleton API is in place and you confirm to proceed.
