#include "Button.h"

// Minimal implementation for Button activation behavior.
// This performs a press->callback->release cycle. Higher-level input
// handling (e.g., setting pressed on mouse-down and releasing on mouse-up)
// should be done by UIManager; this function provides a sensible default.

void Button::OnClick()
{
    // Enter pressed state
    pressed_ = true;
    state = ButtonState::PRESSED;

    // Call the configured click handler if present
    if (onClick)
    {
        try {
            onClick();
        } catch (...) {
            // Swallow exceptions to avoid bringing down the UI loop
        }
    }

    // Immediately transition to released state by default
    state = ButtonState::RELEASED;
    pressed_ = false;
}
