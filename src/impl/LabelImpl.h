#pragma once
#include "../api/UIElement.h"
#include <string>

namespace BangUI::impl {

class LabelImpl : public BangUI::API::UIElement {
public:
    LabelImpl() { type = "Label"; }
    std::string text;
    bool wordWrap = false;
    int fontSize = 14;
    // If true and wordWrap==false, multiple labels stacked will show multiple lines
    // Text color will be taken from background/foreground combos; use properties map to set
    // Implementation will use FontManager to create textures.
};

} // namespace BangUI::impl
