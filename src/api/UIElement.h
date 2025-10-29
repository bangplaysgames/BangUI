// Public API containing the full UIElement functionality migrated from src/lib/UIElement.h
#pragma once
#include "Types.h"
#include <string>
#include <map>
#include <functional>

namespace BangUI::API {

// Tooltip configuration
struct TooltipConfig {
    std::string text;
    int delay = 500; // milliseconds
    std::string position = "auto"; // top, bottom, left, right, auto
    int maxWidth = 300; // pixels
};

// UIElement: Base class for all UI elements — full concrete API surface.
class UIElement {
public:
    // Identity
    std::string id;
    std::string type;
    std::string className; // For BML styling (e.g., "primary", "danger")

    // === LAYOUT & POSITIONING ===

    // Docking/Alignment
    HDock hDock = HDock::None;   // Horizontal docking
    VDock vDock = VDock::None;   // Vertical docking

    // Size
    SizeMode widthMode = SizeMode::Absolute;
    SizeMode heightMode = SizeMode::Absolute;
    float width = 100.0f;
    float height = 100.0f;

    // Position
    float x = 0.0f;
    float y = 0.0f;

    // Padding
    float paddingTop = 0.0f;
    float paddingBottom = 0.0f;
    float paddingLeft = 0.0f;
    float paddingRight = 0.0f;

    // Margin
    float marginTop = 0.0f;
    float marginBottom = 0.0f;
    float marginLeft = 0.0f;
    float marginRight = 0.0f;

    // Visual
    Color backgroundColor = Color{255,255,255,255};
    std::string src;
    float borderWidth = 0.0f;
    Color borderColor = Color{0,0,0,255};
    float cornerRadius = 0.0f;

    // Opacity & z
    float opacity = 1.0f;
    int zIndex = 0;

    // Interaction
    bool visible = true;
    bool enabled = true;
    bool focusable = false;
    int tabIndex = 0;
    bool interactable = true;

    TooltipConfig* tooltip = nullptr;

    std::function<void()> onClick = nullptr;
    std::function<void()> onHover = nullptr;
    std::function<void()> onFocus = nullptr;
    std::function<void()> onBlur = nullptr;

    // Hierarchy
    UIElement* parent = nullptr;

    // Safe content insets
    float contentLeft = 0.0f;
    float contentTop = 0.0f;
    float contentRight = 0.0f;
    float contentBottom = 0.0f;

    // Runtime state
    bool isBeingDragged = false;
    float dragOffsetX = 0.0f;
    float dragOffsetY = 0.0f;
    bool isHovered = false;
    bool isFocused = false;
    bool manualPosition = false;
    float localX = 0.0f;
    float localY = 0.0f;

    std::map<std::string, std::string> properties;

    virtual ~UIElement() {
        if (tooltip) delete tooltip;
    }

    void setPadding(float padding) {
        paddingTop = paddingBottom = paddingLeft = paddingRight = padding;
    }
    void setMargin(float margin) {
        marginTop = marginBottom = marginLeft = marginRight = margin;
    }
    float getContentWidth() const { return width - paddingLeft - paddingRight; }
    float getContentHeight() const { return height - paddingTop - paddingBottom; }
    float getTotalWidth() const { return marginLeft + width + marginRight; }
    float getTotalHeight() const { return marginTop + height + marginBottom; }
    float getSafeContentWidth() const { return (width - contentLeft - contentRight) - paddingLeft - paddingRight; }
    float getSafeContentHeight() const { return (height - contentTop - contentBottom) - paddingTop - paddingBottom; }
    float getSafeContentLeft() const { return x + contentLeft + paddingLeft; }
    float getSafeContentTop() const { return y + contentTop + paddingTop; }
    float getSafeContentRight() const { return x + width - contentRight - paddingRight; }
    float getSafeContentBottom() const { return y + height - contentBottom - paddingBottom; }
};

} // namespace BangUI::API

// Backwards-compatibility aliases in the BangUI namespace so existing
// implementation headers (which live in BangUI::impl) can refer to
// UIElement, Color, and the enums without fully qualifying API::.
namespace BangUI {
    using API::Color;
    using API::SizeMode;
    using API::HDock;
    using API::VDock;
    using API::TooltipConfig;
    using API::UIElement;
}

// Compatibility: also make some API symbols available at global scope for
// existing implementation headers that expect unqualified names like
// `UIElement` or `Color`. This is a minimal shim and does not create new
// type definitions (it's a using-declaration only).
using BangUI::API::UIElement;
using BangUI::API::Color;
using BangUI::API::SizeMode;
using BangUI::API::HDock;
using BangUI::API::VDock;
using BangUI::API::TooltipConfig;
