// Public value types and small enums for BangUI API
#pragma once

#include <cstdint>

namespace BangUI::API {

struct Color {
    uint8_t r{255}, g{255}, b{255}, a{255};
};

struct Size {
    float width{0.0f};
    float height{0.0f};
};

enum class SizeMode { Absolute, Auto, Stretch };
enum class HDock { Left, Center, Right, Fill };
enum class VDock { Top, Center, Bottom, Fill };

} // namespace BangUI::API
