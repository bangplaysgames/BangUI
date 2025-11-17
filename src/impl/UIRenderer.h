#pragma once
#define _USE_MATH_DEFINES
#include <cmath>
#include "../standalone/SoftwareRenderer.h"
#include <algorithm>
#include <string>
#include <vector>
#include <sstream>
#include "../models/Window.h"
#include "../models/Panel.h"
#include "../api/IRenderer.h"
#include "TextureManager.h"
#include "FontManager.h"
#include "NineSliceTexture.h"
#include "ButtonImpl.h"
#include "LabelImpl.h"
#include <iostream>
#include "../api/Theme.h"

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
    // Optional diegetic mapper provided by higher-level renderer
    BangUI::API::IRenderer::DiegeticMapperFn diegeticMapper = nullptr;

    // Draw a properly rounded rectangle using scanline rasterization
    // This fills the rounded rectangle in a single pass by computing left/right x
    // positions for each scanline using the circle equation. It avoids seams and
    // provides pixel-perfect coverage by snapping to the integer pixel grid.
    void drawRoundedRectangle(float x, float y, float w, float h, float radius) {
        if (radius <= 0 || radius > std::min(w, h) / 2.0f) {
            // No rounding or radius too large - draw normal rectangle
            SDL_FRect rect = {x, y, w, h};
            SDL_RenderFillRect(renderer, &rect);
            return;
        }

        // Convert to integer pixel extents for scanline iteration
        int ix = static_cast<int>(floorf(x));
        int iy = static_cast<int>(floorf(y));
        int iw = static_cast<int>(ceilf(x + w)) - ix;
        int ih = static_cast<int>(ceilf(y + h)) - iy;

        // floating radius for math
        float r = radius;
        float top_cy = y + r;
        float bottom_cy = y + h - r;

        for (int dy = 0; dy < ih; ++dy) {
            // center of this pixel row
            float py = static_cast<float>(iy + dy) + 0.5f;

            float left_f = x;
            float right_f = x + w;

            if (py < top_cy) {
                // top rounded region
                float offset = top_cy - py;
                if (offset > r) offset = r;
                float chord = sqrtf(std::max(0.0f, r * r - offset * offset));
                left_f = x + r - chord;
                right_f = x + w - r + chord;
            } else if (py > bottom_cy) {
                // bottom rounded region
                float offset = py - bottom_cy;
                if (offset > r) offset = r;
                float chord = sqrtf(std::max(0.0f, r * r - offset * offset));
                left_f = x + r - chord;
                right_f = x + w - r + chord;
            }

            // Convert float coverage to integer pixel indices using pixel-center test
            int start_p = static_cast<int>(ceilf(left_f - 0.5f));
            int end_p = static_cast<int>(floorf(right_f - 0.5f));

            if (start_p <= end_p) {
                // Use integer 1-pixel high rects for deterministic coverage
                SDL_FRect scanF;
                scanF.x = static_cast<float>(start_p);
                scanF.y = static_cast<float>(iy + dy);
                scanF.w = static_cast<float>(end_p - start_p + 1);
                scanF.h = 1.0f;
                SDL_RenderFillRect(renderer, &scanF);
            }
        }
    }

    // Draw a rectangle where only the top corners are rounded (bottom edge is straight)
    void drawRoundedTopRectangle(float x, float y, float w, float h, float radius) {
        if (radius <= 0 || radius > std::min(w, h) / 2.0f) {
            // No rounding or radius too large - draw normal rectangle
            SDL_FRect rect = {x, y, w, h};
            SDL_RenderFillRect(renderer, &rect);
            return;
        }

        int ix = static_cast<int>(floorf(x));
        int iy = static_cast<int>(floorf(y));
        int iw = static_cast<int>(ceilf(x + w)) - ix;
        int ih = static_cast<int>(ceilf(y + h)) - iy;

        float r = radius;
        float top_cy = y + r;

        for (int dy = 0; dy < ih; ++dy) {
            float py = static_cast<float>(iy + dy) + 0.5f;

            float left_f = x;
            float right_f = x + w;

            if (py < top_cy) {
                float offset = top_cy - py;
                if (offset > r) offset = r;
                float chord = sqrtf(std::max(0.0f, r * r - offset * offset));
                left_f = x + r - chord;
                right_f = x + w - r + chord;
            }

            int start_p = static_cast<int>(ceilf(left_f - 0.5f));
            int end_p = static_cast<int>(floorf(right_f - 0.5f));

            if (start_p <= end_p) {
                SDL_FRect scanF;
                scanF.x = static_cast<float>(start_p);
                scanF.y = static_cast<float>(iy + dy);
                scanF.w = static_cast<float>(end_p - start_p + 1);
                scanF.h = 1.0f;
                SDL_RenderFillRect(renderer, &scanF);
            }
        }
    }

    // Diagnostic helper: read pixels from renderer and write a BMP for inspection
    // Diagnostic helper: render the specified rounded-rect body into an
    // RGBA render-target (cleared to transparent), read back pixels and
    // write a BMP. This preserves the drawn alpha channel so we can
    // inspect whether corner pixels are transparent or opaque.
    void dumpWindowBodyToAlphaBMP(float wx, float wy, float ww, float wh, float wradius,
                                  Uint8 bodyR, Uint8 bodyG, Uint8 bodyB, Uint8 bodyA,
                                  const char* filename) {
        int rw = static_cast<int>(std::min(ww, 4096.0f));
        int rh = static_cast<int>(std::min(wh, 4096.0f));
        if (rw <= 0 || rh <= 0) return;

        // Create an RGBA render target to draw only the window body into
        SDL_Texture* target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                                SDL_TEXTUREACCESS_TARGET, rw, rh);
        if (!target) return;
        SDL_SetTextureBlendMode(target, SDL_BLENDMODE_BLEND);

        // Save previous target so we can restore it
        SDL_Texture* prevTarget = SDL_GetRenderTarget(renderer);

        // Bind our target, clear to transparent, draw the rounded rect at 0,0
        SDL_SetRenderTarget(renderer, target);
        // Clear transparent
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);

        // Draw the window body into the target using the same rasterizer,
        // but translated to target local coordinates (0,0).
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, bodyR, bodyG, bodyB, bodyA);
        drawRoundedRectangle(0.0f, 0.0f, static_cast<float>(rw), static_cast<float>(rh), wradius);

        // Read pixels from the target (SDL_RenderReadPixels reads current target)
        SDL_Surface* surf = SDL_RenderReadPixels(renderer, nullptr);

        // Restore previous target
        SDL_SetRenderTarget(renderer, prevTarget);

        if (!surf) {
            SDL_DestroyTexture(target);
            return;
        }

        SDL_SaveBMP(surf, filename);
        SDL_DestroySurface(surf);
        SDL_DestroyTexture(target);
    }

    // Dump the current framebuffer (what's been rendered so far) to a BMP.
    void dumpFramebufferToBMP(const char* filename) {
        SDL_Surface* surf = SDL_RenderReadPixels(renderer, nullptr);
        if (!surf) return;
        SDL_SaveBMP(surf, filename);
        SDL_DestroySurface(surf);
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

    void drawLineWithThickness(float x1, float y1, float x2, float y2, const BangUI::API::Color& color, float thickness) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        if (thickness <= 1.01f) {
            SDL_RenderLine(renderer, x1, y1, x2, y2);
            return;
        }
        float dx = x2 - x1;
        float dy = y2 - y1;
        float len = sqrtf(dx * dx + dy * dy);
        if (len <= 0.0001f) return;
        float nx = -dy / len;
        float ny = dx / len;
        int steps = std::max(1, static_cast<int>(std::round(thickness)));
        float half = (steps - 1) / 2.0f;
        for (int i = 0; i < steps; ++i) {
            float offset = i - half;
            float ox = nx * offset;
            float oy = ny * offset;
            SDL_RenderLine(renderer, x1 + ox, y1 + oy, x2 + ox, y2 + oy);
        }
    }

    void drawCloseIcon(float x, float y, float size, const BangUI::API::Color& color) {
        float margin = size * 0.25f;
        drawLineWithThickness(x + margin, y + margin, x + size - margin, y + size - margin, color, 2.0f);
        drawLineWithThickness(x + size - margin, y + margin, x + margin, y + size - margin, color, 2.0f);
    }

    void drawMinimizeIcon(float x, float y, float size, const BangUI::API::Color& color) {
        float margin = size * 0.3f;
        float yLine = y + size - margin;
        drawLineWithThickness(x + margin, yLine, x + size - margin, yLine, color, 2.0f);
    }

    void drawMaximizeIcon(float x, float y, float size, const BangUI::API::Color& color) {
        float margin = size * 0.25f;
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_FRect rect{ x + margin, y + margin, size - margin * 2.0f, size - margin * 2.0f };
        SDL_RenderRect(renderer, &rect);
    }

    void drawBurgerIcon(float x, float y, float size, const BangUI::API::Color& primary, const BangUI::API::Color& accent) {
        float margin = size * 0.25f;
        float spacing = (size - margin * 2.0f) / 3.0f;
        drawLineWithThickness(x + margin, y + margin, x + size - margin, y + margin, primary, 2.0f);
        drawLineWithThickness(x + margin, y + margin + spacing, x + size - margin, y + margin + spacing, primary, 2.0f);
        drawLineWithThickness(x + margin, y + margin + 2.0f * spacing, x + size - margin, y + margin + 2.0f * spacing, accent, 2.0f);
    }

    void drawWindowChrome(const Window* win) {
        if (!win) return;
        std::vector<Window::ChromeButtonLayout> layouts;
        win->computeChromeLayout(layouts);
        for (const auto& layout : layouts) {
            const Window::ChromeButton* btn = layout.button;
            if (!btn) continue;
            BangUI::API::Color bg = btn->backgroundColor;
            bg.a = static_cast<Uint8>(static_cast<float>(bg.a) * win->opacity);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
            drawRoundedRectangle(layout.x, layout.y, layout.width, layout.height, btn->cornerRadius);

            BangUI::API::Color iconColor = btn->iconColor;
            iconColor.a = static_cast<Uint8>(static_cast<float>(iconColor.a) * win->opacity);

            switch (btn->type) {
            case Window::ChromeButton::Type::Close:
                drawCloseIcon(layout.x, layout.y, layout.width, iconColor);
                break;
            case Window::ChromeButton::Type::Minimize:
                drawMinimizeIcon(layout.x, layout.y, layout.width, iconColor);
                break;
            case Window::ChromeButton::Type::Maximize:
                drawMaximizeIcon(layout.x, layout.y, layout.width, iconColor);
                break;
            case Window::ChromeButton::Type::Menu:
                drawBurgerIcon(layout.x, layout.y, layout.width, iconColor, iconColor);
                break;
            case Window::ChromeButton::Type::Custom:
                break;
            }

            if (btn->type == Window::ChromeButton::Type::Custom && btn->customDraw) {
                btn->customDraw(renderer, *btn, layout.x, layout.y, layout.width, layout.height);
            }
        }
    }

    // Render a textured rectangle with optional color overlay blending
    // The color's alpha channel determines how much the color blends over the texture
    void renderTexturedRectangle(SDL_Texture* texture, float x, float y, float w, float h,
                                 Uint8 r, Uint8 g, Uint8 b, Uint8 textureA, Uint8 overlayA,
                                 float rotation = 0.0f) {
        SDL_FRect destRect = {x, y, w, h};

        // Apply alpha modulation to the texture so overall opacity can be controlled
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod(texture, textureA);

        // Render the texture, use rotation if requested
        // Rotation is currently not applied here due to SDL compatibility wrappers.
        // If rotation is requested, we fall back to unrotated rendering for now.
        (void)rotation;
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
                                       float radius, Uint8 r, Uint8 g, Uint8 b, Uint8 textureA, Uint8 overlayA,
                                       float rotation = 0.0f) {
        // For rounded rectangles with textures, we need to use a clip mask approach
        // This is a simplified version - render texture first, then overlay color with rounding

        // Create a temporary texture for masking (more complex implementation)
        // For now, we'll render the full texture and then draw rounded color overlay
        SDL_FRect destRect = {x, y, w, h};

        // Apply alpha modulation to the texture so overall opacity can be controlled
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod(texture, textureA);

        // For rounded rectangles, if rotation is requested we render into a temp texture and rotate when blitting
        // Rotation not applied due to SDL compatibility. Render unrotated.
        SDL_RenderTexture(renderer, texture, nullptr, &destRect);
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

    void setDiegeticMapper(BangUI::API::IRenderer::DiegeticMapperFn fn) {
        diegeticMapper = fn;
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

    void renderBackground(const BangUI::API::BackgroundTheme& theme, float width, float height) {
        if (!theme.enabled) return;
        float w = std::max(0.0f, width);
        float h = std::max(0.0f, height);

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, theme.clearColor.r, theme.clearColor.g, theme.clearColor.b, theme.clearColor.a);
        SDL_RenderClear(renderer);

        if (!theme.texture.empty()) {
            float opacity = std::clamp(theme.textureOpacity, 0.0f, 1.0f);
            Uint8 textureAlpha = static_cast<Uint8>(opacity * 255.0f);
            if (textureAlpha > 0) {
                if (theme.textureNinePatch || NineSliceTexture::isNinePatchFile(theme.texture)) {
                    NineSliceTexture* nine = textureManager->loadNineSliceTexture(theme.texture);
                    if (nine) {
                        nine->render(renderer, 0.0f, 0.0f, w, h, textureAlpha);
                    }
                } else {
                    SDL_Texture* tex = textureManager->loadTexture(theme.texture);
                    if (tex) {
                        renderTexturedRectangle(tex, 0.0f, 0.0f, w, h, 255, 255, 255, textureAlpha, 0);
                    }
                }
            }
        }

        for (const auto& shape : theme.shapes) {
            auto resolveX = [&](float value) {
                return shape.normalized ? value * w : value;
            };
            auto resolveY = [&](float value) {
                return shape.normalized ? value * h : value;
            };
            auto resolveScalar = [&](float value) {
                return shape.normalized ? value * std::max(w, h) : value;
            };

            switch (shape.type) {
            case BangUI::API::VectorShapeType::Rectangle: {
                SDL_FRect rect{
                    resolveX(shape.x),
                    resolveY(shape.y),
                    shape.normalized ? shape.width * w : shape.width,
                    shape.normalized ? shape.height * h : shape.height
                };
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, shape.color.r, shape.color.g, shape.color.b, shape.color.a);
                SDL_RenderFillRect(renderer, &rect);
                break;
            }
            case BangUI::API::VectorShapeType::RoundedRectangle: {
                float rx = resolveX(shape.x);
                float ry = resolveY(shape.y);
                float rw = shape.normalized ? shape.width * w : shape.width;
                float rh = shape.normalized ? shape.height * h : shape.height;
                float radius = shape.normalized ? shape.radius * std::min(w, h) : shape.radius;
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, shape.color.r, shape.color.g, shape.color.b, shape.color.a);
                drawRoundedRectangle(rx, ry, rw, rh, radius);
                break;
            }
            case BangUI::API::VectorShapeType::Line: {
                float x1 = resolveX(shape.x);
                float y1 = resolveY(shape.y);
                float x2 = resolveX(shape.x2);
                float y2 = resolveY(shape.y2);
                float thickness = resolveScalar(shape.thickness);
                drawLineWithThickness(x1, y1, x2, y2, shape.color, std::max(1.0f, thickness));
                break;
            }
            }
        }
    }

    // Expose measurement to callers via the public API so input code can
    // compute scroll extents using the same logic as the renderer.
    void measureElementRenderedSize(UIElement* element, float maxW, float maxH, float& outW, float& outH) {
        this->measureElementRenderedSizeImpl(element, maxW, maxH, outW, outH);
    }

    // Render a panel with its properties
    void renderPanel(const Panel* panel) {
        if (!panel->visible) return;

        // If this panel is diegetic, attempt to resolve a screen transform
        float origX = panel->x, origY = panel->y, origW = panel->width, origH = panel->height;
        float mappedX = origX, mappedY = origY, mappedScale = 1.0f, mappedRot = 0.0f;
        bool useMapped = false;
        if (panel->renderSpace == "diegetic" && diegeticMapper && panel->anchorMesh) {
            // anchorMesh.resource holds the pointer we pass to the mapper
            useMapped = diegeticMapper(panel->anchorMesh->resource, panel->anchorBone.c_str(), &mappedX, &mappedY, &mappedScale, &mappedRot);
        }

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
        int texAlphaInt = static_cast<int>((overlayAlpha == 0 ? 255 : overlayAlpha) * panel->opacity + 0.5f);
        if (texAlphaInt < 0) texAlphaInt = 0; if (texAlphaInt > 255) texAlphaInt = 255;
        Uint8 texA = static_cast<Uint8>(texAlphaInt);
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

        // Render panel directly with SDL alpha blending
        if (nineSlice) {
            if (useMapped) {
                nineSlice->render(renderer, mappedX, mappedY, panel->width * mappedScale, panel->height * mappedScale, static_cast<Uint8>(panel->backgroundColor.a * panel->opacity));
            } else {
                nineSlice->render(renderer, panel->x, panel->y, panel->width, panel->height, static_cast<Uint8>(panel->backgroundColor.a * panel->opacity));
            }
        } else if (texture) {
            if (panel->cornerRadius > 0) {
                if (useMapped) renderTexturedRoundedRectangle(texture, mappedX, mappedY, panel->width * mappedScale, panel->height * mappedScale, panel->cornerRadius * mappedScale, r, g, b, texA, overlayA, mappedRot);
                else renderTexturedRoundedRectangle(texture, panel->x, panel->y, panel->width, panel->height, panel->cornerRadius, r, g, b, texA, overlayA, 0.0f);
            } else {
                if (useMapped) renderTexturedRectangle(texture, mappedX, mappedY, panel->width * mappedScale, panel->height * mappedScale, r, g, b, texA, overlayA, mappedRot);
                else renderTexturedRectangle(texture, panel->x, panel->y, panel->width, panel->height, r, g, b, texA, overlayA, 0.0f);
            }
        } else {
            if (useMapped) {
                SDL_SetRenderDrawColor(renderer, r, g, b, overlayA);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                drawRoundedRectangle(mappedX, mappedY, panel->width * mappedScale, panel->height * mappedScale, panel->cornerRadius * mappedScale);
            } else {
                SDL_SetRenderDrawColor(renderer, r, g, b, overlayA);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                drawRoundedRectangle(panel->x, panel->y, panel->width, panel->height, panel->cornerRadius);
            }
        }

    // Note: border drawing is deferred until after children are rendered so
        // Renderer mustn't change panel scroll offsets. UIManager is
        // authoritative for input/state changes. Use local clamping
        // when computing thumb positions.
        // Drag indicator
        if (panel->isBeingDragged) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 128);
            drawRoundedRectangleBorder(panel->x + 5, panel->y + 5, panel->width - 10, panel->height - 10,
                                      panel->cornerRadius > 5 ? panel->cornerRadius - 5 : 0);
        }

                // render children clipped to the panel's safe content area using integer clip rects
                // Render children only if they intersect the panel's safe content area (coarse clipping)
                SDL_FRect safeAreaF;
                safeAreaF.x = panel->getSafeContentLeft();
                safeAreaF.y = panel->getSafeContentTop();
                safeAreaF.w = panel->getSafeContentRight() - panel->getSafeContentLeft();
                safeAreaF.h = panel->getSafeContentBottom() - panel->getSafeContentTop();

                // Convert to integer SDL_Rect for clipping (floor/ceil to be safe)
                SDL_Rect clipRect;
                clipRect.x = static_cast<int>(floorf(safeAreaF.x));
                clipRect.y = static_cast<int>(floorf(safeAreaF.y));
                clipRect.w = static_cast<int>(ceilf(safeAreaF.w));
                clipRect.h = static_cast<int>(ceilf(safeAreaF.h));

                // Expand the clip rect slightly to avoid cutting off rounded corner pixels
                // from child elements (prevents seams caused by tight integer clipping).
                const int clipPad = 2; // pixels
                clipRect.x = std::max(0, clipRect.x - clipPad);
                clipRect.y = std::max(0, clipRect.y - clipPad);
                clipRect.w = clipRect.w + 2 * clipPad;
                clipRect.h = clipRect.h + 2 * clipPad;

                bool didClip = false;
                if (clipRect.w > 0 && clipRect.h > 0) {
                    if (panel->id == "panelB_nowrap" || panel->id == "winD_scroll_both") {
                        std::cerr << "[DIAG][panel] clipRect=(x=" << clipRect.x << ",y=" << clipRect.y << ",w=" << clipRect.w << ",h=" << clipRect.h << ")" << std::endl;
                    }
                    SDL_SetRenderClipRect(renderer, &clipRect);
                    didClip = true;
                }

                for (UIElement* child : panel->content) {
                    if (!child) continue;
                    // Account for panel scroll offsets when computing child positions for clipping/rendering
                    float measuredW = child->width, measuredH = child->height;
                    measureElementRenderedSize(child, safeAreaF.w, safeAreaF.h, measuredW, measuredH);
                    SDL_FRect childRect = { child->x - panel->scrollX, child->y - panel->scrollY, measuredW, measuredH };
                    // Allow a 1-pixel tolerance to avoid accidental exclusion due to float->int rounding
                    const float EPS = 2.0f;
                    bool intersects = !((childRect.x + childRect.w) <= (safeAreaF.x + EPS) ||
                                        (childRect.x) >= (safeAreaF.x + safeAreaF.w - EPS) ||
                                        (childRect.y + childRect.h) <= (safeAreaF.y + EPS) ||
                                        (childRect.y) >= (safeAreaF.y + safeAreaF.h - EPS));
                    if (intersects) {
                        // Temporarily translate renderer by negative scroll to draw children in scrolled coordinates
                        float prevX = child->x;
                        float prevY = child->y;
                        float transX = prevX - panel->scrollX;
                        float transY = prevY - panel->scrollY;
                        child->x = transX;
                        child->y = transY;
                        renderElement(child);
                        child->x = prevX;
                        child->y = prevY;
                    }
                }

                if (didClip) SDL_SetRenderClipRect(renderer, nullptr);

                // Draw border outside the panel bounds so it doesn't reduce safe content.
                if (panel->borderWidth > 0.0f) {
                    uint8_t brc = panel->borderColor.r;
                    uint8_t bgc = panel->borderColor.g;
                    uint8_t bbc = panel->borderColor.b;
                    uint8_t bac = static_cast<Uint8>(panel->borderColor.a * panel->opacity);
                    SDL_SetRenderDrawColor(renderer, brc, bgc, bbc, bac);
                    float bw = panel->borderWidth;
                    drawRoundedRectangleBorder(panel->x - bw, panel->y - bw, panel->width + 2*bw, panel->height + 2*bw, panel->cornerRadius + bw);
                }
                // Draw scrollbars for panel (overlay)
                if (!panel->scrollable.empty()) {
                    // Use per-axis fade timers so each scrollbar only appears when its axis is active
                    float tY = std::max(0.0f, std::min(panel->scrollBarFadeTimerY / panel->scrollBarFadeDuration, 1.0f));
                    float tX = std::max(0.0f, std::min(panel->scrollBarFadeTimerX / panel->scrollBarFadeDuration, 1.0f));
                    Uint8 alphaY = static_cast<Uint8>(255 * tY);
                    Uint8 alphaX = static_cast<Uint8>(255 * tX);
                    float safeLeft = panel->getSafeContentLeft();
                    float safeTop = panel->getSafeContentTop();
                    float safeW = panel->getSafeContentRight() - safeLeft;
                    float safeH = panel->getSafeContentBottom() - safeTop;
                    // compute content extents (measure rendered sizes for elements like Label)
                    float contentW = 0.0f, contentH = 0.0f;
                    for (UIElement* c : panel->content) {
                        if (!c) continue;
                        float measuredW = c->width;
                        float measuredH = c->height;
                        measureElementRenderedSize(c, safeW, safeH, measuredW, measuredH);
                        // Convert from absolute coordinates to content-area-relative coordinates
                        // Clamp to 0 to handle stale child positions when window/panel moves
                        contentW = std::max(contentW, std::max(0.0f, c->x - safeLeft) + measuredW);
                        contentH = std::max(contentH, std::max(0.0f, c->y - safeTop) + measuredH);
                    }
                    // Clamp panel scroll offsets to measured content extents to avoid overscroll
                    // Do NOT modify stored panel scroll values from the renderer.
                    // Previously this code wrote back into panel->scrollX/Y which
                    // changed runtime scrolling behavior. Keep the renderer
                    // read-only and compute clamped values locally where needed
                    // for visual mapping only.
                    // Vertical
                    if ((panel->scrollable == "vertical" || panel->scrollable == "both") && contentH > safeH + 1.0f) {
                        float sbw = 8.0f;
                        float sbx = safeLeft + safeW - sbw - 4.0f;
                        float sby = safeTop + 4.0f;
                        float sbh = safeH - 8.0f;
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, static_cast<Uint8>(alphaY * 0.35f));
                        SDL_FRect bg = {sbx, sby, sbw, sbh};
                        SDL_RenderFillRect(renderer, &bg);
                        float thumbH = std::max(16.0f, sbh * (safeH / contentH));
                        float maxScroll = std::max(0.0f, contentH - safeH);
                        float rel = (maxScroll > 0.0f) ? (panel->scrollY / maxScroll) : 0.0f;
                        float thumbY = sby + rel * (sbh - thumbH);
                        SDL_SetRenderDrawColor(renderer, 200, 200, 200, alphaY);
                        SDL_FRect thumb = {sbx + 1.0f, thumbY, sbw - 2.0f, thumbH};
                        SDL_RenderFillRect(renderer, &thumb);
                    }
                    // Horizontal
                    if ((panel->scrollable == "horizontal" || panel->scrollable == "both") && contentW > safeW + 1.0f) {
                        float sbh = 8.0f;
                        float sbx = safeLeft + 4.0f;
                        float sby = safeTop + safeH - sbh - 4.0f;
                        float sbw = safeW - 8.0f;
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, static_cast<Uint8>(alphaX * 0.35f));
                        SDL_FRect bg = {sbx, sby, sbw, sbh};
                        SDL_RenderFillRect(renderer, &bg);
                        float thumbW = std::max(16.0f, sbw * (safeW / contentW));
                        float maxScroll = std::max(0.0f, contentW - safeW);
                        float rel = (maxScroll > 0.0f) ? (panel->scrollX / maxScroll) : 0.0f;
                        float thumbX = sbx + rel * (sbw - thumbW);
                        SDL_SetRenderDrawColor(renderer, 200, 200, 200, alphaX);
                        SDL_FRect thumb = {thumbX, sby + 1.0f, thumbW, sbh - 2.0f};
                        SDL_RenderFillRect(renderer, &thumb);
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

        // Diegetic mapping for window
        float mappedX = x, mappedY = y, mappedScale = 1.0f, mappedRot = 0.0f;
        bool useMapped = false;
        if (win->renderSpace == "diegetic" && diegeticMapper && win->anchorMesh) {
            useMapped = diegeticMapper(win->anchorMesh->resource, win->anchorBone.c_str(), &mappedX, &mappedY, &mappedScale, &mappedRot);
        }

        auto resolveTexturePath = [&](const std::string& overridePath, const std::string& legacyPath) -> std::string {
            if (!overridePath.empty()) return overridePath;
            return legacyPath;
        };

        std::string bodyTexturePath = resolveTexturePath(win->bodyTexture, win->src);
        NineSliceTexture* bodyNineSlice = nullptr;
        SDL_Texture* bodyTexture = nullptr;
        bool hasBodyTexture = false;

        if (!bodyTexturePath.empty()) {
            if (NineSliceTexture::isNinePatchFile(bodyTexturePath)) {
                bodyNineSlice = textureManager->loadNineSliceTexture(bodyTexturePath);
                hasBodyTexture = (bodyNineSlice != nullptr);
            } else {
                bodyTexture = textureManager->loadTexture(bodyTexturePath);
                hasBodyTexture = (bodyTexture != nullptr);
            }
        }

        NineSliceTexture* titleNineSlice = nullptr;
        SDL_Texture* titleTexture = nullptr;
        bool hasTitleTexture = false;
        if (win->titleBarVisible && !win->titleBarTexture.empty()) {
            if (NineSliceTexture::isNinePatchFile(win->titleBarTexture)) {
                titleNineSlice = textureManager->loadNineSliceTexture(win->titleBarTexture);
                hasTitleTexture = (titleNineSlice != nullptr);
            } else {
                titleTexture = textureManager->loadTexture(win->titleBarTexture);
                hasTitleTexture = (titleTexture != nullptr);
            }
        }

        const bool useBodyTexture = hasBodyTexture;
        const bool useTitleTexture = hasTitleTexture && win->titleBarVisible;

        Uint8 titleR = win->titleBarColor.r;
        Uint8 titleG = win->titleBarColor.g;
        Uint8 titleB = win->titleBarColor.b;
        Uint8 titleA = (win->titleBarVisible && !useTitleTexture)
            ? static_cast<Uint8>(win->titleBarColor.a * win->opacity)
            : 0;
        Uint8 bodyA = static_cast<Uint8>(win->backgroundColor.a * win->opacity);
        Uint8 bodyR = win->backgroundColor.r;
        Uint8 bodyG = win->backgroundColor.g;
        Uint8 bodyB = win->backgroundColor.b;

        const float TEXTURE_OVERLAP = 6.0f;
        float bodyOffset = (useBodyTexture && (useTitleTexture || titleA > 0))
            ? std::max(0.0f, Window::TITLE_BAR_HEIGHT - TEXTURE_OVERLAP)
            : 0.0f;
        float bodyDrawY = y + bodyOffset;
        float bodyDrawH = h - bodyOffset;

        if (useBodyTexture) {
            if (bodyNineSlice) {
                if (useMapped) {
                    bodyNineSlice->render(renderer, mappedX, mappedY + bodyOffset, w * mappedScale, (h - bodyOffset) * mappedScale, static_cast<Uint8>(255 * win->opacity));
                } else {
                    bodyNineSlice->render(renderer, x, bodyDrawY, w, bodyDrawH, static_cast<Uint8>(255 * win->opacity));
                }
                bodyNineSlice->getSafeContentInsets(
                    const_cast<Window*>(win)->contentLeft,
                    const_cast<Window*>(win)->contentTop,
                    const_cast<Window*>(win)->contentRight,
                    const_cast<Window*>(win)->contentBottom
                );
            } else if (bodyTexture) {
                if (useMapped) {
                    renderTexturedRectangle(bodyTexture, mappedX, mappedY + bodyOffset, w * mappedScale, (h - bodyOffset) * mappedScale,
                                            255, 255, 255, static_cast<Uint8>(255 * win->opacity), 0, mappedRot);
                } else {
                    renderTexturedRectangle(bodyTexture, x, bodyDrawY, w, bodyDrawH,
                                            255, 255, 255, static_cast<Uint8>(255 * win->opacity), 0, 0.0f);
                }
                const_cast<Window*>(win)->contentLeft = 0.0f;
                const_cast<Window*>(win)->contentTop = 0.0f;
                const_cast<Window*>(win)->contentRight = 0.0f;
                const_cast<Window*>(win)->contentBottom = 0.0f;
            }
        } else {
            if (radius > 0) {
                SDL_SetRenderDrawColor(renderer, bodyR, bodyG, bodyB, bodyA);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                drawRoundedRectangle(x, bodyDrawY, w, h - bodyOffset, radius);
                if (titleA > 0) {
                    SDL_SetRenderDrawColor(renderer, titleR, titleG, titleB, titleA);
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                    drawRoundedTopRectangle(x, y, w, titleHeight, radius);
                }
            } else {
                SDL_SetRenderDrawColor(renderer, bodyR, bodyG, bodyB, bodyA);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_FRect body = {x, bodyDrawY, w, h - bodyOffset};
                SDL_RenderFillRect(renderer, &body);
                if (titleA > 0) {
                    SDL_SetRenderDrawColor(renderer, titleR, titleG, titleB, titleA);
                    SDL_FRect titleBar = {x, y, w, titleHeight};
                    SDL_RenderFillRect(renderer, &titleBar);
                }
            }
        }

        // --- Debug dump: capture corner pixels for problematic windows (one-shot) ---
        // This is a diagnostic aid; it will write BMPs into the project root when
        // the Docking Demo or any window whose title contains "Docked Bottom" is drawn.
        // It runs only once per name to avoid spamming disk.
        static bool dumpedDocking = false;
        static bool dumpedChild = false;
        if (!dumpedDocking && win->title.find("Docking Demo") != std::string::npos) {
            dumpWindowBodyToAlphaBMP(x, y, std::min(w, 200.0f), std::min(h, 80.0f), radius, bodyR, bodyG, bodyB, bodyA, "corner_dump_docking.bmp");
            dumpedDocking = true;
            // Also save the full framebuffer so we can see overlay compositing
            dumpFramebufferToBMP("corner_dump_docking_full.bmp");
        }
        if (!dumpedChild && win->title.find("Docked Bottom") != std::string::npos) {
            dumpWindowBodyToAlphaBMP(x, y, std::min(w, 200.0f), std::min(h, 120.0f), radius, bodyR, bodyG, bodyB, bodyA, "corner_dump_child.bmp");
            dumpedChild = true;
        }

        // NOTE: close button and title text are drawn after children so they
        // always appear above content. We'll render the window body (title bar
        // background + window body) into a temporary RGBA texture to preserve
        // corner alpha and seams, then composite it back. Close button and
        // title text will be rendered below in the overlay section.

            // Before rendering children, compute measured content extents and clamp scroll offsets
            // This prevents overscroll where children are incorrectly culled because scroll was larger than max
            {
                float safeLeftTmp = win->getSafeContentLeft();
                float safeTopTmp = win->getSafeContentTop();
                float safeWTmp = win->getSafeContentRight() - safeLeftTmp;
                float safeHTmp = win->getSafeContentBottom() - safeTopTmp;
                float contentW = 0.0f, contentH = 0.0f;
                for (UIElement* c : win->content) {
                    if (!c) continue;
                    float measuredW = c->width, measuredH = c->height;
                    measureElementRenderedSize(c, safeWTmp, safeHTmp, measuredW, measuredH);
                    // Convert from absolute coordinates to content-area-relative coordinates
                    // Clamp to 0 to handle stale child positions when window moves
                    contentW = std::max(contentW, std::max(0.0f, c->x - safeLeftTmp) + measuredW);
                    contentH = std::max(contentH, std::max(0.0f, c->y - safeTopTmp) + measuredH);
                }
                if (win->id == "winC_scroll_v") {
                    std::cerr << "[DIAG][winC] measured contentH=" << contentH << " safeH=" << safeHTmp << " scrollY=" << win->scrollY << std::endl;
                    for (UIElement* c : win->content) {
                        if (!c) continue;
                        float mW=c->width, mH=c->height; measureElementRenderedSize(c, safeWTmp, safeHTmp, mW, mH);
                        float crx = c->x - win->scrollX; float cry = c->y - win->scrollY;
                        bool intr = !( (crx + mW) <= (safeLeftTmp + 0.5f) || crx >= (safeLeftTmp + safeWTmp - 0.5f) || (cry + mH) <= (safeTopTmp + 0.5f) || cry >= (safeTopTmp + safeHTmp - 0.5f));
                        std::cerr << "[DIAG][winC] child y="<<c->y<<" mH="<<mH<<" childRectY="<<cry<<" intersects="<<intr<<std::endl;
                    }
                }
                // Renderer must not alter the window's stored scroll offsets.
                // Compute clamped values locally when rendering, leaving the
                // authoritative scroll state (managed by UIManager) untouched.
            }

            // render content clipped to the window's safe content area using integer clip rects
            // Render children only if they intersect the window's safe content area (coarse clipping)
            SDL_FRect safeAreaF;
            safeAreaF.x = win->getSafeContentLeft();
            safeAreaF.y = win->getSafeContentTop();
            safeAreaF.w = win->getSafeContentRight() - win->getSafeContentLeft();
            safeAreaF.h = win->getSafeContentBottom() - win->getSafeContentTop();

            // Convert to integer SDL_Rect for clipping (floor/ceil to be safe)
            SDL_Rect clipRect;
            clipRect.x = static_cast<int>(floorf(safeAreaF.x));
            clipRect.y = static_cast<int>(floorf(safeAreaF.y));
            clipRect.w = static_cast<int>(ceilf(safeAreaF.w));
            clipRect.h = static_cast<int>(ceilf(safeAreaF.h));

            bool didClip = false;
            if (clipRect.w > 0 && clipRect.h > 0) {
                SDL_SetRenderClipRect(renderer, &clipRect);
                didClip = true;
            }

            for (UIElement* child : win->content) {
                if (!child) continue;
                float measuredW = child->width, measuredH = child->height;
                measureElementRenderedSize(child, safeAreaF.w, safeAreaF.h, measuredW, measuredH);
                SDL_FRect childRect = { child->x - win->scrollX, child->y - win->scrollY, measuredW, measuredH };
                // Allow a 1-pixel tolerance to avoid accidental exclusion due to float->int rounding
                const float EPS = 2.0f;
                bool intersects = !((childRect.x + childRect.w) <= (safeAreaF.x + EPS) ||
                                    (childRect.x) >= (safeAreaF.x + safeAreaF.w - EPS) ||
                                    (childRect.y + childRect.h) <= (safeAreaF.y + EPS) ||
                                    (childRect.y) >= (safeAreaF.y + safeAreaF.h - EPS));
                if (win->id == "panelB_nowrap" || win->id == "winD_scroll_both") {
                    std::cerr << "[DIAG][window] id=" << win->id
                              << " scrollX=" << win->scrollX << " scrollY=" << win->scrollY
                              << " safeArea=(" << safeAreaF.x << "," << safeAreaF.y << "," << safeAreaF.w << "," << safeAreaF.h << ")"
                              << " child=(x=" << child->x << ",y=" << child->y << ",w=" << measuredW << ",h=" << measuredH << ")"
                              << " childRect=(x=" << childRect.x << ",y=" << childRect.y << ",w=" << childRect.w << ",h=" << childRect.h << ")"
                              << " intersects=" << intersects << std::endl;
                }
                if (intersects) {
                    float prevX = child->x;
                    float prevY = child->y;
                    child->x = prevX - win->scrollX;
                    child->y = prevY - win->scrollY;
                    renderElement(child);
                    child->x = prevX;
                    child->y = prevY;
                }
            }

            if (didClip) SDL_SetRenderClipRect(renderer, nullptr);

            // Draw simple scrollbars for window if enabled (per-axis fade timers)
            if (!win->scrollable.empty()) {
                float tY = std::max(0.0f, std::min(win->scrollBarFadeTimerY / win->scrollBarFadeDuration, 1.0f));
                float tX = std::max(0.0f, std::min(win->scrollBarFadeTimerX / win->scrollBarFadeDuration, 1.0f));
                Uint8 alphaY = static_cast<Uint8>(255 * tY);
                Uint8 alphaX = static_cast<Uint8>(255 * tX);
                // Vertical scrollbar
                if (win->scrollable == "vertical" || win->scrollable == "both") {
                    float safeLeft = win->getSafeContentLeft();
                    float safeTop = win->getSafeContentTop();
                    float safeW = win->getSafeContentRight() - safeLeft;
                    float safeH = win->getSafeContentBottom() - safeTop;
                    // scrollbar width
                    float sbw = 8.0f;
                    float sbx = safeLeft + safeW - sbw - 4.0f;
                    float sby = safeTop + 4.0f;
                    float sbh = safeH - 8.0f;
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, static_cast<Uint8>(alphaY * 0.35f));
                    SDL_FRect bg = {sbx, sby, sbw, sbh};
                    SDL_RenderFillRect(renderer, &bg);
                    // thumb size proportional to content vs viewport height
                    float contentH = 0.0f;
                    for (UIElement* c : win->content) {
                        float measuredW = c->width, measuredH = c->height;
                        measureElementRenderedSize(c, safeW, safeH, measuredW, measuredH);
                        // Convert from absolute coordinates to content-area-relative coordinates
                        contentH = std::max(contentH, (c->y - safeTop) + measuredH);
                    }
                    float viewportH = safeH;
                    // Thumb size should be computed relative to the track height (sbh)
                    // so that the mapping scroll->thumbPos maps scroll=max to track end.
                    float thumbH = std::max(16.0f, (contentH > 0.0f) ? sbh * (viewportH / contentH) : sbh);
                    float maxScroll = std::max(0.0f, contentH - viewportH);
                    // Clamp scrollY to measured content extents
                    // Do not mutate win->scrollY here. Renderer will use a
                    // local clamped value for mapping to thumb position.
                    // Use a clamped scroll-to-rel calculation and ensure that when
                    // the scroll is at the measured maximum the thumb is flush with
                    // the track end. This only affects the visual mapping and does
                    // not change the stored scroll value on the window.
                    float clampedScroll = std::max(0.0f, std::min(win->scrollY, maxScroll));
                    float rel = (maxScroll > 0.0f) ? (clampedScroll / maxScroll) : 0.0f;
                    // If we're very near the measured max scroll (within 1px), treat
                    // it as the end so small measurement differences don't leave the
                    // thumb visually short of the track end.
                    if (maxScroll > 0.0f && (maxScroll - clampedScroll) <= 1.0f) {
                        rel = 1.0f;
                    }
                    float thumbY = sby + rel * (sbh - thumbH);
                    // Diagnostic snapping notification if the computed thumbY differs
                    // noticeably from the exact expected end when we're at/near max.
                    float expectedEnd = sby + (sbh - thumbH);
                    const float SNAP_EPS = 0.5f;
                    if (rel >= 1.0f && std::fabs(thumbY - expectedEnd) > SNAP_EPS) {
                        std::cerr << "[DIAG][UIRenderer] snapping thumbY for window id=" << win->id
                                  << " from=" << thumbY << " to=" << expectedEnd << " (clampedScroll=" << clampedScroll << ")" << std::endl;
                        thumbY = expectedEnd;
                    }
                    // Diagnostic: report thumb mapping values
                    std::cerr << "[DIAG][UIRenderer] window id=" << win->id << " axis=Y contentH=" << contentH << " viewportH=" << viewportH
                              << " sb_trackY=" << sby << " sb_trackH=" << sbh << " thumbH=" << thumbH << " rel=" << rel
                              << " thumbY=" << thumbY << " expectedEnd=" << (sby + (sbh - thumbH)) << std::endl;
                    SDL_SetRenderDrawColor(renderer, 200, 200, 200, alphaY);
                    SDL_FRect thumb = {sbx + 1.0f, thumbY, sbw - 2.0f, thumbH};
                    SDL_RenderFillRect(renderer, &thumb);
                }
                // Horizontal scrollbar
                if (win->scrollable == "horizontal" || win->scrollable == "both") {
                    float safeLeft = win->getSafeContentLeft();
                    float safeTop = win->getSafeContentTop();
                    float safeW = win->getSafeContentRight() - safeLeft;
                    float safeH = win->getSafeContentBottom() - safeTop;
                    float sbh = 8.0f;
                    float sbx = safeLeft + 4.0f;
                    float sby = safeTop + safeH - sbh - 4.0f;
                    float sbw = safeW - 8.0f;
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, static_cast<Uint8>(alphaX * 0.35f));
                    SDL_FRect bg = {sbx, sby, sbw, sbh};
                    SDL_RenderFillRect(renderer, &bg);
                    // compute content width
                    float contentW = 0.0f;
                    for (UIElement* c : win->content) {
                        float measuredW = c->width, measuredH = c->height;
                        measureElementRenderedSize(c, safeW, safeH, measuredW, measuredH);
                        // Convert from absolute coordinates to content-area-relative coordinates
                        contentW = std::max(contentW, (c->x - safeLeft) + measuredW);
                    }
                    float viewportW = safeW;
                    // Thumb width computed relative to track width (sbw)
                    float thumbW = std::max(16.0f, (contentW > 0.0f) ? sbw * (viewportW / contentW) : sbw);
                    float maxScroll = std::max(0.0f, contentW - viewportW);
                    // Do not mutate win->scrollX here. Renderer will use a
                    // local clamped value for mapping to thumb position.
                    // Visual-only mapping fix (horizontal): compute rel from a
                    // clamped scroll and snap to track end when at max to avoid
                    // visual mismatch without changing scroll semantics.
                    float clampedScrollX = std::max(0.0f, std::min(win->scrollX, maxScroll));
                    float rel = (maxScroll > 0.0f) ? (clampedScrollX / maxScroll) : 0.0f;
                    if (maxScroll > 0.0f && (maxScroll - clampedScrollX) <= 1.0f) {
                        rel = 1.0f;
                    }
                    float thumbX = sbx + rel * (sbw - thumbW);
                    float expectedEndX = sbx + (sbw - thumbW);
                    const float SNAP_EPS_X = 0.5f;
                    if (rel >= 1.0f && std::fabs(thumbX - expectedEndX) > SNAP_EPS_X) {
                        std::cerr << "[DIAG][UIRenderer] snapping thumbX for window id=" << win->id
                                  << " from=" << thumbX << " to=" << expectedEndX << " (clampedScrollX=" << clampedScrollX << ")" << std::endl;
                        thumbX = expectedEndX;
                    }
                    // Diagnostic: report thumb mapping values
                    std::cerr << "[DIAG][UIRenderer] window id=" << win->id << " axis=X contentW=" << contentW << " viewportW=" << viewportW
                              << " sb_trackX=" << sbx << " sb_trackW=" << sbw << " thumbW=" << thumbW << " rel=" << rel
                              << " thumbX=" << thumbX << " expectedEnd=" << (sbx + (sbw - thumbW)) << std::endl;
                    SDL_SetRenderDrawColor(renderer, 200, 200, 200, alphaX);
                    SDL_FRect thumb = {thumbX, sby + 1.0f, thumbW, sbh - 2.0f};
                    SDL_RenderFillRect(renderer, &thumb);
                }
            }

            

        if (useTitleTexture) {
            if (titleNineSlice) {
                titleNineSlice->render(renderer, x, y, w, titleHeight, static_cast<Uint8>(255 * win->opacity));
            } else if (titleTexture) {
                renderTexturedRectangle(titleTexture, x, y, w, titleHeight, 255,255,255,static_cast<Uint8>(255 * win->opacity), 0, 0.0f);
            }
        }

            // Draw border outside the window bounds so it doesn't reduce safe content.
            if (win->borderWidth > 0.0f) {
                uint8_t brc = win->borderColor.r;
                uint8_t bgc = win->borderColor.g;
                uint8_t bbc = win->borderColor.b;
                uint8_t bac = static_cast<Uint8>(win->borderColor.a * win->opacity);
                SDL_SetRenderDrawColor(renderer, brc, bgc, bbc, bac);
                float bw = win->borderWidth;
                drawRoundedRectangleBorder(x - bw, y - bw, w + 2*bw, h + 2*bw, radius + bw);
            }

            drawWindowChrome(win);

            if (!win->title.empty()) {
                int fontSize = static_cast<int>(Window::TITLE_BAR_HEIGHT * 0.6f);
                FontManager::FontInstance* font = fontManager->loadFont("C:/Windows/Fonts/arial.ttf", fontSize);
                if (font) {
                    BangUI::API::Color titleColor = (win->titleTextColor.a > 0) ? win->titleTextColor : win->textColor;
                    SDL_Color textColor = { titleColor.r, titleColor.g, titleColor.b, static_cast<Uint8>(static_cast<float>(titleColor.a) * win->opacity) };
                    float leftBound = win->getTitleTextLeftBound();
                    float rightBound = win->getTitleTextRightBound();
                    float availableW = std::max(0.0f, rightBound - leftBound);

                    if (availableW > 8.0f) {
                        int wholeW = 0, wholeH = 0;
                        SDL_Texture* wholeTex = fontManager->getTextTexture(renderer, font, win->title, textColor, wholeW, wholeH);
                        if (wholeTex && static_cast<float>(wholeW) <= availableW) {
                            float tx = leftBound + std::max(0.0f, (availableW - static_cast<float>(wholeW)) / 2.0f);
                            if (tx + wholeW > rightBound) tx = rightBound - wholeW;
                            if (tx < leftBound) tx = leftBound;
                            float ty = y + (Window::TITLE_BAR_HEIGHT - wholeH) / 2.0f;
                            SDL_FRect dst = {tx, ty, static_cast<float>(wholeW), static_cast<float>(wholeH)};
                            SDL_SetTextureBlendMode(wholeTex, SDL_BLENDMODE_BLEND);
                            SDL_SetTextureAlphaMod(wholeTex, static_cast<Uint8>(255 * win->opacity));
                            renderTexturedRectangle(wholeTex, dst.x, dst.y, dst.w, dst.h, 255,255,255,static_cast<Uint8>(255 * win->opacity), 0, 0.0f);
                        } else {
                            std::string title = win->title;
                            std::string ell = "...";
                            int low = 0, high = (int)title.size();
                            std::string best = "";
                            while (low <= high) {
                                int mid = (low + high) / 2;
                                std::string candidate = title.substr(0, mid) + ell;
                                int cw = 0, ch = 0;
                                SDL_Texture* candTex = fontManager->getTextTexture(renderer, font, candidate, textColor, cw, ch);
                                if (candTex && static_cast<float>(cw) <= availableW) {
                                    best = candidate;
                                    low = mid + 1;
                                } else {
                                    high = mid - 1;
                                }
                            }
                            if (best.empty()) {
                                best = ell;
                            }
                            int bw = 0, bh = 0;
                            SDL_Texture* bestTex = fontManager->getTextTexture(renderer, font, best, textColor, bw, bh);
                            if (bestTex) {
                                float tx = leftBound + std::max(0.0f, (availableW - static_cast<float>(bw)) / 2.0f);
                                if (tx + bw > rightBound) tx = rightBound - bw;
                                if (tx < leftBound) tx = leftBound;
                                float ty = y + (Window::TITLE_BAR_HEIGHT - bh) / 2.0f;
                                SDL_FRect dst = {tx, ty, static_cast<float>(bw), static_cast<float>(bh)};
                                SDL_SetTextureBlendMode(bestTex, SDL_BLENDMODE_BLEND);
                                SDL_SetTextureAlphaMod(bestTex, static_cast<Uint8>(255 * win->opacity));
                                renderTexturedRectangle(bestTex, dst.x, dst.y, dst.w, dst.h, 255,255,255,static_cast<Uint8>(255 * win->opacity), 0, 0.0f);
                            }
                        }
                    }
                }
            }

            // After drawing border and chrome, draw overlay chrome elements such as resize handle and drag indicator
            // so they appear above any child content and border.
            if (win->resizable && win->resizeHandleHot) {
                float handleRegionX = x;
                float handleRegionY = useBodyTexture ? bodyDrawY : y;
                float handleRegionW = w;
                float handleRegionH = useBodyTexture ? bodyDrawH : h;
                float insetX = useBodyTexture ? std::max(0.0f, win->contentRight + win->paddingRight) : 0.0f;
                float insetY = useBodyTexture ? std::max(0.0f, win->contentBottom + win->paddingBottom) : 0.0f;
                drawResizeHandle(handleRegionX, handleRegionY, handleRegionW, handleRegionH, insetX, insetY);
            }

            if (win->isBeingDragged) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 128);
                drawRoundedRectangleBorder(x + 5, y + 5, w - 10, h - 10,
                                          radius > 5 ? radius - 5 : 0);
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
            // Button rendering
            if (BangUI::impl::ButtonImpl* b = dynamic_cast<BangUI::impl::ButtonImpl*>(element)) {
                if (!b->visible) return;
                float x = b->x;
                float y = b->y;
                float w = b->width;
                float h = b->height;

                // Background
                Uint8 br = b->backgroundColor.r;
                Uint8 bg = b->backgroundColor.g;
                Uint8 bb = b->backgroundColor.b;
                Uint8 ba = static_cast<Uint8>(b->backgroundColor.a * b->opacity);
                SDL_SetRenderDrawColor(renderer, br, bg, bb, ba);
                drawRoundedRectangle(x, y, w, h, b->cornerRadius);

                // Border
                SDL_SetRenderDrawColor(renderer, b->borderColor.r, b->borderColor.g, b->borderColor.b, b->borderColor.a);
                drawRoundedRectangleBorder(x, y, w, h, b->cornerRadius);

                // Label centered
                std::string label = b->label;
                if (!label.empty()) {
                    int fontSize = 14;
                    FontManager::FontInstance* font = fontManager->loadFont("C:/Windows/Fonts/arial.ttf", fontSize);
                    if (font) {
                        SDL_Color textColor = { b->textColor.r, b->textColor.g, b->textColor.b, static_cast<Uint8>(b->textColor.a * b->opacity) };
                        int tw = 0, th = 0;
                        SDL_Texture* tex = fontManager->getTextTexture(renderer, font, label, textColor, tw, th);
                        if (tex) {
                            float tx = x + (w - tw) / 2.0f;
                            float ty = y + (h - th) / 2.0f;
                            SDL_FRect dst = {tx, ty, static_cast<float>(tw), static_cast<float>(th)};
                            SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
                            SDL_SetTextureAlphaMod(tex, static_cast<Uint8>(255 * b->opacity));
                            SDL_RenderTexture(renderer, tex, nullptr, &dst);
                        }
                    }
                }
                return;
            }
            // Label rendering
            if (BangUI::impl::LabelImpl* l = dynamic_cast<BangUI::impl::LabelImpl*>(element)) {
                if (!l->visible) return;
                int fontSize = l->fontSize > 0 ? l->fontSize : 14;
                FontManager::FontInstance* font = fontManager->loadFont("C:/Windows/Fonts/arial.ttf", fontSize);
                if (!font) return;
                SDL_Color textColor = { l->textColor.r, l->textColor.g, l->textColor.b, static_cast<Uint8>(l->textColor.a * l->opacity) };

                // If wordWrap, break into lines fitting element width
                if (l->wordWrap) {
                    float maxW = l->getSafeContentWidth();
                    if (maxW <= 0) maxW = l->width - 4.0f;
                    std::string text = l->text.empty() ? l->properties["text"] : l->text;
                    // naive word wrapping
                    std::vector<std::string> lines;
                    std::string current;
                    std::istringstream iss(text);
                    std::string word;
                    while (iss >> word) {
                        std::string cand = current.empty() ? word : current + " " + word;
                        int cw = 0, ch = 0;
                        SDL_Texture* tmp = fontManager->getTextTexture(renderer, font, cand, textColor, cw, ch);
                        if (tmp && static_cast<float>(cw) <= maxW) {
                            current = cand;
                        } else {
                            if (!current.empty()) lines.push_back(current);
                            current = word;
                        }
                    }
                    if (!current.empty()) lines.push_back(current);

                    int lineH = 0;
                    // measure single line height
                    { int tw=0, th=0; SDL_Texture* t = fontManager->getTextTexture(renderer, font, "Ay", textColor, tw, th); if (t) lineH = th; }
                    if (lineH <= 0) lineH = fontSize + 2;
                    float startX = l->getSafeContentLeft();
                    float startY = l->getSafeContentTop();
                    for (size_t i=0;i<lines.size();++i) {
                        int tw=0, th=0;
                        SDL_Texture* tex = fontManager->getTextTexture(renderer, font, lines[i], textColor, tw, th);
                        if (!tex) continue;
                        float tx = startX;
                        float ty = startY + i * lineH - (l->parent && (Panel* )nullptr ? 0.0f : 0.0f);
                        SDL_FRect dst = {tx, ty, static_cast<float>(tw), static_cast<float>(th)};
                        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
                        SDL_SetTextureAlphaMod(tex, static_cast<Uint8>(255 * l->opacity));
                        SDL_RenderTexture(renderer, tex, nullptr, &dst);
                    }
                } else {
                    std::string text = l->text.empty() ? l->properties["text"] : l->text;
                    // If text contains newlines, render each line stacked
                    std::istringstream iss(text);
                    std::string line;
                    float x = l->getSafeContentLeft();
                    float y = l->getSafeContentTop();
                    int lineIndex = 0;
                    while (std::getline(iss, line)) {
                        int tw=0, th=0;
                        SDL_Texture* tex = fontManager->getTextTexture(renderer, font, line, textColor, tw, th);
                        if (!tex) continue;
                        SDL_FRect dst = {x, y + lineIndex * (th + 2), static_cast<float>(tw), static_cast<float>(th)};
                        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
                        SDL_SetTextureAlphaMod(tex, static_cast<Uint8>(255 * l->opacity));
                        SDL_RenderTexture(renderer, tex, nullptr, &dst);
                        ++lineIndex;
                    }
                }
                return;
            }
        }
    }
    // Draw resize handle indicator (triangle in bottom-right corner)
    void drawResizeHandle(float regionX, float regionY, float regionW, float regionH, float insetX = 0.0f, float insetY = 0.0f) {
        float handleSize = Window::RESIZE_HANDLE_SIZE;
        float x1 = regionX + regionW - handleSize - insetX;
        float y1 = regionY + regionH - handleSize - insetY;
        float x2 = regionX + regionW - insetX;
        float y2 = regionY + regionH - insetY;

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

    // Measure an element's rendered size (width/height) taking into account
    // label wrapping and font measurement. Falls back to the element's stored
    // width/height when no special measurement is needed.
    void measureElementRenderedSizeImpl(UIElement* element, float maxW, float maxH, float& outW, float& outH) {
        outW = element->width;
        outH = element->height;
        if (!element) return;
        if (BangUI::impl::LabelImpl* l = dynamic_cast<BangUI::impl::LabelImpl*>(element)) {
            int fontSize = l->fontSize > 0 ? l->fontSize : 14;
            FontManager::FontInstance* font = fontManager->loadFont("C:/Windows/Fonts/arial.ttf", fontSize);
            if (!font) return;
            SDL_Color textColor = {255,255,255,255};
            std::string text = l->text.empty() ? l->properties["text"] : l->text;
            if (l->wordWrap) {
                float availableW = maxW;
                if (availableW <= 0) availableW = element->width > 0 ? element->width : 200.0f;
                std::istringstream iss(text);
                std::string word;
                std::string current;
                std::vector<std::string> lines;
                while (iss >> word) {
                    std::string cand = current.empty() ? word : current + " " + word;
                    int cw = 0, ch = 0;
                    SDL_Texture* tmp = fontManager->getTextTexture(renderer, font, cand, textColor, cw, ch);
                    if (tmp && static_cast<float>(cw) <= availableW) {
                        current = cand;
                    } else {
                        if (!current.empty()) lines.push_back(current);
                        current = word;
                    }
                }
                if (!current.empty()) lines.push_back(current);
                int lineH = 0; { int tw=0, th=0; SDL_Texture* t = fontManager->getTextTexture(renderer, font, "Ay", textColor, tw, th); if (t) lineH = th; }
                if (lineH <= 0) lineH = fontSize + 2;
                int maxLineW = 0; for (auto &ln : lines) { int tw=0, th=0; SDL_Texture* tx = fontManager->getTextTexture(renderer, font, ln, textColor, tw, th); if (tx) maxLineW = std::max(maxLineW, tw); }
                outW = static_cast<float>(maxLineW);
                outH = static_cast<float>(lines.size() * lineH);
            } else {
                // multiline via newlines
                std::istringstream iss(text);
                std::string line;
                int totalH = 0;
                int maxLineW = 0;
                while (std::getline(iss, line)) {
                    int tw=0, th=0;
                    SDL_Texture* tex = fontManager->getTextTexture(renderer, font, line, textColor, tw, th);
                    if (!tex) continue;
                    maxLineW = std::max(maxLineW, tw);
                    totalH += th + 2;
                }
                outW = static_cast<float>(maxLineW);
                outH = static_cast<float>(totalH);
            }
        }
    }
};
