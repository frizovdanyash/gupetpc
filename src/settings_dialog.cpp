#include "settings_dialog.h"
#include "settings_manager.h"
#include "pet_overlay.h"
#include "pet_renderer.h"
#include "skin_store.h"
#include "../res/resource.h"

#include <commctrl.h>
#include <commdlg.h>
#include <gdiplus.h>
#include <vector>
#include <iomanip>
#include <sstream>

namespace GUPet {

using namespace Gdiplus;

SettingsDialog& SettingsDialog::Instance() {
    static SettingsDialog s_instance;
    return s_instance;
}

void SettingsDialog::Show(HWND hParent) {
    if (m_hwnd) {
        SetForegroundWindow(m_hwnd);
        ShowWindow(m_hwnd, SW_RESTORE);
        return;
    }

    m_hwnd = CreateDialogParamW(
        GetModuleHandle(NULL),
        MAKEINTRESOURCEW(IDD_SETTINGS),
        hParent,
        DialogProc,
        (LPARAM)this
    );

    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_SHOW);
        SetForegroundWindow(m_hwnd);
    }
}

void SettingsDialog::Close() {
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }
}

INT_PTR CALLBACK SettingsDialog::DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    SettingsDialog* self = (SettingsDialog*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
        case WM_INITDIALOG:
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)lParam);
            self = (SettingsDialog*)lParam;
            if (self) {
                self->m_hwnd = hwnd;
                self->OnInit(hwnd);
            }
            return TRUE;

        case WM_COMMAND:
            if (self) {
                self->OnCommand(hwnd, LOWORD(wParam), HIWORD(wParam));
            }
            return TRUE;

        case WM_HSCROLL:
            if (self) {
                self->OnHScroll(hwnd, lParam);
            }
            return TRUE;

        case WM_DRAWITEM:
            if (self && wParam == IDC_PREVIEW_AREA) {
                self->OnDrawItem(hwnd, (DRAWITEMSTRUCT*)lParam);
                return TRUE;
            }
            break;

        case WM_CLOSE:
            if (self) {
                self->Close();
            }
            return TRUE;
    }

    return FALSE;
}

void SettingsDialog::OnInit(HWND hwnd) {
    HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APP_ICON));
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

    const auto& s = SettingsManager::Instance().GetSettings();

    // Species
    int speciesId = IDC_SPECIES_GUPET;
    if (s.species == PetSpecies::CAT) speciesId = IDC_SPECIES_CAT;
    else if (s.species == PetSpecies::DOG) speciesId = IDC_SPECIES_DOG;
    CheckRadioButton(hwnd, IDC_SPECIES_GUPET, IDC_SPECIES_DOG, speciesId);

    // Palettes
    HWND hPalette = GetDlgItem(hwnd, IDC_PALETTE_COMBO);
    for (int i = 0; i < PetPalettes::PALETTE_COUNT; i++) {
        SendMessageW(hPalette, CB_ADDSTRING, 0, (LPARAM)PetPalettes::GetName(i));
    }
    SendMessage(hPalette, CB_SETCURSEL, s.palette, 0);

    // Size slider
    HWND hSlider = GetDlgItem(hwnd, IDC_SIZE_SLIDER);
    SendMessage(hSlider, TBM_SETRANGE, TRUE, MAKELPARAM(30, 50));
    int sliderPos = static_cast<int>(s.sizeScale * 10.0f);
    SendMessage(hSlider, TBM_SETPOS, TRUE, sliderPos);

    std::wstringstream ss;
    ss << std::fixed << std::setprecision(1) << s.sizeScale << L"x";
    SetDlgItemTextW(hwnd, IDC_SIZE_VALUE, ss.str().c_str());

    // Sleep combo
    HWND hSleep = GetDlgItem(hwnd, IDC_SLEEP_COMBO);
    static const struct { int mins; const wchar_t* label; } sleepOpts[] = {
        {0, L"Отключено"}, {1, L"1 мин"}, {2, L"2 мин"},
        {3, L"3 мин"}, {5, L"5 мин"}, {10, L"10 мин"}
    };
    int selSleep = 0;
    for (int i = 0; i < 6; i++) {
        SendMessageW(hSleep, CB_ADDSTRING, 0, (LPARAM)sleepOpts[i].label);
        if (s.sleepMinutes == sleepOpts[i].mins) selSleep = i;
    }
    SendMessage(hSleep, CB_SETCURSEL, selSleep, 0);

    // Checkboxes
    CheckDlgButton(hwnd, IDC_CHK_WANDER, s.wander ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, IDC_CHK_PHRASES, s.phrases ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, IDC_CHK_GRAVITY, s.gravity ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, IDC_CHK_SHAKE, s.shake ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, IDC_CHK_HAPTICS, s.haptics ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, IDC_CHK_POOP, s.poop ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, IDC_CHK_DANCE, s.dance ? BST_CHECKED : BST_UNCHECKED);

    PopulateSkins(hwnd);
    UpdateStats(hwnd);
}

