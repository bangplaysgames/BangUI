#pragma once
#include "../api/UIElement.h"
#include <string>

namespace BangUI::impl {

// Concrete implementation for Button. Internal-only header.
class ButtonImpl : public BangUI::API::UIElement {
public:
    ButtonImpl() { type = "Button"; }
    ~ButtonImpl() = default;

    // compatibility helpers (no API override necessary when inheriting concrete UIElement)
    void SetLabel(const std::string& text) { label = text; }
    std::string GetLabel() const { return label; }

    // Concrete behavior
    void OnClickImpl();

    // programmatic helpers
    void Press() { pressed_ = true; }
    void Release() { pressed_ = false; }
    bool IsPressed() const { return pressed_; }

    std::string label;
    bool pressed_ = false;
};

} // namespace BangUI::impl
