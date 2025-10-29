#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include "../models/Window.h"
#include "../models/Panel.h"
#include "LayoutManager.h"
#include "Button.h"

// UIManager: Core library class that manages UI element interactions
// Handles all mouse events, dragging, clicking, and element management
class UIManager {
private:
    std::vector<Panel*> panels;
    std::vector<Window*> windows;

    UIElement* draggedElement = nullptr;
    Window* resizingWindow = nullptr;
    bool isMouseDown = false;
    float resizeStartX = 0.0f;
    float resizeStartY = 0.0f;
    float resizeStartWidth = 0.0f;
    float resizeStartHeight = 0.0f;

    // Application window bounds (container for all UI elements)
    float appWindowWidth = 800.0f;
    float appWindowHeight = 600.0f;
    float appWindowPadding = 10.0f; // Padding from application window edges

public:
    // Set application window dimensions
    void setApplicationWindow(float width, float height, float padding = 10.0f) {
        appWindowWidth = width;
        appWindowHeight = height;
        appWindowPadding = padding;
    }

    // Get application window dimensions
    float getAppWindowWidth() const { return appWindowWidth; }
    float getAppWindowHeight() const { return appWindowHeight; }
    float getAppWindowPadding() const { return appWindowPadding; }
    // Add UI elements to the scene
    void addPanel(Panel* panel) {
        panels.push_back(panel);
    }

    void addWindow(Window* window) {
        windows.push_back(window);
    }

    // Remove UI elements (returns true if found and removed)
    bool removePanel(Panel* panel) {
        auto it = std::find(panels.begin(), panels.end(), panel);
        if (it != panels.end()) {
            panels.erase(it);
            return true;
        }
        return false;
    }

    bool removeWindow(Window* window) {
        auto it = std::find(windows.begin(), windows.end(), window);
        if (it != windows.end()) {
            windows.erase(it);
            return true;
        }
        return false;
    }

    // Handle SDL events - this is the core interaction logic
    void handleEvent(const SDL_Event& e) {
        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) {
            handleMouseDown(e.button.x, e.button.y);
        }
        else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && e.button.button == SDL_BUTTON_LEFT) {
            handleMouseUp();
        }
        else if (e.type == SDL_EVENT_MOUSE_MOTION) {
            handleMouseMove(e.motion.x, e.motion.y);
            if (isMouseDown) {
                if (draggedElement) {
                    handleMouseDrag(e.motion.x, e.motion.y);
                } else if (resizingWindow) {
                    handleWindowResize(e.motion.x, e.motion.y);
                }
            }
        }
    }

    // Get all panels and windows for rendering
    const std::vector<Panel*>& getPanels() const { return panels; }
    const std::vector<Window*>& getWindows() const { return windows; }

    // Cleanup
    ~UIManager() {
        // Note: We don't delete the elements here - ownership remains with the creator
        // If you want the UIManager to own elements, uncomment these:
        // for (Panel* p : panels) delete p;
        // for (Window* w : windows) delete w;
    }

