// Public interface for Button elements.
#pragma once

#include "UIElement.h"
#include <string>

namespace BangUI::API {

class IButton : public UIElement {
public:
    virtual ~IButton() override = default;

    virtual void SetLabel(const std::string& text) = 0;
    virtual std::string GetLabel() const = 0;
};

} // namespace BangUI::API
