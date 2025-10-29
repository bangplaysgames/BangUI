// Single-include header for BangUI convenience. This is the canonical consumer
// include: applications should `#include "BangUI.h"` to get the public API.
//
// Note: BangUI provides built-in input handling within the library (UIManager
// processes SDL events by default). Applications may override or intercept
// input handling by subclassing `BangUI::UIManager` or by calling
// `setEventInterceptor()` on the manager instance.
#pragma once

// Public API headers
#include "../api/UIElement.h"

// Models
#include "../impl/PanelImpl.h"
#include "../impl/WindowImpl.h"
#include "../impl/ButtonImpl.h"

// Managers and subsystems
#include "../impl/UIRenderer.h"
#include "../impl/RendererImpl.h"
#include "../impl/LayoutManager.h"
#include "../impl/UIManagerImpl.h"
#include "../impl/TextureManager.h"
#include "../impl/FontManager.h"

// Utilities
#include "../impl/NineSliceTexture.h"
#include "../impl/BML/BMLParser.h"

// Convenience aliases under the BangUI namespace for easy access in user code.
namespace BangUI {
	using UIManager = impl::UIManagerImpl;
	using UIRenderer = ::UIRenderer;
	// Expose the concrete renderer wrapper so applications can construct
	// an API-compatible renderer via the single-include header.
	using Renderer = impl::RendererImpl;
	using Window = impl::WindowImpl;
	using Panel = impl::PanelImpl;
	using Button = impl::ButtonImpl;
}
