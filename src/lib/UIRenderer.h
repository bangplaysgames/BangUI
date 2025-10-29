#pragma once
#define _USE_MATH_DEFINES
#include <cmath>
#include <SDL3/SDL.h>
#include <algorithm>
#include <vector>
#include "../models/Window.h"
#include "../models/Panel.h"
#include "TextureManager.h"
#include "FontManager.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// UIRenderer: Core library class that handles rendering of UI elements
// Provides consistent rendering for Windows and Panels with proper rounded corners
// Supports texture rendering with color blending
class UIRenderer {
private:
    SDL_Renderer* renderer;
    TextureManager* textureManager;
    FontManager* fontManager;

    // Draw a properly rounded rectangle using composite shapes
    void drawRoundedRectangle(float x, float y, float w, float h, float radius) {
        if (radius <= 0 || radius > std::min(w, h) / 2.0f) {
            // No rounding or radius too large - draw normal rectangle
            SDL_FRect rect = {x, y, w, h};
            SDL_RenderFillRect(renderer, &rect);
            return;
        }

        // Draw the main rectangle parts (avoiding corners)
        // Center horizontal rectangle
        SDL_FRect centerH = {x + radius, y, w - 2 * radius, h};
        SDL_RenderFillRect(renderer, &centerH);

        // Left and right vertical strips
        SDL_FRect leftV = {x, y + radius, radius, h - 2 * radius};
        SDL_RenderFillRect(renderer, &leftV);

        SDL_FRect rightV = {x + w - radius, y + radius, radius, h - 2 * radius};
        SDL_RenderFillRect(renderer, &rightV);

        // Draw filled circles at the four corners using scanline method
        drawFilledCircleQuarter(x + radius, y + radius, radius, 2); // Top-left (quadrant 2)
        drawFilledCircleQuarter(x + w - radius, y + radius, radius, 1); // Top-right (quadrant 1)
        drawFilledCircleQuarter(x + radius, y + h - radius, radius, 3); // Bottom-left (quadrant 3)
        drawFilledCircleQuarter(x + w - radius, y + h - radius, radius, 4); // Bottom-right (quadrant 4)
    }

    // Draw a filled quarter circle (one quadrant) using scanline fill
    void drawFilledCircleQuarter(float cx, float cy, float radius, int quadrant) {
        int r = (int)radius;

        for (int dy = 0; dy <= r; dy++) {
            int dx = (int)sqrt(r * r - dy * dy);

            float y1, x1, x2;

            switch (quadrant) {
                case 1: // Top-right
                    y1 = cy - dy;
                    x1 = cx;
                    x2 = cx + dx;
                    break;
                case 2: // Top-left
                    y1 = cy - dy;
                    x1 = cx - dx;
                    x2 = cx;
                    break;
                case 3: // Bottom-left
                    y1 = cy + dy;
                    x1 = cx - dx;
                    x2 = cx;
                    break;
                case 4: // Bottom-right
                    y1 = cy + dy;
                    x1 = cx;
                    x2 = cx + dx;
                    break;
                default:
                    return;
            }

            SDL_RenderLine(renderer, x1, y1, x2, y1);
        }
    }

    // Draw a rounded rectangle border
    void drawRoundedRectangleBorder(float x, float y, float w, float h, float radius) {
        if (radius <= 0 || radius > std::min(w, h) / 2.0f) {
            // No rounding - draw normal rectangle border
            SDL_FRect rect = {x, y, w, h};
            SDL_RenderRect(renderer, &rect);
            return;
        }

        // Draw the four straight edges
        SDL_RenderLine(renderer, x + radius, y, x + w - radius, y); // Top edge
        SDL_RenderLine(renderer, x + radius, y + h, x + w - radius, y + h); // Bottom edge
        SDL_RenderLine(renderer, x, y + radius, x, y + h - radius); // Left edge
        SDL_RenderLine(renderer, x + w, y + radius, x + w, y + h - radius); // Right edge

        // Draw the four corner arcs
        drawCircleArc(x + radius, y + radius, radius, 180, 270); // Top-left
        drawCircleArc(x + w - radius, y + radius, radius, 270, 360); // Top-right
        drawCircleArc(x + radius, y + h - radius, radius, 90, 180); // Bottom-left
        drawCircleArc(x + w - radius, y + h - radius, radius, 0, 90); // Bottom-right
    }

