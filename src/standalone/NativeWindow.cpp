#include "NativeWindow.h"

#include <algorithm>
#include <cstring>
#include <windowsx.h>

namespace BangUI::Standalone {

using BangUI::API::UIEvent;
using BangUI::API::UIEventType;

namespace {
    constexpr wchar_t kWindowClassName[] = L"BangUIStandaloneWindow";

    uint32_t getKeyModifiers() {
        uint32_t mods = 0;
        if (GetKeyState(VK_SHIFT) & 0x8000) mods |= 1u << 0;
        if (GetKeyState(VK_CONTROL) & 0x8000) mods |= 1u << 1;
        if (GetKeyState(VK_MENU) & 0x8000) mods |= 1u << 2;
        return mods;
    }
}

NativeWindow::NativeWindow() = default;
NativeWindow::~NativeWindow() {
    if (hwnd) {
        DestroyWindow(hwnd);
        hwnd = nullptr;
    }
}

bool NativeWindow::create(const std::wstring& title, int width, int height) {
    HINSTANCE instance = GetModuleHandleW(nullptr);
    WNDCLASSW wc{};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = NativeWindow::WndProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = kWindowClassName;

    static bool registered = false;
    if (!registered) {
        if (!RegisterClassW(&wc)) {
            return false;
        }
        registered = true;
    }

    DWORD style = borderless ? WS_POPUP : WS_OVERLAPPEDWINDOW;
    DWORD exStyle = WS_EX_APPWINDOW;
    if (borderless) {
        style |= WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
        if (resizable) {
            style |= WS_THICKFRAME;
        }
        exStyle |= WS_EX_LAYERED;
    }

    RECT rect{0, 0, width, height};
    if (!borderless) {
        AdjustWindowRectEx(&rect, style, FALSE, exStyle);
    } else {
        rect.right = width;
        rect.bottom = height;
    }

    hwnd = CreateWindowExW(
        exStyle,
        kWindowClassName,
        title.c_str(),
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        instance,
        this);

    if (!hwnd) {
        return false;
    }

    std::memset(&bitmapHeader, 0, sizeof(bitmapHeader));
    bitmapHeader.bV4Size = sizeof(BITMAPV4HEADER);
    bitmapHeader.bV4Width = width;
    bitmapHeader.bV4Height = -height; // top-down
    bitmapHeader.bV4Planes = 1;
    bitmapHeader.bV4BitCount = 32;
    bitmapHeader.bV4V4Compression = BI_BITFIELDS;
    bitmapHeader.bV4RedMask = 0x00FF0000;
    bitmapHeader.bV4GreenMask = 0x0000FF00;
    bitmapHeader.bV4BlueMask = 0x000000FF;
    bitmapHeader.bV4AlphaMask = 0xFF000000;
    bitmapHeader.bV4CSType = LCS_WINDOWS_COLOR_SPACE;

    widthPx = width;
    heightPx = height;
    open = true;
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    return true;
}

void NativeWindow::show() {
    if (!hwnd) return;
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
}

bool NativeWindow::pollEvent(UIEvent& outEvent) {
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            UIEvent quitEvent;
            quitEvent.type = UIEventType::Quit;
            pushEvent(quitEvent);
        } else {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    if (pendingEvents.empty()) {
        return false;
    }

    outEvent = pendingEvents.front();
    pendingEvents.pop();
    return true;
}

void NativeWindow::present(SDL_Renderer* renderer) {
    if (!hwnd || !renderer) return;
    int w = SDL_RendererWidth(renderer);
    int h = SDL_RendererHeight(renderer);
    const auto& pixels = SDL_RendererPixels(renderer);
    if (w <= 0 || h <= 0 || pixels.empty()) return;

    widthPx = w;
    heightPx = h;
    bitmapHeader.bV4Width = w;
    bitmapHeader.bV4Height = -h;

    blitBuffer = pixels;
    auto convertToBGRA = [&](bool premultiplyAlpha) {
        if (blitBuffer.empty()) return;
        const size_t pixelCount = blitBuffer.size() / 4;
        for (size_t i = 0; i < pixelCount; ++i) {
            size_t idx = i * 4;
            uint8_t r = blitBuffer[idx + 0];
            uint8_t g = blitBuffer[idx + 1];
            uint8_t b = blitBuffer[idx + 2];
            uint8_t a = blitBuffer[idx + 3];
            if (premultiplyAlpha) {
                auto premul = [&](uint8_t c) -> uint8_t {
                    return static_cast<uint8_t>((static_cast<uint16_t>(c) * a + 254) / 255);
                };
                r = premul(r);
                g = premul(g);
                b = premul(b);
            }
            blitBuffer[idx + 0] = b;
            blitBuffer[idx + 1] = g;
            blitBuffer[idx + 2] = r;
            blitBuffer[idx + 3] = a;
        }
    };

    if (borderless) {
        convertToBGRA(true);
        HDC screenDC = GetDC(nullptr);
        HDC memDC = nullptr;
        HBITMAP dib = nullptr;
        void* bits = nullptr;
        BITMAPINFO* bmiInfo = reinterpret_cast<BITMAPINFO*>(&bitmapHeader);
        if (screenDC) {
            memDC = CreateCompatibleDC(screenDC);
            dib = CreateDIBSection(screenDC, bmiInfo, DIB_RGB_COLORS, &bits, nullptr, 0);
        }
        if (memDC && dib && bits) {
            memcpy(bits, blitBuffer.data(), blitBuffer.size());
            HGDIOBJ old = SelectObject(memDC, dib);
            POINT srcPt = {0, 0};
            SIZE wndSz = {w, h};
            BLENDFUNCTION blend{};
            blend.BlendOp = AC_SRC_OVER;
            blend.BlendFlags = 0;
            blend.SourceConstantAlpha = 255;
            blend.AlphaFormat = AC_SRC_ALPHA;
            UpdateLayeredWindow(hwnd, screenDC, nullptr, &wndSz, memDC, &srcPt, 0, &blend, ULW_ALPHA);
            SelectObject(memDC, old);
        }
        if (dib) DeleteObject(dib);
        if (memDC) DeleteDC(memDC);
        if (screenDC) ReleaseDC(nullptr, screenDC);
    } else {
        convertToBGRA(false);
        HDC hdc = GetDC(hwnd);
        BITMAPINFO* bmiInfo = reinterpret_cast<BITMAPINFO*>(&bitmapHeader);
        StretchDIBits(
            hdc,
            0,
            0,
            w,
            h,
            0,
            0,
            w,
            h,
            blitBuffer.data(),
            bmiInfo,
            DIB_RGB_COLORS,
            SRCCOPY);
        ReleaseDC(hwnd, hdc);
    }
}

void NativeWindow::pushEvent(const UIEvent& ev) {
    if (ev.type == UIEventType::WindowResize) {
        std::queue<UIEvent> filtered;
        while (!pendingEvents.empty()) {
            UIEvent existing = pendingEvents.front();
            pendingEvents.pop();
            if (existing.type != UIEventType::WindowResize) {
                filtered.push(existing);
            }
        }
        pendingEvents.swap(filtered);
    }
    pendingEvents.push(ev);
}

void NativeWindow::handleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CLOSE: {
        UIEvent ev;
        ev.type = UIEventType::Quit;
        pushEvent(ev);
        break;
    }
    case WM_DESTROY:
        pushEvent(UIEvent{UIEventType::Quit});
        open = false;
        break;
    case WM_SIZE: {
        widthPx = LOWORD(lParam);
        heightPx = HIWORD(lParam);
        UIEvent ev;
        ev.type = UIEventType::WindowResize;
        ev.width = widthPx;
        ev.height = heightPx;
        pushEvent(ev);
        break;
    }
    case WM_MOUSEMOVE: {
        UIEvent ev;
        ev.type = UIEventType::PointerMove;
        ev.mouseX = static_cast<float>(GET_X_LPARAM(lParam));
        ev.mouseY = static_cast<float>(GET_Y_LPARAM(lParam));
        ev.modifiers = getKeyModifiers();
        pushEvent(ev);
        break;
    }
    case WM_LBUTTONDOWN: {
        SetCapture(hwnd);
        UIEvent ev;
        ev.type = UIEventType::PointerDown;
        ev.mouseX = static_cast<float>(GET_X_LPARAM(lParam));
        ev.mouseY = static_cast<float>(GET_Y_LPARAM(lParam));
        ev.modifiers = getKeyModifiers();
        pushEvent(ev);
        break;
    }
    case WM_LBUTTONUP: {
        ReleaseCapture();
        UIEvent ev;
        ev.type = UIEventType::PointerUp;
        ev.mouseX = static_cast<float>(GET_X_LPARAM(lParam));
        ev.mouseY = static_cast<float>(GET_Y_LPARAM(lParam));
        ev.modifiers = getKeyModifiers();
        pushEvent(ev);
        break;
    }
    case WM_MOUSEWHEEL: {
        UIEvent ev;
        ev.type = UIEventType::PointerWheel;
        ev.wheelY = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam));
        ev.mouseX = static_cast<float>(GET_X_LPARAM(lParam));
        ev.mouseY = static_cast<float>(GET_Y_LPARAM(lParam));
        ev.modifiers = getKeyModifiers();
        pushEvent(ev);
        break;
    }
    case WM_MOUSEHWHEEL: {
        UIEvent ev;
        ev.type = UIEventType::PointerWheel;
        ev.wheelX = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam));
        ev.mouseX = static_cast<float>(GET_X_LPARAM(lParam));
        ev.mouseY = static_cast<float>(GET_Y_LPARAM(lParam));
        ev.modifiers = getKeyModifiers();
        pushEvent(ev);
        break;
    }
    case WM_KEYDOWN: {
        UIEvent ev;
        ev.type = UIEventType::KeyDown;
        ev.key = static_cast<uint32_t>(wParam);
        ev.modifiers = getKeyModifiers();
        pushEvent(ev);
        break;
    }
    case WM_KEYUP: {
        UIEvent ev;
        ev.type = UIEventType::KeyUp;
        ev.key = static_cast<uint32_t>(wParam);
        ev.modifiers = getKeyModifiers();
        pushEvent(ev);
        break;
    }
    case WM_SIZING: {
        RECT* rect = reinterpret_cast<RECT*>(lParam);
        if (rect) {
            int newWidth = rect->right - rect->left;
            int newHeight = rect->bottom - rect->top;
            widthPx = newWidth;
            heightPx = newHeight;
            UIEvent ev;
            ev.type = UIEventType::WindowResize;
            ev.width = newWidth;
            ev.height = newHeight;
            pushEvent(ev);
            if (!interactiveResizeActive) {
                interactiveResizeActive = true;
                SetTimer(hwnd, 1, 16, nullptr);
            }
        }
        break;
    }
