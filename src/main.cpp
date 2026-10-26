#include <windows.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <shlwapi.h>

#include "pet_types.h"
#include "settings_manager.h"
#include "skin_store.h"
#include "pet_overlay.h"
#include "tray_icon.h"
#include "settings_dialog.h"
#include "audio_monitor.h"
#include "../res/resource.h"

using namespace Gdiplus;

static void SetupBundledSkins() {
    wchar_t exePath[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    PathRemoveFileSpecW(exePath);

    std::wstring bundledSkinsDir = std::wstring(exePath) + L"\\skins";
    std::wstring iskraSkin = bundledSkinsDir + L"\\Iskra.gupet";

    if (PathFileExistsW(iskraSkin.c_str())) {
        std::string newId;
        std::string error;
        GUPet::SkinStore::Instance().ImportSkin(iskraSkin, newId, error);
    }
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"GUPet_Desktop_Instance_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND existingWnd = FindWindowW(L"GUPetOverlayClass", NULL);
        if (existingWnd) {
            PostMessage(existingWnd, WM_COMMAND, ID_TRAY_SETTINGS, 0);
        }
        CloseHandle(hMutex);
        return 0;
    }

    INITCOMMONCONTROLSEX icex = {0};
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES | ICC_BAR_CLASSES | ICC_UPDOWN_CLASS;
    InitCommonControlsEx(&icex);

    CoInitialize(NULL);

    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken = 0;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    GUPet::SettingsManager::Instance().Init();
    GUPet::SkinStore::Instance().Init(GUPet::SettingsManager::Instance().GetAppDataDirA());
    SetupBundledSkins();

    GUPet::AudioMonitor::Instance().Init();

    if (!GUPet::PetOverlay::Instance().Init()) {
        MessageBoxW(NULL, L"Не удалось инициализировать GUPet.", L"Ошибка", MB_OK | MB_ICONERROR);
        GdiplusShutdown(gdiplusToken);
        CoUninitialize();
        CloseHandle(hMutex);
        return 1;
    }

    GUPet::TrayIcon::Instance().Init(GUPet::PetOverlay::Instance().GetHwnd());

    if (lpCmdLine && (wcsstr(lpCmdLine, L"--settings") || wcsstr(lpCmdLine, L"-s"))) {
        GUPet::SettingsDialog::Instance().Show(GUPet::PetOverlay::Instance().GetHwnd());
    }

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (GUPet::SettingsDialog::Instance().IsOpen() &&
            IsDialogMessageW(GUPet::SettingsDialog::Instance().GetHwnd(), &msg)) {
            continue;
        }

        if (msg.message == WM_TRAYICON) {
            GUPet::TrayIcon::Instance().OnTrayMessage(msg.wParam, msg.lParam);
            continue;
        }

        if (msg.message == WM_COMMAND && msg.wParam == ID_TRAY_SETTINGS) {
            GUPet::SettingsDialog::Instance().Show(GUPet::PetOverlay::Instance().GetHwnd());
            continue;
        }

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    GUPet::TrayIcon::Instance().Shutdown();
    GUPet::PetOverlay::Instance().Shutdown();
    GUPet::AudioMonitor::Instance().Shutdown();

    GdiplusShutdown(gdiplusToken);
    CoUninitialize();
    CloseHandle(hMutex);

    return (int)msg.wParam;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    return wWinMain(hInstance, hPrevInstance, GetCommandLineW(), nCmdShow);
}
