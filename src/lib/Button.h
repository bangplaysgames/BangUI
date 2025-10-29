#pragma once
#include "UIElement.h"
#include <string>

enum ButtonState {
    NORMAL,
    PRESSED,
    RELEASED
};

class Button : public UIElement {
public:
    std::string text;
    ButtonState state = ButtonState::NORMAL;
    void OnClick();
        // transient pressed state (not persistent between frames unless UIManager keeps it)
        bool pressed_ = false;
    public:
        // Programmatic press/release helpers
        void Press() { pressed_ = true; }
        void Release() { pressed_ = false; }
        bool IsPressed() const { return pressed_; }
};
