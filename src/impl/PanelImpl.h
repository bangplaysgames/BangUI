#pragma once
#include "../api/UIElement.h"
#include "DockPolicy.h"
#include "../models/Mesh.h"
#include <string>
#include <vector>

namespace BangUI::impl {

class PanelImpl : public BangUI::API::UIElement {
public:
    PanelImpl() { type = "Panel"; }
    bool resizable = false;
    bool movable = true;
    bool modal = false;
    // Docking policy for this container. Default uses reserved strips to avoid overlap.
    BangUI::impl::DockPolicy dockPolicy = BangUI::impl::DockPolicy::ReserveStrips;
    std::vector<UIElement*> content;
    std::string renderSpace = "orthographic"; // "orthographic" or "diegetic"
    Mesh* anchorMesh = nullptr; // only if diegetic
    std::string anchorBone; // only if diegetic
    std::string scrollable; // "vertical", "horizontal", "both"
    // Content scroll offsets (pixels). Positive scrollY means content moved up (user scrolled down).
    float scrollX = 0.0f;
    float scrollY = 0.0f;

    // Scrollbar fade state (per-axis timer)
    float scrollBarFadeTimerX = 0.0f; // seconds remaining to show horizontal scrollbar
    float scrollBarFadeTimerY = 0.0f; // seconds remaining to show vertical scrollbar
    const float scrollBarFadeDuration = 0.8f; // duration to remain visible after activity

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
