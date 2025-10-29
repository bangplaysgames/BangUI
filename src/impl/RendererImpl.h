#pragma once
#include "../api/IRenderer.h"
#include "UIRenderer.h"

namespace BangUI::impl {

class RendererImpl : public API::IRenderer {
public:
    RendererImpl(SDL_Renderer* sdlRenderer)
        : uiRenderer(sdlRenderer) {
    }

    ~RendererImpl() override = default;

    TextureManager* getTextureManager() override {
        return uiRenderer.getTextureManager();
    }

    FontManager* getFontManager() override {
        return uiRenderer.getFontManager();
    }

    void renderPanel(const API::IPanel* panel) override {
        // cast to concrete Panel for now (compat shim keeps types identical)
        uiRenderer.renderPanel(reinterpret_cast<const Panel*>(panel));
    }

    void renderWindow(const API::IWindow* window) override {
        uiRenderer.renderWindow(reinterpret_cast<const Window*>(window));
    }

private:
    UIRenderer uiRenderer;
};

} // namespace BangUI::Impl
