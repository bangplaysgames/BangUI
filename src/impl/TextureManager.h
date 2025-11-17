#pragma once
#include "../impl/NineSliceTexture.h"
#include "../standalone/ImageLoader.h"
#include "../standalone/SoftwareRenderer.h"
#include <string>
#include <map>
#include <iostream>
#include <filesystem>
#include <vector>
#include <cstring>

// TextureManager: Handles loading, caching, and cleanup of SDL textures
// Uses RGBA8 format for proper alpha channel support
// Supports 9-patch/9-slice textures (.9.png)
class TextureManager {
private:
    SDL_Renderer* renderer;
    std::map<std::string, SDL_Texture*> textureCache;
    std::map<std::string, NineSliceTexture*> nineSliceCache;

public:
    TextureManager(SDL_Renderer* sdlRenderer) : renderer(sdlRenderer) {
        std::cout << "TextureManager initialized (standalone software backend)" << std::endl;
    }

    ~TextureManager() {
        // Clean up all cached textures
        for (auto& pair : textureCache) {
            if (pair.second) {
                SDL_DestroyTexture(pair.second);
            }
        }
        textureCache.clear();

        // Clean up 9-slice textures
        for (auto& pair : nineSliceCache) {
            if (pair.second) {
                delete pair.second;
            }
        }
        nineSliceCache.clear();
    }

    // Load a texture from file path, with caching
    // Returns nullptr if loading fails
    SDL_Texture* loadTexture(const std::string& path) {
        if (path.empty()) {
            return nullptr;
        }

        // Check if texture is already cached
        auto it = textureCache.find(path);
        if (it != textureCache.end()) {
            return it->second;
        }

        int w = 0;
        int h = 0;
        std::vector<uint8_t> pixels;
        if (!BangUI::Standalone::LoadImageRGBA(path, w, h, pixels)) {
            std::cerr << "Unable to load image " << path << std::endl;
            return nullptr;
        }

        SDL_Surface* surface = SDL_CreateSurface(w, h);
        std::memcpy(SDL_SurfacePixels(surface), pixels.data(), pixels.size());

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);

        if (!texture) {
            std::cerr << "Unable to create texture from " << path << std::endl;
            return nullptr;
        }

        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

        // Cache the texture
        textureCache[path] = texture;

        std::cout << "Loaded texture: " << path << std::endl;
        return texture;
    }

    // Get a cached texture (returns nullptr if not loaded)
    SDL_Texture* getTexture(const std::string& path) {
        auto it = textureCache.find(path);
        if (it != textureCache.end()) {
            return it->second;
        }
        return nullptr;
    }

    // Remove a texture from cache and free it
    void unloadTexture(const std::string& path) {
        auto it = textureCache.find(path);
        if (it != textureCache.end()) {
            if (it->second) {
                SDL_DestroyTexture(it->second);
            }
            textureCache.erase(it);
        }
    }

    // Load a 9-patch texture (supports .9.png format)
    NineSliceTexture* loadNineSliceTexture(const std::string& path) {
        if (path.empty()) {
            return nullptr;
        }

        // Check if already cached
        auto it = nineSliceCache.find(path);
        if (it != nineSliceCache.end()) {
            return it->second;
        }

        int w = 0;
        int h = 0;
        std::vector<uint8_t> pixels;
        if (!BangUI::Standalone::LoadImageRGBA(path, w, h, pixels)) {
            std::cerr << "Unable to load 9-patch image " << path << std::endl;
            return nullptr;
        }

        NineSliceTexture* nineSlice = new NineSliceTexture();
        nineSlice->textureWidth = w;
        nineSlice->textureHeight = h;
        nineSlice->isNinePatch = NineSliceTexture::isNinePatchFile(path);

        // Parse 9-patch if applicable
        if (nineSlice->isNinePatch) {
            if (!nineSlice->parseNinePatch(pixels, w, h)) {
                std::cerr << "Failed to parse 9-patch: " << path << std::endl;
                delete nineSlice;
                return nullptr;
            }
        }

        SDL_Surface* surface = SDL_CreateSurface(w, h);
        std::memcpy(SDL_SurfacePixels(surface), pixels.data(), pixels.size());

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);

        if (!texture) {
            std::cerr << "Unable to create texture from 9-patch " << path << std::endl;
            delete nineSlice;
            return nullptr;
        }

        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

        nineSlice->texture = texture;

        // Cache the 9-slice texture
        nineSliceCache[path] = nineSlice;

        std::cout << "Loaded 9-patch texture: " << path << std::endl;
        return nineSlice;
    }

    // Get a cached 9-slice texture
    NineSliceTexture* getNineSliceTexture(const std::string& path) {
        auto it = nineSliceCache.find(path);
        if (it != nineSliceCache.end()) {
            return it->second;
        }
        return nullptr;
    }

    // Clear all cached textures
    void clearCache() {
        for (auto& pair : textureCache) {
            if (pair.second) {
                SDL_DestroyTexture(pair.second);
            }
        }
        textureCache.clear();

        for (auto& pair : nineSliceCache) {
            if (pair.second) {
                delete pair.second;
            }
        }
        nineSliceCache.clear();
    }
};
