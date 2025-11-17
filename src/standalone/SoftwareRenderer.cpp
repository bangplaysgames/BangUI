#include "SoftwareRenderer.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <memory>

namespace {

struct SDL_SurfaceImpl {
    int w;
    int h;
    int pitch;
    std::vector<Uint8> pixels;
};

struct SDL_TextureImpl {
    int w;
    int h;
    bool isRenderTarget;
    SDL_BlendMode blendMode;
    Uint8 alphaMod;
    std::vector<Uint8> pixels;
};

struct SDL_RendererImpl {
    std::unique_ptr<SDL_TextureImpl> defaultTarget;
    SDL_TextureImpl* activeTarget;
    Uint8 drawR{255}, drawG{255}, drawB{255}, drawA{255};
    SDL_BlendMode drawBlend{SDL_BLENDMODE_BLEND};
    bool hasClip{false};
    SDL_Rect clipRect{};
};

SDL_TextureImpl* asTexture(SDL_Texture* texture) {
    return reinterpret_cast<SDL_TextureImpl*>(texture);
}

SDL_SurfaceImpl* asSurface(SDL_Surface* surface) {
    return reinterpret_cast<SDL_SurfaceImpl*>(surface);
}

SDL_RendererImpl* asRenderer(SDL_Renderer* renderer) {
    return reinterpret_cast<SDL_RendererImpl*>(renderer);
}

Uint8 clampToByte(float v) {
    if (v <= 0.0f) return 0;
    if (v >= 255.0f) return 255;
    return static_cast<Uint8>(v);
}

bool withinClip(const SDL_RendererImpl* renderer, int x, int y) {
    if (!renderer->hasClip) return true;
    const SDL_Rect& c = renderer->clipRect;
    return x >= c.x && y >= c.y && x < c.x + c.w && y < c.y + c.h;
}

void blendPixel(SDL_RendererImpl* renderer, int x, int y, Uint8 sr, Uint8 sg, Uint8 sb, Uint8 sa, SDL_BlendMode mode) {
    SDL_TextureImpl* target = renderer->activeTarget;
    if (!target) return;
    if (x < 0 || y < 0 || x >= target->w || y >= target->h) return;
    if (!withinClip(renderer, x, y)) return;

    Uint8* dst = &target->pixels[(y * target->w + x) * 4];

    if (mode == SDL_BLENDMODE_NONE || sa == 255) {
        dst[0] = sr;
        dst[1] = sg;
        dst[2] = sb;
        dst[3] = sa;
        return;
    }

    float srcAlpha = sa / 255.0f;
    float dstAlpha = dst[3] / 255.0f;
    float outAlpha = srcAlpha + dstAlpha * (1.0f - srcAlpha);
    float outR = (sr * srcAlpha + dst[0] * dstAlpha * (1.0f - srcAlpha));
    float outG = (sg * srcAlpha + dst[1] * dstAlpha * (1.0f - srcAlpha));
    float outB = (sb * srcAlpha + dst[2] * dstAlpha * (1.0f - srcAlpha));

    if (outAlpha > 0.0f) {
        dst[0] = clampToByte(outR / outAlpha);
        dst[1] = clampToByte(outG / outAlpha);
        dst[2] = clampToByte(outB / outAlpha);
        dst[3] = clampToByte(outAlpha * 255.0f);
    } else {
        dst[0] = dst[1] = dst[2] = 0;
        dst[3] = 0;
    }
}

void sampleTexture(const SDL_TextureImpl* tex, float u, float v, Uint8& r, Uint8& g, Uint8& b, Uint8& a) {
    if (!tex) {
        r = g = b = a = 0;
        return;
    }

    u = std::clamp(u, 0.0f, 1.0f);
    v = std::clamp(v, 0.0f, 1.0f);
    float srcX = u * (tex->w - 1);
    float srcY = v * (tex->h - 1);
    int x0 = static_cast<int>(std::floor(srcX));
    int y0 = static_cast<int>(std::floor(srcY));
    int x1 = std::min(x0 + 1, tex->w - 1);
    int y1 = std::min(y0 + 1, tex->h - 1);
    float fx = srcX - static_cast<float>(x0);
    float fy = srcY - static_cast<float>(y0);

    auto readPixel = [&](int px, int py) -> const Uint8* {
        return &tex->pixels[(py * tex->w + px) * 4];
    };

    const Uint8* p00 = readPixel(x0, y0);
    const Uint8* p10 = readPixel(x1, y0);
    const Uint8* p01 = readPixel(x0, y1);
    const Uint8* p11 = readPixel(x1, y1);

    auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };

