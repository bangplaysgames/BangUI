// Thin shim: keep legacy `Window` identifier available in the global namespace
#pragma once
#include "../impl/WindowImpl.h"
// Global alias for convenience in examples and internal code
using Window = BangUI::impl::WindowImpl;
