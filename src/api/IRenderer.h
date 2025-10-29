// Renderer abstraction exposed to the public API.
#pragma once

#include "Types.h"

namespace BangUI::API {

class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual void DrawRect(float x, float y, float w, float h, const Color& color) = 0;
};

} // namespace BangUI::API