    float r0 = lerp(static_cast<float>(p00[0]), static_cast<float>(p10[0]), fx);
    float g0 = lerp(static_cast<float>(p00[1]), static_cast<float>(p10[1]), fx);
    float b0 = lerp(static_cast<float>(p00[2]), static_cast<float>(p10[2]), fx);
    float a0 = lerp(static_cast<float>(p00[3]), static_cast<float>(p10[3]), fx);

    float r1 = lerp(static_cast<float>(p01[0]), static_cast<float>(p11[0]), fx);
    float g1 = lerp(static_cast<float>(p01[1]), static_cast<float>(p11[1]), fx);
    float b1 = lerp(static_cast<float>(p01[2]), static_cast<float>(p11[2]), fx);
    float a1 = lerp(static_cast<float>(p01[3]), static_cast<float>(p11[3]), fx);

    float rf = lerp(r0, r1, fy);
    float gf = lerp(g0, g1, fy);
    float bf = lerp(b0, b1, fy);
    float af = lerp(a0, a1, fy);

    r = clampToByte(rf);
    g = clampToByte(gf);
    b = clampToByte(bf);
    a = clampToByte(af);
}

} // namespace

struct SDL_Texture { char opaque; };
struct SDL_Surface { char opaque; };
struct SDL_Renderer { char opaque; };

SDL_Renderer* SDL_CreateSoftwareRenderer(int width, int height) {
    auto impl = new SDL_RendererImpl();
    impl->defaultTarget = std::make_unique<SDL_TextureImpl>();
    impl->defaultTarget->w = width;
    impl->defaultTarget->h = height;
    impl->defaultTarget->isRenderTarget = true;
    impl->defaultTarget->blendMode = SDL_BLENDMODE_BLEND;
    impl->defaultTarget->alphaMod = 255;
    impl->defaultTarget->pixels.resize(width * height * 4, 0);
    impl->activeTarget = impl->defaultTarget.get();
    impl->hasClip = false;
    impl->clipRect = {0, 0, width, height};
    return reinterpret_cast<SDL_Renderer*>(impl);
}

void SDL_DestroyRenderer(SDL_Renderer* renderer) {
    if (!renderer) return;
    delete asRenderer(renderer);
}

void SDL_RendererResize(SDL_Renderer* renderer, int width, int height) {
    if (!renderer) return;
    SDL_RendererImpl* impl = asRenderer(renderer);
    impl->defaultTarget->w = width;
    impl->defaultTarget->h = height;
    impl->defaultTarget->pixels.assign(width * height * 4, 0);
    impl->clipRect = {0, 0, width, height};
}

const std::vector<Uint8>& SDL_RendererPixels(const SDL_Renderer* renderer) {
    static std::vector<Uint8> empty;
    if (!renderer) return empty;
    const SDL_RendererImpl* impl = reinterpret_cast<const SDL_RendererImpl*>(renderer);
    return impl->defaultTarget->pixels;
}

int SDL_RendererWidth(const SDL_Renderer* renderer) {
    if (!renderer) return 0;
    const SDL_RendererImpl* impl = reinterpret_cast<const SDL_RendererImpl*>(renderer);
    return impl->defaultTarget->w;
}

int SDL_RendererHeight(const SDL_Renderer* renderer) {
    if (!renderer) return 0;
    const SDL_RendererImpl* impl = reinterpret_cast<const SDL_RendererImpl*>(renderer);
    return impl->defaultTarget->h;
}

SDL_Texture* SDL_CreateTexture(SDL_Renderer*, Uint32, int access, int w, int h) {
    auto impl = new SDL_TextureImpl();
    impl->w = w;
    impl->h = h;
    impl->isRenderTarget = access == SDL_TEXTUREACCESS_TARGET;
    impl->blendMode = SDL_BLENDMODE_BLEND;
    impl->alphaMod = 255;
    impl->pixels.resize(w * h * 4, 0);
    return reinterpret_cast<SDL_Texture*>(impl);
}

