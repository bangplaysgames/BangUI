#include "FontManager.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstring>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#include "../standalone/thirdparty/stb_truetype.h"

struct FontManager::FontInstance {
    std::string key;
    int size;
    std::vector<unsigned char> data;
    stbtt_fontinfo info;
    float scale;
    int ascent;
    int descent;
    int lineGap;
    FontInstance() : size(0), scale(1.0f), ascent(0), descent(0), lineGap(0) {}
};

FontManager::FontManager() {
    initialized = true;
}

FontManager::~FontManager() {
    for (auto& entry : textCache) {
        if (entry.second.texture) {
            SDL_DestroyTexture(entry.second.texture);
        }
    }
    textCache.clear();
}

FontManager::FontInstance* FontManager::loadFont(const std::string& path, int ptsize) {
    if (!initialized) return nullptr;
    if (path.empty() || ptsize <= 0) return nullptr;

    std::string key = path + "|" + std::to_string(ptsize);
    auto it = fonts.find(key);
    if (it != fonts.end()) return it->second.get();

    std::filesystem::path resolved(path);
    if (!std::filesystem::exists(resolved)) {
#ifdef _WIN32
        resolved = std::filesystem::path("C:/Windows/Fonts/arial.ttf");
#else
        resolved = path;
#endif
    }

    std::ifstream file(resolved, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open font: " << resolved.string() << std::endl;
        return nullptr;
    }

    std::vector<unsigned char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (buffer.empty()) {
        std::cerr << "Font file empty: " << resolved.string() << std::endl;
        return nullptr;
    }

    auto font = std::make_unique<FontInstance>();
    font->data = std::move(buffer);
    if (!stbtt_InitFont(&font->info, font->data.data(), 0)) {
        std::cerr << "stbtt_InitFont failed for: " << resolved.string() << std::endl;
        return nullptr;
    }
    font->size = ptsize;
    font->scale = stbtt_ScaleForPixelHeight(&font->info, static_cast<float>(ptsize));
    stbtt_GetFontVMetrics(&font->info, &font->ascent, &font->descent, &font->lineGap);
    font->key = key;

    FontInstance* ptr = font.get();
    fonts[key] = std::move(font);
    return ptr;
}

SDL_Texture* FontManager::getTextTexture(SDL_Renderer* renderer, FontInstance* font, const std::string& text, SDL_Color color, int& outW, int& outH) {
    outW = 0;
    outH = 0;
    if (!renderer || !font || text.empty()) return nullptr;

    std::string cacheKey = font->key + "|" + text + "|" +
        std::to_string(color.r) + "," + std::to_string(color.g) + "," +
        std::to_string(color.b) + "," + std::to_string(color.a);

    auto cacheIt = textCache.find(cacheKey);
    if (cacheIt != textCache.end()) {
        outW = cacheIt->second.width;
        outH = cacheIt->second.height;
        return cacheIt->second.texture;
    }

    // Measure text dimensions (supports newlines)
    int lineHeight = static_cast<int>((font->ascent - font->descent + font->lineGap) * font->scale);
    if (lineHeight <= 0) lineHeight = font->size;

    int maxWidth = 0;
    int totalHeight = 0;
    size_t lineStart = 0;
    std::vector<std::string> lines;
    while (lineStart <= text.size()) {
        size_t lineEnd = text.find('\n', lineStart);
        if (lineEnd == std::string::npos) lineEnd = text.size();
        std::string line = text.substr(lineStart, lineEnd - lineStart);
        lines.push_back(line);

        int width = 0;
        int prev = 0;
        for (unsigned char ch : line) {
            int advance = 0;
            int lsb = 0;
            stbtt_GetCodepointHMetrics(&font->info, ch, &advance, &lsb);
            width += static_cast<int>(advance * font->scale);
            if (prev) {
                width += static_cast<int>(stbtt_GetCodepointKernAdvance(&font->info, prev, ch) * font->scale);
            }
            prev = ch;
        }
        maxWidth = std::max(maxWidth, width);
        lineStart = lineEnd + 1;
    }

    totalHeight = static_cast<int>(lines.size()) * lineHeight;
    if (maxWidth <= 0) maxWidth = 1;
    if (totalHeight <= 0) totalHeight = lineHeight;

    SDL_Surface* surface = SDL_CreateSurface(maxWidth, totalHeight);
    std::vector<Uint8> surfacePixels(maxWidth * totalHeight * 4, 0);

    int baselineOffset = static_cast<int>(font->ascent * font->scale);

    for (size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex) {
        const std::string& line = lines[lineIndex];
        int cursorX = 0;
        int prev = 0;
        for (unsigned char ch : line) {
            int advance = 0;
            int lsb = 0;
            stbtt_GetCodepointHMetrics(&font->info, ch, &advance, &lsb);

            int glyphW = 0;
            int glyphH = 0;
            int offsetX = 0;
            int offsetY = 0;
            unsigned char* bitmap = stbtt_GetCodepointBitmap(&font->info, 0, font->scale, ch, &glyphW, &glyphH, &offsetX, &offsetY);
            if (bitmap) {
                int xPos = cursorX + static_cast<int>(offsetX);
                int yPos = static_cast<int>(lineIndex * lineHeight + baselineOffset + offsetY);
                for (int gy = 0; gy < glyphH; ++gy) {
                    for (int gx = 0; gx < glyphW; ++gx) {
                        int dstX = xPos + gx;
                        int dstY = yPos + gy;
                        if (dstX < 0 || dstY < 0 || dstX >= maxWidth || dstY >= totalHeight) continue;
                        Uint8 alpha = bitmap[gy * glyphW + gx];
                        Uint8* dst = &surfacePixels[(dstY * maxWidth + dstX) * 4];
                        float a = alpha / 255.0f;
                        dst[0] = static_cast<Uint8>(color.r * a);
                        dst[1] = static_cast<Uint8>(color.g * a);
                        dst[2] = static_cast<Uint8>(color.b * a);
                        dst[3] = static_cast<Uint8>(color.a * a);
                    }
                }
                stbtt_FreeBitmap(bitmap, nullptr);
            }

            cursorX += static_cast<int>(advance * font->scale);
            if (prev) {
                cursorX += static_cast<int>(stbtt_GetCodepointKernAdvance(&font->info, prev, ch) * font->scale);
            }
            prev = ch;
        }
    }

    std::memcpy(SDL_SurfacePixels(surface), surfacePixels.data(), surfacePixels.size());
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    if (!texture) {
        return nullptr;
    }

    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    CachedText cached;
    cached.texture = texture;
    cached.width = maxWidth;
    cached.height = totalHeight;
    textCache[cacheKey] = cached;

    outW = maxWidth;
    outH = totalHeight;
    return texture;
}
