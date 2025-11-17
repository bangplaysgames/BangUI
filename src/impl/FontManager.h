#pragma once

#include "../standalone/SoftwareRenderer.h"
#include <map>
#include <memory>
#include <string>

class FontManager {
public:
    struct FontInstance;

    FontManager();
    ~FontManager();

    bool isInitialized() const { return initialized; }

    FontInstance* loadFont(const std::string& path, int ptsize);
    SDL_Texture* getTextTexture(SDL_Renderer* renderer, FontInstance* font, const std::string& text, SDL_Color color, int& outW, int& outH);

private:
    bool initialized{false};
    struct CachedText {
        SDL_Texture* texture{nullptr};
        int width{0};
        int height{0};
    };

    std::map<std::string, std::unique_ptr<FontInstance>> fonts;
    std::map<std::string, CachedText> textCache;
};

