#pragma once

#include <windows.h>
#include <string>
#include <cstdint>

namespace GUPet {

class SpeechBubble {
public:
    SpeechBubble();
    ~SpeechBubble();

    bool Create();
    void Destroy();

    void Show(const std::wstring& text, int anchorX, int anchorY, int petW, int petH);
    void Hide();
    void HideImmediate();
    void Update(int anchorX, int anchorY, int petW, int petH);

    bool IsVisible() const { return m_visible; }
    HWND GetHwnd() const { return m_hwnd; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void Render();
    void UpdatePosition(int anchorX, int anchorY, int petW, int petH);

    HWND m_hwnd = NULL;
    std::wstring m_text;
    bool m_visible = false;
    uint64_t m_showUntil = 0;
    float m_alpha = 0.0f;
    bool m_fadingIn = false;
    bool m_fadingOut = false;
    int m_width = 0;
    int m_height = 0;
    int m_x = 0;
    int m_y = 0;
};

} // namespace GUPet
