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
#include <algorithm>

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
        KillTimer(m_hwnd, 999);
        DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }
    if (m_hBgBrush) {
        DeleteObject(m_hBgBrush);
        m_hBgBrush = NULL;
    }
    if (m_hCardBrush) {
        DeleteObject(m_hCardBrush);
        m_hCardBrush = NULL;
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

        case WM_TIMER:
            if (wParam == 999 && self) {
                self->RefreshPreview(hwnd);
                return TRUE;
            }
            break;

        case WM_CTLCOLORDLG:
            if (self && self->m_hBgBrush) {
                return (INT_PTR)self->m_hBgBrush;
            }
            break;

        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wParam;
            SetBkMode(hdcStatic, TRANSPARENT);
            SetTextColor(hdcStatic, RGB(30, 27, 26));
            if (self && self->m_hBgBrush) {
                return (INT_PTR)self->m_hBgBrush;
            }
            break;
        }

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

    m_hBgBrush = CreateSolidBrush(RGB(253, 248, 246));
    m_hCardBrush = CreateSolidBrush(RGB(246, 241, 238));

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

    // Size slider (1.0x to 15.0x -> 10 to 150)
    HWND hSlider = GetDlgItem(hwnd, IDC_SIZE_SLIDER);
    SendMessage(hSlider, TBM_SETRANGE, TRUE, MAKELPARAM(10, 150));
    int sliderPos = std::max(10, std::min(150, static_cast<int>(s.sizeScale * 10.0f)));
    SendMessage(hSlider, TBM_SETPOS, TRUE, sliderPos);

    std::wstringstream ss;
    ss << std::fixed << std::setprecision(1) << s.sizeScale << L"x";
    SetDlgItemTextW(hwnd, IDC_SIZE_VALUE, ss.str().c_str());

    std::wstringstream ssEdit;
    ssEdit << std::fixed << std::setprecision(1) << s.sizeScale;
    SetDlgItemTextW(hwnd, IDC_SIZE_EDIT, ssEdit.str().c_str());

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
    CheckDlgButton(hwnd, IDC_CHK_TOPMOST, s.alwaysOnTop ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, IDC_CHK_GHOST, s.ghostMode ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, IDC_CHK_AUTOSTART, s.autostart ? BST_CHECKED : BST_UNCHECKED);

    PopulateSkins(hwnd);
    UpdateStats(hwnd);

    // Start 30 FPS animation timer for live preview
    SetTimer(hwnd, 999, 33, NULL);
}

void SettingsDialog::PopulateSkins(HWND hwnd) {
    HWND hSkin = GetDlgItem(hwnd, IDC_SKIN_COMBO);
    SendMessage(hSkin, CB_RESETCONTENT, 0, 0);

    SendMessageW(hSkin, CB_ADDSTRING, 0, (LPARAM)L"— Встроенный вид —");

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
    if (hPreview) {
        InvalidateRect(hPreview, NULL, FALSE);
    }
}

void SettingsDialog::ApplyCustomSize(HWND hwnd, float scale) {
    scale = std::max(0.5f, std::min(30.0f, scale));

    auto& s = SettingsManager::Instance().GetMutableSettings();
    s.sizeScale = scale;
    SettingsManager::Instance().Save();
    PetOverlay::Instance().ReloadSettings();

    HWND hSlider = GetDlgItem(hwnd, IDC_SIZE_SLIDER);
    int sliderPos = std::max(10, std::min(150, static_cast<int>(scale * 10.0f)));
    SendMessage(hSlider, TBM_SETPOS, TRUE, sliderPos);

    std::wstringstream ss;
    ss << std::fixed << std::setprecision(1) << scale << L"x";
    SetDlgItemTextW(hwnd, IDC_SIZE_VALUE, ss.str().c_str());
}

