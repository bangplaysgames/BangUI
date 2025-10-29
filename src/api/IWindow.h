// Public interface for Window containers
#pragma once

#include "UIElement.h"
#include <vector>

namespace BangUI::API {

class IWindow : public UIElement {
public:
    virtual ~IWindow() override = default;

    // Child management — simplified for API
    virtual std::vector<UIElement*> GetChildren() = 0;
    virtual void AddChild(UIElement* child) = 0;
    virtual bool IsModal() const = 0;
};

} // namespace BangUI::API
