#include "../models/Window.h"
#include "Panel.h"
#include <stdexcept>

void validateRenderSpace(Window* window) {
    if (window->renderSpace == "diegetic") {
        if (!window->anchorMesh || window->anchorBone.empty()) {
            throw std::invalid_argument("Diegetic windows require anchorMesh and anchorBone.");
        }
    }
}

void validateScrollable(Window* window) {
    if (!window->scrollable.empty() && window->content.size() > 0) {
        // Add logic to ensure content is scrollable in specified direction(s)
    }
}

void validateRenderSpace(Panel* panel) {
    if (panel->renderSpace == "diegetic") {
        if (!panel->anchorMesh || panel->anchorBone.empty()) {
            throw std::invalid_argument("Diegetic panels require anchorMesh and anchorBone.");
        }
    }
}

void validateScrollable(Panel* panel) {
    if (!panel->scrollable.empty() && panel->content.size() > 0) {
        // Add logic to ensure content is scrollable in specified direction(s)
    }
}
