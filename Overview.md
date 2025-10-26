# Custom UI Library Design Summary

## Overview
You are developing a custom UI library using **Diligent Engine** for rendering and **SDL** for windowing and input handling, targeting cross-platform applications with support for multiple rendering backends (e.g., DirectX11/12, Vulkan, OpenGL, Metal, WebGPU). The library aims to support both 2D sprite-based UI elements (e.g., buttons, panels) and 3D mesh-based elements, including diegetic UI integrated into the game world (e.g., a health monitoring system attached to a futuristic battle suit’s forearm). You’ve expressed a desire to avoid CSS due to its complexity, opting instead for a custom stylesheet syntax called **Bang Markup Language (BML)**, inspired by YAML but with distinct object separation.

## Core UI Element Properties
All UI elements in your library share a set of global properties inherited from a base class, ensuring consistency across 2D and 3D elements. These properties were explicitly defined as critical for layout and styling:

- **hDock**: Controls horizontal docking/alignment within a parent container. Options are `left`, `center`, `right`, or `fill` (stretches to fill available horizontal space, overriding width if set to `stretch`). This enables flexible positioning, such as aligning a button to the left or centering a panel.
- **vDock**: Controls vertical docking/alignment. Options are `top`, `center`, `bottom`, or `fill` (stretches vertically, overriding height if set to `stretch`). This supports layouts like stacking elements vertically or filling a parent container.
- **width**, **height**: Define element size with three modes:
  - **Absolute**: Fixed pixel measurements (e.g., `100` for 100px).
  - **Auto**: Sizes based on content (e.g., text or texture size for a button).
  - **Stretch**: Expands to fill the parent’s available space, respecting padding and margins.
- **padding-top**, **padding-bottom**, **padding-left**, **padding-right**: Specify internal spacing in pixels from each edge (e.g., `10` for 10px), ensuring content is offset from the element’s boundaries.

Additional common properties were suggested to enhance usability, inspired by standard UI frameworks:
- **visible**: Boolean to show or hide the element (default: `true`).
- **enabled**: Boolean to enable or disable interaction (default: `true`).
- **margin-top**, **margin-bottom**, **margin-left**, **margin-right**: External spacing in pixels, similar to padding but outside the element’s border.
- **backgroundColor**: RGBA color for the element’s background (e.g., hex or RGB).
- **border-width**, **border-color**, **border-radius**: Define border properties (width in pixels, color, and corner radius for rounded edges).
- **opacity**: Float from 0.0 (transparent) to 1.0 (opaque).
- **zIndex**: Integer for stacking order in overlapping scenarios.
- **tooltip**: String for hover text.
- **id**, **class**: Strings for unique identification and stylesheet styling.
- **onClick**, **onHover**, **onFocus**: Event handler callbacks for user interactions.

These properties form the foundation of your `UIElement` base class, ensuring all elements (2D and 3D) support consistent layout and interaction behaviors.

## UI Element Hierarchy and Hybrid Approach
You’ve chosen a **hybrid approach** for structuring UI elements, balancing simplicity and specialization. The hierarchy is:

- **UIElement**: The base class containing global properties (listed above) and virtual methods for rendering, updating animations, and handling SDL events. It defines the core interface for all UI elements, ensuring shared layout logic (e.g., docking) and event handling.
- **UIElement2D**: Extends `UIElement`, adding properties specific to 2D sprite-based elements, such as:
  - **textureSrc**: Path to a sprite image (e.g., PNG for a button texture).
  - **color**: RGBA tint for the sprite.
  - This class handles 2D rendering (e.g., quad-based drawing with Diligent’s pipeline states) and is used for elements like buttons, labels, and panels.