void SDL_DestroyTexture(SDL_Texture* texture) {
    if (!texture) return;
    delete asTexture(texture);
}

SDL_Texture* SDL_CreateTextureFromSurface(SDL_Renderer*, SDL_Surface* surface) {
    if (!surface) return nullptr;
    SDL_SurfaceImpl* surf = asSurface(surface);
    SDL_Texture* tex = SDL_CreateTexture(nullptr, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, surf->w, surf->h);
    SDL_TextureImpl* impl = asTexture(tex);
    impl->pixels = surf->pixels;
    return tex;
}

void SDL_SetTextureBlendMode(SDL_Texture* texture, SDL_BlendMode mode) {
    if (!texture) return;
    asTexture(texture)->blendMode = mode;
}

void SDL_SetTextureAlphaMod(SDL_Texture* texture, Uint8 alpha) {
    if (!texture) return;
    asTexture(texture)->alphaMod = alpha;
}

void SDL_SetRenderTarget(SDL_Renderer* renderer, SDL_Texture* texture) {
    SDL_RendererImpl* impl = asRenderer(renderer);
    if (!texture) {
        impl->activeTarget = impl->defaultTarget.get();
        return;
    }
    SDL_TextureImpl* tex = asTexture(texture);
    if (!tex->isRenderTarget) return;
    impl->activeTarget = tex;
}

SDL_Texture* SDL_GetRenderTarget(SDL_Renderer* renderer) {
    if (!renderer) return nullptr;
    SDL_RendererImpl* impl = asRenderer(renderer);
    if (impl->activeTarget == impl->defaultTarget.get()) return nullptr;
    return reinterpret_cast<SDL_Texture*>(impl->activeTarget);
}

void SDL_SetRenderDrawColor(SDL_Renderer* renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    SDL_RendererImpl* impl = asRenderer(renderer);
    impl->drawR = r;
    impl->drawG = g;
    impl->drawB = b;
    impl->drawA = a;
}

void SDL_SetRenderDrawBlendMode(SDL_Renderer* renderer, SDL_BlendMode mode) {
    SDL_RendererImpl* impl = asRenderer(renderer);
    impl->drawBlend = mode;
}

void SDL_RenderClear(SDL_Renderer* renderer) {
    SDL_RendererImpl* impl = asRenderer(renderer);
    SDL_TextureImpl* target = impl->activeTarget;
    if (!target) return;
    for (int y = 0; y < target->h; ++y) {
        for (int x = 0; x < target->w; ++x) {
            Uint8* dst = &target->pixels[(y * target->w + x) * 4];
            dst[0] = impl->drawR;
            dst[1] = impl->drawG;
            dst[2] = impl->drawB;
            dst[3] = impl->drawA;
        }
    }
}

void SDL_RenderFillRect(SDL_Renderer* renderer, const SDL_FRect* rect) {
    if (!rect) return;
    SDL_RendererImpl* impl = asRenderer(renderer);
    SDL_TextureImpl* target = impl->activeTarget;
    if (!target) return;
    int x0 = static_cast<int>(std::floor(rect->x));
    int y0 = static_cast<int>(std::floor(rect->y));
    int x1 = static_cast<int>(std::ceil(rect->x + rect->w));
    int y1 = static_cast<int>(std::ceil(rect->y + rect->h));
    for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) {
            blendPixel(impl, x, y, impl->drawR, impl->drawG, impl->drawB, impl->drawA, impl->drawBlend);
        }
    }
}

void SDL_RenderRect(SDL_Renderer* renderer, const SDL_FRect* rect) {
    if (!rect) return;
    SDL_RenderLine(renderer, rect->x, rect->y, rect->x + rect->w, rect->y);
    SDL_RenderLine(renderer, rect->x, rect->y + rect->h, rect->x + rect->w, rect->y + rect->h);
    SDL_RenderLine(renderer, rect->x, rect->y, rect->x, rect->y + rect->h);
    SDL_RenderLine(renderer, rect->x + rect->w, rect->y, rect->x + rect->w, rect->y + rect->h);
}

