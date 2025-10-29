// Public value types and small enums for BangUI API
#pragma once

#include <cstdint>

namespace BangUI::API {

struct Color {
    uint8_t r{255}, g{255}, b{255}, a{255};
    Color() = default;
    Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
        : r(red), g(green), b(blue), a(alpha) {}
};

struct Size {
    float width{0.0f};
    float height{0.0f};
};

enum class SizeMode { Absolute, Auto, Stretch };
enum class HDock { Left, Center, Right, None };
enum class VDock { Top, Center, Bottom, None };

} // namespace BangUI::API