- **UIElement3D**: Extends `UIElement2D`, inheriting 2D properties and layout logic but adding 3D mesh-specific properties, such as:
  - **meshSrc**: Path to a 3D model file (e.g., GLTF for a health monitor).
  - **attachToBone**: String specifying a bone name for diegetic UI (e.g., “LeftForearmBone” for the health monitor).
  - **targetModel**: Path to the character model (e.g., battle suit GLTF) for bone attachments.
  - **shaderVertex**, **shaderFragment**: Paths to GLSL/HLSL shaders for custom effects (e.g., vertex displacement for button clicks or health pulses).
  - **offsetPos**, **offsetRot**: 3D vectors for local positioning/rotation relative to a bone or parent.
  - **billboardMode**: Enum (`none`, `full`, `yAxis`) to control whether the element faces the camera (e.g., for readability of the health monitor).
  - This class supports 3D rendering with Diligent’s mesh pipelines and bone-based transformations for diegetic UI.

This hierarchy allows 3D elements to reuse 2D layout logic (e.g., docking within a panel) while supporting specialized 3D features like bone attachments and vertex shaders. For example, a `HealthMonitor` (3D) can be docked like a 2D button in a panel but rendered with a mesh attached to a character’s forearm. This mirrors patterns in engines like Unity or Unreal, where 2D and 3D UI share a common base but diverge for rendering.

## Bang Markup Language (BML) Stylesheet Syntax
To avoid CSS’s complexity, you’ve designed **Bang Markup Language (BML)**, a YAML-inspired stylesheet syntax with a custom delimiter for object separation. BML uses a `#` symbol to denote objects (e.g., UI elements or selectors), followed by key-value pairs for properties, with indentation (using `>`) for clarity. This syntax is human-readable, easy to parse, and supports all your UI requirements, including 2D/3D properties, animations, and pseudo-states (e.g., `:hover`, `:disabled`). Key features include:

- **Object Separation**: Each element or selector is defined with a `#` prefix (e.g., `#btnPath` for a button, `#HealthMonitor` for the health monitor). This distinguishes BML from YAML’s standard structure, making it clear where one element’s definition ends and another begins.
  - Example:
    ```
    #btnPath:
    >texture: "assets/ui/theme/btnPath.png"
    >hDock: left
    >vDock: bottom
    >width: auto
    >padding: 10
    #HealthMonitor:
    >meshSrc: "assets/health_monitor.glb"
    >attachToBone: "LeftForearmBone"
    >billboardMode: yAxis
    ```
- **Selectors**: Support for element types (e.g., `#Button`), classes (e.g., `#.primary`), IDs (e.g., `#submitBtn`), and hierarchies (e.g., `#Panel > Button`). Pseudo-states like `#Button:hover` allow dynamic styling.
- **Variables**: A top-level section for reusable values (e.g., `primaryColor: "#007bff"`) to reduce repetition and support theming.
- **Themes**: Support for light/dark themes via a theme section, applying global overrides (e.g., background colors).
- **Media Queries**: Conditional styling for responsive designs (e.g., based on window size, like `minWidth: 768`).
- **Animations**: Definitions for transitions (e.g., smooth color changes on hover) and keyframe animations (e.g., pulsing for low health), including shader uniforms for 3D effects.
- **Inheritance**: An `extends` keyword to inherit properties from another selector (e.g., a custom button inheriting from `#Button`).

BML’s simplicity avoids CSS’s cascade and specificity issues, making it ideal for your needs. It will be parsed into Diligent pipeline states, textures, meshes, and shader uniforms, with SDL handling event-driven state changes (e.g., hover detection).

## UI Element Types
Your library supports a comprehensive set of UI elements, categorized by function, all inheriting from the appropriate base class (`UIElement`, `UIElement2D`, or `UIElement3D`). These were outlined to cover common UI needs while supporting your specific use case (e.g., the health monitor). The elements are:

### Layout Containers (UIElement2D)
- **Panel**: Groups elements with a layout mode (`stack`, `grid`, `absolute`) and overflow handling (`visible`, `hidden`, `scroll`).
- **StackLayout**: Arranges children vertically or horizontally with spacing and alignment options.
- **GridLayout**: Places children in a 2D grid with row/column definitions and spacing.
- **ScrollView**: Enables scrolling for overflow content, with horizontal/vertical scroll options.
- **TabContainer**: Displays tabs with selectable content panels, with tab positioning options.