void SettingsDialog::OnCommand(HWND hwnd, WORD id, WORD code) {
    auto& s = SettingsManager::Instance().GetMutableSettings();

    switch (id) {
        case IDOK:
        case IDCANCEL:
            Close();
            break;

        case IDC_SIZE_EDIT:
            if (code == EN_KILLFOCUS) {
                wchar_t buf[32] = {0};
                GetDlgItemTextW(hwnd, IDC_SIZE_EDIT, buf, 31);
                try {
                    float f = std::stof(buf);
                    ApplyCustomSize(hwnd, f);
                } catch (...) {}
            }
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
                if (sel >= 0 && sel < PetPalettes::PALETTE_COUNT) {
                    s.palette = sel;
                    SettingsManager::Instance().Save();
                    PetOverlay::Instance().ReloadSettings();
                    RefreshPreview(hwnd);
                }
            }
            break;

        case IDC_SLEEP_COMBO:
            if (code == CBN_SELCHANGE) {
                static const int minsMap[] = {0, 1, 2, 3, 5, 10};
                int sel = (int)SendMessage(GetDlgItem(hwnd, IDC_SLEEP_COMBO), CB_GETCURSEL, 0, 0);
                if (sel >= 0 && sel < 6) {
                    s.sleepMinutes = minsMap[sel];
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

        case IDC_CHK_TOPMOST:
            s.alwaysOnTop = (IsDlgButtonChecked(hwnd, IDC_CHK_TOPMOST) == BST_CHECKED);
            SettingsManager::Instance().Save();
            PetOverlay::Instance().ReloadSettings();
            break;

        case IDC_CHK_GHOST:
            s.ghostMode = (IsDlgButtonChecked(hwnd, IDC_CHK_GHOST) == BST_CHECKED);
            SettingsManager::Instance().Save();
            PetOverlay::Instance().ReloadSettings();
            break;

        case IDC_CHK_AUTOSTART:
            s.autostart = (IsDlgButtonChecked(hwnd, IDC_CHK_AUTOSTART) == BST_CHECKED);
            SettingsManager::Instance().Save();
            break;

        case IDC_BTN_RECENTER:
            PetOverlay::Instance().Recenter();
            break;

        case IDC_BTN_SAY:
            PetOverlay::Instance().Say("idle", true);
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

        std::wstringstream ssEdit;
        ssEdit << std::fixed << std::setprecision(1) << scale;
        SetDlgItemTextW(hwnd, IDC_SIZE_EDIT, ssEdit.str().c_str());
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
    g.SetSmoothingMode(SmoothingModeAntiAlias);

    // Modern card background
    GraphicsPath cardPath;
    int r = 18;
    cardPath.AddArc(0, 0, r, r, 180, 90);
    cardPath.AddArc(w - r - 1, 0, r, r, 270, 90);
    cardPath.AddArc(w - r - 1, h - r - 1, r, r, 0, 90);
    cardPath.AddArc(0, h - r - 1, r, r, 90, 90);
    cardPath.CloseFigure();

    LinearGradientBrush bgGrad(
        Point(0, 0), Point(w, h),
        Color(255, 255, 243, 237),
        Color(255, 245, 230, 222)
    );
    g.FillPath(&bgGrad, &cardPath);

    Pen borderPen(Color(120, 217, 119, 87), 1.5f);
    g.DrawPath(&borderPen, &cardPath);

    // Pixel rendering mode
    g.SetSmoothingMode(SmoothingModeNone);
    g.SetInterpolationMode(InterpolationModeNearestNeighbor);

    const auto& s = SettingsManager::Instance().GetSettings();
    uint64_t now = GetTickCount64();

    PetFrame frame;
    frame.pose = PetPose::IDLE;
    frame.expression = (now / 2000) % 8 == 0 ? PetExpression::BLINK : PetExpression::OPEN;
    frame.phase = now / 1000.0f;

    std::shared_ptr<GUPetSkin> skin = nullptr;
    if (!s.customSkinId.empty()) {
        skin = SkinStore::Instance().LoadSkin(s.customSkinId);
    }

    std::vector<PetParticle> emptyParticles;

    float petW = 18.0f * 4.6f;
    float petH = 17.0f * 4.6f;

    GraphicsState state = g.Save();
    g.TranslateTransform((w - petW) / 2.0f, (h - petH) / 2.0f);

    PetRenderer::DrawPet(
        g,
        petW,
        petH,
        s.species,
        s.palette,
        frame,
        now,
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
