#pragma once

#include "../api/UIElement.h"
#include "DockPolicy.h"
#include "../models/Mesh.h"
#include <functional>
#include <string>
#include <vector>
#include <algorithm>

struct SDL_Renderer;

namespace BangUI::impl {

class WindowImpl : public BangUI::API::UIElement {
public:
    struct ChromeButton {
        enum class Type { Close, Minimize, Maximize, Menu, Custom };
        enum class Anchor { Left, Right };

        Type type{Type::Close};
        Anchor anchor{Anchor::Right};
        bool visible{true};
        Color backgroundColor = Color(200, 60, 60, 255);
        Color iconColor = Color(255, 255, 255, 255);
        float width{20.0f};
        float height{20.0f};
        float cornerRadius{6.0f};
        float padding{6.0f};
        std::function<void(WindowImpl*)> onClick;
        std::function<void(SDL_Renderer*, const ChromeButton&, float, float, float, float)> customDraw;

        static ChromeButton CloseDefault() {
            ChromeButton btn;
            btn.type = Type::Close;
            btn.anchor = Anchor::Right;
            btn.backgroundColor = Color(200, 60, 60, 255);
            btn.iconColor = Color(255, 255, 255, 255);
            return btn;
        }
    };

    struct ChromeButtonLayout {
        const ChromeButton* button{nullptr};
        float x{0.0f};
        float y{0.0f};
        float width{0.0f};
        float height{0.0f};
    };

    WindowImpl() {
        type = "Window";
        chromeButtons.push_back(ChromeButton::CloseDefault());
    }

    std::string title;
    bool resizable = false;
    bool movable = true;
    bool modal = false;
    bool closeable = false;
    bool isMinimized = false;
    bool isMaximizedState = false;
    bool hasStoredBounds = false;
    float storedX = 0.0f;
    float storedY = 0.0f;
    float storedWidth = 0.0f;
    float storedHeight = 0.0f;
    bool delegateMoveToNative = false;
    std::function<void()> beginNativeDragCallback;
    bool resizeHandleHot = false;

    DockPolicy dockPolicy = DockPolicy::ReserveStrips;
    std::vector<UIElement*> content;
    std::string renderSpace = "orthographic";
    Mesh* anchorMesh = nullptr;
    std::string anchorBone;
    std::string scrollable;

    float scrollX = 0.0f;
    float scrollY = 0.0f;

    float scrollBarFadeTimerX = 0.0f;
    float scrollBarFadeTimerY = 0.0f;
    const float scrollBarFadeDuration = 0.8f;

    Color titleBarColor = Color(80, 80, 120, 255);
    std::string closeSrc;
    std::string bodyTexture;
    std::string titleBarTexture;
    bool titleBarVisible = true;

    static constexpr float TITLE_BAR_HEIGHT = 30.0f;
    static constexpr float RESIZE_HANDLE_SIZE = 16.0f;
    static constexpr float CHROME_EDGE_PADDING = 12.0f;

    bool isPointInTitleBar(float px, float py) const {
        return px >= x && px <= x + width &&
               py >= y && py <= y + TITLE_BAR_HEIGHT;
    }

    bool isPointInResizeHandle(float px, float py) const {
        if (!resizable) return false;
        float insetX = std::max(0.0f, contentRight + paddingRight);
        float insetY = std::max(0.0f, contentBottom + paddingBottom);
        float handleX = x + width - RESIZE_HANDLE_SIZE - insetX;
        float handleY = y + height - RESIZE_HANDLE_SIZE - insetY;
        float maxX = x + width - insetX;
        float maxY = y + height - insetY;
        return px >= handleX && px <= maxX &&
               py >= handleY && py <= maxY;
    }

    float getSafeContentLeft() const { return x + contentLeft + paddingLeft; }
    float getSafeContentTop() const { return y + TITLE_BAR_HEIGHT + contentTop + paddingTop; }
    float getSafeContentRight() const { return x + width - contentRight - paddingRight; }
    float getSafeContentBottom() const { return y + height - contentBottom - paddingBottom; }

    float getTitleTextLeftBound() const {
        float left = x + CHROME_EDGE_PADDING;
        for (const auto& btn : chromeButtons) {
            if (!btn.visible || btn.anchor != ChromeButton::Anchor::Left) continue;
            left += btn.width + btn.padding;
        }
        return left;
    }

    float getTitleTextRightBound() const {
        float right = x + width - CHROME_EDGE_PADDING;
        for (const auto& btn : chromeButtons) {
            if (!btn.visible || btn.anchor != ChromeButton::Anchor::Right) continue;
            right -= btn.width;
            right -= btn.padding;
        }
        return right;
    }

    void ensureMinSize(float minW = 100.0f, float minH = 50.0f) {
        if (width < minW) width = minW;
        if (height < minH) height = minH;
    }

    bool isModal() const { return modal; }

    bool hasDiscreteTitleBar() const {
        if (!titleBarVisible) return false;
        if (!titleBarTexture.empty()) return true;
        return titleBarColor.a > 0;
    }

    void addChromeButton(const ChromeButton& button) {
        chromeButtons.push_back(button);
    }

    void clearChromeButtons() {
        chromeButtons.clear();
    }

    std::vector<ChromeButton>& getChromeButtons() { return chromeButtons; }
    const std::vector<ChromeButton>& getChromeButtons() const { return chromeButtons; }

    void computeChromeLayout(std::vector<ChromeButtonLayout>& outLayouts) const {
        outLayouts.clear();
        if (chromeButtons.empty()) return;

        float anchorTop = hasDiscreteTitleBar() ? y : y + 4.0f;
        float anchorHeight = hasDiscreteTitleBar() ? TITLE_BAR_HEIGHT : std::min(TITLE_BAR_HEIGHT, height);
        float leftCursor = x + CHROME_EDGE_PADDING;
        float rightCursor = x + width - CHROME_EDGE_PADDING;

        std::vector<const ChromeButton*> leftButtons;
        std::vector<const ChromeButton*> rightButtons;
        leftButtons.reserve(chromeButtons.size());
        rightButtons.reserve(chromeButtons.size());
        for (const auto& btn : chromeButtons) {
            if (!btn.visible) continue;
            if (btn.anchor == ChromeButton::Anchor::Left) {
                leftButtons.push_back(&btn);
            } else {
                rightButtons.push_back(&btn);
            }
        }

        auto pushLayout = [&](const ChromeButton* btn, bool fromLeft) {
            if (!btn) return;
            float btnW = btn->width;
            float btnH = btn->height;
            float posY = anchorTop + (anchorHeight - btnH) / 2.0f;
            float posX = 0.0f;
            if (fromLeft) {
                posX = leftCursor;
                leftCursor += btnW + btn->padding;
            } else {
                posX = rightCursor - btnW;
                rightCursor = posX - btn->padding;
            }
            outLayouts.push_back(ChromeButtonLayout{btn, posX, posY, btnW, btnH});
        };

        for (const ChromeButton* btn : leftButtons) {
            pushLayout(btn, true);
        }
        for (auto it = rightButtons.rbegin(); it != rightButtons.rend(); ++it) {
            pushLayout(*it, false);
        }
    }

    void rememberBounds() {
        storedX = x;
        storedY = y;
        storedWidth = width;
        storedHeight = height;
        hasStoredBounds = true;
    }

    void restoreBounds() {
        if (!hasStoredBounds) return;
        x = storedX;
        y = storedY;
        width = storedWidth;
        height = storedHeight;
    }

private:
    std::vector<ChromeButton> chromeButtons;
};

} // namespace BangUI::impl
