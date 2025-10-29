#pragma once
#include <SDL3/SDL.h>
#include <string>
#include <iostream>

// NineSliceTexture: Handles 9-patch/9-slice texture loading and rendering
// Supports Android .9.png format with 1-pixel guide borders
class NineSliceTexture {
public:
    SDL_Texture* texture = nullptr;

    // Source rectangle coordinates (excluding 1px border for .9.png)
    int contentLeft = 0;
    int contentTop = 0;
    int contentRight = 0;
    int contentBottom = 0;

    // Stretchable regions (read from guide pixels)
    int stretchLeft = 0;
    int stretchRight = 0;
    int stretchTop = 0;
    int stretchBottom = 0;

    // Original texture dimensions
    int textureWidth = 0;
    int textureHeight = 0;

    // Is this a 9-patch texture? (has .9.png extension)
    bool isNinePatch = false;

    NineSliceTexture() = default;

    ~NineSliceTexture() {
        // Texture is managed by TextureManager, don't destroy here
    }

    // Check if filename indicates 9-patch format
    static bool isNinePatchFile(const std::string& path) {
        if (path.length() < 6) return false;
        return path.substr(path.length() - 6) == ".9.png";
    }

    // Parse 9-patch border pixels to determine stretch regions
    bool parseNinePatch(SDL_Surface* surface) {
        if (!surface) return false;

        int w = surface->w;
        int h = surface->h;

        if (w < 3 || h < 3) {
            std::cerr << "9-patch image too small (must be at least 3x3)" << std::endl;
            return false;
        }

        // Lock surface for pixel access
        if (SDL_MUSTLOCK(surface)) {
            SDL_LockSurface(surface);
        }

        Uint32* pixels = (Uint32*)surface->pixels;
        int pitch = surface->pitch / 4; // pitch in pixels (assuming 32-bit RGBA)

        // Parse top border (horizontal stretch)
        bool foundStart = false;
        for (int x = 1; x < w - 1; x++) {
            Uint32 pixel = pixels[x];
            Uint8 r, g, b, a;
            SDL_GetRGBA(pixel, SDL_GetPixelFormatDetails(surface->format), nullptr, &r, &g, &b, &a);

            // Black pixel (RGB near 0) indicates stretchable region
            bool isBlack = (a > 200) && (r < 50) && (g < 50) && (b < 50);

            if (isBlack && !foundStart) {
                stretchLeft = x - 1; // Exclude border pixel
                foundStart = true;
            }
            if (!isBlack && foundStart && stretchRight == 0) {
                stretchRight = x - 1;
            }
        }

        if (!foundStart) {
            // No stretch region defined, stretch entire center
            stretchLeft = 1;
            stretchRight = w - 2;
        } else if (stretchRight == 0) {
            // Stretch to end
            stretchRight = w - 2;
        }

        // Parse left border (vertical stretch)
        foundStart = false;
        for (int y = 1; y < h - 1; y++) {
            Uint32 pixel = pixels[y * pitch];
            Uint8 r, g, b, a;
            SDL_GetRGBA(pixel, SDL_GetPixelFormatDetails(surface->format), nullptr, &r, &g, &b, &a);

            bool isBlack = (a > 200) && (r < 50) && (g < 50) && (b < 50);

            if (isBlack && !foundStart) {
                stretchTop = y - 1;
                foundStart = true;
            }
            if (!isBlack && foundStart && stretchBottom == 0) {
                stretchBottom = y - 1;
            }
        }

        if (!foundStart) {
            stretchTop = 1;
            stretchBottom = h - 2;
        } else if (stretchBottom == 0) {
            stretchBottom = h - 2;
        }

        // Parse right and bottom borders for content padding (optional)
        // For now, we'll use the full interior as content area
        contentLeft = 1;
        contentTop = 1;
        contentRight = w - 2;
        contentBottom = h - 2;

        if (SDL_MUSTLOCK(surface)) {
            SDL_UnlockSurface(surface);
        }

        std::cout << "9-patch parsed: stretch H(" << stretchLeft << "-" << stretchRight
                  << ") V(" << stretchTop << "-" << stretchBottom << ")" << std::endl;

        return true;
    }