private:
    void handleMouseDown(float mouseX, float mouseY) {
        isMouseDown = true;
        bool handled = false;
        // If a modal window exists, only hit-test that window and its children
        Window* modal = getModalWindow();
        if (modal) {
            handled = checkElementHitRecursive(modal, mouseX, mouseY);
            return;
        }
        // Check top-level windows and panels with recursive child hit-testing
        // Windows first (they render on top)
        for (auto it = windows.rbegin(); it != windows.rend() && !handled; ++it) {
            Window* win = *it;
            // recursively check window and its children
            handled = checkElementHitRecursive(win, mouseX, mouseY);
            if (handled) break;
        }

        // Then panels if no hit yet
        if (!handled) {
            for (auto it = panels.rbegin(); it != panels.rend() && !handled; ++it) {
                Panel* panel = *it;
                handled = checkElementHitRecursive(panel, mouseX, mouseY);
                if (handled) break;
            }
        }
    }

    // Return the top-most modal window if any
    Window* getModalWindow() const {
        for (auto it = windows.rbegin(); it != windows.rend(); ++it) {
            Window* w = *it;
            if (w->modal && w->visible) return w;
        }
        return nullptr;
    }

    // Recursive hit test: children first (top-most), then the element itself.
    bool checkElementHitRecursive(UIElement* element, float mouseX, float mouseY) {
        if (!element || !element->visible || !element->interactable) return false;

        // If element is a container, check its children in reverse (topmost first)
        if (Window* w = dynamic_cast<Window*>(element)) {
            for (auto it = w->content.rbegin(); it != w->content.rend(); ++it) {
                UIElement* child = *it;
                if (checkElementHitRecursive(child, mouseX, mouseY)) return true;
            }

            // Element-specific checks for Window
            // Check close button first
            if (w->closeable && w->isPointInCloseButton(mouseX, mouseY)) {
                // Remove window from its parent or top-level list
                if (w->parent) {
                    // remove from parent's content
                    UIElement* parent = w->parent;
                    if (Panel* p = dynamic_cast<Panel*>(parent)) {
                        auto it = std::find(p->content.begin(), p->content.end(), w);
                        if (it != p->content.end()) p->content.erase(it);
                    } else if (Window* pw = dynamic_cast<Window*>(parent)) {
                        auto it = std::find(pw->content.begin(), pw->content.end(), w);
                        if (it != pw->content.end()) pw->content.erase(it);
                    }
                } else {
                    // top-level window: remove from windows vector
                    auto it = std::find(windows.begin(), windows.end(), w);
                    if (it != windows.end()) windows.erase(it);
                }
                delete w;
                return true;
            }

            // Check resize handle
            if (w->resizable && w->isPointInResizeHandle(mouseX, mouseY)) {
                resizingWindow = w;
                resizeStartX = mouseX;
                resizeStartY = mouseY;
                resizeStartWidth = w->width;
                resizeStartHeight = w->height;
                // Mark manual position so layout won't override during resize
                w->manualPosition = true;
                if (w->parent) {
                    float safeLeft = w->parent->getSafeContentLeft();
                    float safeTop = w->parent->getSafeContentTop();
                    w->localX = w->x - safeLeft;
                    w->localY = w->y - safeTop;
                } else {
                    w->localX = w->x;
                    w->localY = w->y;
                }
                return true;
            }

            // Title bar dragging
            if (w->movable && w->isPointInTitleBar(mouseX, mouseY)) {
                draggedElement = w;
                w->isBeingDragged = true;
                w->dragOffsetX = mouseX - w->x;
                w->dragOffsetY = mouseY - w->y;
                // Mark manual position so layout won't override user dragging
                w->manualPosition = true;
                if (w->parent) {
                    float safeLeft = w->parent->getSafeContentLeft();
                    float safeTop = w->parent->getSafeContentTop();
                    w->localX = w->x - safeLeft;
                    w->localY = w->y - safeTop;
                } else {
                    w->localX = w->x;
                    w->localY = w->y;
                }
                return true;
            }

            // Not handled by this window
            return false;
        }

        if (Panel* p = dynamic_cast<Panel*>(element)) {
            for (auto it = p->content.rbegin(); it != p->content.rend(); ++it) {
                UIElement* child = *it;
                if (checkElementHitRecursive(child, mouseX, mouseY)) return true;
            }

            // Panel hit for dragging
            if (p->movable && p->isPointInPanel(mouseX, mouseY)) {
                draggedElement = p;
                p->isBeingDragged = true;
                p->dragOffsetX = mouseX - p->x;
                p->dragOffsetY = mouseY - p->y;
                // Manual positioning for panels as well
                p->manualPosition = true;
                if (p->parent) {
                    float safeLeft = p->parent->getSafeContentLeft();
                    float safeTop = p->parent->getSafeContentTop();
                    p->localX = p->x - safeLeft;
                    p->localY = p->y - safeTop;
                } else {
                    p->localX = p->x;
                    p->localY = p->y;
                }
                return true;
            }

            return false;
        }

        // Leaf element hit: if it's a Button or has an onClick handler, invoke it.
        if (Button* b = dynamic_cast<Button*>(element)) {
            if (b->onClick) b->onClick();
            return true;
        }
        if (element->onClick) {
            element->onClick();
            return true;
        }

        // For other leaf elements, no generic movable property
        return false;
    }

    void handleMouseUp() {
        isMouseDown = false;
        if (draggedElement) {
            draggedElement->isBeingDragged = false;
            draggedElement = nullptr;
        }
        if (resizingWindow) {
            resizingWindow = nullptr;
        }
    }

    void handleMouseMove(float mouseX, float mouseY) {
        // Update hover states for windows
        for (Window* win : windows) {
            bool wasHovered = win->isHovered;
            // Check if mouse is within window bounds
            bool nowHovered = win->interactable && (mouseX >= win->x && mouseX <= win->x + win->width &&
                            mouseY >= win->y && mouseY <= win->y + win->height);
            win->isHovered = nowHovered;
        }

        // Update hover states for panels
        for (Panel* panel : panels) {
            bool wasHovered = panel->isHovered;
            bool nowHovered = panel->interactable && (mouseX >= panel->x && mouseX <= panel->x + panel->width &&
                            mouseY >= panel->y && mouseY <= panel->y + panel->height);
            panel->isHovered = nowHovered;
        }
    }

    void handleMouseDrag(float mouseX, float mouseY) {
        if (draggedElement) {
            float newX = mouseX - draggedElement->dragOffsetX;
            float newY = mouseY - draggedElement->dragOffsetY;

            // Clamp to container bounds
            clampToContainer(draggedElement, newX, newY);

            // Compute delta and apply to dragged element
            float dx = newX - draggedElement->x;
            float dy = newY - draggedElement->y;

            // Update absolute position for immediate rendering
            draggedElement->x = newX;
            draggedElement->y = newY;

            // If this element is inside a parent, update its local coords
            // so layout recompute will place it correctly relative to parent.
            if (draggedElement->parent) {
                float safeLeft = draggedElement->parent->getSafeContentLeft();
                float safeTop = draggedElement->parent->getSafeContentTop();
                draggedElement->localX = draggedElement->x - safeLeft;
                draggedElement->localY = draggedElement->y - safeTop;
            } else {
                draggedElement->localX = draggedElement->x;
                draggedElement->localY = draggedElement->y;
            }

            // Do NOT recompute children here; the main loop already calls
            // LayoutManager::computeLayout(...) once per frame. Recomputing here
            // causes duplicate layout passes which can double-apply offsets and
            // lead to compounding/exponential movement. Let the regular frame
            // layout handle child position updates.
#ifdef LAYOUT_DEBUG
            if (Window* w = dynamic_cast<Window*>(draggedElement)) {
                std::cout << "[Drag] Moved Window(" << w->id << ") to x=" << w->x << " y=" << w->y << std::endl;
            } else if (Panel* p = dynamic_cast<Panel*>(draggedElement)) {
                std::cout << "[Drag] Moved Panel(" << p->id << ") to x=" << p->x << " y=" << p->y << std::endl;
            }
#endif
        }
    }

    // Translate children (and nested descendants) by dx,dy to preserve relative positions
    void translateChildrenRecursive(UIElement* container, float dx, float dy) {
        // Deprecated: translation is now handled via local coords and layout recompute
        (void)container; (void)dx; (void)dy;
    }

    // Clamp element position to stay within its parent container
    void clampToContainer(UIElement* element, float& x, float& y) {
        float minX, minY, maxX, maxY;

        if (element->parent == nullptr) {
            // No parent = clamp to application window
            minX = appWindowPadding;
            minY = appWindowPadding;
            maxX = appWindowWidth - element->width - appWindowPadding;
            maxY = appWindowHeight - element->height - appWindowPadding;
        } else {
            // Clamp to parent's safe content area
            minX = element->parent->getSafeContentLeft();
            minY = element->parent->getSafeContentTop();
            maxX = element->parent->getSafeContentRight() - element->width;
            maxY = element->parent->getSafeContentBottom() - element->height;
        }

        // Apply clamping
        x = std::max(minX, std::min(x, maxX));
        y = std::max(minY, std::min(y, maxY));
    }

    void handleWindowResize(float mouseX, float mouseY) {
        if (resizingWindow) {
            float deltaX = mouseX - resizeStartX;
            float deltaY = mouseY - resizeStartY;

            // Calculate new dimensions (minimum size of 100x50)
            float newWidth = std::max(100.0f, resizeStartWidth + deltaX);
            float newHeight = std::max(50.0f, resizeStartHeight + deltaY);

            // Clamp to application or parent bounds so window stays visible
            float maxW, maxH;
            float minX = appWindowPadding;
            float minY = appWindowPadding;

            if (resizingWindow->parent == nullptr) {
                // Max width/height so the window's right/bottom edges do not exceed app bounds minus padding
                maxW = appWindowWidth - resizingWindow->x - appWindowPadding;
                maxH = appWindowHeight - resizingWindow->y - appWindowPadding;
            } else {
                // When inside a parent, limit to parent's safe content area
                float parentRight = resizingWindow->parent->getSafeContentRight();
                float parentBottom = resizingWindow->parent->getSafeContentBottom();
                maxW = parentRight - resizingWindow->x;
                maxH = parentBottom - resizingWindow->y;
                // Also clamp min X/Y to parent's left/top if needed
                minX = resizingWindow->parent->getSafeContentLeft();
                minY = resizingWindow->parent->getSafeContentTop();
            }

            if (newWidth > maxW) newWidth = maxW;
            if (newHeight > maxH) newHeight = maxH;

            // Ensure we didn't shrink to negative due to being near the edge
            newWidth = std::max(newWidth, 100.0f);
            newHeight = std::max(newHeight, 50.0f);

            // If resizing would push the window past the right/bottom, adjust position if necessary
            float right = resizingWindow->x + newWidth;
            float bottom = resizingWindow->y + newHeight;
            if (resizingWindow->parent == nullptr) {
                if (right > appWindowWidth - appWindowPadding) {
                    // Move left so right edge fits
                    resizingWindow->x = std::max(minX, appWindowWidth - appWindowPadding - newWidth);
                }
                if (bottom > appWindowHeight - appWindowPadding) {
                    resizingWindow->y = std::max(minY, appWindowHeight - appWindowPadding - newHeight);
                }
            } else {
                float parentRight = resizingWindow->parent->getSafeContentRight();
                float parentBottom = resizingWindow->parent->getSafeContentBottom();
                if (right > parentRight) {
                    resizingWindow->x = std::max(minX, parentRight - newWidth);
                }
                if (bottom > parentBottom) {
                    resizingWindow->y = std::max(minY, parentBottom - newHeight);
                }
            }

            resizingWindow->width = newWidth;
            resizingWindow->height = newHeight;
        }
    }
};
