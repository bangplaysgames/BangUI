#include "UIManagerImpl.h"
#include <algorithm>
#include <iostream>
#include "../api/IRenderer.h"
#include <functional>


namespace BangUI::impl {

    // SDL event watch: callback registered with SDL_AddEventWatch. Forward
    // to the manager instance provided as userdata and recompute layout on
    // relevant window events.
    static bool BangUIEventFilter(void* userdata, SDL_Event* event) {
        if (!userdata || !event) return true;
        UIManagerImpl* mgr = reinterpret_cast<UIManagerImpl*>(userdata);
        // Only handle window size/state events here
        if (event->type == SDL_EVENT_WINDOW_RESIZED) {
            mgr->appWindowWidth = static_cast<float>(event->window.data1);
            mgr->appWindowHeight = static_cast<float>(event->window.data2);
            std::vector<UIElement*> allElements;
            for (PanelImpl* p : mgr->panels) allElements.push_back(p);
            for (WindowImpl* w : mgr->windows) allElements.push_back(w);
            LayoutManager::computeLayout(mgr->appWindowWidth, mgr->appWindowHeight, allElements);
        } else if (event->type == SDL_EVENT_WINDOW_MAXIMIZED || event->type == SDL_EVENT_WINDOW_RESTORED) {
            SDL_Window* win = SDL_GetWindowFromID(event->window.windowID);
            if (win) {
                int w, h;
                SDL_GetWindowSize(win, &w, &h);
                mgr->appWindowWidth = static_cast<float>(w);
                mgr->appWindowHeight = static_cast<float>(h);
                std::vector<UIElement*> allElements;
                for (PanelImpl* p : mgr->panels) allElements.push_back(p);
                for (WindowImpl* wptr : mgr->windows) allElements.push_back(wptr);
                LayoutManager::computeLayout(mgr->appWindowWidth, mgr->appWindowHeight, allElements);
            }
        }
        return true; // don't filter out the event
    }

static bool isLeftMouseDown(const SDL_Event& e) {
    return e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT;
}

// Internal worker that performs the library's default input handling.
void UIManagerImpl::handleEventSDLImpl(const SDL_Event& e) {
    if (e.type == SDL_EVENT_WINDOW_RESIZED) {
        // Update internal application window size and recompute layout for all elements
        appWindowWidth = static_cast<float>(e.window.data1);
        appWindowHeight = static_cast<float>(e.window.data2);
        // Build element list and recompute layout
        std::vector<UIElement*> allElements;
        for (impl::PanelImpl* p : panels) allElements.push_back(p);
        for (impl::WindowImpl* w : windows) allElements.push_back(w);
        LayoutManager::computeLayout(appWindowWidth, appWindowHeight, allElements);
        return;
    }

    // Some window state changes (maximize/restore) may not provide a new size
    // in the event payload. Query the SDL window for the actual size and
    // recompute layout the same way as a manual resize.
    if (e.type == SDL_EVENT_WINDOW_MAXIMIZED || e.type == SDL_EVENT_WINDOW_RESTORED) {
        SDL_Window* win = SDL_GetWindowFromID(e.window.windowID);
        if (win) {
            int w, h;
            SDL_GetWindowSize(win, &w, &h);
            appWindowWidth = static_cast<float>(w);
            appWindowHeight = static_cast<float>(h);
            std::vector<UIElement*> allElements;
            for (impl::PanelImpl* p : panels) allElements.push_back(p);
            for (impl::WindowImpl* wptr : windows) allElements.push_back(wptr);
            LayoutManager::computeLayout(appWindowWidth, appWindowHeight, allElements);
            return;
        }
    }

    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) {
        handleMouseDown(e.button.x, e.button.y);
    }
    else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && e.button.button == SDL_BUTTON_LEFT) {
        handleMouseUp();
    }

    // Mouse wheel handling (SDL3 uses SDL_EVENT_MOUSE_WHEEL)
    if (e.type == SDL_EVENT_MOUSE_WHEEL) {
        float mx, my;
        SDL_GetMouseState(&mx, &my);
        float wheelY = static_cast<float>(e.wheel.y); // positive away from user (scroll up)
        float wheelX = static_cast<float>(e.wheel.x);
    // Use modifier state API which is reliable across platforms
    bool shift = (SDL_GetModState() & SDL_KMOD_SHIFT) != 0;

        // Prefer windows then panels; find topmost scrollable under cursor
        for (auto it = windows.rbegin(); it != windows.rend(); ++it) {
            impl::WindowImpl* w = *it;
            if (!w->visible) continue;
            if (mx >= w->getSafeContentLeft() && mx <= w->getSafeContentRight() && my >= w->getSafeContentTop() && my <= w->getSafeContentBottom()) {
                if (!w->scrollable.empty()) {
                    // compute previous values to detect actual change per-axis
                    float prevX = w->scrollX;
                    float prevY = w->scrollY;
                    if (shift) {
                        // horizontal via shift+wheel: prefer native horizontal wheel (wheelX) if present
                        float delta = (fabsf(wheelX) > 0.0f) ? wheelX : wheelY;
                        if (w->scrollable == "horizontal" || w->scrollable == "both") {
                            w->scrollX -= delta * 20.0f;
                        }
                    } else {
                        if (w->scrollable == "vertical" || w->scrollable == "both") {
                            float beforeScroll = w->scrollY;
                            w->scrollY -= wheelY * 20.0f;
                            if (w->id == "winC_scroll_v") {
                                std::cerr << "[DIAG][UIManager] winC wheel: wheelY=" << wheelY << " beforeScroll=" << beforeScroll << " afterScroll=" << w->scrollY << std::endl;
                            }
                        }
                    }
                    // Compute content extents and clamp to valid range immediately so renderer sees consistent state
                    {
                        float safeLeft = w->getSafeContentLeft();
                        float safeTop = w->getSafeContentTop();
                        float viewportW = w->getSafeContentRight() - safeLeft;
                        float viewportH = w->getSafeContentBottom() - safeTop;
                        float contentW = 0.0f; float contentH = 0.0f;
                        for (UIElement* c : w->content) {
                            if (!c) continue;
                            float mW = c->width, mH = c->height;
                            if (this->activeRenderer) this->activeRenderer->measureElementRenderedSize(c, viewportW, viewportH, mW, mH);
                            // Use original calculation for scrolling behavior (do not convert coordinates)
                            contentW = std::max(contentW, c->x + mW);
                            contentH = std::max(contentH, c->y + mH);
                        }
                        float maxScrollX = std::max(0.0f, contentW - viewportW);
                        float maxScrollY = std::max(0.0f, contentH - viewportH);
                        if (w->id == "winC_scroll_v") {
                            std::cerr << "[DIAG][UIManager] winC measurements: safeTop=" << safeTop << " contentH=" << contentH << " viewportH=" << viewportH << " maxScrollY=" << maxScrollY << std::endl;
                            for (UIElement* c : w->content) {
                                if (!c) continue;
                                std::cerr << "[DIAG][UIManager] winC child: y=" << c->y << " relY=" << (c->y - safeTop) << std::endl;
                            }
                        }
                        float clampedX = std::max(0.0f, std::min(w->scrollX, maxScrollX));
                        float clampedY = std::max(0.0f, std::min(w->scrollY, maxScrollY));
                        if (clampedX != w->scrollX) {
                            std::cerr << "[DIAG][UIManager] clamp window id=" << w->id << " axis=X contentW=" << contentW << " viewportW=" << viewportW << " maxScrollX=" << maxScrollX << " prev=" << w->scrollX << " new=" << clampedX << std::endl;
                            w->scrollX = clampedX;
                        }
                        if (clampedY != w->scrollY) {
                            std::cerr << "[DIAG][UIManager] clamp window id=" << w->id << " axis=Y contentH=" << contentH << " viewportH=" << viewportH << " maxScrollY=" << maxScrollY << " prev=" << w->scrollY << " new=" << clampedY << std::endl;
                            w->scrollY = clampedY;
                        }
                        if (w->scrollX != prevX) w->scrollBarFadeTimerX = w->scrollBarFadeDuration;
                        if (w->scrollY != prevY) w->scrollBarFadeTimerY = w->scrollBarFadeDuration;
                    }
                    return;
                }
            }
        }

        for (auto it = panels.rbegin(); it != panels.rend(); ++it) {
            impl::PanelImpl* p = *it;
            if (!p->visible) continue;
            if (mx >= p->getSafeContentLeft() && mx <= p->getSafeContentRight() && my >= p->getSafeContentTop() && my <= p->getSafeContentBottom()) {
                if (!p->scrollable.empty()) {
                    float prevX = p->scrollX;
                    float prevY = p->scrollY;
                    if (shift) {
                        float delta = (fabsf(wheelX) > 0.0f) ? wheelX : wheelY;
                        if (p->scrollable == "horizontal" || p->scrollable == "both") {
                            p->scrollX -= delta * 20.0f;
                        }
                    } else {
                        if (p->scrollable == "vertical" || p->scrollable == "both") {
                            p->scrollY -= wheelY * 20.0f;
                        }
                    }
                    // Compute content extents and clamp immediately
                    {
                        float safeLeft = p->getSafeContentLeft();
                        float safeTop = p->getSafeContentTop();
                        float viewportW = p->getSafeContentRight() - safeLeft;
                        float viewportH = p->getSafeContentBottom() - safeTop;
                        float contentW = 0.0f; float contentH = 0.0f;
                        for (UIElement* c : p->content) {
                            if (!c) continue;
                            float mW = c->width, mH = c->height;
                            if (this->activeRenderer) this->activeRenderer->measureElementRenderedSize(c, viewportW, viewportH, mW, mH);
                            // Use original calculation for scrolling behavior (do not convert coordinates)
                            contentW = std::max(contentW, c->x + mW);
                            contentH = std::max(contentH, c->y + mH);
                        }
                        float maxScrollX = std::max(0.0f, contentW - viewportW);
                        float maxScrollY = std::max(0.0f, contentH - viewportH);
                        float clampedX = std::max(0.0f, std::min(p->scrollX, maxScrollX));
                        float clampedY = std::max(0.0f, std::min(p->scrollY, maxScrollY));
                        if (clampedX != p->scrollX) {
                            std::cerr << "[DIAG][UIManager] clamp panel id=" << p->id << " axis=X contentW=" << contentW << " viewportW=" << viewportW << " maxScrollX=" << maxScrollX << " prev=" << p->scrollX << " new=" << clampedX << std::endl;
                            p->scrollX = clampedX;
                        }
                        if (clampedY != p->scrollY) {
                            std::cerr << "[DIAG][UIManager] clamp panel id=" << p->id << " axis=Y contentH=" << contentH << " viewportH=" << viewportH << " maxScrollY=" << maxScrollY << " prev=" << p->scrollY << " new=" << clampedY << std::endl;
                            p->scrollY = clampedY;
                        }
                        if (p->scrollX != prevX) p->scrollBarFadeTimerX = p->scrollBarFadeDuration;
                        if (p->scrollY != prevY) p->scrollBarFadeTimerY = p->scrollBarFadeDuration;
                    }
                    return;
                }
            }
        }
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

// Note: public entrypoints (handleEventSDL and handleEvent) are defined
// once in this translation unit. The header declares them; the implementation
// below provides the single definition that demos and the library will link to.

// Input handling implementations removed from .cpp so they can live in the
// header as part of the public inline API. Remaining implementations live
// below.

// Virtual API entry used by external callers that expect the IUIManager
// interface. This forwards to the concrete handler which includes interception
// and virtual hook handling.
void UIManagerImpl::handleEventSDL(const SDL_Event& e) {
    // For compatibility, forward to handleEvent which performs interception
    // and calls the library default processing as needed.
    handleEvent(e);
}

void UIManagerImpl::startEventListening() {
    if (sdlEventListening || !nativeWindow) return;
    // Register our event filter with SDL so the library receives window
    // events even if the application does not forward them.
    bool ok = SDL_AddEventWatch(BangUIEventFilter, this);
    if (ok) sdlEventListening = true;
}

void UIManagerImpl::stopEventListening() {
    if (!sdlEventListening) return;
    SDL_RemoveEventWatch(BangUIEventFilter, this);
    sdlEventListening = false;
}

// Public entry used by demos and apps. This consults a possible interceptor
// then the virtual onEvent hook (allowing subclasses to override behavior),
// then falls back to the default SDL handling worker.
void UIManagerImpl::handleEvent(const SDL_Event& e) {
    if (eventInterceptor) {
        bool consumed = eventInterceptor(e);
        if (consumed) return;
    }
    if (onEvent(e)) return;
    handleEventSDLImpl(e);
}

impl::WindowImpl* UIManagerImpl::getModalWindow() const {
    for (auto it = windows.rbegin(); it != windows.rend(); ++it) {
        impl::WindowImpl* w = *it;
        if (w->modal && w->visible) return w;
    }
    return nullptr;
}

bool UIManagerImpl::checkElementHitRecursive(UIElement* element, float mouseX, float mouseY) {
    if (!element || !element->visible || !element->interactable) return false;

    if (impl::WindowImpl* w = dynamic_cast<impl::WindowImpl*>(element)) {
        // Give window chrome (close button, resize handle, title bar) priority over children
        if (w->closeable && w->isPointInCloseButton(mouseX, mouseY)) {
            if (w->parent) {
                API::UIElement* parent = w->parent;
                if (impl::PanelImpl* p = dynamic_cast<impl::PanelImpl*>(parent)) {
                    auto it = std::find(p->content.begin(), p->content.end(), w);
                    if (it != p->content.end()) p->content.erase(it);
                } else if (impl::WindowImpl* pw = dynamic_cast<impl::WindowImpl*>(parent)) {
                    auto it = std::find(pw->content.begin(), pw->content.end(), w);
                    if (it != pw->content.end()) pw->content.erase(it);
                }
            } else {
                auto it = std::find(windows.begin(), windows.end(), w);
                if (it != windows.end()) windows.erase(it);
            }
            delete w;
            return true;
        }
        if (w->resizable && w->isPointInResizeHandle(mouseX, mouseY)) {
            resizingWindow = w;
            resizeStartX = mouseX;
            resizeStartY = mouseY;
            resizeStartWidth = w->width;
            resizeStartHeight = w->height;
            w->manualPosition = true;
            if (w->parent) {
                float safeLeft = w->parent->getSafeContentLeft();
                float safeTop = w->parent->getSafeContentTop();
                w->localX = w->x - safeLeft;
                w->localY = w->y - safeTop;
            } else { w->localX = w->x; w->localY = w->y; }
            return true;
        }

        // Start dragging when title bar clicked. Docking doesn't prevent starting
        // a drag; movement is constrained during drag according to dock flags.
        if (w->movable && w->isPointInTitleBar(mouseX, mouseY)) {
            draggedElement = w;
            w->isBeingDragged = true;
            w->dragOffsetX = mouseX - w->x;
            w->dragOffsetY = mouseY - w->y;
            w->manualPosition = true;
            if (w->parent) {
                float safeLeft = w->parent->getSafeContentLeft();
                float safeTop = w->parent->getSafeContentTop();
                w->localX = w->x - safeLeft;
                w->localY = w->y - safeTop;
            } else { w->localX = w->x; w->localY = w->y; }
            return true;
        }

        // Otherwise, check children (reverse order so topmost children hit first)
        for (auto it = w->content.rbegin(); it != w->content.rend(); ++it) {
            API::UIElement* child = *it;
            if (checkElementHitRecursive(child, mouseX, mouseY)) return true;
        }

        // If we reached here, no child or chrome handled the click.
        // If the window is interactable and the point lies inside its bounds,
        // swallow the click so it doesn't pass through to elements underneath.
        if (w->interactable && w->visible &&
            mouseX >= w->x && mouseX <= w->x + w->width &&
            mouseY >= w->y && mouseY <= w->y + w->height) {
            return true;
        }
        return false;
    }

    if (impl::PanelImpl* p = dynamic_cast<impl::PanelImpl*>(element)) {
        for (auto it = p->content.rbegin(); it != p->content.rend(); ++it) {
            API::UIElement* child = *it;
            if (checkElementHitRecursive(child, mouseX, mouseY)) return true;
        }


        // For panels, similarly prevent dragging if they are docked to a parent
        // (keep docked panels attached to their parent edges).
        if (p->movable && p->isPointInPanel(mouseX, mouseY)) {
            draggedElement = p;
            p->isBeingDragged = true;
            p->dragOffsetX = mouseX - p->x;
            p->dragOffsetY = mouseY - p->y;
            p->manualPosition = true;
            if (p->parent) {
                float safeLeft = p->parent->getSafeContentLeft();
                float safeTop = p->parent->getSafeContentTop();
                p->localX = p->x - safeLeft;
                p->localY = p->y - safeTop;
            } else { p->localX = p->x; p->localY = p->y; }
            return true;
        }

        // If we reached here, no child handled the click. If the panel is
        // interactable and the point is inside the panel bounds, swallow the
        // click to prevent it passing through to underlying elements.
        if (p->interactable && p->visible &&
            mouseX >= p->x && mouseX <= p->x + p->width &&
            mouseY >= p->y && mouseY <= p->y + p->height) {
            return true;
        }
        return false;
    }

    if (ButtonImpl* b = dynamic_cast<ButtonImpl*>(element)) {
        // Only trigger button click when the pointer is inside the button bounds
        if (b->visible && b->interactable &&
            mouseX >= b->x && mouseX <= b->x + b->width &&
            mouseY >= b->y && mouseY <= b->y + b->height) {
            // Use the concrete click implementation so pressed-state is handled
            b->OnClickImpl();
            return true;
        }
        return false;
    }

    // Generic element click: if the point is inside the element bounds and
    // the element is interactable, consume the click. If it also has an
    // onClick handler, invoke it. If the element is not interactable, allow
    // the click to pass through to underlying elements.
    if (element->visible && element->interactable &&
        mouseX >= element->x && mouseX <= element->x + element->width &&
        mouseY >= element->y && mouseY <= element->y + element->height) {
        if (element->onClick) {
            element->onClick(element);
        }
        return true;
    }

    return false;
}

// Basic helper implementations for mouse handling. These are kept minimal and
// consistent with the previous inline behaviours: they update state and
// dispatch clicks/dragging to elements.
void UIManagerImpl::handleMouseDown(float mouseX, float mouseY) {
    isMouseDown = true;
    // Check modal window first
    impl::WindowImpl* modal = getModalWindow();
    if (modal) {
        // If a modal window is active, only process clicks that hit the modal.
        // Clicks outside the modal should not interact with background elements.
        // If click is on scrollbar thumb of modal, start dragging
        // (fall through to checkElementHitRecursive for usual handling)
        if (checkElementHitRecursive(modal, mouseX, mouseY)) return;
        // Click was outside the modal: swallow the click (do nothing)
        return;
    } else {
        // Iterate windows top-down
        // First, check for scrollbar thumb clicks on windows (topmost first)
        for (auto it = windows.rbegin(); it != windows.rend(); ++it) {
            impl::WindowImpl* w = *it;
            if (!w->visible) continue;
            float safeLeft = w->getSafeContentLeft();
            float safeTop = w->getSafeContentTop();
            float safeW = w->getSafeContentRight() - safeLeft;
            float safeH = w->getSafeContentBottom() - safeTop;
            // vertical
                if (w->scrollable == "vertical" || w->scrollable == "both") {
                float sbw = 8.0f; float sbx = safeLeft + safeW - sbw - 4.0f; float sby = safeTop + 4.0f; float sbh = safeH - 8.0f;
                // content height (convert from absolute to content-area-relative coordinates)
                float contentH = 0.0f; for (UIElement* c : w->content) { if (!c) continue; float mW=c->width,mH=c->height; if (this->activeRenderer) this->activeRenderer->measureElementRenderedSize(c, safeW, safeH, mW, mH); contentH = std::max(contentH, c->y + mH); }
                float viewportH = safeH;
                if (contentH > viewportH + 1.0f) {
                    // Thumb height should be proportional to viewport/content but scaled to the track height (sbh)
                    float thumbH = std::max(16.0f, sbh * (viewportH / contentH));
                    float maxScroll = std::max(0.0f, contentH - viewportH);
                    float thumbRelY = (maxScroll > 0.0f ? (w->scrollY / maxScroll) * (sbh - thumbH) : 0.0f);
                    // Only start drag if mouse is over the visible thumb area (not anywhere in the track)
                    if (mouseX >= sbx && mouseX <= sbx + sbw && mouseY >= sby + thumbRelY && mouseY <= sby + thumbRelY + thumbH) {
                    // start vertical scrollbar drag
                    isScrollbarDragging = true; scrollbarDragTarget = w; scrollbarDragAxis = 'y'; scrollbarDragStartMouse = mouseY; scrollbarDragStartScroll = w->scrollY; return;
                    }
                }
            }
            // horizontal
                if (w->scrollable == "horizontal" || w->scrollable == "both") {
                float sbh = 8.0f; float sbx = safeLeft + 4.0f; float sby = safeTop + safeH - sbh - 4.0f; float sbw = safeW - 8.0f;
                // content width (convert from absolute to content-area-relative coordinates)
                float contentW = 0.0f; for (UIElement* c : w->content) { if (!c) continue; float mW=c->width,mH=c->height; if (this->activeRenderer) this->activeRenderer->measureElementRenderedSize(c, safeW, safeH, mW, mH); contentW = std::max(contentW, c->x + mW); }
                float viewportW = safeW;
                // Thumb width should be proportional to viewport/content but scaled to the track width (sbw)
                float thumbW = std::max(16.0f, sbw * (viewportW / contentW));
                float maxScroll = std::max(0.0f, contentW - viewportW);
                float thumbRelX = (maxScroll > 0.0f ? (w->scrollX / maxScroll) * (sbw - thumbW) : 0.0f);
                if (mouseX >= sbx + thumbRelX && mouseX <= sbx + thumbRelX + thumbW && mouseY >= sby && mouseY <= sby + sbh) {
                    isScrollbarDragging = true; scrollbarDragTarget = w; scrollbarDragAxis = 'x'; scrollbarDragStartMouse = mouseX; scrollbarDragStartScroll = w->scrollX; return;
                }
            }
            // otherwise fall back to usual hit-testing
            if (checkElementHitRecursive(w, mouseX, mouseY)) return;
        }
        // Then panels
        for (auto it = panels.rbegin(); it != panels.rend(); ++it) {
            impl::PanelImpl* p = *it;
            if (!p->visible) continue;
            float safeLeft = p->getSafeContentLeft();
            float safeTop = p->getSafeContentTop();
            float safeW = p->getSafeContentRight() - safeLeft;
            float safeH = p->getSafeContentBottom() - safeTop;
                if (p->scrollable == "vertical" || p->scrollable == "both") {
                float sbw = 8.0f; float sbx = safeLeft + safeW - sbw - 4.0f; float sby = safeTop + 4.0f; float sbh = safeH - 8.0f;
                // content height (convert from absolute to content-area-relative coordinates)
                float contentH = 0.0f; for (UIElement* c : p->content) { if (!c) continue; float mW=c->width,mH=c->height; if (this->activeRenderer) this->activeRenderer->measureElementRenderedSize(c, safeW, safeH, mW, mH); contentH = std::max(contentH, c->y + mH); }
                float viewportH = safeH;
                // Thumb height proportional to track (sbh) times viewport/content ratio
                float thumbH = std::max(16.0f, sbh * (viewportH / contentH));
                float maxScroll = std::max(0.0f, contentH - viewportH);
                float thumbRelY = (maxScroll > 0.0f ? (p->scrollY / maxScroll) * (sbh - thumbH) : 0.0f);
                if (mouseX >= sbx && mouseX <= sbx + sbw && mouseY >= sby + thumbRelY && mouseY <= sby + thumbRelY + thumbH) {
                    isScrollbarDragging = true; scrollbarDragTarget = p; scrollbarDragAxis = 'y'; scrollbarDragStartMouse = mouseY; scrollbarDragStartScroll = p->scrollY; return;
                }
            }
            if (p->scrollable == "horizontal" || p->scrollable == "both") {
                float sbh = 8.0f; float sbx = safeLeft + 4.0f; float sby = safeTop + safeH - sbh - 4.0f; float sbw = safeW - 8.0f;
                // content width (convert from absolute to content-area-relative coordinates)
                float contentW = 0.0f; for (UIElement* c : p->content) { if (!c) continue; float mW=c->width,mH=c->height; if (this->activeRenderer) this->activeRenderer->measureElementRenderedSize(c, safeW, safeH, mW, mH); contentW = std::max(contentW, c->x + mW); }
                float viewportW = safeW;
                // Thumb width proportional to track (sbw) times viewport/content ratio
                float thumbW = std::max(16.0f, sbw * (viewportW / contentW));
                float maxScroll = std::max(0.0f, contentW - viewportW);
                float thumbRelX = (maxScroll > 0.0f ? (p->scrollX / maxScroll) * (sbw - thumbW) : 0.0f);
                if (mouseX >= sbx + thumbRelX && mouseX <= sbx + thumbRelX + thumbW && mouseY >= sby && mouseY <= sby + sbh) {
                    isScrollbarDragging = true; scrollbarDragTarget = p; scrollbarDragAxis = 'x'; scrollbarDragStartMouse = mouseX; scrollbarDragStartScroll = p->scrollX; return;
                }
            }
            if (checkElementHitRecursive(p, mouseX, mouseY)) return;
        }
    }
}

void UIManagerImpl::handleMouseUp() {
    isMouseDown = false;
    if (draggedElement) {
        draggedElement->isBeingDragged = false;
        draggedElement = nullptr;
    }
    resizingWindow = nullptr;
}

void UIManagerImpl::handleMouseMove(float mouseX, float mouseY) {
    // If we're currently dragging a scrollbar thumb, update scroll offsets
    if (isMouseDown && isScrollbarDragging && scrollbarDragTarget) {
        // Try window
    if (impl::WindowImpl* w = dynamic_cast<impl::WindowImpl*>(scrollbarDragTarget)) {
        // compute safe content rect once for window
        float safeLeft = w->getSafeContentLeft();
        float safeTop = w->getSafeContentTop();
        float safeW = w->getSafeContentRight() - safeLeft;
        float safeH = w->getSafeContentBottom() - safeTop;
        if (scrollbarDragAxis == 'y') {
        float sby = safeTop + 4.0f; float sbh = safeH - 8.0f; // track area
                // content height (convert from absolute to content-area-relative coordinates)
                float contentH = 0.0f; for (UIElement* c : w->content) { if (!c) continue; float mW=c->width,mH=c->height; if (this->activeRenderer) this->activeRenderer->measureElementRenderedSize(c, safeW, safeH, mW, mH); contentH = std::max(contentH, c->y + mH); }
                float viewportH = safeH;
                // compute thumb height relative to the scrollbar track (sbh)
                float thumbH = std::max(16.0f, (contentH > 0.0f) ? sbh * (viewportH / contentH) : sbh);
                float usable = sbh - thumbH;
                float mouseDelta = mouseY - scrollbarDragStartMouse;
                float scrollRange = std::max(0.0f, contentH - viewportH);
                if (usable > 0.0f && scrollRange > 0.0f) {
                    float scrollDelta = (mouseDelta / usable) * scrollRange;
                    float newScroll = scrollbarDragStartScroll + scrollDelta;
                    float clamped = std::max(0.0f, std::min(newScroll, scrollRange));
                    if (clamped != newScroll) {
                        std::cerr << "[DIAG][UIManager] drag-clamp window id=" << w->id << " axis=Y contentH=" << contentH << " viewportH=" << viewportH << " scrollRange=" << scrollRange << " pre=" << newScroll << " clamped=" << clamped << std::endl;
                    }
                    w->scrollY = clamped;
                    w->scrollBarFadeTimerY = w->scrollBarFadeDuration;
                }
                return;
            }
            if (scrollbarDragAxis == 'x') {
                float sbx = safeLeft + 4.0f; float sbw = safeW - 8.0f;
                // content width (convert from absolute to content-area-relative coordinates)
                float contentW = 0.0f; for (UIElement* c : w->content) { if (!c) continue; float mW=c->width,mH=c->height; if (this->activeRenderer) this->activeRenderer->measureElementRenderedSize(c, safeW, safeH, mW, mH); contentW = std::max(contentW, c->x + mW); }
                float viewportW = safeW;
                // compute thumb width relative to the scrollbar track (sbw)
                float thumbW = std::max(16.0f, (contentW > 0.0f) ? sbw * (viewportW / contentW) : sbw);
                float usable = sbw - thumbW;
                float mouseDelta = mouseX - scrollbarDragStartMouse;
                float scrollRange = std::max(0.0f, contentW - viewportW);
                if (usable > 0.0f && scrollRange > 0.0f) {
                    float scrollDelta = (mouseDelta / usable) * scrollRange;
                    w->scrollX = scrollbarDragStartScroll + scrollDelta;
                    w->scrollX = std::max(0.0f, std::min(w->scrollX, scrollRange));
                    w->scrollBarFadeTimerX = w->scrollBarFadeDuration;
                }
                return;
            }
        }
        // Try panel
    if (impl::PanelImpl* p = dynamic_cast<impl::PanelImpl*>(scrollbarDragTarget)) {
        // compute safe content rect once for panel
        float safeLeft = p->getSafeContentLeft();
        float safeTop = p->getSafeContentTop();
        float safeW = p->getSafeContentRight() - safeLeft;
        float safeH = p->getSafeContentBottom() - safeTop;
        if (scrollbarDragAxis == 'y') {
        float sby = safeTop + 4.0f; float sbh = safeH - 8.0f; // track area
                // content height (convert from absolute to content-area-relative coordinates)
                float contentH = 0.0f; for (UIElement* c : p->content) { if (!c) continue; float mW=c->width,mH=c->height; if (this->activeRenderer) this->activeRenderer->measureElementRenderedSize(c, safeW, safeH, mW, mH); contentH = std::max(contentH, c->y + mH); }
                float viewportH = safeH;
                float thumbH = std::max(16.0f, (contentH > 0.0f) ? sbh * (viewportH / contentH) : sbh);
                float usable = sbh - thumbH;
                float mouseDelta = mouseY - scrollbarDragStartMouse;
                float scrollRange = std::max(0.0f, contentH - viewportH);
                if (usable > 0.0f && scrollRange > 0.0f) {
                    float scrollDelta = (mouseDelta / usable) * scrollRange;
                    p->scrollY = scrollbarDragStartScroll + scrollDelta;
                    p->scrollY = std::max(0.0f, std::min(p->scrollY, scrollRange));
                    p->scrollBarFadeTimerY = p->scrollBarFadeDuration;
                }
                return;
            }
            if (scrollbarDragAxis == 'x') {
                float sbx = safeLeft + 4.0f; float sbw = safeW - 8.0f;
                // content width (convert from absolute to content-area-relative coordinates)
                float contentW = 0.0f; for (UIElement* c : p->content) { if (!c) continue; float mW=c->width,mH=c->height; if (this->activeRenderer) this->activeRenderer->measureElementRenderedSize(c, safeW, safeH, mW, mH); contentW = std::max(contentW, c->x + mW); }
                float viewportW = safeW;
                float thumbW = std::max(16.0f, (contentW > 0.0f) ? sbw * (viewportW / contentW) : sbw);
                float usable = sbw - thumbW;
                float mouseDelta = mouseX - scrollbarDragStartMouse;
                float scrollRange = std::max(0.0f, contentW - viewportW);
                if (usable > 0.0f && scrollRange > 0.0f) {
                    float scrollDelta = (mouseDelta / usable) * scrollRange;
                    p->scrollX = scrollbarDragStartScroll + scrollDelta;
                    p->scrollX = std::max(0.0f, std::min(p->scrollX, scrollRange));
                    p->scrollBarFadeTimerX = p->scrollBarFadeDuration;
                }
                return;
            }
        }
    }

    // Update hover states (simple approach)
    for (auto p : panels) {
        for (auto child : p->content) {
            if (!child) continue;
            // Ensure programmatic global defaults are applied for elements
            // that were added to a panel after the panel was registered with the manager.
            BangUI::impl::GlobalDefaults::applyToElement(child);
            bool nowHovered = (mouseX >= child->x && mouseX <= child->x + child->width && mouseY >= child->y && mouseY <= child->y + child->height);
            bool prevHovered = child->isHovered;
            child->isHovered = nowHovered;
            if (nowHovered && !prevHovered) {
                if (child->onHover) child->onHover(child);
            } else if (!nowHovered && prevHovered) {
                if (child->onBlur) child->onBlur(child);
            }
        }
    }
    for (auto w : windows) {
        for (auto child : w->content) {
            if (!child) continue;
            // Apply programmatic global defaults lazily for children added after window registration.
            BangUI::impl::GlobalDefaults::applyToElement(child);
            bool nowHovered = (mouseX >= child->x && mouseX <= child->x + child->width && mouseY >= child->y && mouseY <= child->y + child->height);
            bool prevHovered = child->isHovered;
            child->isHovered = nowHovered;
            if (nowHovered && !prevHovered) {
                if (child->onHover) child->onHover(child);
            } else if (!nowHovered && prevHovered) {
                if (child->onBlur) child->onBlur(child);
            }
        }
    }
}

void UIManagerImpl::handleMouseDrag(float mouseX, float mouseY) {
    if (!draggedElement) return;
    // Compute candidate new absolute coordinates
    float candidateX = mouseX - draggedElement->dragOffsetX;
    float candidateY = mouseY - draggedElement->dragOffsetY;

    // If element has a parent and is docked on an axis, keep it attached
    // to the parent's safe edge on that axis and only allow movement on
    // the non-docked axis.
    if (draggedElement->parent) {
        UIElement* parent = draggedElement->parent;
        // Horizontal docking affects x; vertical docking affects y
        if (draggedElement->hDock != HDock::None) {
            // Determine x based on hDock
            switch (draggedElement->hDock) {
                case HDock::Left:
                    candidateX = parent->getSafeContentLeft() + draggedElement->marginLeft;
                    break;
                case HDock::Center:
                    candidateX = parent->getSafeContentLeft() + (parent->getSafeContentWidth() - draggedElement->width) / 2.0f;
                    break;
                case HDock::Right:
                    candidateX = parent->getSafeContentRight() - draggedElement->width - draggedElement->marginRight;
                    break;
                default: break;
            }
        }
        if (draggedElement->vDock != VDock::None) {
            switch (draggedElement->vDock) {
                case VDock::Top:
                    candidateY = parent->getSafeContentTop() + draggedElement->marginTop;
                    break;
                case VDock::Center:
                    candidateY = parent->getSafeContentTop() + (parent->getSafeContentHeight() - draggedElement->height) / 2.0f;
                    break;
                case VDock::Bottom:
                    candidateY = parent->getSafeContentBottom() - draggedElement->height - draggedElement->marginBottom;
                    break;
                default: break;
            }
        }
    }

    // Clamp to parent safe area if applicable
    clampToContainer(draggedElement, candidateX, candidateY);

    // Commit
    draggedElement->x = candidateX;
    draggedElement->y = candidateY;
    // Update local coordinates relative to parent so layout passes preserve the dragged position
    if (draggedElement->parent) {
        float safeLeft = draggedElement->parent->getSafeContentLeft();
        float safeTop = draggedElement->parent->getSafeContentTop();
        draggedElement->localX = draggedElement->x - safeLeft;
        draggedElement->localY = draggedElement->y - safeTop;
    } else {
        draggedElement->localX = draggedElement->x;
        draggedElement->localY = draggedElement->y;
    }
}

void UIManagerImpl::clampToContainer(UIElement* element, float& xOut, float& yOut) {
    if (!element) return;
    if (element->parent) {
        UIElement* parent = element->parent;
        float left = parent->getSafeContentLeft();
        float top = parent->getSafeContentTop();
        float right = parent->getSafeContentRight() - element->width;
        float bottom = parent->getSafeContentBottom() - element->height;
        if (xOut < left) xOut = left;
        if (xOut > right) xOut = right;
        if (yOut < top) yOut = top;
        if (yOut > bottom) yOut = bottom;
    } else {
        // No parent -> clamp to application window bounds using padding
        float left = appWindowPadding;
        float top = appWindowPadding;
        float right = appWindowWidth - appWindowPadding - element->width;
        float bottom = appWindowHeight - appWindowPadding - element->height;
        if (xOut < left) xOut = left;
        if (xOut > right) xOut = right;
        if (yOut < top) yOut = top;
        if (yOut > bottom) yOut = bottom;
    }
}

void UIManagerImpl::handleWindowResize(float mouseX, float mouseY) {
    if (!resizingWindow) return;
    float dx = mouseX - resizeStartX;
    float dy = mouseY - resizeStartY;
    float newW = std::max(1.0f, resizeStartWidth + dx);
    float newH = std::max(1.0f, resizeStartHeight + dy);

    bool triggerLayout = false;

    if (resizingWindow->parent) {
        UIElement* parent = resizingWindow->parent;
        float safeW = parent->getSafeContentWidth();
        float safeH = parent->getSafeContentHeight();

        // If parent is not Auto-sized on an axis, clamp the child's size to parent's safe size
        if (parent->widthMode != SizeMode::Auto && newW > safeW) newW = safeW;
        if (parent->heightMode != SizeMode::Auto && newH > safeH) newH = safeH;

        // If parent is Auto-sized on an axis, allow child's size to grow and trigger layout recompute
        if (parent->widthMode == SizeMode::Auto && newW != resizingWindow->width) triggerLayout = true;
        if (parent->heightMode == SizeMode::Auto && newH != resizingWindow->height) triggerLayout = true;
    } else {
        // Top-level window: clamp to application bounds
        float maxW = std::max(1.0f, appWindowWidth - appWindowPadding * 2.0f);
        float maxH = std::max(1.0f, appWindowHeight - appWindowPadding * 2.0f);
        if (newW > maxW) newW = maxW;
        if (newH > maxH) newH = maxH;
    }

    resizingWindow->width = newW;
    resizingWindow->height = newH;

    // After resizing, if parent Auto should adapt, recompute layout for all elements
    if (triggerLayout) {
        std::vector<UIElement*> allElements;
        for (impl::PanelImpl* p : panels) allElements.push_back(p);
        for (impl::WindowImpl* w : windows) allElements.push_back(w);
        LayoutManager::computeLayout(appWindowWidth, appWindowHeight, allElements);
    }
}

// The helper methods were inlined into the header to keep handleEvent
// inline-friendly; remaining implementation in this file supports rendering
// and other non-inline behaviours.

void UIManagerImpl::Update(float dt) {
    (void)dt; // reserved for future animation/logic updates
    // Recompute layout each frame using the current application window
    // dimensions stored on the manager. This centralizes layout so apps
    // don't need to call LayoutManager::computeLayout themselves.
    // Size changes are handled by SDL window events (resize/maximize/restore)
    // in handleEventSDLImpl. Avoid querying the native window size every
    // frame for performance reasons; the app may still call
    // setApplicationWindow() if it wants to override dimensions manually.

    // (Removed diagnostic logging)

    // Ensure programmatic global defaults are applied to every element (and their children)
    // before computing layout so prototypes act as true defaults on first render.
    std::function<void(API::UIElement*)> applyRecursively = [&](API::UIElement* el) {
        if (!el) return;
        BangUI::impl::GlobalDefaults::applyToElement(el);
        // If the element is a panel or window, apply recursively to its content
        if (impl::PanelImpl* p = dynamic_cast<impl::PanelImpl*>(el)) {
            for (auto child : p->content) applyRecursively(child);
        } else if (impl::WindowImpl* w = dynamic_cast<impl::WindowImpl*>(el)) {
            for (auto child : w->content) applyRecursively(child);
        }
    };

    std::vector<UIElement*> allElements;
    for (impl::PanelImpl* p : panels) {
        applyRecursively(p);
        allElements.push_back(p);
    }
    for (impl::WindowImpl* w : windows) {
        applyRecursively(w);
        allElements.push_back(w);
    }
    LayoutManager::computeLayout(appWindowWidth, appWindowHeight, allElements);

    // Debug: print positions of panelB_nowrap children to diagnose stacking issues
    for (impl::PanelImpl* p : panels) {
        if (p->id == "panelB_nowrap") {
            std::cerr << "[DEBUG] panelB_nowrap children positions:" << std::endl;
            for (UIElement* c : p->content) {
                if (!c) continue;
                std::cerr << "  child id='" << c->id << "' x=" << c->x << " y=" << c->y << " localX=" << c->localX << " localY=" << c->localY << "\n";
            }
        }
    }

    // Update scrollbar fade timers (decrease) per-axis
    for (impl::PanelImpl* p : panels) {
        if (p->scrollBarFadeTimerX > 0.0f) p->scrollBarFadeTimerX = std::max(0.0f, p->scrollBarFadeTimerX - dt);
        if (p->scrollBarFadeTimerY > 0.0f) p->scrollBarFadeTimerY = std::max(0.0f, p->scrollBarFadeTimerY - dt);
    }
    for (impl::WindowImpl* w : windows) {
        if (w->scrollBarFadeTimerX > 0.0f) w->scrollBarFadeTimerX = std::max(0.0f, w->scrollBarFadeTimerX - dt);
        if (w->scrollBarFadeTimerY > 0.0f) w->scrollBarFadeTimerY = std::max(0.0f, w->scrollBarFadeTimerY - dt);
    }
}

void UIManagerImpl::Render(API::IRenderer* renderer) {
    if (!renderer) return;
    // Expose the active renderer so input handling can query measurements
    this->activeRenderer = renderer;
    for (impl::PanelImpl* p : panels) renderer->renderPanel(reinterpret_cast<const API::IPanel*>(p));
    for (impl::WindowImpl* w : windows) renderer->renderWindow(reinterpret_cast<const API::IWindow*>(w));
    this->activeRenderer = nullptr;
}

// Define destructor out-of-line to ensure the vtable is emitted in this TU.
UIManagerImpl::~UIManagerImpl() {}

// Storage for the optional event interceptor (defined in header as member)
} // namespace BangUI::impl
