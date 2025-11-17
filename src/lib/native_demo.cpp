#include "../impl/UIManagerImpl.h"
#include "../impl/RendererImpl.h"
#include "../standalone/NativeWindow.h"
#include "../standalone/SoftwareRenderer.h"
#include "../impl/PanelImpl.h"
#include "../impl/WindowImpl.h"
#include "../impl/ButtonImpl.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

using namespace BangUI;

int main(int argc, char** argv) {
    namespace fs = std::filesystem;
    fs::path assetRoot = fs::current_path();
    if (argc > 0) {
        fs::path exePath = fs::absolute(argv[0]);
        assetRoot = exePath.parent_path();
        // exe path is build/src/<config>, so go up three levels to reach repo root
        for (int i = 0; i < 3 && assetRoot.has_parent_path(); ++i) {
            assetRoot = assetRoot.parent_path();
        }
    }
    auto resolveAsset = [&](const std::string& relative) -> std::string {
        fs::path candidate = assetRoot / relative;
        if (fs::exists(candidate)) {
            return candidate.string();
        }
        return relative;
    };
    Standalone::NativeWindow window;
    int initialWidth = 1280;
    int initialHeight = 720;
    if (!window.create(L"BangUI Standalone Demo", initialWidth, initialHeight)) {
        std::cerr << "Failed to create native window" << std::endl;
        return 1;
    }
    window.setCaptionHitHeight(0);
    window.setTopResizeEnabled(false);
    bool running = true;
    std::function<void(float)> renderFrame;
    bool layoutDirty = true;

    SDL_Renderer* renderer = SDL_CreateSoftwareRenderer(initialWidth, initialHeight);
    if (!renderer) {
        std::cerr << "Failed to initialize software renderer" << std::endl;
        return 1;
    }

    impl::UIManagerImpl uiManager;
    impl::RendererImpl rendererImpl(renderer);
    uiManager.setApplicationWindow(static_cast<float>(initialWidth), static_cast<float>(initialHeight), 0.0f);

    BangUI::API::BackgroundTheme theme;
    theme.enabled = false;
    uiManager.setBackgroundTheme(theme);

    auto* appFrame = new impl::WindowImpl();
    appFrame->id = "app_frame";
    appFrame->title = "BangUI Demo";
    appFrame->width = static_cast<float>(initialWidth);
    appFrame->height = static_cast<float>(initialHeight);
    appFrame->x = 0.0f;
    appFrame->y = 0.0f;
    appFrame->movable = true;
    appFrame->resizable = true;
    appFrame->closeable = false;
    appFrame->interactable = true;
    appFrame->titleBarVisible = true;
    appFrame->titleTextColor = API::Color(32, 222, 249, 220);
    appFrame->bodyTexture = resolveAsset("window.9.png");
    appFrame->titleBarTexture = resolveAsset("titlebar.9.png");
    appFrame->zIndex = -100;
    appFrame->backgroundColor = API::Color(0, 0, 0, 0);
    appFrame->titleBarColor = API::Color(0, 0, 0, 0);
    appFrame->borderWidth = 0.0f;
    appFrame->properties["nativeResize"] = "true";
    appFrame->delegateMoveToNative = true;
    appFrame->beginNativeDragCallback = [&window]() { window.beginNativeDrag(); };
    appFrame->clearChromeButtons();

    auto addChromeButton = [&](const impl::WindowImpl::ChromeButton& btn) -> impl::WindowImpl::ChromeButton* {
        appFrame->addChromeButton(btn);
        auto& buttons = appFrame->getChromeButtons();
        return &buttons.back();
    };

    impl::WindowImpl::ChromeButton appBurger;
    appBurger.type = impl::WindowImpl::ChromeButton::Type::Menu;
    appBurger.anchor = impl::WindowImpl::ChromeButton::Anchor::Left;
    appBurger.backgroundColor = API::Color(0, 0, 0, 0);
    appBurger.iconColor = API::Color(32, 222, 249, 220);
    appBurger.onClick = [](impl::WindowImpl*) {
        std::cout << "[Demo] Application menu clicked" << std::endl;
    };
    addChromeButton(appBurger);

    impl::WindowImpl::ChromeButton appToggle;
    appToggle.type = impl::WindowImpl::ChromeButton::Type::Custom;
    appToggle.backgroundColor = API::Color(55, 55, 75, 220);
    appToggle.iconColor = API::Color(32, 222, 249, 220);
    appToggle.onClick = [&window, &renderFrame](impl::WindowImpl*) {
        window.toggleMaximize();
        if (renderFrame) renderFrame(0.0f);
    };
    appToggle.customDraw = [&window](SDL_Renderer* renderer, const impl::WindowImpl::ChromeButton& btn,
                                     float x, float y, float w, float h) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, btn.iconColor.r, btn.iconColor.g, btn.iconColor.b, btn.iconColor.a);
        float minDim = (w < h ? w : h);
        float margin = minDim * 0.25f;
        if (window.isMaximized()) {
            SDL_FRect topRect = {x + margin, y + margin + 2.0f, w - margin * 2.0f, h - margin * 2.5f};
            SDL_RenderRect(renderer, &topRect);
            SDL_FRect bottomRect = {x + margin + 3.0f, y + margin - 1.0f, w - margin * 2.0f, h - margin * 2.5f};
            SDL_RenderRect(renderer, &bottomRect);
        } else {
            SDL_FRect rect = {x + margin, y + margin, w - margin * 2.0f, h - margin * 2.0f};
            SDL_RenderRect(renderer, &rect);
        }
    };

    impl::WindowImpl::ChromeButton appMinimize;
    appMinimize.type = impl::WindowImpl::ChromeButton::Type::Minimize;
    appMinimize.backgroundColor = API::Color(55, 55, 75, 220);
    appMinimize.iconColor = API::Color(32, 222, 249, 220);
    appMinimize.onClick = [&window](impl::WindowImpl*) {
        window.minimize();
    };

    auto appClose = impl::WindowImpl::ChromeButton::CloseDefault();
    appClose.backgroundColor = API::Color(255, 15, 15, 220);
    appClose.onClick = [&window, &running](impl::WindowImpl*) {
        running = false;
        window.close();
    };
    addChromeButton(appMinimize);
    addChromeButton(appToggle);
    addChromeButton(appClose);
    uiManager.addWindow(appFrame);

    auto* window1 = new impl::WindowImpl();
    window1->id = "window1";
    window1->title = "Standalone BangUI";
    window1->width = 420;
    window1->height = 260;
    window1->x = 120;
    window1->y = 80;
    window1->movable = true;
    window1->resizable = true;
    window1->closeable = true;
    window1->backgroundColor = API::Color(40, 110, 200, 220);
    window1->titleBarColor = API::Color(25, 25, 25, 255);
    window1->clearChromeButtons();
    impl::WindowImpl::ChromeButton burger;
    burger.type = impl::WindowImpl::ChromeButton::Type::Menu;
    burger.anchor = impl::WindowImpl::ChromeButton::Anchor::Left;
    burger.backgroundColor = API::Color(35, 35, 50, 200);
    burger.iconColor = API::Color(255, 198, 145, 255);
    burger.onClick = [](impl::WindowImpl* win) {
        std::cout << "[Demo] Burger menu clicked on '" << (win ? win->title : "<window>") << "'" << std::endl;
    };
    window1->addChromeButton(burger);

    impl::WindowImpl::ChromeButton minimize;
    minimize.type = impl::WindowImpl::ChromeButton::Type::Minimize;
    minimize.backgroundColor = API::Color(45, 45, 65, 220);
    minimize.iconColor = API::Color(255, 198, 145, 255);
    window1->addChromeButton(minimize);

    impl::WindowImpl::ChromeButton maximize;
    maximize.type = impl::WindowImpl::ChromeButton::Type::Maximize;
    maximize.backgroundColor = API::Color(55, 55, 75, 220);
    maximize.iconColor = API::Color(255, 198, 145, 255);
    window1->addChromeButton(maximize);

    auto closeButton = impl::WindowImpl::ChromeButton::CloseDefault();
    closeButton.backgroundColor = API::Color(210, 70, 70, 240);
    window1->addChromeButton(closeButton);
    uiManager.addWindow(window1);

    auto* window2 = new impl::WindowImpl();
    window2->id = "window2";
    window2->title = "Docked Controls";
    window2->widthMode = SizeMode::Absolute;
    window2->heightMode = SizeMode::Absolute;
    window2->width = 320;
    window2->height = 200;
    window2->x = 680;
    window2->y = 360;
    window2->movable = true;
    window2->resizable = true;
    window2->backgroundColor = API::Color(200, 110, 40, 220);
    window2->titleBarColor = API::Color(35, 35, 35, 220);
    window2->clearChromeButtons();
    window2->closeable = true;
    window2->addChromeButton(impl::WindowImpl::ChromeButton::CloseDefault());
    uiManager.addWindow(window2);

    renderFrame = [&](float dt) {
        int currentWidth = window.clientWidth();
        int currentHeight = window.clientHeight();
        if (currentWidth > 0 && currentHeight > 0) {
            if (currentWidth != SDL_RendererWidth(renderer) || currentHeight != SDL_RendererHeight(renderer)) {
                SDL_RendererResize(renderer, currentWidth, currentHeight);
                uiManager.setApplicationWindow(static_cast<float>(currentWidth), static_cast<float>(currentHeight), 0.0f);
                appFrame->width = static_cast<float>(currentWidth);
                appFrame->height = static_cast<float>(currentHeight);
                layoutDirty = true;
            }
        }
        if (layoutDirty || dt > 0.0f) {
            uiManager.Update(dt);
            layoutDirty = false;
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        uiManager.Render(&rendererImpl);
        window.present(renderer);
    };
    renderFrame(0.0f);
    auto lastResizeRender = std::chrono::steady_clock::now();
    window.setResizeCallback([&]() {
        layoutDirty = true;
        renderFrame(0.0f);
    });

    auto lastTime = std::chrono::steady_clock::now();

    while (running && window.isOpen()) {
        API::UIEvent event;
        const int maxEventsPerFrame = 64;
        int processed = 0;
        while (processed < maxEventsPerFrame && window.pollEvent(event)) {
            if (event.type == API::UIEventType::Quit) {
                running = false;
                break;
            }
            if (event.type == API::UIEventType::WindowResize && event.width > 0 && event.height > 0) {
                SDL_RendererResize(renderer, event.width, event.height);
                uiManager.setApplicationWindow(static_cast<float>(event.width), static_cast<float>(event.height), 0.0f);
                appFrame->width = static_cast<float>(event.width);
                appFrame->height = static_cast<float>(event.height);
                layoutDirty = true;
            }
            uiManager.handleEvent(event);
            layoutDirty = true;
            processed++;
        }

        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        renderFrame(dt);
    }

    SDL_DestroyRenderer(renderer);
    return 0;
}