case WM_EXITSIZEMOVE: {
    if (interactiveResizeActive) {
        KillTimer(hwnd, 1);
        interactiveResizeActive = false;
    }
    if (resizeCallback) resizeCallback();
    break;
}
case WM_TIMER: {
    if (wParam == 1 && interactiveResizeActive && resizeCallback) {
        resizeCallback();
    }
    break;
}
    default:
        break;
    }
}

LRESULT CALLBACK NativeWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    NativeWindow* self = nullptr;
    if (msg == WM_NCCREATE) {
        auto createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = reinterpret_cast<NativeWindow*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<NativeWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (self) {
        if (self->borderless) {
            if (msg == WM_NCCALCSIZE) {
                if (wParam && lParam) {
                    NCCALCSIZE_PARAMS* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
                    HMONITOR monitor = MonitorFromRect(&params->rgrc[0], MONITOR_DEFAULTTONEAREST);
                    MONITORINFO info{};
                    info.cbSize = sizeof(info);
                    if (GetMonitorInfo(monitor, &info)) {
                        params->rgrc[0] = info.rcWork;
                    }
                    return 0;
                }
            }
            if (msg == WM_NCPAINT) {
                return 0;
            }
            if (msg == WM_GETMINMAXINFO) {
                MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
                HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
                MONITORINFO info{};
                info.cbSize = sizeof(info);
                if (GetMonitorInfo(monitor, &info)) {
                    int workWidth = info.rcWork.right - info.rcWork.left;
                    int workHeight = info.rcWork.bottom - info.rcWork.top;
                    mmi->ptMaxPosition.x = info.rcWork.left - info.rcMonitor.left;
                    mmi->ptMaxPosition.y = info.rcWork.top - info.rcMonitor.top;
                    mmi->ptMaxSize.x = workWidth;
                    mmi->ptMaxSize.y = workHeight;
                    mmi->ptMaxTrackSize = mmi->ptMaxSize;
                }
                return 0;
            }
        }
        if (msg == WM_NCHITTEST && self->borderless) {
            POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hwnd, &pt);
            RECT client{};
            GetClientRect(hwnd, &client);
            int width = client.right - client.left;
            int height = client.bottom - client.top;
            bool allowResize = self->resizable && self->resizeBorder > 0;
            int margin = self->resizeBorder;
            bool onLeft = allowResize && pt.x >= 0 && pt.x < margin;
            bool onRight = allowResize && pt.x <= width && pt.x > width - margin;
            bool onTop = allowResize && pt.y >= 0 && pt.y < margin;
            bool onBottom = allowResize && pt.y <= height && pt.y > height - margin;
            if (allowResize) {
                if (onTop && onLeft && self->topResizeEnabled) return HTTOPLEFT;
                if (onTop && onRight && self->topResizeEnabled) return HTTOPRIGHT;
                if (onBottom && onLeft) return HTBOTTOMLEFT;
                if (onBottom && onRight) return HTBOTTOMRIGHT;
                if (onTop && self->topResizeEnabled) return HTTOP;
                if (onBottom) return HTBOTTOM;
                if (onLeft) return HTLEFT;
                if (onRight) return HTRIGHT;
            }
            bool withinCaptionBand = self->captionHitHeight > 0 && pt.y >= 0 && pt.y < self->captionHitHeight;
            bool withinCaptionRange = pt.x >= self->captionExcludeLeft && pt.x <= width - self->captionExcludeRight;
            if (withinCaptionBand && withinCaptionRange) {
                return HTCAPTION;
            }
            return HTCLIENT;
        }
        self->handleMessage(msg, wParam, lParam);
    }

    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void NativeWindow::minimize() {
    if (!hwnd) return;
    ShowWindow(hwnd, SW_MINIMIZE);
}

bool NativeWindow::isMaximized() const {
    if (!hwnd) return false;
    return IsZoomed(hwnd) != FALSE;
}

void NativeWindow::toggleMaximize() {
    if (!hwnd) return;
    if (isMaximized()) {
        ShowWindow(hwnd, SW_RESTORE);
    } else {
        ShowWindow(hwnd, SW_MAXIMIZE);
    }
}

void NativeWindow::close() {
    if (!hwnd) return;
    PostMessage(hwnd, WM_CLOSE, 0, 0);
}

void NativeWindow::beginNativeDrag() {
    if (!hwnd) return;
    ReleaseCapture();
    SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
}

} // namespace BangUI::Standalone
