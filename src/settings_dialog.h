#pragma once

#include <windows.h>
#include <string>

namespace GUPet {

class SettingsDialog {
public:
    static SettingsDialog& Instance();

    void Show(HWND hParent);
    void Close();
    bool IsOpen() const { return m_hwnd != NULL; }
    HWND GetHwnd() const { return m_hwnd; }

private:
    SettingsDialog() = default;

    static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void OnInit(HWND hwnd);
    void OnCommand(HWND hwnd, WORD id, WORD code);
    void OnHScroll(HWND hwnd, LPARAM lParam);
    void OnDrawItem(HWND hwnd, DRAWITEMSTRUCT* dis);

    void PopulateSkins(HWND hwnd);
    void UpdateStats(HWND hwnd);
    void RefreshPreview(HWND hwnd);
    void ApplyCustomSize(HWND hwnd, float scale);

    HWND m_hwnd = NULL;
    HBRUSH m_hBgBrush = NULL;
    HBRUSH m_hCardBrush = NULL;
};

} // namespace GUPet
