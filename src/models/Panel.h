// Thin shim: keep legacy `Panel` identifier available in the global namespace
#pragma once
#include "../impl/PanelImpl.h"
// Global alias for convenience in examples and internal code
using Panel = BangUI::impl::PanelImpl;
