// Minimal public interface for the UIManager
#pragma once

#include "UIElement.h"
#include <memory>

namespace BangUI::API {

class IRenderer; // forward

class IUIManager {
public:
    virtual ~IUIManager() = default;
    virtual void Update(float dt) = 0;
    virtual void Render(IRenderer* renderer) = 0;
};

} // namespace BangUI::API
