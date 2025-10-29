// Concrete implementation details for UIElement (impl-only).
#pragma once
#include <string>
#include <map>
#include <cstdint>
#include <functional>

struct Color {
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;
    Color() = default;
    Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
        : r(red), g(green), b(blue), a(alpha) {}
};

enum class SizeMode { Absolute, Auto, Stretch };
enum class HDock { Left, Center, Right, None };
enum class VDock { Top, Center, Bottom, None };

struct TooltipConfig {
    std::string text;
    int delay = 500;
    std::string position = "auto";
    int maxWidth = 300;
};

class UIElement {
public:
    std::string id;
    std::string type;
    std::string className;

    HDock hDock = HDock::None;
    VDock vDock = VDock::None;

    SizeMode widthMode = SizeMode::Absolute;
    SizeMode heightMode = SizeMode::Absolute;
    float width = 100.0f;
    float height = 100.0f;

    float x = 0.0f;
    float y = 0.0f;

    float paddingTop = 0.0f;
    float paddingBottom = 0.0f;
    float paddingLeft = 0.0f;
    float paddingRight = 0.0f;

    float marginTop = 0.0f;
    float marginBottom = 0.0f;
    float marginLeft = 0.0f;
    float marginRight = 0.0f;

    Color backgroundColor = Color(255,255,255,255);
    std::string src;
    float borderWidth = 0.0f;
    Color borderColor = Color(0,0,0,255);
    float cornerRadius = 0.0f;

    float opacity = 1.0f;
    int zIndex = 0;

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

    UIElement* parent = nullptr;

    float contentLeft = 0.0f;
    float contentTop = 0.0f;
    float contentRight = 0.0f;
    float contentBottom = 0.0f;

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
