# Window Title Bar Color Support

BangUI windows now support customizable title bar colors with full RGBA8 color values, including alpha transparency.

## Feature Overview

The `Window` class now has a `titleBarColor` property that allows full control over the title bar appearance:
- **R, G, B**: Color channels (0-255)
- **A (Alpha)**: Transparency (0=fully transparent, 255=fully opaque)

## Property

### Location
[Window.h:20](../src/models/Window.h#L20)

### Type
```cpp
Color titleBarColor;
```

### Default Value
```cpp
Color(80, 80, 120, 255)  // Dark blue-gray, fully opaque
```

## Usage Examples

### Basic Solid Colors

```cpp
Window* window = new Window();
window->title = "My Window";

// Red title bar
window->titleBarColor = Color(200, 50, 50, 255);

// Blue title bar
window->titleBarColor = Color(50, 100, 200, 255);

// Green title bar
window->titleBarColor = Color(50, 180, 50, 255);

// Dark gray title bar
window->titleBarColor = Color(60, 60, 60, 255);
```

### Semi-Transparent Title Bars

```cpp
Window* window = new Window();

// 75% opaque red title bar
window->titleBarColor = Color(200, 50, 50, 192);

// 50% opaque blue title bar
window->titleBarColor = Color(50, 100, 200, 128);

// 25% opaque green title bar
window->titleBarColor = Color(50, 180, 50, 64);
```

### Themed Windows

```cpp
// Error/Warning window with red title bar
Window* errorWindow = new Window();
errorWindow->title = "Error";
errorWindow->titleBarColor = Color(180, 30, 30, 255);

// Success window with green title bar
Window* successWindow = new Window();
successWindow->title = "Success";
successWindow->titleBarColor = Color(30, 150, 30, 255);

// Info window with blue title bar
Window* infoWindow = new Window();
infoWindow->title = "Information";
infoWindow->titleBarColor = Color(40, 100, 180, 255);
```

## Implementation Details

### Rendering
The `UIRenderer::renderWindow()` method now reads the `titleBarColor` property directly from the window object instead of receiving it as a parameter. This simplifies the API and makes window rendering more self-contained.

**Old signature:**
```cpp
void renderWindow(const Window* win, Uint8 titleR, Uint8 titleG, Uint8 titleB,
                 Uint8 bodyR, Uint8 bodyG, Uint8 bodyB);
```

**New signature:**
```cpp
void renderWindow(const Window* win, Uint8 bodyR, Uint8 bodyG, Uint8 bodyB);
```

### Alpha Channel Support
The alpha channel is fully respected during rendering:
- Uses SDL's `SDL_SetRenderDrawColor()` with the alpha value
- Blends properly with elements behind the window
- Works with both rounded and square corners

### Compatibility
- Works seamlessly with textured windows
- Compatible with all window corner radius settings
- No performance impact compared to previous hardcoded colors

## Design Rationale

### Why RGBA8?
- **Industry Standard**: RGBA8 (8 bits per channel) is the standard color format in graphics
- **Full Control**: Provides complete control over color and transparency
- **Performance**: Efficient and well-supported by SDL3
- **Consistency**: Matches other color properties in BangUI (backgroundColor, borderColor)

### Why a Property Instead of Parameters?
- **Encapsulation**: Window owns its visual properties
- **Simpler API**: Less parameters to pass around
- **Flexibility**: Easy to change programmatically or load from config
- **Consistency**: Matches the pattern used for body colors (backgroundColor)

## Best Practices

### Usability
1. **Contrast**: Ensure title text will be visible against the title bar color
2. **Transparency**: Avoid fully transparent title bars (alpha=0) as they make windows hard to interact with
3. **Consistency**: Use similar title bar colors for related windows

### Theming
Consider creating color constants or a theme system:

```cpp
// Define theme colors
const Color THEME_PRIMARY_TITLE = Color(60, 100, 180, 255);
const Color THEME_SECONDARY_TITLE = Color(80, 80, 120, 255);
const Color THEME_ERROR_TITLE = Color(180, 30, 30, 255);
const Color THEME_SUCCESS_TITLE = Color(30, 150, 30, 255);

// Apply to windows
window1->titleBarColor = THEME_PRIMARY_TITLE;
window2->titleBarColor = THEME_SECONDARY_TITLE;
```

### Accessibility
- Maintain sufficient contrast between title bar and body
- Consider colorblind users when choosing colors
- Test with different opacity values

## Examples in Code

### Main Demo
[sdl_diligent_demo.cpp](../src/lib/sdl_diligent_demo.cpp) shows:
- Window with dark red title bar
- Window with purple title bar

### Texture Demo
[texture_demo.cpp](../src/examples/texture_demo.cpp) shows:
- Green title bar matching textured window body
- Semi-transparent purple title bar

## Future Enhancements

Possible improvements:
- Title bar gradients
- Title bar textures/images
- Animated title bar colors
- Theme system with predefined color palettes
- Title bar hover/active state colors