    // Draw a circular arc (portion of circle outline)
    void drawCircleArc(float cx, float cy, float radius, int startAngle, int endAngle) {
        int segments = 16; // Number of line segments for a 90-degree arc
        float angleStep = (endAngle - startAngle) / (float)segments;

        for (int i = 0; i < segments; i++) {
            float angle1 = (startAngle + angleStep * i) * M_PI / 180.0f;
            float angle2 = (startAngle + angleStep * (i + 1)) * M_PI / 180.0f;

            float x1 = cx + radius * cos(angle1);
            float y1 = cy + radius * sin(angle1);
            float x2 = cx + radius * cos(angle2);
            float y2 = cy + radius * sin(angle2);

            SDL_RenderLine(renderer, x1, y1, x2, y2);
        }
    }

    // Render a textured rectangle with optional color overlay blending
    // The color's alpha channel determines how much the color blends over the texture
    void renderTexturedRectangle(SDL_Texture* texture, float x, float y, float w, float h,
                                 Uint8 r, Uint8 g, Uint8 b, Uint8 textureA, Uint8 overlayA) {
        SDL_FRect destRect = {x, y, w, h};

        // Apply alpha modulation to the texture so overall opacity can be controlled
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod(texture, textureA);

        // First, render the texture
        SDL_RenderTexture(renderer, texture, nullptr, &destRect);

        // If overlay alpha > 0, blend the color over the texture based on overlay strength
        if (overlayA > 0) {
            SDL_SetRenderDrawColor(renderer, r, g, b, overlayA);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_RenderFillRect(renderer, &destRect);
        }
    }

    // Render a textured rounded rectangle with optional color overlay blending
    void renderTexturedRoundedRectangle(SDL_Texture* texture, float x, float y, float w, float h,
                                       float radius, Uint8 r, Uint8 g, Uint8 b, Uint8 textureA, Uint8 overlayA) {
        // For rounded rectangles with textures, we need to use a clip mask approach
        // This is a simplified version - render texture first, then overlay color with rounding

        // Create a temporary texture for masking (more complex implementation)
        // For now, we'll render the full texture and then draw rounded color overlay
        SDL_FRect destRect = {x, y, w, h};

        // Apply alpha modulation to the texture so overall opacity can be controlled
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod(texture, textureA);

        SDL_RenderTexture(renderer, texture, nullptr, &destRect);

        // Blend color over texture with rounded rectangle shape
        if (overlayA > 0) {
            SDL_SetRenderDrawColor(renderer, r, g, b, overlayA);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            drawRoundedRectangle(x, y, w, h, radius);
        }
    }

public:
    UIRenderer(SDL_Renderer* sdlRenderer) : renderer(sdlRenderer) {
        textureManager = new TextureManager(sdlRenderer);
        fontManager = new FontManager();
    }

    ~UIRenderer() {
        delete textureManager;
        delete fontManager;
    }

    // Get the texture manager for external texture loading
    TextureManager* getTextureManager() {
        return textureManager;
    }

    FontManager* getFontManager() {
        return fontManager;
    }

