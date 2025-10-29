# Custom Close Button Texture

BangUI windows now support custom textures for the close button via the `closeSrc` property.

## Overview

Instead of the default red X button, you can specify a custom texture image to use for the close button on windows. This allows for complete visual customization to match your application's theme.

## Feature Details

### Default Close Button
- **Appearance**: Red background (RGB 200, 60, 60) with white X symbol
- **Size**: 16x16 pixels
- **Style**: Simple, clear, universally recognized

### Custom Close Button
- **Texture Support**: PNG, JPG, and other formats via SDL3_image
- **Alpha Channel**: Full RGBA8 support for transparency
- **Size**: Scaled to 16x16 pixels to fit in title bar
- **Automatic Fallback**: Uses default if texture fails to load

## Usage

### Using a Custom Close Button Texture

```cpp
Window* window = new Window();
window->title = "Custom Close Button";
window->closeable = true;
window->closeSrc = "assets/icons/close_button.png";
```

### Path Specifications

```cpp
// Relative path (from executable directory)
window->closeSrc = "icons/close.png";

// Absolute path
window->closeSrc = "D:/MyGame/assets/ui/close.png";

// Empty string or no assignment uses default X button
window->closeSrc = "";  // Default behavior
```

### Best Practices for Close Button Images

#### Recommended Specifications
- **Size**: 16x16 pixels (native size, no scaling artifacts)
- **Format**: PNG with alpha channel for transparency
- **Background**: Transparent background recommended
- **Icon Color**: Should contrast with title bar color
- **Style**: Simple, recognizable symbol (X, cross, etc.)

#### Design Guidelines
```cpp
// Good: Clear, simple icon with transparency
window->closeSrc = "icons/close_16x16.png";

// OK: Larger image will be scaled down
window->closeSrc = "icons/close_32x32.png";

// Not Recommended: Very large image (slower loading, scaling artifacts)
window->closeSrc = "icons/close_1024x1024.png";
```

## Implementation Details