void SettingsDialog::PopulateSkins(HWND hwnd) {
    HWND hSkin = GetDlgItem(hwnd, IDC_SKIN_COMBO);
    SendMessage(hSkin, CB_RESETCONTENT, 0, 0);

    SendMessageW(hSkin, CB_ADDSTRING, 0, (LPARAM)L"— Стандартный вид —");

    auto skins = SkinStore::Instance().ListSkins();
    const auto& s = SettingsManager::Instance().GetSettings();
    int selIndex = 0;

    for (size_t i = 0; i < skins.size(); i++) {
        std::wstring label;
        int len = MultiByteToWideChar(CP_UTF8, 0, skins[i].name.c_str(), (int)skins[i].name.size(), NULL, 0);
        label.resize(len);
        MultiByteToWideChar(CP_UTF8, 0, skins[i].name.c_str(), (int)skins[i].name.size(), &label[0], len);

        SendMessageW(hSkin, CB_ADDSTRING, 0, (LPARAM)label.c_str());
        if (s.customSkinId == skins[i].id) {
            selIndex = (int)i + 1;
        }
    }
    SendMessage(hSkin, CB_SETCURSEL, selIndex, 0);
}

void SettingsDialog::UpdateStats(HWND hwnd) {
    const auto& s = SettingsManager::Instance().GetSettings();
    std::wstringstream ss;
    ss << L"Поглаживаний: " << s.petsCount << L"   |   Кучек убрано: " << s.poopsCleaned;
    SetDlgItemTextW(hwnd, IDC_TXT_STATS, ss.str().c_str());
}

void SettingsDialog::RefreshPreview(HWND hwnd) {
    HWND hPreview = GetDlgItem(hwnd, IDC_PREVIEW_AREA);
    InvalidateRect(hPreview, NULL, TRUE);
}

