#include <SDL3/SDL.h>
#include <iostream>
#include <cmath>
#include "BangUI.h"

using namespace BangUI;

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
    // Use the API-compatible renderer wrapper which implements API::IRenderer
    BangUI::Renderer rendererImpl(sdlRenderer);

    // Provide the native SDL window to the manager so it can query size
    // itself and remain authoritative for application bounds.
    uiManager.setNativeWindow(reinterpret_cast<void*>(sdlWindow));

    std::cout << "\n=== Creating UI Elements with Docking ===" << std::endl;

    Global_Button = new BangUI::impl::ButtonImpl();
    Global_Button->heightMode = SizeMode::Absolute;
    Global_Button->widthMode = SizeMode::Auto;
    Global_Button->height = 20.0f;
    Global_Button->onHover = [](UIElement* el) {
        if (!el) return;
        auto clamp = [](int v) {
            if (v < 0) return 0;
            if (v > 255) return 255;
            return v;
        };
        int nr = clamp(int(el->backgroundColor.r) + 40);
        int ng = clamp(int(el->backgroundColor.g) + 40);
        int nb = clamp(int(el->backgroundColor.b) + 40);
        el->backgroundColor.r = static_cast<uint8_t>(nr);
        el->backgroundColor.g = static_cast<uint8_t>(ng);
        el->backgroundColor.b = static_cast<uint8_t>(nb);
    };
    Global_Button->onBlur = [](UIElement* el) {
        if (!el) return;
        auto clamp = [](int v) {
            if (v < 0) return 0;
            if (v > 255) return 255;
            return v;
        };
        int nr = clamp(int(el->backgroundColor.r) - 40);
        int ng = clamp(int(el->backgroundColor.g) - 40);
        int nb = clamp(int(el->backgroundColor.b) - 40);
        el->backgroundColor.r = static_cast<uint8_t>(nr);
        el->backgroundColor.g = static_cast<uint8_t>(ng);
        el->backgroundColor.b = static_cast<uint8_t>(nb);
    };


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
    window1->backgroundColor = Color(195, 195, 40, 200);
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
    window2->title = "Docked Bottom Child Window inside Panel1";
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
    panel1->backgroundColor = Color(10, 200, 255, 255); // Blue
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

    // --- Demo window showing docking stacking (ReserveStrips) ---
    Window* demoWin = new Window();
    demoWin->id = "dock_demo";
    demoWin->title = "Docking Demo";
    demoWin->widthMode = SizeMode::Absolute;
    demoWin->heightMode = SizeMode::Absolute;
    demoWin->width = 640;
    demoWin->height = 360;
    demoWin->cornerRadius = 6.0f;
    demoWin->movable = true;
    demoWin->closeable = true;
    demoWin->resizable = true;
    // center the demo window
    demoWin->x = 50.0f;
    demoWin->y = 50.0f;

    // Top toolbar panel inside demo window
    Panel* topToolbar = new Panel();
    topToolbar->id = "toolbar";
    topToolbar->vDock = VDock::Top;
    topToolbar->hDock = HDock::None;
    topToolbar->widthMode = SizeMode::Stretch;
    topToolbar->heightMode = SizeMode::Absolute;
    topToolbar->height = 48.0f;
    topToolbar->backgroundColor = Color(40,40,40,200);
    topToolbar->parent = demoWin;

    // Add several toolbar buttons (these use vDock=Top so they are placed left-to-right)
    for (int i = 0; i < 5; ++i) {
        Button* b = new Button();
        b->id = std::string("tb") + std::to_string(i);
        b->label = std::string("T") + std::to_string(i+1);
        b->backgroundColor = Color(70 + i*2,70 + i*2,90 + i*2,255);
        b->vDock = VDock::Top;
        b->widthMode = SizeMode::Auto;
        b->heightMode = SizeMode::Absolute;
        b->height = 36.0f;
        b->marginLeft = 6.0f;
        b->marginTop = 6.0f;
        b->parent = topToolbar;
        topToolbar->content.push_back(b);
    }

    // Left sidebar panel inside demo window
    Panel* leftSidebar = new Panel();
    leftSidebar->id = "left_sidebar";
    leftSidebar->hDock = HDock::Left;
    leftSidebar->vDock = VDock::None;
    leftSidebar->widthMode = SizeMode::Absolute;
    leftSidebar->width = 140.0f;
    leftSidebar->heightMode = SizeMode::Stretch;
    leftSidebar->backgroundColor = Color(50,50,60,220);
    leftSidebar->parent = demoWin;

    // Add a vertical stack of buttons in the left sidebar (hDock=Left so they stack top-to-bottom)
    for (int i = 0; i < 6; ++i) {
        Button* b = new Button();
        b->id = std::string("ls") + std::to_string(i);
        b->label = std::string("Btn") + std::to_string(i+1);
        b->backgroundColor = Color(90 + i*2,90 + i*2,110 + i*2,255);
        b->hDock = HDock::Left;
        b->widthMode = SizeMode::Absolute;
        b->width = 120.0f;
        b->heightMode = SizeMode::Absolute;
        b->height = 32.0f;
        b->marginTop = 6.0f;
        b->parent = leftSidebar;
        leftSidebar->content.push_back(b);
    }

    // Center content panel (remaining area)
    Panel* centerContent = new Panel();
    centerContent->id = "center_area";
    centerContent->hDock = HDock::None;
    centerContent->vDock = VDock::None;
    centerContent->widthMode = SizeMode::Stretch;
    centerContent->heightMode = SizeMode::Stretch;
    centerContent->backgroundColor = Color(200,12,200,50);
    centerContent->parent = demoWin;

    // Add these panels to the demo window
    demoWin->content.push_back(topToolbar);
    demoWin->content.push_back(leftSidebar);
    demoWin->content.push_back(centerContent);

    // Add demo window to manager
    uiManager.addWindow(demoWin);

    // --- New label + scrolling demos ---
    // Long mythos text split across multiple elements
    const std::string mythos =
        "Nyarlathotep moved among the cities like a shadow that remembers its own name. "
        "He whispered to men on sleepless nights, and his words became storms on the horizon. "
        "In the deep places where the sea forgets itself, something answered: a vast, slow dreaming.\n";

    // Panel A: word-wrapped single label (multiple lines)
    Panel* panelA = new Panel();
    panelA->id = "panelA_wrap";
    panelA->widthMode = SizeMode::Absolute;
    panelA->heightMode = SizeMode::Absolute;
    panelA->width = 260;
    panelA->height = 120;
    panelA->x = 10.0f;
    panelA->y = 430.0f;
    panelA->backgroundColor = Color(20,20,40,200);
    panelA->movable = true;
    // Label with word wrap
    Label* la = new Label();
    la->parent = panelA;
    la->x = panelA->getSafeContentLeft();
    la->y = panelA->getSafeContentTop();
    la->widthMode = SizeMode::Stretch;
    la->heightMode = SizeMode::Auto;
    la->text = mythos + "It spoke of C'thulu, a shape beneath the waves.";
    la->wordWrap = true;
    la->fontSize = 14;
    panelA->content.push_back(la);
    uiManager.addPanel(panelA);

    // Panel B: multiple labels without word wrap (each label is a line)
    Panel* panelB = new Panel();
    panelB->id = "panelB_nowrap";
    panelB->widthMode = SizeMode::Absolute;
    panelB->heightMode = SizeMode::Absolute;
    panelB->width = 260;
    panelB->height = 120;
    panelB->x = 280.0f;
    panelB->y = 430.0f;
    panelB->backgroundColor = Color(30,20,40,200);
    panelB->movable = true;
    // multiple labels
    std::vector<std::string> lines = {
        "Nyarlathotep whispered to kings.",
        "C'thulu dreamed in the drowned city.",
        "The stars leaned closer than they ought.",
        "A name unspoken gnawed at the dark."};
    float ly = panelB->getSafeContentTop();
    for (auto &s : lines) {
        Label* l = new Label();
        l->parent = panelB;
        l->x = panelB->getSafeContentLeft();
        l->y = ly;
        l->manualPosition = true;
        l->localX = l->x - panelB->getSafeContentLeft();
        l->localY = l->y - panelB->getSafeContentTop();
        l->text = s;
        l->wordWrap = false;
        l->fontSize = 14;
        l->widthMode = SizeMode::Auto;
        l->heightMode = SizeMode::Auto;
        panelB->content.push_back(l);
        ly += 20.0f;
    }
    uiManager.addPanel(panelB);

    // Window C: scrollable vertically with a single huge wrapped label
    Window* winC = new Window();
    winC->id = "winC_scroll_v";
    winC->src = "testwindows.9.png";
    winC->title = "Scrollable Window (Vertical)";
    winC->widthMode = SizeMode::Absolute;
    winC->heightMode = SizeMode::Absolute;
    winC->width = 300;
    winC->height = 180;
    winC->x = 10.0f;
    winC->y = 560.0f;
    winC->movable = true;
    winC->resizable = true;
    winC->backgroundColor = Color(40,40,60,220);
    winC->scrollable = "vertical";
    // Add a long label that will overflow vertically
    Label* lc = new Label();
    lc->parent = winC;
    lc->x = winC->getSafeContentLeft();
    lc->y = winC->getSafeContentTop();
    lc->widthMode = SizeMode::Stretch;
    lc->heightMode = SizeMode::Stretch;
    lc->textColor = Color(0,0,0,255);
    lc->text = mythos + mythos + "And the sea gave up one slow breath after another.";
    lc->wordWrap = true;
    lc->fontSize = 14;
    winC->content.push_back(lc);
    uiManager.addWindow(winC);

    // Window D: scrollable both directions with multiple large labels
    Window* winD = new Window();
    winD->id = "winD_scroll_both";
    winD->title = "Scrollable Window (Both)";
    winD->widthMode = SizeMode::Absolute;
    winD->heightMode = SizeMode::Absolute;
    winD->backgroundColor = Color(40,40,60,220);
    winD->width = 300;
    winD->height = 180;
    winD->x = 330.0f;
    winD->y = 560.0f;
    winD->movable = true;
    winD->resizable = true;
    winD->scrollable = "both";
    // Create several oversized labels
    for (int i=0;i<4;++i) {
        Label* ld = new Label();
        ld->parent = winD;
        ld->x = winD->getSafeContentLeft() + i * 220.0f; // stagger horizontally to force horizontal scroll
        ld->y = winD->getSafeContentTop() + i * 80.0f; // stagger vertically to force vertical scroll
        ld->text = "Nyarlathotep and C'thulu: the tale grows and wraps where the mind cannot follow. " + std::to_string(i+1);
        ld->wordWrap = false;
        ld->fontSize = 16;
        ld->width = 200.0f;
        ld->height = 60.0f;
        ld->widthMode = SizeMode::Absolute;
        ld->heightMode = SizeMode::Absolute;
        winD->content.push_back(ld);
    }
    uiManager.addWindow(winD);

    Button* Button1 = new Button();
    Button1->id = "button1";
    Button1->label = "Open Modal";
    Button1->backgroundColor = Color(25, 40, 200, 255);
    Button1->cornerRadius = 8.0f;
    Button1->vDock = VDock::Top;
    Button1->hDock = HDock::Center;
    Button1->widthMode = SizeMode::Auto;
    Button1->parent = panel2;
    // When clicked, create a new modal window with a Close button inside it.
    Button1->onClick = [&uiManager, sdlWindow](UIElement* src) {
        // Create modal window
        Window* modal = new Window();
        modal->id = "modal_dialog";
        modal->title = "Modal Dialog";
        modal->modal = true;
        modal->widthMode = SizeMode::Absolute;
        modal->heightMode = SizeMode::Absolute;
        modal->width = 360;
        modal->height = 180;
        modal->movable = false;
        modal->closeable = false; // we'll provide an internal close button
        modal->zIndex = 100; // ensure on top

        // Center the modal in the application window
        int ww = 800, wh = 600;
        if (sdlWindow) SDL_GetWindowSize(sdlWindow, &ww, &wh);
        modal->x = static_cast<float>((ww - modal->width) / 2);
        modal->y = static_cast<float>((wh - modal->height) / 2);

        // Add a close button inside the modal
        Button* closeBtn = new Button();
        closeBtn->id = "modal_close";
        closeBtn->label = "Close";
        closeBtn->backgroundColor = Color(200, 25, 25, 255);
        closeBtn->cornerRadius = 8.0f;
        closeBtn->vDock = VDock::Bottom;
        closeBtn->hDock = HDock::Center;
        closeBtn->parent = modal;
        // Hook: remove modal from manager and delete it
        closeBtn->onClick = [&uiManager, modal](UIElement* src) {
            // Remove and delete the modal window
            uiManager.removeWindow(modal);
            delete modal;
        };

        // Positioning: let layout manage it, but give the button a default size
        closeBtn->widthMode = SizeMode::Absolute;
        closeBtn->heightMode = SizeMode::Absolute;
        closeBtn->width = 80;
        closeBtn->height = 30;
        // Add to modal content
        modal->content.push_back(closeBtn);

        // Add modal to manager so it becomes interactive
        uiManager.addWindow(modal);
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

            // Window resize events are handled internally by UIManager (recomputes layout)

            // Let the UI manager handle all mouse/events
            uiManager.handleEvent(e);
        }

    // Update manager (runs layout internally and queries the native
    // window size when available).
    uiManager.Update(dt);

        // Rendering through the manager's Render method
        SDL_SetRenderDrawColor(sdlRenderer, 40, 40, 40, 255);
        SDL_RenderClear(sdlRenderer);
    uiManager.Render(static_cast<BangUI::API::IRenderer*>(&rendererImpl));
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
