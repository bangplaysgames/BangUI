#include "RendererImpl.h"

// Implementation file intentionally left minimal. All logic delegates to UIRenderer.

void BangUI::impl::RendererImpl::setDiegeticMapper(DiegeticMapperFn fn) {
	diegeticMapper = fn;
	uiRenderer.setDiegeticMapper(fn);
}