void SettingsDialog::OnCommand(HWND hwnd, WORD id, WORD code) {
    auto& s = SettingsManager::Instance().GetMutableSettings();

    switch (id) {
        case IDOK:
        case IDCANCEL:
            Close();
            break;

        case IDC_SPECIES_GUPET:
            s.species = PetSpecies::GUPET;
            s.customSkinId.clear();
            SettingsManager::Instance().Save();
            PetOverlay::Instance().ReloadSettings();
            PopulateSkins(hwnd);
            RefreshPreview(hwnd);
            break;

        case IDC_SPECIES_CAT:
            s.species = PetSpecies::CAT;
            s.customSkinId.clear();
            SettingsManager::Instance().Save();
            PetOverlay::Instance().ReloadSettings();
            PopulateSkins(hwnd);
            RefreshPreview(hwnd);
            break;

        case IDC_SPECIES_DOG:
            s.species = PetSpecies::DOG;
            s.customSkinId.clear();
            SettingsManager::Instance().Save();
            PetOverlay::Instance().ReloadSettings();
            PopulateSkins(hwnd);
            RefreshPreview(hwnd);
            break;

        case IDC_PALETTE_COMBO:
            if (code == CBN_SELCHANGE) {
                int sel = (int)SendMessage(GetDlgItem(hwnd, IDC_PALETTE_COMBO), CB_GETCURSEL, 0, 0);
                if (sel >= 0) {
                    s.palette = sel;
                    SettingsManager::Instance().Save();
                    PetOverlay::Instance().ReloadSettings();
                    RefreshPreview(hwnd);
                }
            }
            break;

        case IDC_SLEEP_COMBO:
            if (code == CBN_SELCHANGE) {
                int sel = (int)SendMessage(GetDlgItem(hwnd, IDC_SLEEP_COMBO), CB_GETCURSEL, 0, 0);
                static const int mins[] = {0, 1, 2, 3, 5, 10};
                if (sel >= 0 && sel < 6) {
                    s.sleepMinutes = mins[sel];
                    SettingsManager::Instance().Save();
                }
            }
            break;

        case IDC_CHK_WANDER:
            s.wander = (IsDlgButtonChecked(hwnd, IDC_CHK_WANDER) == BST_CHECKED);
            SettingsManager::Instance().Save();
            break;

        case IDC_CHK_PHRASES:
            s.phrases = (IsDlgButtonChecked(hwnd, IDC_CHK_PHRASES) == BST_CHECKED);
            SettingsManager::Instance().Save();
            break;

        case IDC_CHK_GRAVITY:
            s.gravity = (IsDlgButtonChecked(hwnd, IDC_CHK_GRAVITY) == BST_CHECKED);
            SettingsManager::Instance().Save();
            break;

        case IDC_CHK_SHAKE:
            s.shake = (IsDlgButtonChecked(hwnd, IDC_CHK_SHAKE) == BST_CHECKED);
            SettingsManager::Instance().Save();
            break;

        case IDC_CHK_HAPTICS:
            s.haptics = (IsDlgButtonChecked(hwnd, IDC_CHK_HAPTICS) == BST_CHECKED);
            SettingsManager::Instance().Save();
            break;

        case IDC_CHK_POOP:
            s.poop = (IsDlgButtonChecked(hwnd, IDC_CHK_POOP) == BST_CHECKED);
            SettingsManager::Instance().Save();
            break;

        case IDC_CHK_DANCE:
            s.dance = (IsDlgButtonChecked(hwnd, IDC_CHK_DANCE) == BST_CHECKED);
            SettingsManager::Instance().Save();
            break;

        case IDC_BTN_RECENTER:
            PetOverlay::Instance().Recenter();
            break;

        case IDC_BTN_POOP_NOW:
            PetOverlay::Instance().PoopNow();
            break;

        case IDC_BTN_CLEAN_POOP:
            PetOverlay::Instance().CleanAllPoops();
            UpdateStats(hwnd);
            break;

        case IDC_SKIN_COMBO:
            if (code == CBN_SELCHANGE) {
                int sel = (int)SendMessage(GetDlgItem(hwnd, IDC_SKIN_COMBO), CB_GETCURSEL, 0, 0);
                if (sel == 0) {
                    s.customSkinId.clear();
                } else {
                    auto skins = SkinStore::Instance().ListSkins();
                    if (sel - 1 < (int)skins.size()) {
                        s.customSkinId = skins[sel - 1].id;
                    }
                }
                SettingsManager::Instance().Save();
                PetOverlay::Instance().ReloadSettings();
                RefreshPreview(hwnd);
            }
            break;

        case IDC_BTN_RESET_SKIN:
            s.customSkinId.clear();
            SettingsManager::Instance().Save();
            PetOverlay::Instance().ReloadSettings();
            PopulateSkins(hwnd);
            RefreshPreview(hwnd);
            break;

        case IDC_BTN_IMPORT_SKIN: {
            wchar_t szFile[MAX_PATH] = {0};
            OPENFILENAMEW ofn = {0};
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
            ofn.lpstrFilter = L"GUPet Skin (*.gupet;*.zip)\0*.gupet;*.zip\0Все файлы (*.*)\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

            if (GetOpenFileNameW(&ofn)) {
                std::string newId;
                std::string error;
                if (SkinStore::Instance().ImportSkin(szFile, newId, error)) {
                    s.customSkinId = newId;
                    SettingsManager::Instance().Save();
                    PetOverlay::Instance().ReloadSettings();
                    PopulateSkins(hwnd);
                    RefreshPreview(hwnd);
                    MessageBoxW(hwnd, L"Скин успешно установлен!", L"GUPet", MB_OK | MB_ICONINFORMATION);
                } else {
                    std::wstring werr;
                    int len = MultiByteToWideChar(CP_UTF8, 0, error.c_str(), (int)error.size(), NULL, 0);
                    werr.resize(len);
                    MultiByteToWideChar(CP_UTF8, 0, error.c_str(), (int)error.size(), &werr[0], len);
                    MessageBoxW(hwnd, werr.c_str(), L"Ошибка импорта", MB_OK | MB_ICONERROR);
                }
            }
            break;
        }
    }
}

