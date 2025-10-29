#pragma once
#include "UIElement.h"
#include <vector>
#include <map>
#include <string>
#include <SDL3_image/SDL_image.h>
#include <iostream>

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

            // If the child was manually positioned by the user, preserve its absolute
            // x/y while still allowing size computation. computeElementLayout may set
            // x/y based on docking; save/restore to avoid overwriting the user's position.
            if (child->manualPosition) {
#ifdef LAYOUT_DEBUG
                std::cout << "[Layout] Parent(" << container->id << ") safeLeft=" << safeLeft << " safeTop=" << safeTop
                          << " child(" << child->id << ") manual saveX=" << child->x << " saveY=" << child->y << std::endl;
#endif
                float savedX = child->x;
                float savedY = child->y;
                // Compute sizes (and other layout-affecting properties)
                computeElementLayout(child, safeW, safeH);
                // Restore absolute position set by the user
                child->x = savedX;
                child->y = savedY;

                // IMPORTANT: do NOT recompute child->localX/localY here from the
                // absolute position. localX/localY should be the authoritative
                // local offsets (set when the user began dragging/resizing). If we
                // recalculate them each frame from the preserved absolute position,
                // the child will effectively become pinned to world coordinates
                // and won't move when the parent moves. Use the existing localX/localY.
#ifdef LAYOUT_DEBUG
                std::cout << "[Layout] Parent(" << container->id << ") x=" << container->x << " safeLeft=" << safeLeft << " child(" << child->id << ") manual localX=" << child->localX << " localY=" << child->localY << std::endl;
#endif
            } else {
                // Normal layout-managed child: ensure child->x/y are local
                // coordinates before calling computeElementLayout. For elements
                // with HDock/VDock::None we must initialize a sane local origin
                // (use margins) so stale absolute positions don't carry over.
                child->x = child->marginLeft;
                child->y = child->marginTop;
                // Compute layout (now using local coordinates relative to parent)
                computeElementLayout(child, safeW, safeH);
                child->localX = child->x;
                child->localY = child->y;
            }

            // Convert local coords to absolute positions (this moves children along with parent)
            child->x = safeLeft + child->localX;
            child->y = safeTop + child->localY;
#ifdef LAYOUT_DEBUG
            std::cout << "[Layout] Parent(" << container->id << ") x=" << container->x << " safeLeft=" << container->getSafeContentLeft() << " child(" << child->id << ") finalAbsX=" << child->x << " finalAbsY=" << child->y << " localX=" << child->localX << " localY=" << child->localY << " manual=" << (child->manualPosition?1:0) << std::endl;
#endif

            // Recurse for nested containers
            if (Window* wchild = dynamic_cast<Window*>(child)) {
                computeChildrenLayoutForContainer(wchild);
            } else if (Panel* pchild = dynamic_cast<Panel*>(child)) {
                computeChildrenLayoutForContainer(pchild);
            }
        }
    }

    // Compute layout for a single element within a container
    static void computeElementLayout(UIElement* element, float containerWidth, float containerHeight) {
        // Compute effective width/height with Stretch first
        float effectiveWidth = element->width;
        float effectiveHeight = element->height;

        if (element->widthMode == SizeMode::Stretch) {
            effectiveWidth = containerWidth - element->marginLeft - element->marginRight;
        }

        if (element->heightMode == SizeMode::Stretch) {
            effectiveHeight = containerHeight - element->marginTop - element->marginBottom;
        }

        // Handle Auto sizing by inspecting content (textures) when available
        if (element->widthMode == SizeMode::Auto || element->heightMode == SizeMode::Auto) {
            // Only consider texture content for Auto sizing
            std::string src;
            // Panels and Windows store src in UIElement base
            src = element->src;

            if (!src.empty()) {
                auto sz = getImageSizeCached(src);
                int texW = sz.first;
                int texH = sz.second;

                // If both dimensions are Auto, use native texture size
                if (element->widthMode == SizeMode::Auto && element->heightMode == SizeMode::Auto) {
                    if (texW > 0 && texH > 0) {
                        effectiveWidth = (float)texW;
                        effectiveHeight = (float)texH;
                    }
                }
                // If width is Auto but height is fixed/stretched, maintain aspect ratio
                else if (element->widthMode == SizeMode::Auto && element->heightMode != SizeMode::Auto) {
                    if (texH > 0) {
                        effectiveWidth = effectiveHeight * (static_cast<float>(texW) / static_cast<float>(texH));
                    }
                }
                // If height is Auto but width is fixed/stretched, maintain aspect ratio
                else if (element->heightMode == SizeMode::Auto && element->widthMode != SizeMode::Auto) {
                    if (texW > 0) {
                        effectiveHeight = effectiveWidth * (static_cast<float>(texH) / static_cast<float>(texW));
                    }
                }
            }
            // If no texture available, fall back to current element sizes (no change)
        }

        // Update element size
        element->width = effectiveWidth;
        element->height = effectiveHeight;

        // If the element was manually positioned by the user, preserve its x/y
        // so layout passes don't override user dragging. We still computed sizes above.
        if (element->manualPosition) {
            return;
        }

        // Compute horizontal position based on hDock
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

        // Compute vertical position based on vDock
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
