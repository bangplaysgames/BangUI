#include "factories.h"
#include "UIManagerImpl.h"
#include "RendererImpl.h"
#include <SDL3/SDL.h>

namespace BangUI::impl {

std::unique_ptr<API::IUIManager> CreateDefaultUIManager() {
    return std::unique_ptr<API::IUIManager>(new impl::UIManagerImpl());
}

std::unique_ptr<API::IRenderer> CreateDefaultRenderer() {
    // Create a basic SDL window+renderer for library default. This is a convenience factory
    // for demos; production embedders should construct their own SDL_Renderer and use
    // CreateRendererFromSDL(SDL_Renderer*) instead.
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        return nullptr;
    }
    // SDL3 CreateWindow signature takes title, width, height, and flags
    SDL_Window* win = SDL_CreateWindow("BangUI-Renderer-Stub", 800, 600, SDL_WINDOW_RESIZABLE);
    if (!win) {
        SDL_Quit();
        return nullptr;
    }
    SDL_Renderer* sdlR = SDL_CreateRenderer(win, nullptr);
    if (!sdlR) {
        SDL_DestroyWindow(win);
        SDL_Quit();
        return nullptr;
    }

    return std::unique_ptr<API::IRenderer>(new impl::RendererImpl(sdlR));
}

} // namespace BangUI::impl