    // Render a panel with its properties
    void renderPanel(const Panel* panel) {
        if (!panel->visible) return;

        // Check if panel has a 9-slice texture
        NineSliceTexture* nineSlice = nullptr;
        SDL_Texture* texture = nullptr;

    // Get Colors
    Uint8 r = panel->backgroundColor.r;
    Uint8 g = panel->backgroundColor.g;
    Uint8 b = panel->backgroundColor.b;
    Uint8 a = panel->backgroundColor.a;

    // overlayAlpha is the alpha of the background color used for color overlays
    Uint8 overlayAlpha = a;
    // textureAlpha: if background alpha is 0 (no overlay color), use full texture alpha (255),
    // otherwise use the background alpha. Multiply by panel opacity to get final texture alpha.
    int texAlphaInt = static_cast<int>((overlayAlpha == 0 ? 255 : overlayAlpha) * panel->opacity + 0.5f);
    if (texAlphaInt < 0) texAlphaInt = 0; if (texAlphaInt > 255) texAlphaInt = 255;
    Uint8 texA = static_cast<Uint8>(texAlphaInt);
    // overlayAlpha multiplied by opacity for color overlays
    int overlayAlphaInt = static_cast<int>(overlayAlpha * panel->opacity + 0.5f);
    if (overlayAlphaInt < 0) overlayAlphaInt = 0; if (overlayAlphaInt > 255) overlayAlphaInt = 255;
    Uint8 overlayA = static_cast<Uint8>(overlayAlphaInt);

        if (!panel->src.empty()) {
            if (NineSliceTexture::isNinePatchFile(panel->src)) {
                nineSlice = textureManager->loadNineSliceTexture(panel->src);
            } else {
                texture = textureManager->loadTexture(panel->src);
            }
        }

        if (nineSlice) {
            // Render 9-slice texture (handles stretching automatically)
            nineSlice->render(renderer, panel->x, panel->y, panel->width, panel->height, static_cast<Uint8>(panel->backgroundColor.a * panel->opacity));

            // Set safe content area to center patch only
            nineSlice->getSafeContentInsets(
                const_cast<Panel*>(panel)->contentLeft,
                const_cast<Panel*>(panel)->contentTop,
                const_cast<Panel*>(panel)->contentRight,
                const_cast<Panel*>(panel)->contentBottom
            );

            if (overlayA > 0) {
                SDL_SetRenderDrawColor(renderer, r, g, b, overlayA);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                drawRoundedRectangle(panel->x, panel->y, panel->width, panel->height, panel->cornerRadius);
            }
        } else if (texture) {
            // Render regular textured panel with color overlay based on backgroundColor alpha
            // Regular texture - entire area is safe
            const_cast<Panel*>(panel)->contentLeft = 0.0f;
            const_cast<Panel*>(panel)->contentTop = 0.0f;
            const_cast<Panel*>(panel)->contentRight = 0.0f;
            const_cast<Panel*>(panel)->contentBottom = 0.0f;

            if (panel->cornerRadius > 0) {
                    renderTexturedRoundedRectangle(texture, panel->x, panel->y, panel->width, panel->height,
                                                  panel->cornerRadius, r, g, b, texA, overlayA);
            } else {
                // Use texA and overlayA so textures colorize correctly and respect opacity
                    renderTexturedRectangle(texture, panel->x, panel->y, panel->width, panel->height,
                                           r, g, b, texA, overlayA);
            }
        } else {
            // Draw the solid color rounded rectangle shape
            SDL_SetRenderDrawColor(renderer, r, g, b, overlayA);
            drawRoundedRectangle(panel->x, panel->y, panel->width, panel->height, panel->cornerRadius);
        }

        // Border (lighter color) - now rounded to match the shape!
        uint8_t br = panel->borderColor.r;
        uint8_t bg = panel->borderColor.g;
        uint8_t bb = panel->borderColor.b;
        uint8_t ba = panel->borderColor.a;
        SDL_SetRenderDrawColor(renderer, br, bg, bb, ba);
        drawRoundedRectangleBorder(panel->x, panel->y, panel->width, panel->height, panel->cornerRadius);

        // Drag indicator
        if (panel->isBeingDragged) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 128);
            drawRoundedRectangleBorder(panel->x + 5, panel->y + 5, panel->width - 10, panel->height - 10,
                                      panel->cornerRadius > 5 ? panel->cornerRadius - 5 : 0);
        }
            // render children clipped to the panel's safe content area using integer clip rects
            // Render children only if they intersect the panel's safe content area (coarse clipping)
            SDL_FRect safeArea;
            safeArea.x = panel->getSafeContentLeft();
            safeArea.y = panel->getSafeContentTop();
            safeArea.w = panel->getSafeContentRight() - panel->getSafeContentLeft();
            safeArea.h = panel->getSafeContentBottom() - panel->getSafeContentTop();

            for (UIElement* child : panel->content) {
                if (!child) continue;
                SDL_FRect childRect = { child->x, child->y, child->width, child->height };
                bool intersects = !(childRect.x + childRect.w <= safeArea.x ||
                                    childRect.x >= safeArea.x + safeArea.w ||
                                    childRect.y + childRect.h <= safeArea.y ||
                                    childRect.y >= safeArea.y + safeArea.h);
                if (intersects) {
                    renderElement(child);
                }
            }
    }

    // Render a window with title bar and close button
    void renderWindow(const Window* win) {
        if (!win->visible) return;

        float x = win->x;
        float y = win->y;
        float w = win->width;
        float h = win->height;
        float radius = win->cornerRadius;
        float titleHeight = Window::TITLE_BAR_HEIGHT;

        // Clamp radius
        if (radius > std::min(w, h) / 2.0f) {
            radius = std::min(w, h) / 2.0f;
        }

        // Check if window has a texture
        NineSliceTexture* nineSlice = nullptr;
        SDL_Texture* texture = nullptr;
        bool hasTexture = false;

        if (!win->src.empty()) {
            if (NineSliceTexture::isNinePatchFile(win->src)) {
                nineSlice = textureManager->loadNineSliceTexture(win->src);
                hasTexture = (nineSlice != nullptr);
            } else {
                texture = textureManager->loadTexture(win->src);
                hasTexture = (texture != nullptr);
            }
        }

        // If texture exists, render ONLY the texture (no title bar, no border, no color)
        if (hasTexture) {
            if (nineSlice) {
                nineSlice->render(renderer, x, y, w, h);
                // Set safe content area to center patch only
                nineSlice->getSafeContentInsets(
                    const_cast<Window*>(win)->contentLeft,
                    const_cast<Window*>(win)->contentTop,
                    const_cast<Window*>(win)->contentRight,
                    const_cast<Window*>(win)->contentBottom
                );
            } else if (texture) {
                SDL_FRect destRect = {x, y, w, h};
                SDL_RenderTexture(renderer, texture, nullptr, &destRect);
                // Regular texture - entire area is safe
                const_cast<Window*>(win)->contentLeft = 0.0f;
                const_cast<Window*>(win)->contentTop = 0.0f;
                const_cast<Window*>(win)->contentRight = 0.0f;
                const_cast<Window*>(win)->contentBottom = 0.0f;
            }
        } else {
            // No texture - render traditional window with title bar and body
            // Get title bar color from window property
            Uint8 titleR = win->titleBarColor.r;
            Uint8 titleG = win->titleBarColor.g;
            Uint8 titleB = win->titleBarColor.b;
            Uint8 titleA = win->titleBarColor.a * win->opacity;
            Uint8 bodyA = win->backgroundColor.a * win->opacity;
            Uint8 bodyR = win->backgroundColor.r;
            Uint8 bodyG = win->backgroundColor.g;
            Uint8 bodyB = win->backgroundColor.b;

            if (radius > 0) {
                // Draw title bar with top-rounded corners
                // First draw the body color as base
                SDL_SetRenderDrawColor(renderer, bodyR, bodyG, bodyB, bodyA);

                // Title bar center
                SDL_FRect titleCenter = {x + radius, y, w - 2 * radius, titleHeight};
                SDL_RenderFillRect(renderer, &titleCenter);

                // Title bar sides
                SDL_FRect titleLeft = {x, y + radius, radius, titleHeight - radius};
                SDL_RenderFillRect(renderer, &titleLeft);

                SDL_FRect titleRight = {x + w - radius, y + radius, radius, titleHeight - radius};
                SDL_RenderFillRect(renderer, &titleRight);

                // Top corners
                drawFilledCircleQuarter(x + radius, y + radius, radius, 2); // Top-left
                drawFilledCircleQuarter(x + w - radius, y + radius, radius, 1); // Top-right

                // Then overlay the title bar color with alpha blending
                if (titleA > 0) {
                    SDL_SetRenderDrawColor(renderer, titleR, titleG, titleB, titleA);
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

                    // Title bar center overlay
                    SDL_RenderFillRect(renderer, &titleCenter);

                    // Title bar sides overlay
                    SDL_RenderFillRect(renderer, &titleLeft);
                    SDL_RenderFillRect(renderer, &titleRight);

                    // Top corners overlay
                    drawFilledCircleQuarter(x + radius, y + radius, radius, 2);
                    drawFilledCircleQuarter(x + w - radius, y + radius, radius, 1);
                }

                // Draw body with bottom-rounded corners
                SDL_SetRenderDrawColor(renderer, bodyR, bodyG, bodyB, bodyA);

                // Body center
                SDL_FRect bodyCenter = {x + radius, y + titleHeight, w - 2 * radius, h - titleHeight};
                SDL_RenderFillRect(renderer, &bodyCenter);

                // Body sides
                SDL_FRect bodyLeft = {x, y + titleHeight, radius, h - titleHeight - radius};
                SDL_RenderFillRect(renderer, &bodyLeft);

                SDL_FRect bodyRight = {x + w - radius, y + titleHeight, radius, h - titleHeight - radius};
                SDL_RenderFillRect(renderer, &bodyRight);

                // Bottom corners
                drawFilledCircleQuarter(x + radius, y + h - radius, radius, 3); // Bottom-left
                drawFilledCircleQuarter(x + w - radius, y + h - radius, radius, 4); // Bottom-right

            } else {
                // No rounding - draw simple rectangles
                // First draw body color as base for title bar
                SDL_SetRenderDrawColor(renderer, bodyR, bodyG, bodyB, bodyA);
                SDL_FRect titleBar = {x, y, w, titleHeight};
                SDL_RenderFillRect(renderer, &titleBar);

                // Then overlay title bar color with alpha blending
                if (titleA > 0) {
                    SDL_SetRenderDrawColor(renderer, titleR, titleG, titleB, titleA);
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                    SDL_RenderFillRect(renderer, &titleBar);
                }

                // Draw body
                SDL_SetRenderDrawColor(renderer, bodyR, bodyG, bodyB, bodyA);
                SDL_FRect body = {x, y + titleHeight, w, h - titleHeight};
                SDL_RenderFillRect(renderer, &body);
            }

            // Border - only when no texture
            SDL_SetRenderDrawColor(renderer, std::min(255, bodyR + 55), std::min(255, bodyG + 50), std::min(255, bodyB + 75), 255);
            drawRoundedRectangleBorder(x, y, w, h, radius);
        }

        // Close button
        if (win->closeable) {
            float closeX = x + w - Window::CLOSE_BUTTON_SIZE - Window::CLOSE_BUTTON_MARGIN * 2;
            float closeY = y + (titleHeight - Window::CLOSE_BUTTON_SIZE) / 2;

            // Check if using custom texture
            SDL_Texture* closeTexture = nullptr;
            if (!win->closeSrc.empty()) {
                closeTexture = textureManager->loadTexture(win->closeSrc);
            }

            if (closeTexture) {
                // Render custom close button texture
                SDL_FRect closeBtn = {closeX, closeY, Window::CLOSE_BUTTON_SIZE, Window::CLOSE_BUTTON_SIZE};
                SDL_RenderTexture(renderer, closeTexture, nullptr, &closeBtn);
            } else {
                // Default close button rendering
                // Button background
                SDL_SetRenderDrawColor(renderer, 200, 60, 60, 255);
                SDL_FRect closeBtn = {closeX, closeY, Window::CLOSE_BUTTON_SIZE, Window::CLOSE_BUTTON_SIZE};
                SDL_RenderFillRect(renderer, &closeBtn);

                // X symbol
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                drawCloseButton(closeX, closeY, Window::CLOSE_BUTTON_SIZE);
            }
        }

        // Title text rendering (if title present and a non-texture window)
        if (!win->title.empty() && !hasTexture) {
            // Choose a default font size that fits title bar height
            int fontSize = static_cast<int>(Window::TITLE_BAR_HEIGHT * 0.6f);
            TTF_Font* font = fontManager->loadFont("C:/Windows/Fonts/arial.ttf", fontSize);
            if (font) {
                SDL_Color textColor = {255,255,255, static_cast<Uint8>(255 * win->opacity)};
                int textW = 0, textH = 0;
                SDL_Texture* titleTex = fontManager->getTextTexture(renderer, font, win->title, textColor, textW, textH);
                if (titleTex) {
                    // Draw title left-aligned with some padding
                    float tx = x + 8.0f;
                    float ty = y + (Window::TITLE_BAR_HEIGHT - textH) / 2.0f;
                    SDL_FRect dst = {tx, ty, static_cast<float>(textW), static_cast<float>(textH)};
                    SDL_SetTextureBlendMode(titleTex, SDL_BLENDMODE_BLEND);
                    SDL_SetTextureAlphaMod(titleTex, static_cast<Uint8>(255 * win->opacity));
                    SDL_RenderTexture(renderer, titleTex, nullptr, &dst);
                }
            }
        }

        // Resize handle indicator (only visible when hovering)
        if (win->resizable && win->isHovered) {
            drawResizeHandle(x, y, w, h);
        }

        // Drag indicator
        if (win->isBeingDragged) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 128);
            drawRoundedRectangleBorder(x + 5, y + 5, w - 10, h - 10,
                                      radius > 5 ? radius - 5 : 0);
        }
            // render content clipped to the window's safe content area using integer clip rects
            // Render children only if they intersect the window's safe content area (coarse clipping)
            SDL_FRect safeArea;
            safeArea.x = win->getSafeContentLeft();
            safeArea.y = win->getSafeContentTop();
            safeArea.w = win->getSafeContentRight() - win->getSafeContentLeft();
            safeArea.h = win->getSafeContentBottom() - win->getSafeContentTop();

            for (UIElement* child : win->content) {
                if (!child) continue;
                SDL_FRect childRect = { child->x, child->y, child->width, child->height };
                bool intersects = !(childRect.x + childRect.w <= safeArea.x ||
                                    childRect.x >= safeArea.x + safeArea.w ||
                                    childRect.y + childRect.h <= safeArea.y ||
                                    childRect.y >= safeArea.y + safeArea.h);
                if (intersects) {
                    renderElement(child);
                }
            }
    }

