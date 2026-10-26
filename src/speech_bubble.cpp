#include "speech_bubble.h"

#include <windows.h>
#include <gdiplus.h>
#include <algorithm>

namespace GUPet {

using namespace Gdiplus;

static const wchar_t* BUBBLE_CLASS_NAME = L"GUPetSpeechBubbleClass";

SpeechBubble::SpeechBubble() {}

SpeechBubble::~SpeechBubble() {
    Destroy();
}

bool SpeechBubble::Create() {
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = BUBBLE_CLASS_NAME;
    RegisterClassExW(&wc);

    m_hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT,
        BUBBLE_CLASS_NAME,
        L"GUPet — реплика",
        WS_POPUP,
        0, 0, 10, 10,
        NULL, NULL, GetModuleHandle(NULL), this
    );

    return m_hwnd != NULL;
}

void SpeechBubble::Destroy() {
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }
}

LRESULT CALLBACK SpeechBubble::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* cs = (CREATESTRUCTW*)lParam;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        return TRUE;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void SpeechBubble::Show(const std::wstring& text, int anchorX, int anchorY, int petW, int petH) {
    if (!m_hwnd) return;

    m_text = text;
    m_visible = true;
    m_alpha = 0.05f;
    m_fadingIn = true;
    m_fadingOut = false;
    m_showUntil = GetTickCount64() + 2200 + text.length() * 55;

    HDC hdcScreen = GetDC(NULL);
    Graphics g(hdcScreen);
    FontFamily fontFamily(L"Segoe UI");
    Font font(&fontFamily, 9.5f, FontStyleRegular, UnitPoint);
    RectF layoutRect(0, 0, 240, 200);
    StringFormat format;
    format.SetAlignment(StringAlignmentCenter);
    format.SetLineAlignment(StringAlignmentCenter);

    RectF bounds;
    g.MeasureString(m_text.c_str(), (int)m_text.length(), &font, layoutRect, &format, &bounds);
    ReleaseDC(NULL, hdcScreen);

    m_width = std::max(56, static_cast<int>(bounds.Width) + 24);
    m_height = std::max(30, static_cast<int>(bounds.Height) + 16);

    UpdatePosition(anchorX, anchorY, petW, petH);
    ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
    Render();
}

void SpeechBubble::Hide() {
    if (!m_visible) return;
    m_fadingIn = false;
    m_fadingOut = true;
}

void SpeechBubble::UpdatePosition(int anchorX, int anchorY, int petW, int petH) {
    RECT workArea;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);

    m_x = anchorX + petW / 2 - m_width / 2;
    m_x = std::max((int)workArea.left + 8, std::min((int)workArea.right - m_width - 8, m_x));

    m_y = anchorY - m_height - 6;
    if (m_y < workArea.top + 8) {
        m_y = anchorY + petH + 6;
    }
    m_y = std::max((int)workArea.top + 8, std::min((int)workArea.bottom - m_height - 8, m_y));
}

void SpeechBubble::Update(int anchorX, int anchorY, int petW, int petH) {
    if (!m_visible) return;

    uint64_t now = GetTickCount64();
    if (m_fadingIn) {
        m_alpha += 0.15f;
        if (m_alpha >= 1.0f) {
            m_alpha = 1.0f;
            m_fadingIn = false;
        }
        Render();
    } else if (m_fadingOut) {
        m_alpha -= 0.12f;
        if (m_alpha <= 0.0f) {
            m_alpha = 0.0f;
            m_visible = false;
            m_fadingOut = false;
            ShowWindow(m_hwnd, SW_HIDE);
            return;
        }
        Render();
    } else {
        if (now >= m_showUntil) {
            Hide();
            return;
        }
    }

    UpdatePosition(anchorX, anchorY, petW, petH);
    SetWindowPos(m_hwnd, HWND_TOPMOST, m_x, m_y, m_width, m_height,
                 SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOSIZE);
}

void SpeechBubble::Render() {
    if (!m_hwnd || m_width <= 0 || m_height <= 0) return;

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = m_width;
    bmi.bmiHeader.biHeight = -m_height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    void* pBits = nullptr;
    HBITMAP hBmp = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcMem, hBmp);

    Bitmap bmp(m_width, m_height, m_width * 4, PixelFormat32bppPARGB, (BYTE*)pBits);
    Graphics g(&bmp);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

    int alpha = static_cast<int>(m_alpha * 240.0f);
    int bgAlpha = static_cast<int>(m_alpha * 245.0f);

    float r = 12.0f;
    float w = (float)m_width;
    float h = (float)m_height;

    GraphicsPath path;
    path.AddArc(1.0f, 1.0f, r * 2.0f, r * 2.0f, 180, 90);
    path.AddArc(w - 1.0f - r * 2.0f, 1.0f, r * 2.0f, r * 2.0f, 270, 90);
    path.AddArc(w - 1.0f - r * 2.0f, h - 1.0f - r * 2.0f, r * 2.0f, r * 2.0f, 0, 90);
    path.AddArc(1.0f, h - 1.0f - r * 2.0f, r * 2.0f, r * 2.0f, 90, 90);
    path.CloseFigure();

    SolidBrush bgBrush(Color(bgAlpha, 255, 251, 248));
    g.FillPath(&bgBrush, &path);

    Pen borderPen(Color(alpha, 220, 205, 198), 1.2f);
    g.DrawPath(&borderPen, &path);

    FontFamily fontFamily(L"Segoe UI");
    Font font(&fontFamily, 9.5f, FontStyleRegular, UnitPoint);
    SolidBrush textBrush(Color(alpha, 42, 37, 35));
    StringFormat format;
    format.SetAlignment(StringAlignmentCenter);
    format.SetLineAlignment(StringAlignmentCenter);

    RectF textRect(6.0f, 4.0f, w - 12.0f, h - 8.0f);
    g.DrawString(m_text.c_str(), (int)m_text.length(), &font, textRect, &format, &textBrush);

    POINT ptSrc = {0, 0};
    POINT ptDst = {m_x, m_y};
    SIZE sizeWnd = {m_width, m_height};
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};

    UpdateLayeredWindow(m_hwnd, hdcScreen, &ptDst, &sizeWnd, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(hdcMem, hOldBmp);
    DeleteObject(hBmp);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

} // namespace GUPet
