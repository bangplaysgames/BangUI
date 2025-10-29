// Factories for creating default implementation instances. These are impl-side
// helpers and not part of the minimal public API.
#pragma once

#include "../api/IUIManager.h"
#include "../api/IRenderer.h"
#include <memory>

namespace BangUI::impl {

std::unique_ptr<API::IUIManager> CreateDefaultUIManager();
std::unique_ptr<API::IRenderer> CreateDefaultRenderer();

} // namespace BangUI::impl
