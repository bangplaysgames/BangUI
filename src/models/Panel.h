#pragma once
#include "UIElement.h"
#include "Mesh.h"
#include <vector>

class Panel : public UIElement {
public:
    bool resizable = false;
    bool movable = true;
    bool modal = false;
    std::vector<UIElement*> content;
    std::string renderSpace = "orthographic"; // "orthographic" or "diegetic"
    Mesh* anchorMesh = nullptr; // only if diegetic
    std::string anchorBone; // only if diegetic
    std::string scrollable; // "vertical", "horizontal", "both"

    // Check if point is in panel body (for dragging)
    bool isPointInPanel(float px, float py) const {
        return px >= x && px <= x + width &&
               py >= y && py <= y + height;
    }
};
