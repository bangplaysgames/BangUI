#pragma once
#include "../api/UIElement.h"
#include <vector>
#include <map>
#include <string>
#include <SDL3_image/SDL_image.h>
#include <iostream>

// Implementation depends on concrete models for Window/Panel which are in the models/ directory
// These are lightweight and kept as before.
#include "../models/Window.h"
#include "../models/Panel.h"

// Temporary debug trace for layout/drag issues. Remove or undefine when done.
#define LAYOUT_DEBUG

// Simple cache for image sizes to avoid repeated disk loads
static std::map<std::string, std::pair<int,int>> g_texture_size_cache;

static std::pair<int,int> getImageSizeCached(const std::string& path) {
    if (path.empty()) return {0,0};
    auto it = g_texture_size_cache.find(path);
    if (it != g_texture_size_cache.end()) return it->second;

#ifdef NO_SDL_IMAGE
    // In unit tests we may not link SDL_image. Return 0 size to avoid linking.
    g_texture_size_cache[path] = {0,0};
    return {0,0};
#else
    SDL_Surface* surf = IMG_Load(path.c_str());
    if (!surf) {
        g_texture_size_cache[path] = {0,0};
        return {0,0};
    }
    int w = surf->w;
    int h = surf->h;
    SDL_DestroySurface(surf);
    g_texture_size_cache[path] = {w,h};
    return {w,h};
#endif
}

// LayoutManager: Computes element positions based on docking and size modes
// This is a core library component that implements the layout system from Overview.md
class LayoutManager {
public:
    // Compute layout for a container's children
    // containerWidth/containerHeight: Available space in the container
    // elements: List of elements to layout
    static void computeLayout(float containerWidth, float containerHeight,
                             std::vector<UIElement*>& elements) {
        // First compute layout for top-level elements (application-level container)
        for (UIElement* element : elements) {
            if (!element || !element->visible) continue;

            computeElementLayout(element, containerWidth, containerHeight);
        }

        // Then compute layout for children recursively relative to their parent safe content
        for (UIElement* element : elements) {
            if (!element) continue;
            // If element is a container, compute its children's layouts
            Window* win = dynamic_cast<Window*>(element);
            Panel* panel = dynamic_cast<Panel*>(element);
            if (win) {
                computeChildrenLayoutForContainer(win);
            } else if (panel) {
                computeChildrenLayoutForContainer(panel);
            }
        }

        // Iteratively expand parents that use SizeMode::Auto until stable.
        // This handles nested Auto parents: repeat until no parent changed or we hit max iterations.
        const int MAX_ITER = 8;
        for (int iter = 0; iter < MAX_ITER; ++iter) {
            bool anyChange = false;

            for (UIElement* element : elements) {
                if (!element) continue;

                Window* win = dynamic_cast<Window*>(element);
                Panel* panel = dynamic_cast<Panel*>(element);

                std::vector<UIElement*> children;
                if (win) {
                    for (UIElement* c : win->content) if (c && c->visible) children.push_back(c);
                } else if (panel) {
                    for (UIElement* c : panel->content) if (c && c->visible) children.push_back(c);
                }

                if (children.empty()) continue;

                float maxChildRight = -FLT_MAX, maxChildBottom = -FLT_MAX;
                for (UIElement* c : children) {
                    maxChildRight = std::max(maxChildRight, c->x + c->width + c->marginRight);
                    maxChildBottom = std::max(maxChildBottom, c->y + c->height + c->marginBottom);
                }

                if (maxChildRight == -FLT_MAX) continue;

                float requiredW = maxChildRight - element->x;
                float requiredH = maxChildBottom - element->y;

                // clamp required sizes so parents don't expand beyond the container
                if (requiredW > containerWidth) requiredW = containerWidth;
                if (requiredH > containerHeight) requiredH = containerHeight;

                if (element->widthMode == SizeMode::Auto && requiredW > element->width) {
                    element->width = requiredW;
                    anyChange = true;
                }
                if (element->heightMode == SizeMode::Auto && requiredH > element->height) {
                    element->height = requiredH;
                    anyChange = true;
                }
            }

            if (!anyChange) break;

            // If something changed, recompute top-level layouts and then children recursively
            for (UIElement* element : elements) {
                if (!element || !element->visible) continue;
                computeElementLayout(element, containerWidth, containerHeight);
            }
            for (UIElement* element : elements) {
                if (!element) continue;
                Window* win = dynamic_cast<Window*>(element);
                Panel* panel = dynamic_cast<Panel*>(element);
                if (win) computeChildrenLayoutForContainer(win);
                else if (panel) computeChildrenLayoutForContainer(panel);
            }
        }
    }