### Input Controls (Mostly UIElement2D, Some UIElement3D for 3D Variants)
- **Button**: Clickable element with text, icon, and style (e.g., primary, danger). Supports 2D sprites or 3D meshes (e.g., a 3D button with vertex shader click animation).
- **TextInput**: Single-line text entry with placeholder, value, and input type (e.g., text, password).
- **TextArea**: Multi-line text input with row count and wrapping options.
- **Checkbox**: Toggleable boolean input with a label.
- **RadioButton**: Grouped selection with mutual exclusivity.
- **Slider**: Range selector with min/max, step, and orientation.
- **Dropdown**: Selectable list with single or multi-select options.
- **DatePicker**: Calendar-based date selection with format options.
- **ColorPicker**: Color selection with optional alpha channel.

### Display Elements (Mostly UIElement2D)
- **Label**: Static text with font styling and alignment.
- **Image**: Displays an image with scaling modes (e.g., contain, cover).
- **ProgressBar**: Shows progress (0-100) with indeterminate mode.
- **Divider**: Horizontal or vertical separator line.

### Advanced/Composite Elements (Mix of UIElement2D and UIElement3D)
- **ListView**: Scrollable list with item templates and virtualization for performance.
- **TreeView**: Hierarchical list with expandable nodes.
- **Table**: Data grid with sortable columns.
- **Menu**: Dropdown or context menu with nested items.
- **Tooltip**: Hover popup with content.
- **Canvas**: Custom drawing surface for lines/shapes.
- **WebView**: Embedded web content with URL or HTML.
- **HealthMonitor**: A diegetic 3D element attached to a character’s bone (e.g., “LeftForearmBone”), displaying health via a mesh with shader animations (e.g., pulsing when low).

## 3D Mesh Support and Diegetic UI
Your library supports **3D mesh-based elements**, particularly for diegetic UI like the health monitor attached to a battle suit’s forearm. Key decisions include:

- **Mesh Integration**: 3D elements use GLTF models (loaded via Diligent’s GLTF loader), supporting skeletal animations and named bones as standard attachment points (sockets). For example, the health monitor attaches to a bone like “LeftForearmBone” in the suit’s rigged model, following its transformations during animations.
- **Bone Sockets**: Rigged meshes (standard in game-ready GLTF/FBX files) include named bones, which your library uses as sockets. The health monitor’s position is computed by combining the bone’s world transform with a local offset (position/rotation), ensuring it moves with the character’s arm.
- **Billboard Mode**: Optional billboarding (`full` or `yAxis`) ensures the health monitor faces the camera for readability, adjustable via BML.
- **Shader Animations**: Vertex shaders animate 3D effects, such as displacing vertices for a button click or pulsing the health monitor when health is low (e.g., `<20%`). Uniforms (e.g., `healthValue`, `pulseStrength`) are updated in the render loop.

## Animation System
Your library supports **animations** for both 2D and 3D elements, integrated into BML:

- **Transitions**: Smooth property changes (e.g., `backgroundColor`, `opacity`, `scale`) when states change (e.g., hover). Defined in BML with duration and easing (e.g., `ease-in-out`).
- **Keyframes**: Sequence-based animations (e.g., pulsing red for low health) with percentage-based keyframes, duration, iterations, and shader uniforms. For example, the health monitor’s `lowHealth` animation flashes red and pulses via a vertex shader.
- **Implementation**: A timing loop (SDL-driven, ~60fps) updates animations by interpolating properties or uniforms based on progress and easing functions (e.g., linear, cubic). 3D elements pass uniforms to shaders for GPU-accelerated effects.

