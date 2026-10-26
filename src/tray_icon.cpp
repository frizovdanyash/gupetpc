#include "tray_icon.h"
#include "pet_overlay.h"
#include "settings_manager.h"
#include "settings_dialog.h"
#include "web_server.h"
#include "../res/resource.h"

namespace GUPet {

TrayIcon& TrayIcon::Instance() {
    static TrayIcon s_instance;
    return s_instance;
}

bool TrayIcon::Init(HWND hMainWnd) {
    m_hwnd = hMainWnd;

    memset(&m_nid, 0, sizeof(m_nid));
    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = m_hwnd;
    m_nid.uID = 1;
    m_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    m_nid.uCallbackMessage = WM_TRAYICON;
    m_nid.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APP_ICON));
    wcscpy(m_nid.szTip, L"GUPet — пиксельный питомец");

    m_added = Shell_NotifyIconW(NIM_ADD, &m_nid);
    return m_added;
}

void TrayIcon::Shutdown() {
    if (m_added) {
        Shell_NotifyIconW(NIM_DELETE, &m_nid);
        m_added = false;
    }
}

void TrayIcon::OnTrayMessage(WPARAM wParam, LPARAM lParam) {
    if (lParam == WM_RBUTTONUP) {
        ShowMenu();
    } else if (lParam == WM_LBUTTONDBLCLK || lParam == WM_LBUTTONUP) {
        WebServer::Instance().OpenSettingsInBrowser();
    }
}

void TrayIcon::ShowMenu() {
    HMENU hMenu = CreatePopupMenu();
    const auto& s = SettingsManager::Instance().GetSettings();

    AppendMenuW(hMenu, MF_STRING, ID_TRAY_SHOW_HIDE, s.show ? L"Скрыть питомца" : L"Показать питомца");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_RECENTER, L"Позвать в центр");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_POOP, L"Сделать кучку");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_CLEAN_POOPS, L"Очистить все кучки");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

    HMENU hSubSpecies = CreatePopupMenu();
    AppendMenuW(hSubSpecies, MF_STRING | (s.species == PetSpecies::GUPET && s.customSkinId.empty() ? MF_CHECKED : 0),
                10, L"GUPet");
    AppendMenuW(hSubSpecies, MF_STRING | (s.species == PetSpecies::CAT && s.customSkinId.empty() ? MF_CHECKED : 0),
                11, L"Кошка");
    AppendMenuW(hSubSpecies, MF_STRING | (s.species == PetSpecies::DOG && s.customSkinId.empty() ? MF_CHECKED : 0),
                12, L"Собака");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hSubSpecies, L"Вид питомца");

    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_SETTINGS, L"Настройки...");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Выход");

    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(m_hwnd);
    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hwnd, NULL);
    DestroyMenu(hMenu);

    switch (cmd) {
        case ID_TRAY_SHOW_HIDE:
            PetOverlay::Instance().SetVisible(!s.show);
            break;
        case ID_TRAY_RECENTER:
            PetOverlay::Instance().Recenter();
            break;
        case ID_TRAY_POOP:
            PetOverlay::Instance().PoopNow();
            break;
        case ID_TRAY_CLEAN_POOPS:
            PetOverlay::Instance().CleanAllPoops();
            break;
        case 10: {
            auto& mut = SettingsManager::Instance().GetMutableSettings();
            mut.species = PetSpecies::GUPET;
            mut.customSkinId.clear();
            SettingsManager::Instance().Save();
            PetOverlay::Instance().ReloadSettings();
            break;
        }
        case 11: {
            auto& mut = SettingsManager::Instance().GetMutableSettings();
            mut.species = PetSpecies::CAT;
            mut.customSkinId.clear();
            SettingsManager::Instance().Save();
            PetOverlay::Instance().ReloadSettings();
            break;
        }
        case 12: {
            auto& mut = SettingsManager::Instance().GetMutableSettings();
            mut.species = PetSpecies::DOG;
            mut.customSkinId.clear();
            SettingsManager::Instance().Save();
            PetOverlay::Instance().ReloadSettings();
            break;
        }
        case ID_TRAY_SETTINGS:
            WebServer::Instance().OpenSettingsInBrowser();
            break;
        case ID_TRAY_EXIT:
            PostQuitMessage(0);
            break;
    }
}

} // namespace GUPet
