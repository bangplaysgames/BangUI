#pragma once

#include <cstdint>
#include <string>
#include <vector>

using Uint8 = uint8_t;
using Uint32 = uint32_t;

struct SDL_Color {
    Uint8 r{255}, g{255}, b{255}, a{255};
};

struct SDL_FRect {
    float x{0.0f};
    float y{0.0f};
    float w{0.0f};
    float h{0.0f};
};

struct SDL_Rect {
    int x{0};
    int y{0};
    int w{0};
    int h{0};
};

enum SDL_BlendMode {
    SDL_BLENDMODE_NONE = 0,
    SDL_BLENDMODE_BLEND = 1
};

enum SDL_TextureAccess {
    SDL_TEXTUREACCESS_STATIC = 0,
    SDL_TEXTUREACCESS_STREAMING = 1,
    SDL_TEXTUREACCESS_TARGET = 2
};

constexpr Uint32 SDL_PIXELFORMAT_RGBA8888 = 1;

struct SDL_Texture;
struct SDL_Surface;
struct SDL_Renderer;

SDL_Renderer* SDL_CreateSoftwareRenderer(int width, int height);
void SDL_DestroyRenderer(SDL_Renderer* renderer);
void SDL_RendererResize(SDL_Renderer* renderer, int width, int height);
const std::vector<Uint8>& SDL_RendererPixels(const SDL_Renderer* renderer);
int SDL_RendererWidth(const SDL_Renderer* renderer);
int SDL_RendererHeight(const SDL_Renderer* renderer);

SDL_Texture* SDL_CreateTexture(SDL_Renderer* renderer, Uint32 format, int access, int w, int h);
void SDL_DestroyTexture(SDL_Texture* texture);
SDL_Texture* SDL_CreateTextureFromSurface(SDL_Renderer* renderer, SDL_Surface* surface);
void SDL_SetTextureBlendMode(SDL_Texture* texture, SDL_BlendMode mode);
void SDL_SetTextureAlphaMod(SDL_Texture* texture, Uint8 alpha);

void SDL_SetRenderTarget(SDL_Renderer* renderer, SDL_Texture* texture);
SDL_Texture* SDL_GetRenderTarget(SDL_Renderer* renderer);

void SDL_SetRenderDrawColor(SDL_Renderer* renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
void SDL_SetRenderDrawBlendMode(SDL_Renderer* renderer, SDL_BlendMode mode);
void SDL_RenderClear(SDL_Renderer* renderer);
void SDL_RenderFillRect(SDL_Renderer* renderer, const SDL_FRect* rect);
void SDL_RenderRect(SDL_Renderer* renderer, const SDL_FRect* rect);
void SDL_RenderLine(SDL_Renderer* renderer, float x1, float y1, float x2, float y2);
void SDL_SetRenderClipRect(SDL_Renderer* renderer, const SDL_Rect* rect);
void SDL_RenderTexture(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_FRect* src, const SDL_FRect* dst);

SDL_Surface* SDL_RenderReadPixels(SDL_Renderer* renderer, const SDL_Rect* rect);
SDL_Surface* SDL_ConvertSurface(SDL_Surface* surface, Uint32 format);
SDL_Surface* SDL_CreateSurface(int w, int h);
void SDL_DestroySurface(SDL_Surface* surface);
void SDL_SaveBMP(SDL_Surface* surface, const std::string& path);
int SDL_SurfaceWidth(const SDL_Surface* surface);
int SDL_SurfaceHeight(const SDL_Surface* surface);
int SDL_SurfacePitch(const SDL_Surface* surface);
Uint8* SDL_SurfacePixels(SDL_Surface* surface);
const Uint8* SDL_SurfacePixels(const SDL_Surface* surface);
