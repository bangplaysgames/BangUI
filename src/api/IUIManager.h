// Minimal public interface for the UIManager
#pragma once

#include "UIElement.h"
#include "Events.h"
#include "Theme.h"
#include <memory>

namespace BangUI::API {

class IRenderer; // forward

class IUIManager {
public:
    virtual ~IUIManager() = default;
    virtual void Update(float dt) = 0;
    virtual void Render(IRenderer* renderer) = 0;
    virtual bool onEvent(const UIEvent& e) { (void)e; return false; }
    // Additional shimbed methods implemented by the impl during migration
    virtual void setApplicationWindow(float width, float height, float padding = 10.0f) = 0;
    virtual float getAppWindowWidth() const = 0;
    virtual float getAppWindowHeight() const = 0;
    virtual float getAppWindowPadding() const = 0;
    // Provide an optional native window handle to the manager. The pointer
    // type is intentionally void* to avoid coupling the public API to a
    // specific windowing library. Implementations may reinterpret_cast the
    // pointer to their platform type (for example SDL_Window*).
    virtual void setNativeWindow(void* nativeWindow) { (void)nativeWindow; }
    // Start/stop listening to native windowing events (optional). When
    // started, the manager will register an internal event watch (for
    // example with SDL) and update its internal state (window size, etc.)
    // in response to those events. Default implementations are no-ops.
    virtual void startEventListening() { }
    virtual void stopEventListening() { }
    virtual void handleEvent(const UIEvent& e) = 0;

    // Background theming controls
    virtual void setBackgroundTheme(const BackgroundTheme& theme) = 0;
    virtual const BackgroundTheme& getBackgroundTheme() const = 0;
};

} // namespace BangUI::API