private:
    // Dispatch rendering for a generic UIElement
    void renderElement(UIElement* element) {
        if (!element) return;
        if (Window* w = dynamic_cast<Window*>(element)) {
            renderWindow(w);
        } else if (Panel* p = dynamic_cast<Panel*>(element)) {
            renderPanel(p);
        } else {
            // Unknown element type: no-op for now
        }
    }
    // Draw close button X
    void drawCloseButton(float x, float y, float size) {
        float margin = 4;
        float x1 = x + margin;
        float y1 = y + margin;
        float x2 = x + size - margin;
        float y2 = y + size - margin;

        // Draw X with thick lines
        for (int offset = -1; offset <= 1; offset++) {
            SDL_RenderLine(renderer, x1 + offset, y1, x2 + offset, y2);
            SDL_RenderLine(renderer, x2 + offset, y1, x1 + offset, y2);
            SDL_RenderLine(renderer, x1, y1 + offset, x2, y2 + offset);
            SDL_RenderLine(renderer, x2, y1 + offset, x1, y2 + offset);
        }
    }

    // Draw resize handle indicator (triangle in bottom-right corner)
    void drawResizeHandle(float winX, float winY, float winW, float winH) {
        float handleSize = Window::RESIZE_HANDLE_SIZE;
        float x1 = winX + winW - handleSize;
        float y1 = winY + winH - handleSize;
        float x2 = winX + winW;
        float y2 = winY + winH;

        // Draw a filled triangle with 50% transparency gray
        SDL_SetRenderDrawColor(renderer, 128, 128, 128, 128);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        // Fill triangle using scanline approach
        for (int dy = 0; dy < handleSize; dy++) {
            float lineY = y1 + dy;
            float lineStartX = x1 + dy; // Diagonal edge
            float lineEndX = x2;
            SDL_RenderLine(renderer, lineStartX, lineY, lineEndX, lineY);
        }

        // Add grip lines for visual indication
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 180);
        for (int i = 0; i < 3; i++) {
            float offset = 4.0f + i * 4.0f;
            SDL_RenderLine(renderer, x2 - offset, y2 - 2, x2 - 2, y2 - offset);
        }
    }
};