void SettingsDialog::OnHScroll(HWND hwnd, LPARAM lParam) {
    HWND hSlider = GetDlgItem(hwnd, IDC_SIZE_SLIDER);
    if ((HWND)lParam == hSlider) {
        int pos = (int)SendMessage(hSlider, TBM_GETPOS, 0, 0);
        float scale = pos / 10.0f;

        auto& s = SettingsManager::Instance().GetMutableSettings();
        s.sizeScale = scale;
        SettingsManager::Instance().Save();
        PetOverlay::Instance().ReloadSettings();

        std::wstringstream ss;
        ss << std::fixed << std::setprecision(1) << scale << L"x";
        SetDlgItemTextW(hwnd, IDC_SIZE_VALUE, ss.str().c_str());
    }
}

void SettingsDialog::OnDrawItem(HWND hwnd, DRAWITEMSTRUCT* dis) {
    if (!dis || dis->CtlID != IDC_PREVIEW_AREA) return;

    RECT rc = dis->rcItem;
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    HDC hdcMem = CreateCompatibleDC(dis->hDC);
    HBITMAP hBmp = CreateCompatibleBitmap(dis->hDC, w, h);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcMem, hBmp);

    Graphics g(hdcMem);
    g.SetSmoothingMode(SmoothingModeNone);
    g.SetInterpolationMode(InterpolationModeNearestNeighbor);

    SolidBrush bgBrush(Color(245, 245, 248));
    g.FillRectangle(&bgBrush, 0, 0, w, h);

    Pen borderPen(Color(220, 220, 225), 1.0f);
    g.DrawRectangle(&borderPen, 0, 0, w - 1, h - 1);

    const auto& s = SettingsManager::Instance().GetSettings();
    PetFrame frame;
    frame.pose = PetPose::IDLE;
    frame.expression = PetExpression::OPEN;

    std::shared_ptr<GUPetSkin> skin = nullptr;
    if (!s.customSkinId.empty()) {
        skin = SkinStore::Instance().LoadSkin(s.customSkinId);
    }

    std::vector<PetParticle> emptyParticles;

    float petW = 18.0f * 3.8f;
    float petH = 17.0f * 3.8f;

    GraphicsState state = g.Save();
    g.TranslateTransform((w - petW) / 2.0f, (h - petH) / 2.0f + 2.0f);

    PetRenderer::DrawPet(
        g,
        petW,
        petH,
        s.species,
        s.palette,
        frame,
        0,
        emptyParticles,
        skin
    );

    g.Restore(state);

    BitBlt(dis->hDC, rc.left, rc.top, w, h, hdcMem, 0, 0, SRCCOPY);

    SelectObject(hdcMem, hOldBmp);
    DeleteObject(hBmp);
    DeleteDC(hdcMem);
}

} // namespace GUPet
