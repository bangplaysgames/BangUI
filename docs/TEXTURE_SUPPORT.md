# Texture Support in BangUI

BangUI now supports loading and rendering textures on Windows and Panels with proper alpha channel handling and color blending.

## Features

### 1. Texture Loading
- Supports PNG, JPG, and other common image formats via SDL3_image
- Automatic RGBA8 format conversion for consistent alpha channel support
- Texture caching to avoid loading the same texture multiple times
- Graceful fallback to solid colors if texture fails to load

### 2. Alpha Channel Support
- Textures are loaded in RGBA8888 format
- Full alpha transparency support from source images
- Proper alpha blending between texture and overlay colors
- Window title bars support RGBA8 colors with alpha transparency

### 3. Color Blending
- The `backgroundColor.a` (alpha) property controls color overlay intensity
- Alpha = 0: Pure texture (no color overlay)
- Alpha = 255: Pure color (texture fully covered)
- Alpha = 128: 50% blend between texture and color
- Color blending uses SDL's BLEND mode for smooth overlay effects

### 4. Window Title Bar Colors
- Full RGBA8 color control for window title bars via `titleBarColor` property
- Supports transparent title bars (alpha channel)
- Default color: Color(80, 80, 120, 255) - dark blue-gray

### 5. Custom Close Button Textures
- Windows can use custom textures for close buttons via `closeSrc` property
- Supports all image formats (PNG, JPG, etc.)
- Automatic fallback to default X button if texture fails to load
- Full alpha transparency support

## Usage

### Setting a Texture on a Panel

```cpp
Panel* panel = new Panel();
panel->src = "path/to/texture.png";  // Set texture path

// Optional: Add color tint/overlay with alpha blending
panel->backgroundColor = Color(255, 100, 100, 128);  // Red tint at 50% opacity
```

### Setting a Texture on a Window

```cpp
Window* window = new Window();
window->src = "path/to/texture.png";  // Texture for window body

// Optional: Add color overlay
window->backgroundColor = Color(100, 255, 100, 100);  // Green tint

// Optional: Customize title bar color with RGBA
window->titleBarColor = Color(50, 150, 50, 255);  // Dark green title bar
```

### Customizing Window Title Bar Colors

```cpp
Window* window = new Window();

// Solid opaque title bar
window->titleBarColor = Color(150, 30, 30, 255);  // Dark red

// Semi-transparent title bar
window->titleBarColor = Color(120, 60, 180, 200);  // Purple with 78% opacity

// Fully transparent title bar (not recommended for usability)
window->titleBarColor = Color(0, 0, 0, 0);  // Invisible
```

### Custom Close Button Texture

```cpp
Window* window = new Window();
window->closeable = true;

// Use custom close button texture
window->closeSrc = "icons/close_button.png";

// Empty string or no assignment uses default X button
window->closeSrc = "";  // Default X button
```

### Color Blending Examples

```cpp
// Pure texture (no color overlay)
element->src = "texture.png";
element->backgroundColor = Color(0, 0, 0, 0);  // Transparent overlay

// Light red tint over texture
element->src = "texture.png";
element->backgroundColor = Color(255, 100, 100, 64);  // 25% red overlay

// Heavy blue tint over texture
element->src = "texture.png";
element->backgroundColor = Color(100, 100, 255, 192);  // 75% blue overlay

// Solid color (no texture visible)
element->src = "texture.png";
element->backgroundColor = Color(255, 255, 255, 255);  // 100% opaque white
```

## Implementation Details

### TextureManager Class
Located in `src/lib/TextureManager.h`:
- Manages texture loading and caching
- Handles SDL3_image integration
- Converts all textures to RGBA8888 format
- Automatically sets blend mode for alpha support
- Cleans up textures on destruction

### UIRenderer Updates
Located in `src/lib/UIRenderer.h`:
- New methods: `renderTexturedRectangle()` and `renderTexturedRoundedRectangle()`
- Renders texture first, then applies color overlay based on alpha
- Supports both rounded and sharp corners with textures
- Maintains existing rendering for non-textured elements

### UIElement Property
Located in `src/lib/UIElement.h`:
- New `src` property: `std::string src;`
- Path to texture image file
- Empty string means no texture (solid color rendering)

### Window Title Bar Color
Located in `src/models/Window.h`:
- New `titleBarColor` property: `Color titleBarColor;`
- Full RGBA8 support for window title bars
- Default value: `Color(80, 80, 120, 255)` (dark blue-gray)
- Alpha channel controls title bar transparency

## Rounded Corners with Textures

When using `cornerRadius` with textures:
- The texture is rendered in full rectangle
- Color overlay is applied with rounded shape
- For perfect rounded texture masking, consider using pre-masked texture images

## Performance Considerations

1. **Texture Caching**: Textures are cached by path, so multiple elements can share the same texture efficiently
2. **RGBA8 Format**: All textures are converted to RGBA8888 for consistency, which may use more memory than compressed formats
3. **Blend Mode**: Alpha blending has minimal performance impact with modern hardware

## Error Handling

- If a texture file doesn't exist or fails to load, the element falls back to solid color rendering
- Error messages are logged to stderr for debugging
- No crashes occur from missing textures

## Example Demos

### Texture Demo
See `src/examples/texture_demo.cpp` for a complete working example showing:
- Multiple panels with the same texture but different color overlays
- Windows with textured bodies and custom title bar colors
- Comparison between textured and solid-color elements
- Alpha blending demonstrations
- Semi-transparent title bars

### Main Demo
See `src/lib/sdl_diligent_demo.cpp` for the main demo showing:
- Custom title bar colors on windows
- Both opaque and semi-transparent title bars
- Integration with the layout system

## Supported Image Formats

Via SDL3_image, the following formats are supported:
- PNG (with full alpha channel)
- JPG/JPEG
- BMP
- GIF
- TGA
- WEBP
- And others supported by SDL3_image

## Future Enhancements

Possible improvements:
- Texture tiling/stretching modes
- UV coordinate control
- Texture rotation and scaling
- Perfect rounded corner masking using clip regions
- Texture animations
- Nine-patch/9-slice scaling for UI elements
