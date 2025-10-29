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
        for (auto it = w->content.rbegin(); it != w->content.rend(); ++it) {
            API::UIElement* child = *it;
            if (checkElementHitRecursive(child, mouseX, mouseY)) return true;
        }

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

        return false;
    }

    if (ButtonImpl* b = dynamic_cast<ButtonImpl*>(element)) {
        if (b->onClick) b->onClick();
        return true;
    }
    if (element->onClick) { element->onClick(); return true; }

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
        if (checkElementHitRecursive(modal, mouseX, mouseY)) return;
    } else {
        // Iterate windows top-down
        for (auto it = windows.rbegin(); it != windows.rend(); ++it) {
            impl::WindowImpl* w = *it;
            if (!w->visible) continue;
            if (checkElementHitRecursive(w, mouseX, mouseY)) return;
        }
        // Then panels
        for (auto it = panels.rbegin(); it != panels.rend(); ++it) {
            impl::PanelImpl* p = *it;
            if (!p->visible) continue;
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
    // Update hover states (simple approach)
    for (auto p : panels) {
        for (auto child : p->content) {
            if (!child) continue;
            bool nowHovered = (mouseX >= child->x && mouseX <= child->x + child->width && mouseY >= child->y && mouseY <= child->y + child->height);
            child->isHovered = nowHovered;
        }
    }
    for (auto w : windows) {
        for (auto child : w->content) {
            if (!child) continue;
            bool nowHovered = (mouseX >= child->x && mouseX <= child->x + child->width && mouseY >= child->y && mouseY <= child->y + child->height);
            child->isHovered = nowHovered;
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

    std::vector<UIElement*> allElements;
    for (impl::PanelImpl* p : panels) allElements.push_back(p);
    for (impl::WindowImpl* w : windows) allElements.push_back(w);
    LayoutManager::computeLayout(appWindowWidth, appWindowHeight, allElements);
}

void UIManagerImpl::Render(API::IRenderer* renderer) {
    if (!renderer) return;
    // (Removed diagnostic logging)
    for (impl::PanelImpl* p : panels) renderer->renderPanel(reinterpret_cast<const API::IPanel*>(p));
    for (impl::WindowImpl* w : windows) renderer->renderWindow(reinterpret_cast<const API::IWindow*>(w));
}

// Define destructor out-of-line to ensure the vtable is emitted in this TU.
UIManagerImpl::~UIManagerImpl() {}

// Storage for the optional event interceptor (defined in header as member)
} // namespace BangUI::impl
