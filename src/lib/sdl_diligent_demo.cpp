#include <SDL3/SDL.h>
#include <iostream>
#include <cmath>
#include "../models/Window.h"
#include "../models/Panel.h"
#include "UIManager.h"
#include "UIRenderer.h"
#include "LayoutManager.h"

int main(int argc, char* argv[]) {
    std::cout << "=== BangUI Interactive Demo ===" << std::endl;
    std::cout << "Testing docking and size modes" << std::endl;

    // Initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create window and renderer
    SDL_Window* sdlWindow = SDL_CreateWindow("BangUI Interactive Demo", 800, 600, SDL_WINDOW_RESIZABLE);
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

    // Set application window bounds for clamping
    uiManager.setApplicationWindow(800.0f, 600.0f, 0.0f);

    std::cout << "\n=== Creating UI Elements with Docking ===" << std::endl;

    // Panel 2 - Center docked
    Panel* panel2 = new Panel();
    panel2->id = "panel2";
    panel2->hDock = HDock::Center;
    panel2->vDock = VDock::Top;
    panel2->widthMode = SizeMode::Stretch;
    panel2->heightMode = SizeMode::Absolute;
    panel2->width = 250;
    panel2->height = 200;
    panel2->marginTop = 10;
    panel2->cornerRadius = 0.0f;
    panel2->movable = false;
    panel2->backgroundColor = Color(30, 200, 100, 175); // Green
    panel2->zIndex = 0;
    uiManager.addPanel(panel2);
    std::cout << "Panel 2: Center-Top docked, 250x200px, sharp corners" << std::endl;

    // Window 1 - Bottom-left with close button and custom title bar color
    Window* window1 = new Window();
    window1->id = "window1";
    window1->title = "Docked Bottom-Left";
    window1->vDock = VDock::None;
    window1->widthMode = SizeMode::Absolute;
    window1->heightMode = SizeMode::Absolute;
    window1->backgroundColor = Color(195, 195, 40, 128);
    window1->width = 300;
    window1->height = 180;
    window1->marginBottom = 0;
    window1->marginLeft = 0;
    window1->marginTop = 0;
    window1->marginRight = 0;
    window1->setPadding(0);
    window1->cornerRadius = 8.0f;
    window1->movable = true;
    window1->closeable = true;
    window1->resizable = true;
    window1->titleBarColor = Color(0, 0, 0, 0); // STOP CHANGING THIS LINE!!!!!
    // window1->src = "textbox.9.png"; // 9-patch texture
    window1->zIndex = 1;
    uiManager.addWindow(window1);
    std::cout << "Window 1: Bottom-Left, resizable, closeable, red title bar" << std::endl;

    // Window 2 - Bottom-right with custom title bar color
    Window* window2 = new Window();
    window2->id = "window2";
    window2->title = "Docked Bottom-Right";
    window2->hDock = HDock::None;
    window2->vDock = VDock::Bottom;
    window2->widthMode = SizeMode::Absolute;
    window2->heightMode = SizeMode::Absolute;
    window2->width = 200;
    window2->height = 150;
    window2->marginBottom = 10;
    window2->marginRight = 10;
    window2->cornerRadius = 12.0f;
    window2->movable = true;
    window2->closeable = false;
    window2->resizable = true; // Enable resizing
    window2->titleBarColor = Color(80, 30, 120, 255); // Purple title bar
    window2->backgroundColor = Color(150, 50, 200, 255);
    // Window2 will be added as a child of panel1 after panel1 is created below
    std::cout << "Window 2: prepared for adding to a panel (will attach to panel1 later)" << std::endl;

    // Panel 1 - Top-left docked with absolute size
    Panel* panel1 = new Panel();
    panel1->id = "panel1";
    panel1->hDock = HDock::None;
    panel1->vDock = VDock::None;
    panel1->widthMode = SizeMode::Auto;
    panel1->heightMode = SizeMode::Auto;
    panel1->height = 250;
    panel1->width = 250;
    panel1->marginTop = 10;
    panel1->marginLeft = 10;
    panel1->cornerRadius = 20.0f;
    panel1->movable = true;
    panel1->backgroundColor = Color(10, 200, 255, 175); // Blue
    panel1->zIndex = 0;
    uiManager.addPanel(panel1);
    std::cout << "Panel 1: Top-Left docked, 200x150px, rounded" << std::endl;

    // Attach previously prepared window2 as a child of panel1
    window2->parent = panel1;
    panel1->content.push_back(window2);
    std::cout << "Window 2: attached as a child of panel1" << std::endl;

    Panel* panel3 = new Panel();
    panel3->id = "logoPanel";
    panel3->backgroundColor = Color(255, 255, 255, 0);
    panel3->borderColor = Color(255, 255, 255, 0);
    panel3->hDock = HDock::Center;
    panel3->vDock = VDock::Center;
    panel3->widthMode = SizeMode::Auto;
    panel3->heightMode = SizeMode::Stretch;
    panel3->src = "bpg.png"; // Logo image
    panel3->interactable = false;
    uiManager.addPanel(panel3);

    Button* Button1 = new Button();
    Button1->id = "button1";
    Button1->text = "Test Button";
    Button1->parent = panel2;
    Button1->OnClick() override {
        std::cout << "Button clicked!" << std::endl;
    };
    panel2->content.push_back(Button1);


    std::cout << "\n=== Controls ===" << std::endl;
    std::cout << "- Elements auto-position based on dock settings" << std::endl;
    std::cout << "- Drag Windows by title bar" << std::endl;
    std::cout << "- Drag Panels anywhere on their surface" << std::endl;
    std::cout << "- Hover bottom-right corner of resizable windows to see resize handle" << std::endl;
    std::cout << "- Drag resize handle to resize windows" << std::endl;
    std::cout << "- Click X to close windows" << std::endl;
    std::cout << "- ESC or close window to quit" << std::endl;

    std::cout << "\n=== Starting Render Loop ===" << std::endl;

    bool running = true;
    SDL_Event e;

    // Opacity automation state for panel3
    Uint32 lastTicks = SDL_GetTicks();
    float phase = 0.0f;               // accumulated time for the sine
    const float period = 3.0f;        // seconds per full fade cycle (configurable)
    const float smoothing = 8.0f;     // higher = snappier follow, lower = smoother

    // Get window size for layout
    int windowWidth, windowHeight;
    SDL_GetWindowSize(sdlWindow, &windowWidth, &windowHeight);

    while (running) {
    // Smooth Fade Logo: compute target via sine, then apply exponential smoothing
    Uint32 nowTicks = SDL_GetTicks();
    float dt = (nowTicks - lastTicks) / 1000.0f;
    if (dt < 0) dt = 0.0f;
    lastTicks = nowTicks;

    phase += dt;
    // keep phase bounded to avoid float overflow over long runs
    if (phase > 1e6f) phase = fmod(phase, period);

    // Sine-based target opacity in [0,1]
    float target = 0.5f * (1.0f + sinf(2.0f * 3.14159265358979323846f * phase / period));

    // Exponential smoothing factor (frame-rate independent)
    float alpha = 1.0f - expf(-smoothing * dt);
    // Lerp current opacity toward target
    panel3->opacity = panel3->opacity + (target - panel3->opacity) * alpha;

        // Event handling
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            }
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) {
                running = false;
            }

            // Handle window resize events (SDL3: SDL_EVENT_WINDOW_RESIZED, SDL2: SDL_WINDOWEVENT)
            if (e.type == SDL_EVENT_WINDOW_RESIZED) {
                // SDL3 provides new size in event.window.data1 / data2
                windowWidth = e.window.data1;
                windowHeight = e.window.data2;
                // Recompute layout immediately for the new size
                std::vector<UIElement*> allElements;
                for (Panel* p : uiManager.getPanels()) allElements.push_back(p);
                for (Window* w : uiManager.getWindows()) allElements.push_back(w);
                LayoutManager::computeLayout(windowWidth, windowHeight, allElements);
                uiManager.setApplicationWindow(windowWidth, windowHeight, 0.0f);
            }

            // Let the UI manager handle all mouse/events
            uiManager.handleEvent(e);
        }

        // Compute layout for all elements (respects docking)
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
            uiRenderer.renderPanel(panel);
        }

        // Render all windows (title bar color now from window property)
        for (const Window* win : uiManager.getWindows()) {
            uiRenderer.renderWindow(win);
        }

        SDL_RenderPresent(sdlRenderer);
        SDL_Delay(16); // ~60 FPS
    }

    std::cout << "\n=== Shutting Down ===" << std::endl;

    // Cleanup
    delete panel1;
    delete panel2;
    for (const Window* win : uiManager.getWindows()) {
        delete win;
    }

    SDL_DestroyRenderer(sdlRenderer);
    SDL_DestroyWindow(sdlWindow);
    SDL_Quit();

    std::cout << "BangUI demo completed successfully!" << std::endl;
    return 0;
}
