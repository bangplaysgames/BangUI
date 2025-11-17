#pragma once

#include <string>
#include <vector>

#include "Types.h"

namespace BangUI::API {

enum class VectorShapeType {
    Rectangle,
    RoundedRectangle,
    Line
};

struct VectorShape {
    VectorShapeType type{VectorShapeType::Rectangle};
    float x{0.0f};
    float y{0.0f};
    float width{0.0f};
    float height{0.0f};
    float radius{0.0f};
    float x2{0.0f};
    float y2{0.0f};
    float thickness{1.0f};
    bool normalized{false};
    Color color{255, 255, 255, 255};
};

struct BackgroundTheme {
    bool enabled{false};
    Color clearColor{12, 12, 18, 255};
    std::string texture;
    float textureOpacity{1.0f};
    bool textureNinePatch{false};
    std::vector<VectorShape> shapes;
};

} // namespace BangUI::API
