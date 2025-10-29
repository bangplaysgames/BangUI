#pragma once
#include "../api/UIElement.h"
#include "../models/Mesh.h"
#include <string>
#include <vector>

namespace BangUI::impl {

class WindowImpl : public BangUI::API::UIElement {
public:
    std::string title;
    bool resizable = false;
    bool movable = true;
    bool modal = false;
    bool closeable = false;
    std::vector<UIElement*> content;
    std::string renderSpace = "orthographic"; // "orthographic" or "diegetic"
    Mesh* anchorMesh = nullptr; // only if diegetic
    std::string anchorBone; // only if diegetic
    std::string scrollable; // "vertical", "horizontal", "both"

    Color titleBarColor = Color(80,80,120,255);
    std::string closeSrc;

    static constexpr float TITLE_BAR_HEIGHT = 30.0f;
    static constexpr float CLOSE_BUTTON_SIZE = 16.0f;
    static constexpr float CLOSE_BUTTON_MARGIN = 2.0f;
    static constexpr float RESIZE_HANDLE_SIZE = 16.0f;

    bool isPointInTitleBar(float px, float py) const {
        return px >= x && px <= x + width &&
               py >= y && py <= y + TITLE_BAR_HEIGHT;
    }

    bool isPointInCloseButton(float px, float py) const {
        if (!closeable) return false;
        float closeX = x + width - CLOSE_BUTTON_SIZE - CLOSE_BUTTON_MARGIN;
        float closeY = y + CLOSE_BUTTON_MARGIN;
        return px >= closeX && px <= closeX + CLOSE_BUTTON_SIZE &&
               py >= closeY && py <= closeY + CLOSE_BUTTON_SIZE;
    }

    bool isPointInResizeHandle(float px, float py) const {
        if (!resizable) return false;
        float handleX = x + width - RESIZE_HANDLE_SIZE;
        float handleY = y + height - RESIZE_HANDLE_SIZE;
        return px >= handleX && px <= x + width &&
               py >= handleY && py <= y + height;
    }

    // Safe content area helpers
    float getSafeContentLeft() const { return x + contentLeft + paddingLeft; }
    float getSafeContentTop() const { return y + contentTop + paddingTop; }
    float getSafeContentRight() const { return x + width - contentRight - paddingRight; }
    float getSafeContentBottom() const { return y + height - contentBottom - paddingBottom; }

    // Get close button rectangle
    void getCloseButtonRect(float& outX, float& outY, float& outW, float& outH) const {
        outW = CLOSE_BUTTON_SIZE; outH = CLOSE_BUTTON_SIZE;
        outX = x + width - CLOSE_BUTTON_SIZE - CLOSE_BUTTON_MARGIN;
        outY = y + CLOSE_BUTTON_MARGIN;
    }

    // Helper to clamp window size to minimums
    void ensureMinSize(float minW = 100.0f, float minH = 50.0f) {
        if (width < minW) width = minW;
        if (height < minH) height = minH;
    }

    bool isModal() const { return modal; }
};

} // namespace BangUI::impl
