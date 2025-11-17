#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace BangUI::Standalone {

bool LoadImageRGBA(const std::string& path, int& width, int& height, std::vector<uint8_t>& pixels);
bool ProbeImageSize(const std::string& path, int& width, int& height);

} // namespace BangUI::Standalone
