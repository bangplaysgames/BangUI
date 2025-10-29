# 9-Patch Texture Support

BangUI now supports Android-style 9-patch (.9.png) textures for windows and panels, allowing scalable UI elements with proper corner preservation.

## Overview

9-patch textures (also known as 9-slice or nine-slice textures) are a powerful technique for creating scalable UI elements. They divide an image into 9 sections:

```
┌────┬──────────┬────┐
│ TL │   Top    │ TR │  TL/TR/BL/BR = Corners (never stretched)
├────┼──────────┼────┤  Top/Bottom = Edges (stretched horizontally)
│ L  │  Center  │ R  │  Left/Right = Edges (stretched vertically)
├────┼──────────┼────┤  Center = Middle (stretched both ways)
│ BL │  Bottom  │ BR │
└────┴──────────┴────┘
```

## Android .9.png Format

BangUI supports the Android 9-patch format, which uses a 1-pixel border to define stretch zones:

### File Naming
- Must end with `.9.png` extension
- Example: `button.9.png`, `textbox.9.png`, `panel.9.png`

### Border Pixels (Guide Pixels)
The 1-pixel border defines stretch zones using **black pixels** (RGB near 0, alpha > 200):

- **Top edge**: Defines horizontal stretch region
- **Left edge**: Defines vertical stretch region
- **Right edge** (optional): Defines content padding (not yet implemented)
- **Bottom edge** (optional): Defines content padding (not yet implemented)

### Visual Example
```
Original image (1134x656):
┌─────────────────────────────┐
│ ■■■■■■■■■■■■■■■             │  ← Top: Black pixels mark horizontal stretch
│■                          ■ │  ← Left/Right: Black marks vertical stretch
│■        Actual            ■ │
│■        Content           ■ │
│■        Area              ■ │
│             ■■■■■■■■■■■■■■■ │  ← Bottom: Marks content padding (optional)
└─────────────────────────────┘
```

After parsing:
- Border pixels are **excluded** from rendering
- Stretch zones are identified from black pixels
- Content area is 1132x654 (original minus 2-pixel border)

## Usage

### Basic Usage with Windows

```cpp
Window* window = new Window();
window->title = "Scalable Window";
window->src = "textbox.9.png";  // Auto-detected as 9-patch
window->width = 400;
window->height = 300;
```

### Basic Usage with Panels

```cpp
Panel* panel = new Panel();
panel->src = "panel.9.png";
panel->width = 250;
panel->height = 200;
```

### Path Specifications

```cpp
// Relative path (from executable directory)
window->src = "ui/button.9.png";

// Absolute path
window->src = "D:/MyGame/assets/ui/textbox.9.png";

// Auto-detection based on extension
// If filename ends with .9.png, 9-patch rendering is used
// Otherwise, regular texture stretching is used
```

## Implementation Details

