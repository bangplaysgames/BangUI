#pragma once
#include <vector>
#include <functional>
#include "../api/IUIManager.h"
#include "../api/UIElement.h"
#include "../api/Theme.h"
#include "../impl/WindowImpl.h"
#include "../impl/PanelImpl.h"
#include "../impl/LayoutManager.h"
#include "../impl/ButtonImpl.h"
#include "../impl/BML/BMLParser.h"
#include "../impl/BML/GlobalDefaults.h"

namespace BangUI::impl {

class UIManagerImpl : public BangUI::API::IUIManager {
public:
    std::vector<impl::PanelImpl*> panels;
    std::vector<impl::WindowImpl*> windows;

    UIElement* draggedElement = nullptr;
    impl::WindowImpl* resizingWindow = nullptr;
    bool isMouseDown = false;
    float resizeStartX = 0.0f;
    float resizeStartY = 0.0f;
    float resizeStartWidth = 0.0f;
    float resizeStartHeight = 0.0f;

    // Scrollbar dragging state
    bool isScrollbarDragging = false;
    // which element is being scrolled by drag (could be PanelImpl or WindowImpl)
    UIElement* scrollbarDragTarget = nullptr;
    // axis: 'x' or 'y'
    char scrollbarDragAxis = '\0';
    // drag start positions
    float scrollbarDragStartMouse = 0.0f;
    float scrollbarDragStartScroll = 0.0f;

    float appWindowWidth = 800.0f;
    float appWindowHeight = 600.0f;
    float appWindowPadding = 10.0f;
    // Optional opaque native window pointer (e.g. SDL_Window*) set by callers
    void* nativeWindow = nullptr;

    void setApplicationWindow(float width, float height, float padding = 10.0f) override {
        appWindowWidth = width; appWindowHeight = height; appWindowPadding = padding;
    }

    void setNativeWindow(void* nativeWindowPtr) override { nativeWindow = nativeWindowPtr; }
    void startEventListening() override {}
    void stopEventListening() override {}

    float getAppWindowWidth() const override { return appWindowWidth; }
    float getAppWindowHeight() const override { return appWindowHeight; }
    float getAppWindowPadding() const override { return appWindowPadding; }
    void setBackgroundTheme(const API::BackgroundTheme& theme) override { backgroundTheme = theme; }
    const API::BackgroundTheme& getBackgroundTheme() const override { return backgroundTheme; }

    void addPanel(impl::PanelImpl* panel) {
        panels.push_back(panel);
        // apply Global_<Type> styles from BML if available
    auto& parser = BangUI::impl::BMLParser::getGlobalParser();
        parser.applyGlobalStyleToElement(panel->type, panel);
        // apply programmatic global defaults
        BangUI::impl::GlobalDefaults::applyToElement(panel);
    }
    void addWindow(impl::WindowImpl* window) {
        windows.push_back(window);
    auto& parser = BangUI::impl::BMLParser::getGlobalParser();
        parser.applyGlobalStyleToElement(window->type, window);
        BangUI::impl::GlobalDefaults::applyToElement(window);
    }

    bool removePanel(impl::PanelImpl* panel) {
        auto it = std::find(panels.begin(), panels.end(), panel);
        if (it != panels.end()) { panels.erase(it); return true; }
        return false;
    }

    bool removeWindow(impl::WindowImpl* window) {
        auto it = std::find(windows.begin(), windows.end(), window);
        if (it != windows.end()) { windows.erase(it); return true; }
        return false;
    }

    ~UIManagerImpl() override;

    void handleEvent(const API::UIEvent& e) override;

    // API-compatible methods (implemented in the .cpp)
    void Update(float dt) override;
    void Render(API::IRenderer* renderer) override;
    // Active renderer used to query measurements during input handling
    API::IRenderer* activeRenderer = nullptr;
    bool onEvent(const API::UIEvent& e) override { return API::IUIManager::onEvent(e); }

    // Allow callers to provide a std::function-based handler that will be
    // invoked before library handling. If the handler returns true the event
    // is considered handled by the application.
    void setEventInterceptor(std::function<bool(const API::UIEvent&)> interceptor) {
        eventInterceptor = interceptor;
    }

    // API exposures (adapters) matching original names used by demo
    const std::vector<impl::PanelImpl*>& getPanels() const { return panels; }
    const std::vector<impl::WindowImpl*>& getWindows() const { return windows; }

private:
    std::function<bool(const API::UIEvent&)> eventInterceptor;
    API::BackgroundTheme backgroundTheme;
    impl::WindowImpl* getModalWindow() const;
    bool checkElementHitRecursive(UIElement* element, float mouseX, float mouseY);
    void handleMouseDown(float mouseX, float mouseY);
    void handleMouseUp();
    void handleMouseMove(float mouseX, float mouseY);
    void handleMouseDrag(float mouseX, float mouseY);
    void clampToContainer(UIElement* element, float& x, float& y);
    void handleWindowResize(float mouseX, float mouseY);
    void processEvent(const API::UIEvent& e);
    bool handleChromeButtonClick(impl::WindowImpl* window, const impl::WindowImpl::ChromeButton& button);

    // The implementations for the helper methods live in the .cpp file.
};

} // namespace BangUI::impl
