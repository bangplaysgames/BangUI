#include "TestWindow.h"
#include "Panel.h"
#include "Mesh.h"
#include <iostream>

int main() {
    TestWindow testWin;
    std::cout << "Test Window Title: " << testWin.title << std::endl;
    std::cout << "Resizable: " << testWin.resizable << std::endl;
    std::cout << "Movable: " << testWin.movable << std::endl;
    std::cout << "Modal: " << testWin.modal << std::endl;
    std::cout << "RenderSpace: " << testWin.renderSpace << std::endl;
    std::cout << "Scrollable: " << testWin.scrollable << std::endl;
    // Add more test output for elements as they are built
    return 0;
}
