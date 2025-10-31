#pragma once
#include "../impl/NineSliceTexture.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <string>
#include <map>
#include <iostream>
#include <filesystem>

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
        // SDL3_image doesn't require initialization - it auto-detects formats
        std::cout << "TextureManager initialized (SDL3_image auto-detects PNG/JPG)" << std::endl;
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

        // Load image as SDL_Surface. Try original path first, then a fallback via current working directory
        SDL_Surface* loadedSurface = IMG_Load(path.c_str());
        if (!loadedSurface) {
            try {
                std::filesystem::path p(path);
                if (p.is_relative()) {
                    std::filesystem::path alt = std::filesystem::current_path() / p;
                    std::string altStr = alt.string();
                    loadedSurface = IMG_Load(altStr.c_str());
                    if (loadedSurface) std::cout << "Loaded texture via fallback path: " << altStr << std::endl;
                }
            } catch (...) {}
        }
        if (!loadedSurface) {
            std::cerr << "Unable to load image " << path << "! SDL_image Error: " << SDL_GetError() << std::endl;
            return nullptr;
        }

        // Convert surface to RGBA8888 format for consistent alpha channel handling
        SDL_Surface* formattedSurface = SDL_ConvertSurface(loadedSurface, SDL_PIXELFORMAT_RGBA8888);
        SDL_DestroySurface(loadedSurface);

        if (!formattedSurface) {
            std::cerr << "Unable to convert surface to RGBA8888! SDL Error: " << SDL_GetError() << std::endl;
            return nullptr;
        }

        // Create texture from surface
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, formattedSurface);
        SDL_DestroySurface(formattedSurface);

        if (!texture) {
            std::cerr << "Unable to create texture from " << path << "! SDL Error: " << SDL_GetError() << std::endl;
            return nullptr;
        }

        // Set blend mode to support alpha blending
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

        // Load image as SDL_Surface. Try original path first, then a fallback via current working directory
        SDL_Surface* loadedSurface = IMG_Load(path.c_str());
        if (!loadedSurface) {
            try {
                std::filesystem::path p(path);
                if (p.is_relative()) {
                    std::filesystem::path alt = std::filesystem::current_path() / p;
                    std::string altStr = alt.string();
                    loadedSurface = IMG_Load(altStr.c_str());
                    if (loadedSurface) std::cout << "Loaded 9-patch via fallback path: " << altStr << std::endl;
                }
            } catch (...) {}
        }
        if (!loadedSurface) {
            std::cerr << "Unable to load 9-patch image " << path << "! SDL_image Error: " << SDL_GetError() << std::endl;
            return nullptr;
        }

        // Create 9-slice texture object
        NineSliceTexture* nineSlice = new NineSliceTexture();
        nineSlice->textureWidth = loadedSurface->w;
        nineSlice->textureHeight = loadedSurface->h;
        nineSlice->isNinePatch = NineSliceTexture::isNinePatchFile(path);

        // Parse 9-patch if applicable
        if (nineSlice->isNinePatch) {
            if (!nineSlice->parseNinePatch(loadedSurface)) {
                std::cerr << "Failed to parse 9-patch: " << path << std::endl;
                SDL_DestroySurface(loadedSurface);
                delete nineSlice;
                return nullptr;
            }
        }

        // Convert surface to RGBA8888 format
        SDL_Surface* formattedSurface = SDL_ConvertSurface(loadedSurface, SDL_PIXELFORMAT_RGBA8888);
        SDL_DestroySurface(loadedSurface);

        if (!formattedSurface) {
            std::cerr << "Unable to convert 9-patch surface to RGBA8888! SDL Error: " << SDL_GetError() << std::endl;
            delete nineSlice;
            return nullptr;
        }

        // Create texture from surface
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, formattedSurface);
        SDL_DestroySurface(formattedSurface);

        if (!texture) {
            std::cerr << "Unable to create texture from 9-patch " << path << "! SDL Error: " << SDL_GetError() << std::endl;
            delete nineSlice;
            return nullptr;
        }

        // Set blend mode to support alpha blending
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
