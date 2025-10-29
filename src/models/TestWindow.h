#pragma once
#include "Window.h"
#include "Panel.h"
#include "Mesh.h"
#include <vector>

class TestWindow : public Window {
public:
    TestWindow() {
        title = "BangUI Test Window";
        resizable = true;
        movable = true;
        modal = false;
        renderSpace = "orthographic";
        anchorMesh = nullptr;
        anchorBone = "";
        scrollable = "vertical";
        // Add initial test elements here
    }
};
