#pragma once

#include <cstdint>

namespace BangUI::API {

enum class UIEventType {
    None,
    Quit,
    PointerDown,
    PointerUp,
    PointerMove,
    PointerWheel,
    KeyDown,
    KeyUp,
    WindowResize
};

struct UIEvent {
    UIEventType type{UIEventType::None};
    float mouseX{0.0f};
    float mouseY{0.0f};
    float wheelX{0.0f};
    float wheelY{0.0f};
    uint32_t key{0};
    uint32_t modifiers{0};
    int width{0};
    int height{0};
};

constexpr uint32_t UIEventModifier_Shift = 1u << 0;
constexpr uint32_t UIEventModifier_Control = 1u << 1;
constexpr uint32_t UIEventModifier_Alt = 1u << 2;

} // namespace BangUI::API
