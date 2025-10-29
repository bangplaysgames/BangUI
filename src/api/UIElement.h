// Minimal public API for UIElement. Stable, lightweight interface only.
#pragma once

#include "Types.h"
#include <functional>
#include <string>

namespace BangUI::API {

class UIElement {
public:
    virtual ~UIElement() = default;

    virtual std::string GetId() const = 0;
    virtual void SetId(const std::string& id) = 0;

    virtual Size GetSize() const = 0;
    virtual void SetSize(const Size& s) = 0;

    virtual void SetOnClick(std::function<void()> cb) = 0;

    // Minimal position/visibility APIs
    virtual void SetVisible(bool v) = 0;
    virtual bool IsVisible() const = 0;
};

} // namespace BangUI::API
