#include <SDL3/SDL.h>
#include <iostream>
#include "../lib/BangUI.h"

int main(int argc, char* argv[]) {
    std::cout << "=== BangUI Texture Demo ===" << std::endl;
    std::cout << "Demonstrating texture loading with alpha blending" << std::endl;

    // Initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create window and renderer
    SDL_Window* sdlWindow = SDL_CreateWindow("BangUI Texture Demo", 800, 600, 0);
    if (!sdlWindow) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* sdlRenderer = SDL_CreateRenderer(sdlWindow, nullptr);
    if (!sdlRenderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(sdlWindow);
        SDL_Quit();
        return 1;
    }

    std::cout << "SDL initialized successfully" << std::endl;

    // Create the BangUI library manager and renderer
    UIManager uiManager;
    UIRenderer uiRenderer(sdlRenderer);

    std::cout << "\n=== Creating UI Elements with Textures ===" << std::endl;

    // Panel 1 - Textured panel (if texture file exists)
    Panel* panel1 = new Panel();
    panel1->id = "textured_panel";
    panel1->hDock = HDock::Left;
    panel1->widthMode = SizeMode::Absolute;
    panel1->heightMode = SizeMode::Absolute;
    panel1->width = 200;
    panel1->height = 200;
    panel1->marginTop = 10;
    panel1->marginLeft = 10;
    panel1->cornerRadius = 10.0f;
    panel1->movable = true;
    // Set texture path - this will be loaded if the file exists
    panel1->src = "texture.png";
    // Set background color with alpha for blending over texture
    panel1->backgroundColor = Color(255, 100, 100, 128); // Red tint with 50% alpha
    uiManager.addPanel(panel1);
    std::cout << "Panel 1: Textured with red overlay (alpha=128)" << std::endl;

    // Panel 2 - Same texture, different color overlay
    Panel* panel2 = new Panel();
    panel2->id = "textured_panel_2";
    panel2->hDock = HDock::Center;
    panel2->vDock = VDock::Top;
    panel2->widthMode = SizeMode::Absolute;
    panel2->heightMode = SizeMode::Absolute;
    panel2->width = 250;
    panel2->height = 200;
    panel2->marginTop = 10;
    panel2->cornerRadius = 15.0f;
    panel2->movable = true;
    panel2->src = "texture.png";
    // Blue tint with 75 alpha
    panel2->backgroundColor = Color(100, 100, 255, 75);
    uiManager.addPanel(panel2);
    std::cout << "Panel 2: Same texture, blue overlay (alpha=75)" << std::endl;

    // Panel 3 - No texture, solid color
    Panel* panel3 = new Panel();
    panel3->id = "solid_panel";
    panel3->hDock = HDock::Right;
    panel3->vDock = VDock::Top;
    panel3->widthMode = SizeMode::Absolute;
    panel3->heightMode = SizeMode::Absolute;
    panel3->width = 150;
    panel3->height = 150;
    panel3->marginTop = 10;
    panel3->marginRight = 10;
    panel3->cornerRadius = 5.0f;
    panel3->movable = true;
    uiManager.addPanel(panel3);
    std::cout << "Panel 3: Solid color (no texture)" << std::endl;

    // Window 1 - Textured window body with custom title bar color
    Window* window1 = new Window();
    window1->id = "textured_window";
    window1->title = "Textured Window";
    window1->vDock = VDock::Bottom;
    window1->widthMode = SizeMode::Absolute;
    window1->heightMode = SizeMode::Absolute;
    window1->width = 300;
    window1->height = 180;
    window1->marginBottom = 10;
    window1->marginLeft = 10;
    window1->cornerRadius = 8.0f;
    window1->movable = true;
    window1->closeable = true;
    window1->src = "texture.png";
    // Green tint over texture
    window1->backgroundColor = Color(100, 255, 100, 100);
    window1->titleBarColor = Color(50, 150, 50, 255); // Dark green title bar
    uiManager.addWindow(window1);
    std::cout << "Window 1: Textured body with green overlay and green title bar" << std::endl;

    // Window 2 - No texture, with semi-transparent title bar
    Window* window2 = new Window();
    window2->id = "solid_window";
    window2->title = "Solid Window";
    window2->hDock = HDock::Right;
    window2->vDock = VDock::Bottom;
    window2->widthMode = SizeMode::Absolute;
    window2->heightMode = SizeMode::Absolute;
    window2->width = 200;
    window2->height = 150;
    window2->marginBottom = 10;
    window2->marginRight = 10;
    window2->cornerRadius = 12.0f;
    window2->movable = true;
    window2->closeable = true;
    window2->titleBarColor = Color(120, 60, 180, 200); // Purple title bar with transparency
    uiManager.addWindow(window2);
    std::cout << "Window 2: Solid color with semi-transparent purple title bar" << std::endl;

    std::cout << "\n=== Controls ===" << std::endl;
    std::cout << "- If 'texture.png' exists, it will be loaded with color overlays" << std::endl;
    std::cout << "- Panels show different alpha blending over the same texture" << std::endl;
    std::cout << "- Drag elements to move them around" << std::endl;
    std::cout << "- ESC or close window to quit" << std::endl;
    std::cout << "\nNote: Place a 'texture.png' in the executable directory to see textures!" << std::endl;

    std::cout << "\n=== Starting Render Loop ===" << std::endl;

    bool running = true;
    SDL_Event e;

    // Get window size for layout
    int windowWidth, windowHeight;
    SDL_GetWindowSize(sdlWindow, &windowWidth, &windowHeight);

    while (running) {
        // Event handling
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            }
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) {
                running = false;
            }

            // Let the UI manager handle all mouse events
            uiManager.handleEvent(e);
        }

        // Compute layout for all elements
        std::vector<UIElement*> allElements;
        for (Panel* p : uiManager.getPanels()) {
            allElements.push_back(p);
        }
        for (Window* w : uiManager.getWindows()) {
            allElements.push_back(w);
        }
        LayoutManager::computeLayout(windowWidth, windowHeight, allElements);

        // Rendering
        SDL_SetRenderDrawColor(sdlRenderer, 40, 40, 40, 255);
        SDL_RenderClear(sdlRenderer);

        // Render all panels
        for (const Panel* panel : uiManager.getPanels()) {
            if (panel->id == "textured_panel") {
                uiRenderer.renderPanel(panel, 255, 100, 100); // Red base
            } else if (panel->id == "textured_panel_2") {
                uiRenderer.renderPanel(panel, 100, 100, 255); // Blue base
            } else {
                uiRenderer.renderPanel(panel, 30, 200, 100); // Green
            }
        }

        // Render all windows (title bar color now from window property)
        for (const Window* win : uiManager.getWindows()) {
            if (win->id == "textured_window") {
                uiRenderer.renderWindow(win, 100, 255, 100); // Green body
            } else {
                uiRenderer.renderWindow(win, 120, 50, 180); // Purple body
            }
        }

        SDL_RenderPresent(sdlRenderer);
        SDL_Delay(16); // ~60 FPS
    }

    std::cout << "\n=== Shutting Down ===" << std::endl;

    // Cleanup
    delete panel1;
    delete panel2;
    delete panel3;
    for (const Window* win : uiManager.getWindows()) {
        delete win;
    }

    SDL_DestroyRenderer(sdlRenderer);
    SDL_DestroyWindow(sdlWindow);
    SDL_Quit();

    std::cout << "BangUI texture demo completed successfully!" << std::endl;
    return 0;
}
