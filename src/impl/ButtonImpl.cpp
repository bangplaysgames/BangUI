#include "ButtonImpl.h"

namespace BangUI::impl {

void ButtonImpl::OnClickImpl()
{
    // Enter pressed state
    pressed_ = true;

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
    pressed_ = false;
}

} // namespace BangUI::impl
