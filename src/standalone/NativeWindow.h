#pragma once

#include <cstdint>
#include <queue>
#include <string>
#include <vector>
#include <functional>

#include <windows.h>

#include "../api/Events.h"
#include "SoftwareRenderer.h"

namespace BangUI::Standalone {

class NativeWindow {
public:
    NativeWindow();
    ~NativeWindow();

    bool create(const std::wstring& title, int width, int height);
    void show();
    bool pollEvent(BangUI::API::UIEvent& outEvent);
    void present(SDL_Renderer* renderer);
    bool isOpen() const { return open; }
    int clientWidth() const { return widthPx; }
    int clientHeight() const { return heightPx; }
    void minimize();
    void toggleMaximize();
    bool isMaximized() const;
    void close();
    void setCaptionHitHeight(int h) { captionHitHeight = h; }
    void setResizable(bool enable) { resizable = enable; }
    bool isResizable() const { return resizable; }
    void setResizeBorder(int px) { resizeBorder = px; }
    bool isBorderless() const { return borderless; }
    void setResizeCallback(std::function<void()> cb) { resizeCallback = std::move(cb); }
    void setCaptionExclusion(int leftPixels, int rightPixels) { captionExcludeLeft = leftPixels; captionExcludeRight = rightPixels; }
    void beginNativeDrag();
    void setTopResizeEnabled(bool enable) { topResizeEnabled = enable; }

private:
    HWND hwnd{nullptr};
    bool open{false};
    int widthPx{0};
    int heightPx{0};
    BITMAPV4HEADER bitmapHeader{};
    std::vector<uint8_t> blitBuffer;
    std::queue<BangUI::API::UIEvent> pendingEvents;
    bool borderless{true};
    bool resizable{true};
    int captionHitHeight{48};
    int resizeBorder{8};
    std::function<void()> resizeCallback;
    bool interactiveResizeActive{false};
    int captionExcludeLeft{0};
    int captionExcludeRight{0};
    bool topResizeEnabled{true};

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void handleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    void pushEvent(const BangUI::API::UIEvent& ev);
};

} // namespace BangUI::Standalone