### Detection
[NineSliceTexture.h:38-41](../src/lib/NineSliceTexture.h#L38-L41)

```cpp
static bool isNinePatchFile(const std::string& path) {
    if (path.length() < 6) return false;
    return path.substr(path.length() - 6) == ".9.png";
}
```

### Border Parsing
[NineSliceTexture.h:44-131](../src/lib/NineSliceTexture.h#L44-L131)

The parser:
1. Locks the SDL_Surface for pixel access
2. Scans the top edge (y=0) for black pixels to determine horizontal stretch
3. Scans the left edge (x=0) for black pixels to determine vertical stretch
4. Sets default stretch zones if no black pixels found (entire center stretches)

```cpp
// Parse top border (horizontal stretch)
for (int x = 1; x < w - 1; x++) {
    Uint32 pixel = pixels[x];
    Uint8 r, g, b, a;
    SDL_GetRGBA(pixel, SDL_GetPixelFormatDetails(surface->format), nullptr, &r, &g, &b, &a);

    bool isBlack = (a > 200) && (r < 50) && (g < 50) && (b < 50);

    if (isBlack && !foundStart) {
        stretchLeft = x - 1;  // Exclude border pixel
        foundStart = true;
    }
    // ... (continues for stretch region detection)
}
```

### Rendering
[NineSliceTexture.h:134-211](../src/lib/NineSliceTexture.h#L134-L211)

The renderer:
1. Calculates dimensions for all 9 patches
2. Renders corners at original size (no stretching)
3. Stretches edges in one direction only
4. Stretches center in both directions

```cpp
// Example: Top-left corner (never stretched)
renderPatch(renderer,
           contentLeft, contentTop, leftWidth, topHeight,
           x, y, destLeftWidth, destTopHeight);

// Example: Center (stretched both ways)
renderPatch(renderer,
           stretchLeft, stretchTop, centerWidth, centerHeight,
           x + destLeftWidth, y + destTopHeight,
           destCenterWidth, destCenterHeight);
```

### Integration with UIRenderer
[UIRenderer.h:156-188](../src/lib/UIRenderer.h#L156-L188)

UIRenderer automatically detects .9.png files and uses NineSliceTexture rendering:

```cpp
if (NineSliceTexture::isNinePatchFile(element->src)) {
    NineSliceTexture* nineSlice = textureManager->loadNineSliceTexture(element->src);
    if (nineSlice) {
        nineSlice->render(renderer, x, y, w, h);
        // Color overlay with alpha blending (if color alpha > 0)
        if (colorA > 0) {
            SDL_SetRenderDrawColor(renderer, colorR, colorG, colorB, colorA);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            renderRoundedRect(x, y, w, h, element->cornerRadius);
        }
        return;
    }
}
// Falls back to regular texture rendering if 9-patch fails
```

## Creating 9-Patch Textures

### Manual Creation (Photoshop/GIMP)

1. **Create your design**: Design at target resolution + 2 pixels (for border)
2. **Add 1-pixel border**: Expand canvas by 1 pixel on all sides
3. **Mark stretch zones**:
   - Use black pixels (#000000, full opacity) on top edge for horizontal stretch
   - Use black pixels on left edge for vertical stretch
4. **Mark content area** (optional):
   - Use black pixels on right edge for content right padding
   - Use black pixels on bottom edge for content bottom padding
5. **Save as PNG**: Export with filename ending in `.9.png`

### Example Dimensions

For a 200x100 design:
- Final image size: 202x102
- Content area: pixels (1,1) to (200,100)
- Border: pixels at (0,y), (x,0), (201,y), (x,101)

### Photoshop Action Script
```javascript
// Expand canvas by 1 pixel on all sides
app.activeDocument.resizeCanvas(
    app.activeDocument.width + 2,
    app.activeDocument.height + 2,
    AnchorPosition.MIDDLECENTER
);

// Create border layer
var borderLayer = app.activeDocument.artLayers.add();
borderLayer.name = "9-patch guides";

// Draw black pixel line on top edge (x: 50-150, y: 0) for horizontal stretch
// Draw black pixel line on left edge (x: 0, y: 25-75) for vertical stretch
// ... (use selection tools to paint black pixels)
```

### Testing Your 9-Patch

```cpp
// Test at different sizes to verify stretch behavior
Window* testWindow = new Window();
testWindow->src = "my_texture.9.png";

// Small size
testWindow->width = 150;
testWindow->height = 100;
// Verify corners look correct

// Large size
testWindow->width = 600;
testWindow->height = 400;
// Verify center stretches without distortion
```

## Common Stretch Patterns

### Full Center Stretch
```
Top edge:    ████████████████████  (all black)
Left edge:   █ (all black vertically)
             █
             █
Result: Entire center area stretches, only corner pixels preserved
```

### Partial Stretch (Preserve Decorative Borders)
```
Top edge:    ________████████_____  (black only in middle)
Left edge:   _
             █  (black only in middle)
             █
             _
Result: Corner decorations + edge borders preserved, only true center stretches
```

### No Stretch (Regular Texture)
```
Top edge:    ____________________  (no black pixels)
Left edge:   _
             _
             _
Result: Parser falls back to full center stretch as default
```

## Performance Considerations

### Memory
- 9-patch textures are cached by TextureManager
- Original image is loaded once, stored as SDL_Texture
- Multiple UI elements can share the same 9-patch texture efficiently

### Rendering
- 9 draw calls per element (one for each patch)
- Minimal CPU overhead for patch calculation (cached dimensions)
- GPU efficiently handles multiple textured quads

### Optimization Tips
```cpp
// Good: Share 9-patch across multiple elements
std::string buttonTex = "button.9.png";
button1->src = buttonTex;
button2->src = buttonTex;  // Reuses cached texture and parsed stretch zones
button3->src = buttonTex;

// Good: Use appropriate source image size
// A 200x100 source image works well for elements ranging 100-800 pixels
// No need for massive source images

// Avoid: Different 9-patch per element (unless necessary)
button1->src = "button1.9.png";
button2->src = "button2.9.png";  // Each loads separately
```

## Examples

### Scalable Button
```cpp
Window* button = new Window();
button->title = "Click Me";
button->src = "button.9.png";
button->width = 120;
button->height = 40;
button->resizable = false;
// 9-patch ensures rounded corners stay perfect at any button size
```

### Resizable Dialog Box
```cpp
Window* dialog = new Window();
dialog->title = "Settings";
dialog->src = "dialog.9.png";
dialog->width = 400;
dialog->height = 300;
dialog->resizable = true;
// 9-patch preserves decorative borders during window resize
```

### Themed Panel
```cpp
Panel* panel = new Panel();
panel->src = "panel_fantasy.9.png";
panel->width = 250;
panel->height = 200;
// 9-patch allows ornate corners/edges while center stretches
```

## Troubleshooting

### Black Lines Appear in Rendered Texture
**Problem**: Border guide pixels are being rendered

**Cause**: Image doesn't end with `.9.png` or parser failed

**Solution**:
- Ensure filename ends with `.9.png`
- Check console for "9-patch parsed" message
- Verify border pixels are exactly 1 pixel wide

### Stretching Looks Wrong
**Problem**: Wrong regions are stretching

**Cause**: Guide pixels are positioned incorrectly

**Solution**:
- Open image in image editor
- Verify black pixels on top/left edges mark correct zones
- Ensure black pixels are RGB(0,0,0) or very dark (R,G,B < 50)
- Ensure alpha is high (> 200)

### Corners Are Distorted
**Problem**: Corners stretch when they shouldn't

**Cause**: Stretch region extends too far to edges

**Solution**:
- Leave gap between stretch region and corners
- Example: For 100px wide image, mark pixels 25-75 as stretch zone, leaving 25px corners on each side

### Content Area Has Wrong Dimensions
**Problem**: Text/content doesn't fit properly

**Cause**: Content padding not yet implemented

**Status**:
- Right and bottom edge content padding is planned but not yet implemented
- Currently, content area is full interior (1 pixel inside border)

### "Unable to load 9-patch image" Error
**Problem**: SDL_image can't load the PNG file

**Cause**: SDL3_image doesn't have PNG support

**Solution**: Install SDL3_image with PNG feature:
```bash
cd vcpkg
./vcpkg install sdl3-image[png]:x64-windows --recurse
cmake --build build --config Debug
```

## SDL3 API Notes

### Pixel Access
SDL3 API for pixel format handling:
```cpp
// SDL3 (correct)
SDL_GetRGBA(pixel, SDL_GetPixelFormatDetails(surface->format), nullptr, &r, &g, &b, &a);

// SDL2 (incorrect for SDL3)
SDL_GetRGBA(pixel, surface->format, &r, &g, &b, &a);
```

### Surface Conversion
```cpp
// Convert to consistent RGBA8888 format
SDL_Surface* formattedSurface = SDL_ConvertSurface(loadedSurface, SDL_PIXELFORMAT_RGBA8888);
```

## Related Features

- **Texture Support**: [TEXTURE_SUPPORT.md](./TEXTURE_SUPPORT.md)
- **TextureManager**: [TextureManager.h](../src/lib/TextureManager.h)
- **NineSliceTexture**: [NineSliceTexture.h](../src/lib/NineSliceTexture.h)
- **UIRenderer**: [UIRenderer.h](../src/lib/UIRenderer.h)
- **Window Properties**: [Window.h](../src/models/Window.h)
- **Panel Properties**: [Panel.h](../src/models/Panel.h)

## Future Enhancements

Possible improvements:
- Content padding support (right/bottom border parsing)
- Automatic 9-patch generation from regular images
- Editor/visualizer for 9-patch stretch zones
- SVG-based scalable UI (alternative to 9-patch)
- Animated 9-patch textures
- Multiple stretch zones per edge (complex patterns)
- Tile mode vs stretch mode for center region