void SDL_RenderLine(SDL_Renderer* renderer, float x1, float y1, float x2, float y2) {
    SDL_RendererImpl* impl = asRenderer(renderer);
    SDL_TextureImpl* target = impl->activeTarget;
    if (!target) return;

    int ix1 = static_cast<int>(std::round(x1));
    int iy1 = static_cast<int>(std::round(y1));
    int ix2 = static_cast<int>(std::round(x2));
    int iy2 = static_cast<int>(std::round(y2));

    int dx = std::abs(ix2 - ix1);
    int sx = ix1 < ix2 ? 1 : -1;
    int dy = -std::abs(iy2 - iy1);
    int sy = iy1 < iy2 ? 1 : -1;
    int err = dx + dy;

    int x = ix1;
    int y = iy1;
    while (true) {
        blendPixel(impl, x, y, impl->drawR, impl->drawG, impl->drawB, impl->drawA, impl->drawBlend);
        if (x == ix2 && y == iy2) break;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y += sy;
        }
    }
}

void SDL_SetRenderClipRect(SDL_Renderer* renderer, const SDL_Rect* rect) {
    SDL_RendererImpl* impl = asRenderer(renderer);
    if (!rect) {
        impl->hasClip = false;
        impl->clipRect = {0, 0, impl->activeTarget ? impl->activeTarget->w : 0, impl->activeTarget ? impl->activeTarget->h : 0};
        return;
    }
    impl->hasClip = true;
    impl->clipRect = *rect;
}

void SDL_RenderTexture(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_FRect* srcRect, const SDL_FRect* dstRect) {
    if (!renderer || !texture || !dstRect) return;
    SDL_RendererImpl* impl = asRenderer(renderer);
    SDL_TextureImpl* tex = asTexture(texture);
    SDL_TextureImpl* target = impl->activeTarget;
    if (!target) return;

    SDL_FRect src{};
    if (srcRect) {
        src = *srcRect;
    } else {
        src = {0.0f, 0.0f, static_cast<float>(tex->w), static_cast<float>(tex->h)};
    }

    int x0 = static_cast<int>(std::floor(dstRect->x));
    int y0 = static_cast<int>(std::floor(dstRect->y));
    int x1 = static_cast<int>(std::ceil(dstRect->x + dstRect->w));
    int y1 = static_cast<int>(std::ceil(dstRect->y + dstRect->h));

    for (int y = y0; y < y1; ++y) {
        float v = dstRect->h > 0.0f ? (static_cast<float>(y) + 0.5f - dstRect->y) / dstRect->h : 0.0f;
        for (int x = x0; x < x1; ++x) {
            float u = dstRect->w > 0.0f ? (static_cast<float>(x) + 0.5f - dstRect->x) / dstRect->w : 0.0f;
            float texU = src.x + u * src.w;
            float texV = src.y + v * src.h;
            Uint8 r, g, b, a;
            float normU = tex->w > 0 ? texU / static_cast<float>(tex->w) : 0.0f;
            float normV = tex->h > 0 ? texV / static_cast<float>(tex->h) : 0.0f;
            sampleTexture(tex, normU, normV, r, g, b, a);
            float alpha = tex->alphaMod / 255.0f;
            a = clampToByte(a * alpha);
            blendPixel(impl, x, y, r, g, b, a, tex->blendMode);
        }
    }
}

SDL_Surface* SDL_RenderReadPixels(SDL_Renderer* renderer, const SDL_Rect* rect) {
    SDL_RendererImpl* impl = asRenderer(renderer);
    SDL_TextureImpl* target = impl->activeTarget;
    if (!target) return nullptr;

    int x = 0, y = 0, w = target->w, h = target->h;
    if (rect) {
        x = rect->x;
        y = rect->y;
        w = rect->w;
        h = rect->h;
    }

    auto surface = new SDL_SurfaceImpl();
    surface->w = w;
    surface->h = h;
    surface->pitch = w * 4;
    surface->pixels.resize(w * h * 4);

    for (int row = 0; row < h; ++row) {
        for (int col = 0; col < w; ++col) {
            int srcX = std::clamp(x + col, 0, target->w - 1);
            int srcY = std::clamp(y + row, 0, target->h - 1);
            const Uint8* src = &target->pixels[(srcY * target->w + srcX) * 4];
            Uint8* dst = &surface->pixels[(row * w + col) * 4];
            dst[0] = src[0];
            dst[1] = src[1];
            dst[2] = src[2];
            dst[3] = src[3];
        }
    }
    return reinterpret_cast<SDL_Surface*>(surface);
}

