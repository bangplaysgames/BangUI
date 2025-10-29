// Public interface for Panel containers
#pragma once

#include "UIElement.h"
#include <vector>

namespace BangUI::API {

class IPanel : public UIElement {
public:
    virtual ~IPanel() override = default;
    virtual std::vector<UIElement*> GetChildren() = 0;
    virtual void AddChild(UIElement* child) = 0;
};

} // namespace BangUI::API
