#pragma once
#include "../../api/UIElement.h"
#include "../PanelImpl.h"
#include "../WindowImpl.h"
#include "../ButtonImpl.h"

namespace BangUI {
namespace impl {
namespace GlobalDefaults {

// Global default instances. Users can set these programmatically, e.g.
// BangUI::Global_Button->backgroundColor = Color(...);
inline ButtonImpl* Global_Button = nullptr;
inline PanelImpl* Global_Panel = nullptr;
inline WindowImpl* Global_Window = nullptr;

static inline void copyIfMissing(API::UIElement* target, const API::UIElement* src) {
	if (!target || !src) return;
	// Copy basic typed fields only if target's properties map doesn't already contain corresponding keys.
	// For convenience we treat a field as "missing" when it's equal to a default value.

	// Background color (treat white as unset)
	if (target->backgroundColor.r == 255 && target->backgroundColor.g == 255 && target->backgroundColor.b == 255 && target->backgroundColor.a == 255) {
		target->backgroundColor = src->backgroundColor;
	}
	// corner radius
	if (target->cornerRadius == 0.0f && src->cornerRadius != 0.0f) {
		target->cornerRadius = src->cornerRadius;
	}
	// opacity
	if (target->opacity == 1.0f && src->opacity != 1.0f) {
		target->opacity = src->opacity;
	}

	// Size and size-mode: if the target is using default Absolute/Auto and default numeric sizes,
	// copy over prototype values so Global_Button->height/width propagate.
	if (target->widthMode == API::SizeMode::Absolute && src->widthMode != API::SizeMode::Absolute) {
		target->widthMode = src->widthMode;
	}
	if (target->heightMode == API::SizeMode::Absolute && src->heightMode != API::SizeMode::Absolute) {
		target->heightMode = src->heightMode;
	}
	// Copy numeric dimensions if target still has obvious default values (100.0f) or 0
	if ((target->width == 100.0f || target->width == 0.0f) && src->width != 100.0f) {
		target->width = src->width;
	}
	if ((target->height == 100.0f || target->height == 0.0f) && src->height != 100.0f) {
		target->height = src->height;
	}
	// titleBarColor (only for windows)
	if (auto w = dynamic_cast<impl::WindowImpl*>(target)) {
		const impl::WindowImpl* sw = dynamic_cast<const impl::WindowImpl*>(src);
		if (sw && w->titleBarColor.r == 0 && w->titleBarColor.g == 0 && w->titleBarColor.b == 0 && w->titleBarColor.a == 0) {
			w->titleBarColor = sw->titleBarColor;
		}
	}

	// Copy callbacks if missing
	if (!target->onHover && src->onHover) target->onHover = src->onHover;
	if (!target->onClick && src->onClick) target->onClick = src->onClick;
	if (!target->onBlur && src->onBlur) target->onBlur = src->onBlur;
	if (!target->onFocus && src->onFocus) target->onFocus = src->onFocus;
}

inline void applyToElement(API::UIElement* element) {
	if (!element) return;
	if (element->type == "Button" && Global_Button) {
		copyIfMissing(element, Global_Button);
	} else if (element->type == "Panel" && Global_Panel) {
		copyIfMissing(element, Global_Panel);
	} else if (element->type == "Window" && Global_Window) {
		copyIfMissing(element, Global_Window);
	}
}

} // namespace GlobalDefaults
} // namespace impl
} // namespace BangUI

