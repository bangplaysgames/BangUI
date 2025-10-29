#pragma once
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include <map>
#include <iostream>

// Simple FontManager: loads TTF fonts and creates text textures (cached per-string)
class FontManager {
public:
    FontManager() {
        if (!TTF_Init()) {
            std::cerr << "TTF_Init failed: " << SDL_GetError() << std::endl;
            initialized = false;
        } else {
            initialized = true;
        }
    }

    ~FontManager() {
        // free cached textures
        for (auto &kv : textCache) {
            if (kv.second) SDL_DestroyTexture(kv.second);
        }
        textCache.clear();

        // free fonts
        for (auto &kv : fonts) {
            if (kv.second) TTF_CloseFont(kv.second);
        }
        fonts.clear();

        if (initialized) TTF_Quit();
    }

    bool isInitialized() const { return initialized; }

    // Load a font from path at given size. Returns nullptr on failure.
    TTF_Font* loadFont(const std::string &path, int ptsize) {
        std::string key = path + "|" + std::to_string(ptsize);
        auto it = fonts.find(key);
        if (it != fonts.end()) return it->second;

        TTF_Font* f = TTF_OpenFont(path.c_str(), ptsize);
        if (!f) {
            // Try a common Windows fallback if running on Windows
#ifdef _WIN32
            if (path != "C:/Windows/Fonts/arial.ttf") {
                f = TTF_OpenFont("C:/Windows/Fonts/arial.ttf", ptsize);
            }
#endif
        }

        if (!f) {
            std::cerr << "Failed to load font '" << path << "' (size=" << ptsize << "): " << SDL_GetError() << std::endl;
            return nullptr;
        }

        fonts[key] = f;
        return f;
    }

    // Create or return a cached texture for the given text (UTF-8). Returns nullptr on failure.
    SDL_Texture* getTextTexture(SDL_Renderer* renderer, TTF_Font* font, const std::string &text, SDL_Color color, int &outW, int &outH) {
        if (!font || !initialized || !renderer) return nullptr;

        std::string fontId = std::to_string(reinterpret_cast<uintptr_t>(font));
        std::string key = fontId + "|" + text + "|" + std::to_string(color.r) + "," + std::to_string(color.g) + "," + std::to_string(color.b) + "," + std::to_string(color.a);

        auto it = textCache.find(key);
        if (it != textCache.end()) {
            // get size from cachedSizes
            auto sit = cachedSizes.find(key);
            if (sit != cachedSizes.end()) {
                outW = sit->second.first;
                outH = sit->second.second;
            }
            return it->second;
        }

        // Render blended text to a surface then create texture
        // Use the SDL3_ttf API: TTF_RenderText_Blended with length=0 for null-terminated text
        SDL_Surface* surf = TTF_RenderText_Blended(font, text.c_str(), 0, color);
        if (!surf) {
            std::cerr << "TTF_RenderText_Blended failed: " << SDL_GetError() << std::endl;
            return nullptr;
        }

        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
        if (!tex) {
            std::cerr << "SDL_CreateTextureFromSurface failed: " << SDL_GetError() << std::endl;
            SDL_DestroySurface(surf);
            return nullptr;
        }

        outW = surf->w;
        outH = surf->h;
        SDL_DestroySurface(surf);

        // Cache
        textCache[key] = tex;
        cachedSizes[key] = std::make_pair(outW, outH);

        return tex;
    }

private:
    bool initialized = false;
    std::map<std::string, TTF_Font*> fonts;
    std::map<std::string, SDL_Texture*> textCache;
    std::map<std::string, std::pair<int,int>> cachedSizes;
};