## Render Space for Containers
You’ve specified that only **container-type elements** (e.g., `Panel`, `Window`) will have a `renderSpace` property to determine whether they render in:
- **Screen Space**: 2D orthographic projection, typical for HUDs or menus, using `hDock` and `vDock` for layout.
- **World Space**: 3D world coordinates, often for diegetic UI, where the container’s transform aligns with a game object (e.g., a panel floating above a character). For the health monitor, this is handled by `attachToBone`, but a `Window` could be placed in world space for holographic displays.

This property ensures containers can host both 2D and 3D children consistently, with `UIElement3D` elements defaulting to world space when bone-attached.

## Rendering and Input with Diligent and SDL
- **Diligent Engine**: Handles rendering for both 2D (quads with textures) and 3D (meshes with shaders) elements. It abstracts backend differences (DirectX, Vulkan, etc.), with runtime backend selection via config. GLTF models are loaded for 3D elements, and pipeline states optimize 2D/3D rendering.
- **SDL**: Manages window creation, event polling (mouse, keyboard, gamepad), and the main loop. Events are dispatched to UI elements (e.g., clicks on 2D buttons via bounds checking, 3D elements via raycasting). For diegetic UI, SDL supports gestures (e.g., key press to zoom on the health monitor).
- **Integration**: The render loop updates animations, resolves layouts (docking, sizing), and calls `Render` on each element. 2D elements use orthographic matrices, while 3D elements use perspective or bone-based transforms.

## Specific Example: Health Monitor
The health monitor is a flagship 3D diegetic UI element, demonstrating your library’s capabilities:
- **Type**: `UIElement3D`, attached to the “LeftForearmBone” of a battle suit’s GLTF model.
- **Properties**: Includes `meshSrc` (e.g., a screen-like mesh), `attachToBone`, `targetModel`, `offsetPos`/`offsetRot` for positioning, and `billboardMode` for camera-facing options.
- **Styling**: Defined in BML (e.g., `#HealthMonitor`) with material properties (e.g., `emissiveColor`) and animations (e.g., `lowHealth` flashes red and pulses via a shader).
- **Behavior**: Updates `healthValue` (0-100) to scale a health bar or trigger effects. Interacts via SDL events (e.g., raycasted clicks or keybinds to focus).
- **Rendering**: Uses Diligent’s GLTF loader to follow the bone’s transform, with shaders for dynamic effects (e.g., vertex displacement for pulsing).

## Implementation Considerations
- **Performance**: Optimize 2D rendering with quad batching and 3D with low-poly meshes and instancing (for multiplayer). Diligent’s multi-backend support ensures consistency.
- **Testing**: Prototype with Diligent’s Tutorial13_SkeletalAnimation for bone attachments and Tutorial02_Cube for 2D quads. Test readability of diegetic UI (e.g., health monitor visibility in combat).
- **Asset Pipeline**: Ensure rigged GLTF models have consistent bone names (e.g., “LeftForearmBone”). Use Blender for creation/validation.
- **Fallbacks**: If a bone is missing, render in screen space or log errors. Provide non-diegetic HUD options for accessibility.
- **Extensibility**: The hybrid hierarchy and BML support future additions like VR (using SDL’s VR extensions and Diligent’s stereo rendering) or new element types.

## Conclusion
Your UI library is designed to be a powerful, flexible system for both 2D and 3D UI, with a hybrid class hierarchy (`UIElement` → `UIElement2D` → `UIElement3D`) that supports shared properties (`hDock`, `vDock`, `width`, `height`, `padding`) and specialized features (sprites for 2D, meshes and bone attachments for 3D). The **BML** stylesheet syntax, with its YAML-like structure and `#` object separators, provides a simple, CSS-free way to style elements, animations, and shaders. Container elements like panels and windows have a `renderSpace` property to toggle between screen and world space, enabling diegetic UI like the health monitor, which attaches to a battle suit’s forearm and uses vertex shaders for dynamic effects. Diligent Engine ensures robust multi-backend rendering, while SDL handles input and windowing, making your library suitable for immersive game UIs. This design supports your vision for a futuristic, interactive interface while remaining extensible and maintainable.