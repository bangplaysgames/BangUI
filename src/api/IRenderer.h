// Renderer abstraction exposed to the public API.
#pragma once

#include "Types.h"

// Forward declarations for managers provided by the concrete renderer implementation.
class TextureManager;
class FontManager;

namespace BangUI::API {

class IPanel;
class IWindow;

class IRenderer {
public:
    virtual ~IRenderer() = default;

    // Access managers for loading textures and fonts
    virtual ::TextureManager* getTextureManager() = 0;
    virtual ::FontManager* getFontManager() = 0;

    // High-level rendering entry points used by UIManager/demo
    virtual void renderPanel(const IPanel* panel) = 0;
    virtual void renderWindow(const IWindow* window) = 0;
};

} // namespace BangUI::API
