#pragma once
#include <string>
#include <map>
#include <cstdint>
#include <functional>

// RGBA Color structure for consistent color representation
struct Color {
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255; // Alpha channel

    Color() = default;
    Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
        : r(red), g(green), b(blue), a(alpha) {}
};

// Size modes for width and height
enum class SizeMode {
    Absolute,  // Fixed pixel value
    Auto,      // Size based on content
    Stretch    // Fill parent's available space
};

// Docking options
enum class HDock {
    Left,
    Center,
    Right,
    None
};

enum class VDock {
    Top,
    Center,
    Bottom,
    None
};

// Tooltip configuration
struct TooltipConfig {
    std::string text;
    int delay = 500; // milliseconds
    std::string position = "auto"; // top, bottom, left, right, auto
    int maxWidth = 300; // pixels
};

// UIElement: Base class for all UI elements
// Defines global properties that ALL UI elements inherit per Overview.md
class UIElement {
public:
    // Identity
    std::string id;
    std::string type;
    std::string className; // For BML styling (e.g., "primary", "danger")

    // === LAYOUT & POSITIONING (Global Properties from Overview.md) ===

    // Docking/Alignment
    HDock hDock = HDock::None;   // Horizontal docking: left, center, right, fill
    VDock vDock = VDock::None;    // Vertical docking: top, center, bottom, fill

    // Size
    SizeMode widthMode = SizeMode::Absolute;
    SizeMode heightMode = SizeMode::Absolute;
    float width = 100.0f;   // In pixels when mode is Absolute
    float height = 100.0f;  // In pixels when mode is Absolute

    // Position (for absolute positioning, calculated by layout system)
    float x = 0.0f;
    float y = 0.0f;

    // Padding (internal spacing from edges, in pixels)
    float paddingTop = 0.0f;
    float paddingBottom = 0.0f;
    float paddingLeft = 0.0f;
    float paddingRight = 0.0f;

    // Margin (external spacing from edges, in pixels)
    float marginTop = 0.0f;
    float marginBottom = 0.0f;
    float marginLeft = 0.0f;
    float marginRight = 0.0f;

    // === VISUAL PROPERTIES (Global Properties from Overview.md) ===

    Color backgroundColor = Color(255, 255, 255, 255);

    // Texture
    std::string src; // Path to texture image file (PNG, JPG, etc.)

    // Border
    float borderWidth = 0.0f;
    Color borderColor = Color(0, 0, 0, 255);
    float cornerRadius = 0.0f; // For rounded corners (borderRadius)

    // Opacity
    float opacity = 1.0f; // 0.0 (transparent) to 1.0 (opaque)

    // Z-ordering
    int zIndex = 0; // Stacking order for overlapping elements

    // === INTERACTION PROPERTIES (Global Properties from Overview.md) ===

    bool visible = true;   // Show/hide element
    bool enabled = true;   // Enable/disable interaction
    bool focusable = false; // Can receive keyboard focus
    int tabIndex = 0;      // Keyboard navigation order

    // Whether the element can interact with mouse events. When false, clicks pass through
    // to underlying elements.
    bool interactable = true;

    // Tooltip (global property)
    TooltipConfig* tooltip = nullptr; // Optional tooltip

    // === EVENT HANDLERS (Global Properties from Overview.md) ===

    std::function<void()> onClick = nullptr;
    std::function<void()> onHover = nullptr;
    std::function<void()> onFocus = nullptr;
    std::function<void()> onBlur = nullptr;

    // === CONTAINER HIERARCHY ===

    UIElement* parent = nullptr; // Parent container (nullptr = application window)

    // Safe area for content (affected by 9-patch borders)
    // These define the area where child content should be placed
    float contentLeft = 0.0f;   // Left edge of safe content area
    float contentTop = 0.0f;    // Top edge of safe content area
    float contentRight = 0.0f;  // Right edge of safe content area (relative to element)
    float contentBottom = 0.0f; // Bottom edge of safe content area (relative to element)

    // === RUNTIME STATE (Managed by UIManager/Renderer) ===

    bool isBeingDragged = false;
    float dragOffsetX = 0.0f;
    float dragOffsetY = 0.0f;
    bool isHovered = false;
    bool isFocused = false;
    // If true, the element's absolute position was set manually (by user dragging)
    // and the layout system should avoid overwriting its x/y during layout passes.
    bool manualPosition = false;
    // Position relative to parent safe content origin (used for nested layout)
    float localX = 0.0f;
    float localY = 0.0f;

    // === LEGACY/EXTENSIBILITY ===

    // Properties map for future extensibility and BML custom properties
    std::map<std::string, std::string> properties;

    // === DESTRUCTOR ===

    virtual ~UIElement() {
        if (tooltip) delete tooltip;
    }

    // === HELPER METHODS ===

    // Set padding for all sides at once
    void setPadding(float padding) {
        paddingTop = paddingBottom = paddingLeft = paddingRight = padding;
    }

    // Set margin for all sides at once
    void setMargin(float margin) {
        marginTop = marginBottom = marginLeft = marginRight = margin;
    }

    // Get computed width (accounting for padding)
    float getContentWidth() const {
        return width - paddingLeft - paddingRight;
    }

    // Get computed height (accounting for padding)
    float getContentHeight() const {
        return height - paddingTop - paddingBottom;
    }

    // Get total width including margin
    float getTotalWidth() const {
        return marginLeft + width + marginRight;
    }

    // Get total height including margin
    float getTotalHeight() const {
        return marginTop + height + marginBottom;
    }

    // Get safe content area width (accounting for padding and content bounds)
    float getSafeContentWidth() const {
        return (width - contentLeft - contentRight) - paddingLeft - paddingRight;
    }

    // Get safe content area height (accounting for padding and content bounds)
    float getSafeContentHeight() const {
        return (height - contentTop - contentBottom) - paddingTop - paddingBottom;
    }

    // Get absolute safe content bounds in screen coordinates
    float getSafeContentLeft() const {
        return x + contentLeft + paddingLeft;
    }

    float getSafeContentTop() const {
        return y + contentTop + paddingTop;
    }

    float getSafeContentRight() const {
        return x + width - contentRight - paddingRight;
    }

    float getSafeContentBottom() const {
        return y + height - contentBottom - paddingBottom;
    }
};