SDL_Surface* SDL_ConvertSurface(SDL_Surface* surface, Uint32) {
    if (!surface) return nullptr;
    SDL_SurfaceImpl* src = asSurface(surface);
    auto copy = new SDL_SurfaceImpl();
    copy->w = src->w;
    copy->h = src->h;
    copy->pitch = src->pitch;
    copy->pixels = src->pixels;
    return reinterpret_cast<SDL_Surface*>(copy);
}

SDL_Surface* SDL_CreateSurface(int w, int h) {
    auto surf = new SDL_SurfaceImpl();
    surf->w = w;
    surf->h = h;
    surf->pitch = w * 4;
    surf->pixels.resize(w * h * 4);
    return reinterpret_cast<SDL_Surface*>(surf);
}

void SDL_DestroySurface(SDL_Surface* surface) {
    if (!surface) return;
    delete asSurface(surface);
}

void SDL_SaveBMP(SDL_Surface* surface, const std::string& path) {
    if (!surface) return;
    SDL_SurfaceImpl* surf = asSurface(surface);
    std::ofstream out(path, std::ios::binary);
    if (!out) return;

    int fileSize = 54 + surf->w * surf->h * 4;
    unsigned char header[54] = {
        'B','M',
        static_cast<unsigned char>(fileSize), static_cast<unsigned char>(fileSize >> 8),
        static_cast<unsigned char>(fileSize >> 16), static_cast<unsigned char>(fileSize >> 24),
        0,0,0,0,
        54,0,0,0,
        40,0,0,0,
        static_cast<unsigned char>(surf->w), static_cast<unsigned char>(surf->w >> 8),
        static_cast<unsigned char>(surf->w >> 16), static_cast<unsigned char>(surf->w >> 24),
        static_cast<unsigned char>(surf->h), static_cast<unsigned char>(surf->h >> 8),
        static_cast<unsigned char>(surf->h >> 16), static_cast<unsigned char>(surf->h >> 24),
        1,0,
        32,0,
        0,0,0,0,
        0,0,0,0,
        0,0,0,0,
        0,0,0,0,
        0,0,0,0,
        0,0,0,0
    };
    out.write(reinterpret_cast<const char*>(header), sizeof(header));
    for (int y = 0; y < surf->h; ++y) {
        for (int x = 0; x < surf->w; ++x) {
            const Uint8* src = &surf->pixels[(y * surf->w + x) * 4];
            out.put(static_cast<char>(src[2]));
            out.put(static_cast<char>(src[1]));
            out.put(static_cast<char>(src[0]));
            out.put(static_cast<char>(src[3]));
        }
    }
}

int SDL_SurfaceWidth(const SDL_Surface* surface) {
    if (!surface) return 0;
    return reinterpret_cast<const SDL_SurfaceImpl*>(surface)->w;
}

int SDL_SurfaceHeight(const SDL_Surface* surface) {
    if (!surface) return 0;
    return reinterpret_cast<const SDL_SurfaceImpl*>(surface)->h;
}

int SDL_SurfacePitch(const SDL_Surface* surface) {
    if (!surface) return 0;
    return reinterpret_cast<const SDL_SurfaceImpl*>(surface)->pitch;
}

Uint8* SDL_SurfacePixels(SDL_Surface* surface) {
    if (!surface) return nullptr;
    return reinterpret_cast<SDL_SurfaceImpl*>(surface)->pixels.data();
}

const Uint8* SDL_SurfacePixels(const SDL_Surface* surface) {
    if (!surface) return nullptr;
    return reinterpret_cast<const SDL_SurfaceImpl*>(surface)->pixels.data();
}
