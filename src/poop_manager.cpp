#include "poop_manager.h"
#include "pet_renderer.h"
#include "settings_manager.h"

#include <windows.h>
#include <gdiplus.h>
#include <algorithm>
#include <random>

namespace GUPet {

using namespace Gdiplus;

static const wchar_t* POOP_CLASS_NAME = L"GUPetPoopClass";

PoopManager& PoopManager::Instance() {
    static PoopManager s_instance;
    return s_instance;
}

void PoopManager::Init(std::function<void(int px, int py)> onCleanedCallback) {
    m_onCleaned = onCleanedCallback;

    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = POOP_CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_HAND);
    RegisterClassExW(&wc);
}

void PoopManager::Shutdown() {
    CleanAll(false);
}

LRESULT CALLBACK PoopManager::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    PoopItem* item = (PoopItem*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
        case WM_LBUTTONDOWN:
            if (item && !item->cleaning) {
                item->cleaning = true;
                item->appearing = false;
                if (SettingsManager::Instance().GetSettings().haptics) {
                    MessageBeep(MB_OK);
                }
            }
            return 0;

        case WM_RBUTTONUP: {
            HMENU hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_STRING, 1, L"Убрать эту кучку");
            AppendMenuW(hMenu, MF_STRING, 2, L"Очистить все кучки");

            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hwnd);
            int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);

            if (cmd == 1 && item) {
                item->cleaning = true;
            } else if (cmd == 2) {
                PoopManager::Instance().CleanAll(true);
            }
            return 0;
        }

        case WM_DESTROY:
            return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void PoopManager::SpawnPoop(int petX, int petY, int petW, int petH, int look) {
    if (m_poops.size() >= MAX_POOPS) {
        RemovePoop(0, false);
    }

    int poopW = 36;
    int poopH = 28;

    int side = (look == 0) ? ((rand() % 2 == 0) ? -1 : 1) : -look;
    int centerX = petX + petW / 2 + static_cast<int>(side * petW * 0.42f);

    RECT workArea;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);

    int px = std::max((int)workArea.left, std::min((int)workArea.right - poopW, centerX - poopW / 2));
    int py = std::max((int)workArea.top, std::min((int)workArea.bottom - poopH, petY + petH - poopH));

    HWND hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        POOP_CLASS_NAME,
        L"GUPet — кучка",
        WS_POPUP,
        px, py, poopW, poopH,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );

    if (!hwnd) return;

    PoopItem item;
    item.hwnd = hwnd;
    item.x = px;
    item.y = py;
    item.width = poopW;
    item.height = poopH;
    item.scale = 0.1f;
    item.alpha = 1.0f;
    item.appearing = true;
    item.cleaning = false;
    item.bornAt = GetTickCount64();

    m_poops.push_back(item);
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&m_poops.back());

    ShowWindow(hwnd, SW_SHOWNOACTIVATE);
    RenderPoop(m_poops.back());
}

void PoopManager::RenderPoop(PoopItem& item) {
    if (!item.hwnd || item.width <= 0 || item.height <= 0) return;

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = item.width;
    bmi.bmiHeader.biHeight = -item.height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    void* pBits = nullptr;
    HBITMAP hBmp = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcMem, hBmp);

    Bitmap bmp(item.width, item.height, item.width * 4, PixelFormat32bppPARGB, (BYTE*)pBits);
    Graphics g(&bmp);
    g.SetSmoothingMode(SmoothingModeNone);
    g.SetInterpolationMode(InterpolationModeNearestNeighbor);

    PetRenderer::DrawPoop(g, (float)item.width, (float)item.height, item.scale, item.alpha);

    POINT ptSrc = {0, 0};
    POINT ptDst = {item.x, item.y};
    SIZE sizeWnd = {item.width, item.height};
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};

    UpdateLayeredWindow(item.hwnd, hdcScreen, &ptDst, &sizeWnd, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(hdcMem, hOldBmp);
    DeleteObject(hBmp);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

void PoopManager::RemovePoop(size_t index, bool count) {
    if (index >= m_poops.size()) return;
    HWND h = m_poops[index].hwnd;
    int px = m_poops[index].x + m_poops[index].width / 2;
    int py = m_poops[index].y + m_poops[index].height / 2;

    DestroyWindow(h);
    m_poops.erase(m_poops.begin() + index);

    // Re-bind user data pointers because vector memory moved
    for (size_t i = 0; i < m_poops.size(); i++) {
        SetWindowLongPtr(m_poops[i].hwnd, GWLP_USERDATA, (LONG_PTR)&m_poops[i]);
    }

    if (count) {
        auto& s = SettingsManager::Instance().GetMutableSettings();
        s.poopsCleaned++;
        SettingsManager::Instance().Save();

        if (m_onCleaned) {
            m_onCleaned(px, py);
        }
    }
}

void PoopManager::CleanAll(bool count) {
    while (!m_poops.empty()) {
        RemovePoop(0, count);
    }
}

void PoopManager::Update() {
    for (size_t i = 0; i < m_poops.size(); ) {
        auto& item = m_poops[i];
        bool changed = false;

        if (item.appearing) {
            item.scale += 0.15f;
            if (item.scale >= 1.0f) {
                item.scale = 1.0f;
                item.appearing = false;
            }
            changed = true;
        } else if (item.cleaning) {
            item.scale -= 0.18f;
            item.alpha -= 0.18f;
            if (item.scale <= 0.05f || item.alpha <= 0.05f) {
                RemovePoop(i, true);
                continue;
            }
            changed = true;
        }

        if (changed) {
            RenderPoop(item);
        }
        i++;
    }
}

} // namespace GUPet
