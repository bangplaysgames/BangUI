#include "factories.h"
#include "UIManagerImpl.h"
#include "RendererImpl.h"
#include "../standalone/SoftwareRenderer.h"

namespace BangUI::impl {

std::unique_ptr<API::IUIManager> CreateDefaultUIManager() {
    return std::unique_ptr<API::IUIManager>(new impl::UIManagerImpl());
}

std::unique_ptr<API::IRenderer> CreateDefaultRenderer() {
    SDL_Renderer* renderer = SDL_CreateSoftwareRenderer(800, 600);
    if (!renderer) {
        return nullptr;
    }
    return std::unique_ptr<API::IRenderer>(new impl::RendererImpl(renderer));
}

} // namespace BangUI::impl
