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
#include "DockPolicy.h"

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

        // Parent safe content rectangle. Use container-specific helpers when available
        float safeLeft, safeTop, safeRight, safeBottom;
        if (Window* w = dynamic_cast<Window*>(container)) {
            safeLeft = w->getSafeContentLeft();
            safeTop = w->getSafeContentTop();
            safeRight = w->getSafeContentRight();
            safeBottom = w->getSafeContentBottom();
        } else if (Panel* p = dynamic_cast<Panel*>(container)) {
            safeLeft = p->getSafeContentLeft();
            safeTop = p->getSafeContentTop();
            safeRight = p->getSafeContentRight();
            safeBottom = p->getSafeContentBottom();
        } else {
            safeLeft = container->getSafeContentLeft();
            safeTop = container->getSafeContentTop();
            safeRight = container->getSafeContentRight();
            safeBottom = container->getSafeContentBottom();
        }
        float safeW = safeRight - safeLeft;
        float safeH = safeBottom - safeTop;

    // Choose docking policy from container (default ReserveStrips)
    BangUI::impl::DockPolicy policy = BangUI::impl::DockPolicy::ReserveStrips;
    if (Window* w = dynamic_cast<Window*>(container)) policy = w->dockPolicy;
    else if (Panel* p = dynamic_cast<Panel*>(container)) policy = p->dockPolicy;

    if (policy == BangUI::impl::DockPolicy::ReserveStrips) {
            // Reserve top and bottom strips first
            std::vector<UIElement*> topChildren;
            std::vector<UIElement*> bottomChildren;
            std::vector<UIElement*> leftChildren;
            std::vector<UIElement*> rightChildren;
            std::vector<UIElement*> centerChildren;
            std::vector<UIElement*> noneChildren;

            for (UIElement* child : children) {
                if (!child) continue;
                if (child->vDock == VDock::Top) topChildren.push_back(child);
                else if (child->vDock == VDock::Bottom) bottomChildren.push_back(child);
                else if (child->hDock == HDock::Left) leftChildren.push_back(child);
                else if (child->hDock == HDock::Right) rightChildren.push_back(child);
                else if (child->hDock == HDock::Center || child->vDock == VDock::Center) centerChildren.push_back(child);
                else noneChildren.push_back(child);
            }

            // Compute heights for top/bottom using full safe width
            float topStrip = 0.0f;
            for (UIElement* c : topChildren) {
                computeElementLayout(c, safeW, safeH);
                topStrip += c->marginTop + c->height + c->marginBottom;
            }
            float bottomStrip = 0.0f;
            for (UIElement* c : bottomChildren) {
                computeElementLayout(c, safeW, safeH);
                bottomStrip += c->marginTop + c->height + c->marginBottom;
            }

            // Reserve top/bottom
            float innerLeft = 0.0f, innerTop = 0.0f, innerRight = safeW, innerBottom = safeH;
            innerTop += topStrip;
            innerBottom -= bottomStrip;
            float innerW = innerRight - innerLeft;
            float innerH = innerBottom - innerTop;

            // Compute widths for left/right within inner height
            float leftStrip = 0.0f;
            for (UIElement* c : leftChildren) {
                computeElementLayout(c, innerW, innerH);
                leftStrip += c->marginLeft + c->width + c->marginRight;
            }
            float rightStrip = 0.0f;
            for (UIElement* c : rightChildren) {
                computeElementLayout(c, innerW, innerH);
                rightStrip += c->marginLeft + c->width + c->marginRight;
            }

            // Reserve left/right
            innerLeft += leftStrip;
            innerRight -= rightStrip;
            innerW = innerRight - innerLeft;
            innerH = innerBottom - innerTop;

            // Place Top children: allow left/center/right alignment within the top strip
            {
                std::vector<UIElement*> leftGroup, centerGroup, rightGroup;
                for (UIElement* c : topChildren) {
                    if (!c) continue;
                    if (c->hDock == HDock::Center) centerGroup.push_back(c);
                    else if (c->hDock == HDock::Right) rightGroup.push_back(c);
                    else leftGroup.push_back(c); // default/None/Left -> left group
                }

                auto computeWidth = [&](const std::vector<UIElement*>& list) {
                    float wsum = 0.0f;
                    for (UIElement* c : list) wsum += c->marginLeft + c->width + c->marginRight;
                    return wsum;
                };

                float leftW = computeWidth(leftGroup);
                float rightW = computeWidth(rightGroup);
                float centerW = computeWidth(centerGroup);

                // Available width for center group between left and right groups
                float availForCenter = safeW - leftW - rightW;
                if (availForCenter < 0) availForCenter = 0;

                // Place left group from left edge
                float cursor = 0.0f;
                for (UIElement* c : leftGroup) {
                    float localX = cursor + c->marginLeft;
                    float localY = c->marginTop;
                    if (!c->manualPosition) {
                        c->localX = localX;
                        c->localY = localY;
                    }
                    cursor = localX + c->width + c->marginRight;
                    c->x = safeLeft + c->localX;
                    c->y = safeTop + c->localY;
                    if (Window* wchild = dynamic_cast<Window*>(c)) computeChildrenLayoutForContainer(wchild);
                    else if (Panel* pchild = dynamic_cast<Panel*>(c)) computeChildrenLayoutForContainer(pchild);
                }

                // Place center group centered in the remaining area
                float centerStart = leftW + (availForCenter - centerW) / 2.0f;
                float ccur = centerStart;
                for (UIElement* c : centerGroup) {
                    float localX = ccur + c->marginLeft;
                    float localY = c->marginTop;
                    if (!c->manualPosition) {
                        c->localX = localX;
                        c->localY = localY;
                    }
                    ccur = localX + c->width + c->marginRight;
                    c->x = safeLeft + c->localX;
                    c->y = safeTop + c->localY;
                    if (Window* wchild = dynamic_cast<Window*>(c)) computeChildrenLayoutForContainer(wchild);
                    else if (Panel* pchild = dynamic_cast<Panel*>(c)) computeChildrenLayoutForContainer(pchild);
                }

                // Place right group from right edge inward
                float rcursor = 0.0f;
                for (UIElement* c : rightGroup) {
                    float localX = safeW - rcursor - c->marginRight - c->width;
                    float localY = c->marginTop;
                    if (!c->manualPosition) {
                        c->localX = localX;
                        c->localY = localY;
                    }
                    rcursor += c->marginLeft + c->width + c->marginRight;
                    c->x = safeLeft + c->localX;
                    c->y = safeTop + c->localY;
                    if (Window* wchild = dynamic_cast<Window*>(c)) computeChildrenLayoutForContainer(wchild);
                    else if (Panel* pchild = dynamic_cast<Panel*>(c)) computeChildrenLayoutForContainer(pchild);
                }
            }

            // Place Bottom children: allow left/center/right alignment within the bottom strip
            {
                std::vector<UIElement*> leftGroup, centerGroup, rightGroup;
                for (UIElement* c : bottomChildren) {
                    if (!c) continue;
                    if (c->hDock == HDock::Center) centerGroup.push_back(c);
                    else if (c->hDock == HDock::Right) rightGroup.push_back(c);
                    else leftGroup.push_back(c);
                }

                auto computeWidth = [&](const std::vector<UIElement*>& list) {
                    float wsum = 0.0f;
                    for (UIElement* c : list) wsum += c->marginLeft + c->width + c->marginRight;
                    return wsum;
                };

                float leftW = computeWidth(leftGroup);
                float rightW = computeWidth(rightGroup);
                float centerW = computeWidth(centerGroup);

                float availForCenter = safeW - leftW - rightW;
                if (availForCenter < 0) availForCenter = 0;

                // y base for bottom strip
                float baseY = safeH - bottomStrip;

                // Place left group
                float cursor = 0.0f;
                for (UIElement* c : leftGroup) {
                    float localX = cursor + c->marginLeft;
                    float localY = baseY + c->marginTop;
                    if (!c->manualPosition) {
                        c->localX = localX;
                        c->localY = localY;
                    }
                    cursor = localX + c->width + c->marginRight;
                    c->x = safeLeft + c->localX;
                    c->y = safeTop + c->localY;
                    if (Window* wchild = dynamic_cast<Window*>(c)) computeChildrenLayoutForContainer(wchild);
                    else if (Panel* pchild = dynamic_cast<Panel*>(c)) computeChildrenLayoutForContainer(pchild);
                }

                // Place center group centered in remaining area
                float centerStart = leftW + (availForCenter - centerW) / 2.0f;
                float ccur = centerStart;
                for (UIElement* c : centerGroup) {
                    float localX = ccur + c->marginLeft;
                    float localY = baseY + c->marginTop;
                    if (!c->manualPosition) {
                        c->localX = localX;
                        c->localY = localY;
                    }
                    ccur = localX + c->width + c->marginRight;
                    c->x = safeLeft + c->localX;
                    c->y = safeTop + c->localY;
                    if (Window* wchild = dynamic_cast<Window*>(c)) computeChildrenLayoutForContainer(wchild);
                    else if (Panel* pchild = dynamic_cast<Panel*>(c)) computeChildrenLayoutForContainer(pchild);
                }

                // Place right group from right edge inward
                float rcursor = 0.0f;
                for (UIElement* c : rightGroup) {
                    float localX = safeW - rcursor - c->marginRight - c->width;
                    float localY = baseY + c->marginTop;
                    if (!c->manualPosition) {
                        c->localX = localX;
                        c->localY = localY;
                    }
                    rcursor += c->marginLeft + c->width + c->marginRight;
                    c->x = safeLeft + c->localX;
                    c->y = safeTop + c->localY;
                    if (Window* wchild = dynamic_cast<Window*>(c)) computeChildrenLayoutForContainer(wchild);
                    else if (Panel* pchild = dynamic_cast<Panel*>(c)) computeChildrenLayoutForContainer(pchild);
                }
            }

            // Place Left children: top-to-bottom inside left strip
            float leftCursorV = 0.0f;
            for (UIElement* c : leftChildren) {
                float localX = c->marginLeft;
                float localY = innerTop + leftCursorV + c->marginTop - topStrip; // adjust relative to safeTop
                // Instead, compute relative to inner top
                localY = (innerTop - topStrip) + leftCursorV + c->marginTop; // conserve previous behavior
                // To be safer, place relative to innerTop
                localY = innerTop + leftCursorV + c->marginTop;
                if (!c->manualPosition) {
                    c->localX = localX;
                    c->localY = localY;
                }
                leftCursorV = leftCursorV + c->height + c->marginTop + c->marginBottom;
                c->x = safeLeft + c->localX;
                c->y = safeTop + c->localY;
                if (Window* wchild = dynamic_cast<Window*>(c)) computeChildrenLayoutForContainer(wchild);
                else if (Panel* pchild = dynamic_cast<Panel*>(c)) computeChildrenLayoutForContainer(pchild);
            }

            // Place Right children: top-to-bottom inside right strip
            float rightCursorV = 0.0f;
            for (UIElement* c : rightChildren) {
                // place from right edge moving left by margin and width
                float localX = innerRight - c->marginRight - c->width;
                float localY = innerTop + rightCursorV + c->marginTop;
                if (!c->manualPosition) {
                    c->localX = localX;
                    c->localY = localY;
                }
                rightCursorV = rightCursorV + c->height + c->marginTop + c->marginBottom;
                c->x = safeLeft + c->localX;
                c->y = safeTop + c->localY;
                if (Window* wchild = dynamic_cast<Window*>(c)) computeChildrenLayoutForContainer(wchild);
                else if (Panel* pchild = dynamic_cast<Panel*>(c)) computeChildrenLayoutForContainer(pchild);
            }

            // Place center children and none children in the remaining inner rectangle
            std::vector<UIElement*> middle = centerChildren;
            middle.insert(middle.end(), noneChildren.begin(), noneChildren.end());

            // For center group horizontally, lay out left-to-right centered as a group
            if (!middle.empty()) {
                // compute sizes for middle using innerW/innerH
                float groupWidth = 0.0f;
                float maxHeight = 0.0f;
                for (UIElement* c : middle) {
                    computeElementLayout(c, innerW, innerH);
                    groupWidth += c->marginLeft + c->width + c->marginRight;
                    maxHeight = std::max(maxHeight, c->height + c->marginTop + c->marginBottom);
                }
                float startX = innerLeft + (innerW - groupWidth) / 2.0f;
                float cursorX = startX;
                float centerY = innerTop + (innerH - maxHeight) / 2.0f;
                for (UIElement* c : middle) {
                    float localX = cursorX + c->marginLeft;
                    float localY = centerY + c->marginTop;
                    // Respect manualPosition: do not overwrite manually-set localX/localY
                    if (!c->manualPosition) {
                        c->localX = localX;
                        c->localY = localY;
                    }
                    c->x = safeLeft + c->localX;
                    c->y = safeTop + c->localY;
                    cursorX = localX + c->width + c->marginRight;
                    if (Window* wchild = dynamic_cast<Window*>(c)) computeChildrenLayoutForContainer(wchild);
                    else if (Panel* pchild = dynamic_cast<Panel*>(c)) computeChildrenLayoutForContainer(pchild);
                }
            }

        } else {
            // Fallback: cursor stacking (original behavior)
            float leftCursor = 0.0f;
            float rightCursor = safeW;
            float topCursor = 0.0f;
            float bottomCursor = safeH;

            for (UIElement* child : children) {
                if (!child) continue;

                if (child->manualPosition) {
                    float savedLocalX = child->localX;
                    float savedLocalY = child->localY;
                    computeElementLayout(child, safeW, safeH);
                    child->localX = savedLocalX;
                    child->localY = savedLocalY;
                } else {
                    computeElementLayout(child, safeW, safeH);
                    float localX = 0.0f;
                    switch (child->hDock) {
                        case HDock::Left:
                            localX = leftCursor + child->marginLeft;
                            leftCursor = localX + child->width + child->marginRight;
                            break;
                        case HDock::Right:
                            localX = rightCursor - child->marginRight - child->width;
                            rightCursor = localX - child->marginLeft;
                            break;
                        case HDock::Center:
                            localX = (safeW - child->width) / 2.0f;
                            break;
                        case HDock::None:
                        default:
                            localX = child->marginLeft;
                            break;
                    }
                    float localY = 0.0f;
                    switch (child->vDock) {
                        case VDock::Top:
                            localY = topCursor + child->marginTop;
                            topCursor = localY + child->height + child->marginBottom;
                            break;
                        case VDock::Bottom:
                            localY = bottomCursor - child->marginBottom - child->height;
                            bottomCursor = localY - child->marginTop;
                            break;
                        case VDock::Center:
                            localY = (safeH - child->height) / 2.0f;
                            break;
                        case VDock::None:
                        default:
                            localY = child->marginTop;
                            break;
                    }
                    child->localX = localX;
                    child->localY = localY;
                }
                child->x = safeLeft + child->localX;
                child->y = safeTop + child->localY;
                if (Window* wchild = dynamic_cast<Window*>(child)) computeChildrenLayoutForContainer(wchild);
                else if (Panel* pchild = dynamic_cast<Panel*>(child)) computeChildrenLayoutForContainer(pchild);
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
