# Window Resizing Feature

BangUI now supports interactive window resizing through a visual resize handle in the bottom-right corner.

## Overview

When a window has `resizable = true`, users can resize it by dragging the resize handle that appears in the bottom-right corner when hovering over the window.

## Features

### 1. Resize Handle
- **Visual Indicator**: A semi-transparent gray triangle appears in the bottom-right corner
- **Hover-Based**: Only visible when the mouse is hovering over the window
- **Grip Lines**: Three diagonal lines provide visual feedback
- **Size**: 16x16 pixels by default (configurable via `Window::RESIZE_HANDLE_SIZE`)

### 2. Resize Behavior
- **Drag to Resize**: Click and drag the resize handle to change window dimensions
- **Minimum Size**: Windows have a minimum size of 100x50 pixels to prevent unusable windows
- **Live Resize**: Window updates in real-time as you drag
- **Independent of Movement**: Resizing doesn't interfere with window dragging

### 3. Visual Feedback
- **50% Transparent Gray Triangle**: Subtle indicator that doesn't obstruct content
- **Grip Pattern**: Three diagonal lines in the corner suggest draggability
- **Color**: RGB(128, 128, 128) at 50% opacity for the triangle, RGB(200, 200, 200) at ~70% opacity for grip lines

## Usage

### Enable Window Resizing

```cpp
Window* window = new Window();
window->title = "Resizable Window";
window->resizable = true;  // Enable resizing
window->width = 300;
window->height = 200;
```

### Configure Minimum Size (Optional)

The default minimum size is 100x50 pixels. To change this, you can modify the resize logic in UIManager:

```cpp
// In UIManager::handleWindowResize()
float newWidth = std::max(YOUR_MIN_WIDTH, resizeStartWidth + deltaX);
float newHeight = std::max(YOUR_MIN_HEIGHT, resizeStartHeight + deltaY);
```

### Disable Resizing

```cpp
window->resizable = false;  // Default value, resize handle won't appear
```

## Implementation Details

### Window Class
[Window.h:29](../src/models/Window.h#L29)
- Added `RESIZE_HANDLE_SIZE` constant (16.0f)
- Added `isPointInResizeHandle()` method to detect mouse position in resize area

### UIManager
[UIManager.h](../src/lib/UIManager.h)
- Tracks `resizingWindow` pointer and resize start position
- `handleMouseMove()`: Updates hover states for all windows
- `handleMouseDown()`: Detects resize handle clicks (prioritized before drag)
- `handleWindowResize()`: Calculates and applies new dimensions during drag
- `handleMouseUp()`: Releases resize state

### UIRenderer
[UIRenderer.h:370-396](../src/lib/UIRenderer.h#L370-L396)
- `drawResizeHandle()`: Renders the visual indicator
- Triangle is drawn using scanline filling
- Grip lines are rendered as diagonal strokes
- Only visible when `win->resizable && win->isHovered`

## Hover State Management

The resize handle only appears when the mouse is hovering over the window. This is managed automatically by UIManager:

```cpp
// Hover detection in UIManager::handleMouseMove()
bool nowHovered = mouseX >= win->x && mouseX <= win->x + win->width &&
                 mouseY >= win->y && mouseY <= win->y + win->height;
win->isHovered = nowHovered;
```

## Interaction Priority

When the user clicks on a window, the system checks in this order:
1. **Close button** (if closeable)
2. **Resize handle** (if resizable)
3. **Title bar** (if movable)

This ensures resize operations take precedence over dragging when the user clicks the corner.

## Resize Handle Detection

The resize handle occupies a 16x16 pixel square in the bottom-right corner:

```cpp
bool Window::isPointInResizeHandle(float px, float py) const {
    if (!resizable) return false;
    float handleX = x + width - RESIZE_HANDLE_SIZE;
    float handleY = y + height - RESIZE_HANDLE_SIZE;
    return px >= handleX && px <= x + width &&
           py >= handleY && py <= y + height;
}
```

## Visual Design

### Colors
- **Triangle Fill**: RGB(128, 128, 128, 128) - Medium gray at 50% opacity
- **Grip Lines**: RGB(200, 200, 200, 180) - Light gray at ~70% opacity
- **Blending**: Uses SDL `SDL_BLENDMODE_BLEND` for smooth transparency

### Appearance
```
┌─────────────────┐
│                 │
│     Window      │
│                 │
│               ╱ │  <- Resize handle
└─────────────╱───┘     (visible on hover)
```

The grip lines create a familiar "resize" visual pattern:
```
    ╱ ╱ ╱
```

## Best Practices

### When to Use Resizing
- **Content-Rich Windows**: Windows with dynamic content that benefits from more space
- **User Customization**: When users might want to adjust layout to their preferences
- **Multi-Purpose Windows**: Windows that display different amounts of content at different times

### When NOT to Use Resizing
- **Fixed-Size Dialogs**: Simple message boxes or confirmation dialogs
- **Icon Windows**: Small status indicators or tool palettes
- **Splash Screens**: Loading screens or about boxes

### UX Considerations
1. **Default Size**: Set a sensible default size that works for most content
2. **Minimum Size**: Ensure minimum size is large enough to show critical UI elements
3. **Content Reflow**: Make sure window content reflows appropriately when resized
4. **Aspect Ratio**: Consider whether you need to maintain aspect ratio (not currently supported)

## Example Code

### Basic Resizable Window

```cpp
Window* window = new Window();
window->title = "My Resizable Window";
window->width = 400;
window->height = 300;
window->resizable = true;
window->movable = true;
window->closeable = true;
```

### Tool Window (Not Resizable)

```cpp
Window* toolWindow = new Window();
toolWindow->title = "Tools";
toolWindow->width = 200;
toolWindow->height = 150;
toolWindow->resizable = false;  // Keep fixed size
toolWindow->movable = true;
```

## Performance Notes

- **Hover Detection**: Runs every mouse move event, but uses simple bounds checking (O(n) where n = number of windows)
- **Rendering**: Resize handle only renders when hovering, minimal performance impact
- **Live Resize**: Window dimensions update immediately, layout system should handle efficiently

## Limitations

### Current Implementation
- No maximum size constraint (grows indefinitely)
- No aspect ratio locking
- Single resize handle (bottom-right only)
- No edge resizing (top, left, etc.)
- No keyboard-based resizing

### Potential Future Enhancements
- Eight resize handles (corners + edges)
- Aspect ratio constraints
- Maximum size limits
- Snap-to-grid resizing
- Keyboard shortcuts for precise resizing
- Animation when programmatically resizing

## Debugging

If resize handle isn't appearing:
1. Check `window->resizable = true`
2. Verify window is being hovered (check `isHovered` flag)
3. Ensure window dimensions are large enough (>16px in both dimensions)
4. Confirm UIManager is processing `SDL_EVENT_MOUSE_MOTION` events

If resizing feels sluggish:
1. Check render loop frame rate
2. Ensure layout recalculation isn't too expensive
3. Profile `handleWindowResize()` execution time

## Related Features

- **Window Movement**: [UIManager.h](../src/lib/UIManager.h)
- **Close Button**: [Window.h](../src/models/Window.h)
- **Custom Title Bar Colors**: [WINDOW_TITLEBAR_COLOR.md](./WINDOW_TITLEBAR_COLOR.md)
- **Texture Support**: [TEXTURE_SUPPORT.md](./TEXTURE_SUPPORT.md)