    // Helper: compute layout for children of a container recursively. Child positions
    // are computed relative to the parent's safe content area and then converted
    // to absolute coordinates by offsetting by the parent's safe content origin.
    static void computeChildrenLayoutForContainer(UIElement* container) {
        if (!container) return;

        // Determine children list depending on type
        std::vector<UIElement*> children;
        if (Window* w = dynamic_cast<Window*>(container)) {
            for (UIElement* c : w->content) if (c && c->visible) children.push_back(c);
        } else if (Panel* p = dynamic_cast<Panel*>(container)) {
            for (UIElement* c : p->content) if (c && c->visible) children.push_back(c);
        } else {
            return;
        }

        // Parent safe content dimensions
        float safeW = container->getSafeContentWidth();
        float safeH = container->getSafeContentHeight();
        float safeLeft = container->getSafeContentLeft();
        float safeTop = container->getSafeContentTop();

        for (UIElement* child : children) {
            if (!child) continue;
            if (child->manualPosition) {
                // For manual-positioned children we compute layout to update size
                // but preserve the child-local coordinates (localX/localY) so
                // that subsequent layout passes don't reposition them.
                float savedLocalX = child->localX;
                float savedLocalY = child->localY;
                computeElementLayout(child, safeW, safeH);
                child->localX = savedLocalX;
                child->localY = savedLocalY;
            } else {
                // Non-manual children get their local coords computed from docking/margins
                child->x = child->marginLeft;
                child->y = child->marginTop;
                computeElementLayout(child, safeW, safeH);
                child->localX = child->x;
                child->localY = child->y;
            }
            // Convert local coords to absolute coordinates using parent's safe origin
            child->x = safeLeft + child->localX;
            child->y = safeTop + child->localY;
            if (Window* wchild = dynamic_cast<Window*>(child)) {
                computeChildrenLayoutForContainer(wchild);
            } else if (Panel* pchild = dynamic_cast<Panel*>(child)) {
                computeChildrenLayoutForContainer(pchild);
            }
        }
    }

    static void computeElementLayout(UIElement* element, float containerWidth, float containerHeight) {
        float effectiveWidth = element->width;
        float effectiveHeight = element->height;

        if (element->widthMode == SizeMode::Stretch) {
            effectiveWidth = containerWidth - element->marginLeft - element->marginRight;
        }

        if (element->heightMode == SizeMode::Stretch) {
            effectiveHeight = containerHeight - element->marginTop - element->marginBottom;
        }

        if (element->widthMode == SizeMode::Auto || element->heightMode == SizeMode::Auto) {
            std::string src;
            src = element->src;

            if (!src.empty()) {
                auto sz = getImageSizeCached(src);
                int texW = sz.first;
                int texH = sz.second;

                if (element->widthMode == SizeMode::Auto && element->heightMode == SizeMode::Auto) {
                    if (texW > 0 && texH > 0) {
                        effectiveWidth = (float)texW;
                        effectiveHeight = (float)texH;
                    }
                }
                else if (element->widthMode == SizeMode::Auto && element->heightMode != SizeMode::Auto) {
                    if (texH > 0) {
                        effectiveWidth = effectiveHeight * (static_cast<float>(texW) / static_cast<float>(texH));
                    }
                }
                else if (element->heightMode == SizeMode::Auto && element->widthMode != SizeMode::Auto) {
                    if (texW > 0) {
                        effectiveHeight = effectiveWidth * (static_cast<float>(texH) / static_cast<float>(texW));
                    }
                }
            }
        }

        element->width = effectiveWidth;
        element->height = effectiveHeight;

        if (element->manualPosition) {
            return;
        }

        switch (element->hDock) {
            case HDock::Left:
                element->x = element->marginLeft;
                break;
            case HDock::Center:
                element->x = (containerWidth - effectiveWidth) / 2.0f;
                break;
            case HDock::Right:
                element->x = containerWidth - effectiveWidth - element->marginRight;
                break;
            case HDock::None:
                break;
        }

        switch (element->vDock) {
            case VDock::Top:
                element->y = element->marginTop;
                break;
            case VDock::Center:
                element->y = (containerHeight - effectiveHeight) / 2.0f;
                break;
            case VDock::Bottom:
                element->y = containerHeight - effectiveHeight - element->marginBottom;
                break;
            case VDock::None:
                break;
        }
    }
};
