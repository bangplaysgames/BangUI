// Renderer abstraction exposed to the public API.
#pragma once

#include "Types.h"
#include "Theme.h"

// Forward declarations for managers provided by the concrete renderer implementation.
class TextureManager;
class FontManager;
#include "UIElement.h"

namespace BangUI::API {

class IPanel;
class IWindow;

class IRenderer {
public:
    virtual ~IRenderer() = default;

    // Access managers for loading textures and fonts
    virtual ::TextureManager* getTextureManager() = 0;
    virtual ::FontManager* getFontManager() = 0;

    // Background rendering for the application window
    virtual void renderBackground(const BackgroundTheme& theme, float width, float height) = 0;

    // High-level rendering entry points used by UIManager/demo
    virtual void renderPanel(const IPanel* panel) = 0;
    virtual void renderWindow(const IWindow* window) = 0;
    // Measure an element's rendered size (width/height) taking into account
    // label wrapping and font measurement. Implementations should match the
    // behaviour used during rendering so scroll extents are consistent.
    virtual void measureElementRenderedSize(UIElement* element, float maxW, float maxH, float& outW, float& outH) = 0;

    // Diegetic mapping: optional callback to map a Mesh* and bone name to
    // a screen-space transform. The callback should return true on success
    // and populate outX/outY/outScale/outRotation. If not set or returns
    // false, the renderer should fall back to orthographic rendering.
    using DiegeticMapperFn = bool(*)(void* meshResource, const char* boneName, float* outX, float* outY, float* outScale, float* outRotation);
    virtual void setDiegeticMapper(DiegeticMapperFn fn) = 0;
};

} // namespace BangUI::API
