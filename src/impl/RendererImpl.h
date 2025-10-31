#pragma once
#include "../api/IRenderer.h"
#include "UIRenderer.h"

namespace BangUI::impl {

class RendererImpl : public API::IRenderer {
public:
    RendererImpl(SDL_Renderer* sdlRenderer)
        : uiRenderer(sdlRenderer) {
        diegeticMapper = nullptr;
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

    void measureElementRenderedSize(API::UIElement* element, float maxW, float maxH, float& outW, float& outH) override {
        // forward to the concrete renderer implementation
        uiRenderer.measureElementRenderedSize(element, maxW, maxH, outW, outH);
    }

    void setDiegeticMapper(DiegeticMapperFn fn) override;

private:
    UIRenderer uiRenderer;
    DiegeticMapperFn diegeticMapper;
};

} // namespace BangUI::Impl
