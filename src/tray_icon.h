#pragma once

#include <windows.h>

#define WM_TRAYICON (WM_USER + 101)

namespace GUPet {

class TrayIcon {
public:
    static TrayIcon& Instance();

    bool Init(HWND hMainWnd);
    void Shutdown();

    void ShowMenu();
    void OnTrayMessage(WPARAM wParam, LPARAM lParam);

private:
    TrayIcon() = default;
    ~TrayIcon() { Shutdown(); }

    HWND m_hwnd = NULL;
    NOTIFYICONDATAW m_nid = {0};
    bool m_added = false;
};

} // namespace GUPet