    // Get the safe content area bounds (center patch) for a given element size
    // Returns the insets from each edge where content should be placed
    void getSafeContentInsets(float& left, float& top, float& right, float& bottom) const {
        if (!isNinePatch) {
            // No 9-patch = no insets, entire area is safe
            left = top = right = bottom = 0.0f;
            return;
        }

        // Calculate edge sizes (corners + borders, not the center)
        left = (float)stretchLeft;
        top = (float)stretchTop;
        right = (float)(contentRight - stretchRight);
        bottom = (float)(contentBottom - stretchBottom);
    }

    // Render the 9-patch texture to fit the target rectangle
    void render(SDL_Renderer* renderer, float x, float y, float w, float h, Uint8 alpha = 255) {
        if (!texture || !isNinePatch) {
            // Regular texture rendering
            SDL_FRect dest = {x, y, w, h};
            SDL_RenderTexture(renderer, texture, nullptr, &dest);
            return;
        }

        // Calculate patch dimensions (excluding 1px border)
        int leftWidth = stretchLeft;
        int rightWidth = contentRight - stretchRight;
        int topHeight = stretchTop;
        int bottomHeight = contentBottom - stretchBottom;
        int centerWidth = stretchRight - stretchLeft;
        int centerHeight = stretchBottom - stretchTop;

        // Calculate destination dimensions
        float destLeftWidth = (float)leftWidth;
        float destRightWidth = (float)rightWidth;
        float destTopHeight = (float)topHeight;
        float destBottomHeight = (float)bottomHeight;
        float destCenterWidth = w - destLeftWidth - destRightWidth;
        float destCenterHeight = h - destTopHeight - destBottomHeight;

        // Ensure we have space for corners
        if (destCenterWidth < 0) destCenterWidth = 0;
        if (destCenterHeight < 0) destCenterHeight = 0;

        // Render 9 patches
    // Top-left corner
    renderPatch(renderer,
           contentLeft, contentTop, leftWidth, topHeight,
           x, y, destLeftWidth, destTopHeight, alpha);

        // Top edge (stretched horizontally)
    renderPatch(renderer,
           stretchLeft, contentTop, centerWidth, topHeight,
           x + destLeftWidth, y, destCenterWidth, destTopHeight, alpha);

        // Top-right corner
    renderPatch(renderer,
           stretchRight, contentTop, rightWidth, topHeight,
           x + destLeftWidth + destCenterWidth, y, destRightWidth, destTopHeight, alpha);

        // Left edge (stretched vertically)
    renderPatch(renderer,
           contentLeft, stretchTop, leftWidth, centerHeight,
           x, y + destTopHeight, destLeftWidth, destCenterHeight, alpha);

        // Center (stretched both ways)
    renderPatch(renderer,
           stretchLeft, stretchTop, centerWidth, centerHeight,
           x + destLeftWidth, y + destTopHeight, destCenterWidth, destCenterHeight, alpha);

        // Right edge (stretched vertically)
    renderPatch(renderer,
           stretchRight, stretchTop, rightWidth, centerHeight,
           x + destLeftWidth + destCenterWidth, y + destTopHeight,
           destRightWidth, destCenterHeight, alpha);

        // Bottom-left corner
    renderPatch(renderer,
           contentLeft, stretchBottom, leftWidth, bottomHeight,
           x, y + destTopHeight + destCenterHeight,
           destLeftWidth, destBottomHeight, alpha);

        // Bottom edge (stretched horizontally)
    renderPatch(renderer,
           stretchLeft, stretchBottom, centerWidth, bottomHeight,
           x + destLeftWidth, y + destTopHeight + destCenterHeight,
           destCenterWidth, destBottomHeight, alpha);

        // Bottom-right corner
    renderPatch(renderer,
           stretchRight, stretchBottom, rightWidth, bottomHeight,
           x + destLeftWidth + destCenterWidth, y + destTopHeight + destCenterHeight,
           destRightWidth, destBottomHeight, alpha);
    }

private:
    void renderPatch(SDL_Renderer* renderer,
                    int srcX, int srcY, int srcW, int srcH,
                    float destX, float destY, float destW, float destH,
                    Uint8 alpha = 255) {
        if (srcW <= 0 || srcH <= 0 || destW <= 0 || destH <= 0) return;

        SDL_FRect src = {(float)srcX, (float)srcY, (float)srcW, (float)srcH};
        SDL_FRect dest = {destX, destY, destW, destH};
        // Modulate texture alpha per-patch so opacity works for 9-patch rendering
        SDL_SetTextureAlphaMod(texture, alpha);
        SDL_RenderTexture(renderer, texture, &src, &dest);
    }
};