### Window Property
[Window.h:23](../src/models/Window.h#L23)

```cpp
std::string closeSrc;  // Path to close button texture (empty = default X)
```

### Rendering Logic
[UIRenderer.h:311-337](../src/lib/UIRenderer.h#L311-L337)

```cpp
// Check if using custom texture
SDL_Texture* closeTexture = nullptr;
if (!win->closeSrc.empty()) {
    closeTexture = textureManager->loadTexture(win->closeSrc);
}

if (closeTexture) {
    // Render custom close button texture
    SDL_FRect closeBtn = {closeX, closeY,
                         Window::CLOSE_BUTTON_SIZE,
                         Window::CLOSE_BUTTON_SIZE};
    SDL_RenderTexture(renderer, closeTexture, nullptr, &closeBtn);
} else {
    // Default close button rendering (red background + white X)
    // ...
}
```

### Texture Caching
- Close button textures are cached by the TextureManager
- Multiple windows can share the same close button texture efficiently
- Textures remain loaded until application exit

## Examples

### Themed Windows

```cpp
// Dark theme window
Window* darkWindow = new Window();
darkWindow->closeable = true;
darkWindow->titleBarColor = Color(30, 30, 30, 255);
darkWindow->closeSrc = "themes/dark/close.png";

// Light theme window
Window* lightWindow = new Window();
lightWindow->closeable = true;
lightWindow->titleBarColor = Color(240, 240, 240, 255);
lightWindow->closeSrc = "themes/light/close.png";

// Game style window
Window* gameWindow = new Window();
gameWindow->closeable = true;
gameWindow->titleBarColor = Color(60, 40, 30, 255);
gameWindow->closeSrc = "ui/fantasy_close.png";
```

### Conditional Customization

```cpp
// Use custom button if file exists, otherwise default
Window* window = new Window();
window->closeable = true;

// Try to use custom, but TextureManager handles failure gracefully
window->closeSrc = "custom_close.png";
// If file doesn't exist, automatically falls back to default X
```

### Different Styles Per Window Type

```cpp
// Error window with alert-style close button
Window* errorWindow = new Window();
errorWindow->title = "Error";
errorWindow->closeable = true;
errorWindow->closeSrc = "icons/close_alert.png";
errorWindow->titleBarColor = Color(180, 30, 30, 255);

// Info window with standard close button
Window* infoWindow = new Window();
infoWindow->title = "Information";
infoWindow->closeable = true;
infoWindow->closeSrc = "icons/close_info.png";
infoWindow->titleBarColor = Color(40, 100, 180, 255);
```

## Creating Custom Close Button Images

### Photoshop/GIMP Template
```
Size: 16x16 pixels
Resolution: 72 DPI
Color Mode: RGBA
Background: Transparent

Recommended Elements:
- Center icon (X, cross, etc.)
- 2-3px padding from edges
- Anti-aliased edges
- High contrast against typical title bars
```

### Simple ASCII Art Representation
```
Default X button (16x16):
┌────────────────┐
│ ████████████   │  Red background
│ ███      ███   │
│ ████   ████    │  White X symbol
│ ████████       │
│ ████   ████    │
│ ███      ███   │
│ ████████████   │
└────────────────┘
```

### Code to Generate Simple Close Button (Pseudocode)
```python
# Create 16x16 PNG with transparent background
image = new Image(16, 16, RGBA)
image.fill(transparent)

# Draw X symbol
draw_line(image, (4,4) to (12,12), white, 2px)
draw_line(image, (12,4) to (4,12), white, 2px)

# Optional: Add circular background
draw_circle(image, center=(8,8), radius=7, color=red, alpha=200)

image.save("custom_close.png")
```

## Design Patterns

### Minimalist Style
```cpp
// Small, simple icon
window->closeSrc = "ui/minimal_close.png";
// Image content: thin lines, no background, 12x12 actual icon size
```

### Game UI Style
```cpp
// Thematic close button matching game aesthetic
window->closeSrc = "ui/medieval_close.png";
// Image content: ornate X, matches game art style
```

### Modern Flat Design
```cpp
// Flat design with solid color
window->closeSrc = "ui/flat_close.png";
// Image content: solid color X, no gradients or shadows
```

### Material Design
```cpp
// Material Design inspired
window->closeSrc = "ui/material_close.png";
// Image content: subtle shadow, elevation effect
```

## Error Handling

### Missing Texture File
```cpp
window->closeSrc = "nonexistent.png";
// Result: Falls back to default X button
// Console: "Unable to load image nonexistent.png! ..."
```

### Invalid File Format
```cpp
window->closeSrc = "close.txt";  // Not an image
// Result: Falls back to default X button
// Console: Error message from SDL_image
```

### Empty Path
```cpp
window->closeSrc = "";  // Empty string
// Result: Uses default X button (no error)
```

## Performance Considerations

### Texture Loading
- First use: Loads from disk (slight delay)
- Subsequent uses: Retrieved from cache (instant)
- Recommendation: Preload textures at startup for critical windows

### Memory Usage
- 16x16 RGBA8: 1,024 bytes per texture
- Shared across multiple windows using same texture
- Minimal memory impact

### Optimization Tips
```cpp
// Good: Share texture across windows
std::string closeIcon = "ui/close.png";
window1->closeSrc = closeIcon;
window2->closeSrc = closeIcon;  // Reuses cached texture

// Good: Use appropriate file size
// 16x16 PNG is ~500 bytes, loads quickly

// Less optimal: Different texture per window
window1->closeSrc = "ui/close1.png";
window2->closeSrc = "ui/close2.png";
// Each loads separately, uses more memory
```

## Accessibility Notes

### Contrast
- Ensure close button is visible against title bar
- Test with different title bar colors
- Provide sufficient contrast ratio (WCAG 2.1 AA: 4.5:1 minimum)

### Size
- 16x16 is minimum recommended size
- Icon should fill most of the space (12-14px active area)
- Clear, unambiguous symbol

### Fallback
- Default X button is universally recognized
- Custom icons should also be clearly identifiable as "close"

## Testing Checklist

- [ ] Custom texture loads correctly
- [ ] Close button appears in correct position
- [ ] Click area works (not just visual)
- [ ] Missing texture falls back to default
- [ ] Texture looks good on different backgrounds
- [ ] Alpha transparency works correctly
- [ ] No performance issues with texture loading

## Related Features

- **Texture Support**: [TEXTURE_SUPPORT.md](./TEXTURE_SUPPORT.md)
- **TextureManager**: [TextureManager.h](../src/lib/TextureManager.h)
- **Window Properties**: [Window.h](../src/models/Window.h)
- **Title Bar Colors**: [WINDOW_TITLEBAR_COLOR.md](./WINDOW_TITLEBAR_COLOR.md)

## Future Enhancements

Possible improvements:
- Hover state textures (different image on hover)
- Pressed state textures (visual feedback on click)
- Animated close buttons
- Vector-based close button (SVG support)
- Close button size customization
- Close button position customization (left vs right)
