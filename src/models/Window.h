#pragma once
#include "UIElement.h"
#include "Mesh.h"
#include <vector>

class Window : public UIElement {
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

    // Title bar color with full RGBA support
    Color titleBarColor = Color(80, 80, 120, 255); // Default dark blue-gray

    // Custom close button texture (optional)
    std::string closeSrc; // Path to close button texture (if empty, uses default X)

    // Window-specific rendering constants
    static constexpr float TITLE_BAR_HEIGHT = 30.0f;
    static constexpr float CLOSE_BUTTON_SIZE = 16.0f;
    static constexpr float CLOSE_BUTTON_MARGIN = 2.0f;
    static constexpr float RESIZE_HANDLE_SIZE = 16.0f;

    // Check if point is in title bar (for dragging)
    bool isPointInTitleBar(float px, float py) const {
        return px >= x && px <= x + width &&
               py >= y && py <= y + TITLE_BAR_HEIGHT;
    }

    // Check if point is in close button
    bool isPointInCloseButton(float px, float py) const {
        if (!closeable) return false;
        float closeX = x + width - CLOSE_BUTTON_SIZE - CLOSE_BUTTON_MARGIN;
        float closeY = y + CLOSE_BUTTON_MARGIN;
        return px >= closeX && px <= closeX + CLOSE_BUTTON_SIZE &&
               py >= closeY && py <= closeY + CLOSE_BUTTON_SIZE;
    }

    // Check if point is in resize handle (bottom-right corner)
    bool isPointInResizeHandle(float px, float py) const {
        if (!resizable) return false;
        // Triangle area in bottom-right corner
        float handleX = x + width - RESIZE_HANDLE_SIZE;
        float handleY = y + height - RESIZE_HANDLE_SIZE;
        return px >= handleX && px <= x + width &&
               py >= handleY && py <= y + height;
    }

    bool isModal() const { return modal; }
};
