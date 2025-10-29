#pragma once
#include "../api/UIElement.h"
#include "../models/Mesh.h"
#include <string>
#include <vector>

namespace BangUI::impl {

class PanelImpl : public BangUI::API::UIElement {
public:
    bool resizable = false;
    bool movable = true;
    bool modal = false;
    std::vector<UIElement*> content;
    std::string renderSpace = "orthographic"; // "orthographic" or "diegetic"
    Mesh* anchorMesh = nullptr; // only if diegetic
    std::string anchorBone; // only if diegetic
    std::string scrollable; // "vertical", "horizontal", "both"

    bool isPointInPanel(float px, float py) const {
        return px >= x && px <= x + width &&
               py >= y && py <= y + height;
    }

    // Safe content area helpers (match UIElement helpers expectations)
    float getSafeContentLeft() const { return x + contentLeft + paddingLeft; }
    float getSafeContentTop() const { return y + contentTop + paddingTop; }
    float getSafeContentRight() const { return x + width - contentRight - paddingRight; }
    float getSafeContentBottom() const { return y + height - contentBottom - paddingBottom; }

    // Convenience: check if a point is inside the panel's content area
    bool isPointInContent(float px, float py) const {
        float left = getSafeContentLeft();
        float top = getSafeContentTop();
        float right = getSafeContentRight();
        float bottom = getSafeContentBottom();
        return px >= left && px <= right && py >= top && py <= bottom;
    }
};

} // namespace BangUI::impl
