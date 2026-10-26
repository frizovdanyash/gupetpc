#include "pet_overlay.h"
#include "pet_renderer.h"
#include "pet_palettes.h"
#include "phrases.h"
#include "audio_monitor.h"
#include "settings_manager.h"
#include "poop_manager.h"
#include "../res/resource.h"

#include <windows.h>
#include <gdiplus.h>
#include <cmath>
#include <algorithm>
#include <random>

namespace GUPet {

using namespace Gdiplus;

static const wchar_t* OVERLAY_CLASS_NAME = L"GUPetOverlayClass";

static float RandomBetween(float a, float b) {
    static std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(a, b);
    return dist(rng);
}

static uint64_t RandomBetweenU64(uint64_t a, uint64_t b) {
    static std::mt19937 rng(42);
    std::uniform_int_distribution<uint64_t> dist(a, b);
    return dist(rng);
}

PetOverlay& PetOverlay::Instance() {
    static PetOverlay s_instance;
    return s_instance;
}

bool PetOverlay::Init() {
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = OVERLAY_CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassExW(&wc);

    m_hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        OVERLAY_CLASS_NAME,
        L"GUPet",
        WS_POPUP,
        100, 100, 80, 80,
        NULL, NULL, GetModuleHandle(NULL), this
    );

    if (!m_hwnd) return false;

    m_bubble.Create();

    PoopManager::Instance().Init([this](int px, int py) {
        for (int i = 0; i < 4; i++) {
            SpawnParticle(ParticleKind::SPARK, (float)px, (float)py);
        }
        Say("clean");
    });

    ReloadSettings();
    ResizeWindow();
    RestorePosition();

    uint64_t now = GetTickCount64();
    m_lastTickAt = now;
    m_lastInteractionAt = now;
    m_nextActionAt = now + 3500;
    m_nextBlinkAt = now + 2000;
    m_nextPoopAt = now + RandomBetweenU64(120000, 300000);
    m_nextPeekAt = now + 45000;

    SetTimer(m_hwnd, 1, 16, NULL);

    if (SettingsManager::Instance().GetSettings().show) {
        ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
        Say("hello");
    }

    return true;
}

void PetOverlay::Shutdown() {
    if (m_hwnd) {
        KillTimer(m_hwnd, 1);
        m_bubble.Destroy();
        PoopManager::Instance().Shutdown();
        DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }
}

LRESULT CALLBACK PetOverlay::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    PetOverlay* self = (PetOverlay*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
        case WM_NCCREATE: {
            CREATESTRUCTW* cs = (CREATESTRUCTW*)lParam;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
            return TRUE;
        }

        case WM_TIMER:
            if (self) self->OnTick();
            return 0;

        case WM_LBUTTONDOWN:
            if (self) {
                SetCapture(hwnd);
                self->OnLButtonDown(LOWORD(lParam), HIWORD(lParam));
            }
            return 0;

        case WM_MOUSEMOVE:
            if (self) self->OnMouseMove(LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_LBUTTONUP:
            if (self) {
                ReleaseCapture();
                self->OnLButtonUp(LOWORD(lParam), HIWORD(lParam));
            }
            return 0;

        case WM_RBUTTONUP:
            if (self) self->OnRButtonUp(LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_DESTROY:
            return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void PetOverlay::ReloadSettings() {
    const auto& s = SettingsManager::Instance().GetSettings();
    if (!s.customSkinId.empty()) {
        m_customSkin = SkinStore::Instance().LoadSkin(s.customSkinId);
    } else {
        m_customSkin = nullptr;
    }
    ResizeWindow();
}

void PetOverlay::ResizeWindow() {
    const auto& s = SettingsManager::Instance().GetSettings();
    float scale = std::max(0.2f, s.sizeScale);
    m_unitPx = scale * 3.8f;
    m_windowW = std::max(24, static_cast<int>(m_unitPx * 18.0f));
    m_windowH = std::max(24, static_cast<int>(m_unitPx * 17.0f));

    RefreshBounds();
    UpdateLayout(true);
}

void PetOverlay::RefreshBounds() {
    RECT workArea = {0, 0, 1920, 1080};
    HMONITOR hMon = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    if (hMon && GetMonitorInfoW(hMon, &mi)) {
        workArea = mi.rcWork;
    } else {
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
    }

    m_minX = workArea.left;
    m_maxX = std::max(m_minX, (int)workArea.right - m_windowW);
    m_minY = workArea.top;
    m_maxY = std::max(m_minY, (int)workArea.bottom - m_windowH);

    if (m_state != PetState::PEEK) {
        m_posX = std::max((float)m_minX, std::min((float)m_maxX, m_posX));
        m_posY = std::max((float)m_minY, std::min((float)m_maxY, m_posY));
    }
}

void PetOverlay::RestorePosition() {
    const auto& s = SettingsManager::Instance().GetSettings();
    RefreshBounds();
    if (s.positionSaved) {
        m_posX = std::max((float)m_minX, std::min((float)m_maxX, s.posX));
        m_posY = std::max((float)m_minY, std::min((float)m_maxY, s.posY));
    } else {
        m_posX = (m_minX + m_maxX) * 0.75f;
        m_posY = (float)m_maxY;
    }
    UpdateLayout(true);
}

void PetOverlay::SavePosition() {
    auto& s = SettingsManager::Instance().GetMutableSettings();
    s.posX = m_posX;
    s.posY = m_posY;
    s.positionSaved = true;
    SettingsManager::Instance().Save();
}

void PetOverlay::SetPosition(float x, float y, bool allowOffscreen) {
    m_posX = allowOffscreen ? x : std::max((float)m_minX, std::min((float)m_maxX, x));
    m_posY = std::max((float)m_minY, std::min((float)m_maxY, y));
    UpdateLayout();
}

void PetOverlay::UpdateLayout(bool force) {
    if (!m_hwnd) return;
    const auto& s = SettingsManager::Instance().GetSettings();
    HWND insertAfter = s.alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST;
    SetWindowPos(m_hwnd, insertAfter, (int)m_posX, (int)m_posY, m_windowW, m_windowH,
                 SWP_NOACTIVATE | (force ? 0 : SWP_NOSIZE));

    LONG_PTR exStyle = GetWindowLongPtr(m_hwnd, GWL_EXSTYLE);
    if (s.ghostMode) {
        if (!(exStyle & WS_EX_TRANSPARENT)) {
            SetWindowLongPtr(m_hwnd, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT);
        }
    } else {
        if (exStyle & WS_EX_TRANSPARENT) {
            SetWindowLongPtr(m_hwnd, GWL_EXSTYLE, exStyle & ~WS_EX_TRANSPARENT);
        }
    }
}

void PetOverlay::SetVisible(bool visible) {
    auto& s = SettingsManager::Instance().GetMutableSettings();
    s.show = visible;
    SettingsManager::Instance().Save();

    if (visible) {
        ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
        Say("hello");
    } else {
        ShowWindow(m_hwnd, SW_HIDE);
        m_bubble.Hide();
    }
}

void PetOverlay::Recenter() {
    RefreshBounds();
    m_posX = (m_minX + m_maxX) / 2.0f;
    m_posY = (m_minY + m_maxY) / 2.0f;
    m_state = PetState::IDLE;
    m_velocityX = 0.0f;
    m_velocityY = 0.0f;
    m_peekSide = 0;
    m_peekPending = 0;
    m_lastInteractionAt = GetTickCount64();
    UpdateLayout(true);

    Hop(m_unitPx * 80.0f);
    for (int i = 0; i < 3; i++) SpawnParticle(ParticleKind::SPARK);
    Say("hello");
    SavePosition();
}

void PetOverlay::PoopNow() {
    PoopManager::Instance().SpawnPoop((int)m_posX, (int)m_posY, m_windowW, m_windowH, m_look);
    Say("poop");
    SetExpression(PetExpression::HAPPY, 1700);
    for (int i = 0; i < 2; i++) SpawnParticle(ParticleKind::SPARK);
}

void PetOverlay::CleanAllPoops() {
    PoopManager::Instance().CleanAll(true);
}

void PetOverlay::Say(const std::string& category, bool force) {
    const auto& s = SettingsManager::Instance().GetSettings();
    if (!force && !s.phrases) return;

    std::wstring phrase = PetPhrases::GetRandom(category, m_lastPhrase);
    if (!phrase.empty()) {
        m_lastPhrase = phrase;
        m_bubble.Show(phrase, (int)m_posX, (int)m_posY, m_windowW, m_windowH);
    }
}

void PetOverlay::Hop(float strength) {
    if (m_state != PetState::IDLE && m_state != PetState::WALK) return;
    m_hopActive = true;
    m_hopVelocity = -strength;
    if (m_hopOffset > 0.0f) m_hopOffset = 0.0f;
}

void PetOverlay::SetExpression(PetExpression expr, uint64_t durationMs) {
    m_frame.expression = expr;
    m_expressionUntil = GetTickCount64() + durationMs;
}

void PetOverlay::SpawnParticle(ParticleKind kind, float originX, float originY) {
    if (m_particles.size() >= 12) {
        m_particles.erase(m_particles.begin());
    }

    PetParticle p;
    p.kind = kind;
    p.bornAtMs = GetTickCount64();
    p.durationMs = (kind == ParticleKind::ZZZ) ? 1600 : 1100;
    p.driftX = RandomBetween(-m_unitPx * 5.0f, m_unitPx * 5.0f);

    float cell = m_unitPx * 0.72f;
    if (originX >= 0.0f) {
        p.x = originX - m_posX;
        p.y = originY - m_posY;
    } else {
        p.x = m_windowW / 2.0f - cell * 2.5f + RandomBetween(-cell * 2.0f, cell * 2.0f);
        p.y = (kind == ParticleKind::ZZZ) ? (m_unitPx * 4.2f) : (m_unitPx * 3.1f);
    }

    m_particles.push_back(p);
}

void PetOverlay::UpdateParticles(uint64_t now) {
    for (size_t i = 0; i < m_particles.size(); ) {
        if (now - m_particles[i].bornAtMs >= m_particles[i].durationMs) {
            m_particles.erase(m_particles.begin() + i);
        } else {
            i++;
        }
    }
}

void PetOverlay::OnTick() {
    if (!SettingsManager::Instance().GetSettings().show) return;

    AudioMonitor::Instance().Update();
    bool playing = AudioMonitor::Instance().IsPlaying();
    if (playing != m_mediaPlaying) {
        m_mediaPlaying = playing;
        if (m_mediaPlaying && SettingsManager::Instance().GetSettings().dance) {
            m_lastInteractionAt = GetTickCount64();
            m_idlePose = PetPose::IDLE;
            m_idlePoseUntil = 0;
            if (m_state == PetState::SLEEP) m_state = PetState::IDLE;
            if (m_state == PetState::PEEK) EndPeek(GetTickCount64());
            Say("music");
        } else {
            m_nextActionAt = GetTickCount64() + RandomBetweenU64(2000, 4000);
            if (RandomBetween(0.0f, 1.0f) < 0.7f) Say("music_end");
        }
    }

    Step();
    PoopManager::Instance().Update();
    m_bubble.Update((int)m_posX, (int)m_posY, m_windowW, m_windowH);
}

void PetOverlay::Step() {
    uint64_t now = GetTickCount64();
    RefreshBounds();

    float dt = std::max(0.0f, std::min(0.05f, (now - m_lastTickAt) / 1000.0f));
    m_lastTickAt = now;

    if (m_state == PetState::PRESS && (now - m_mouseDownAt >= 450)) {
        BeginPet(now);
    }

    switch (m_state) {
        case PetState::FLY:
            StepFlight(dt, now);
            break;
        case PetState::HANG:
            StepHang(now);
            break;
        case PetState::PEEK:
            StepPeek(dt, now);
            break;
        case PetState::PET:
            if (now >= m_nextHeartAt) {
                m_nextHeartAt = now + 400;
                SpawnParticle(ParticleKind::HEART);
            }
            break;
        case PetState::IDLE:
        case PetState::WALK:
        case PetState::SLEEP:
            StepGround(dt, now);
            break;
        default:
            break;
    }

    if (m_hopActive) {
        m_hopOffset += m_hopVelocity * dt;
        m_hopVelocity += m_unitPx * 600.0f * dt;
        if (m_hopOffset >= 0.0f) {
            m_hopOffset = 0.0f;
            m_hopVelocity = 0.0f;
            m_hopActive = false;
            m_squashUntil = now + 120;
        }
    }

    UpdateParticles(now);
    Render(now);
}

void PetOverlay::StepGround(float dt, uint64_t now) {
    const auto& s = SettingsManager::Instance().GetSettings();

    if (s.gravity && m_state != PetState::SLEEP && m_posY < m_maxY - 2.0f && !m_hopActive) {
        m_state = PetState::FLY;
        m_velocityX = 0.0f;
        m_velocityY = 0.0f;
        return;
    }

    if (m_state == PetState::WALK) {
        int dir = (m_walkTarget >= m_posX) ? 1 : -1;
        m_look = dir;
        m_posX += dir * m_unitPx * 12.0f * dt * s.walkSpeed;

        bool reached = (dir > 0) ? (m_posX >= m_walkTarget) : (m_posX <= m_walkTarget);
        bool edge = (dir < 0 && m_posX <= m_minX) || (dir > 0 && m_posX >= m_maxX);

        if (reached || edge) {
            m_posX = std::max((float)m_minX, std::min((float)m_maxX, m_posX));
            m_state = PetState::IDLE;
            m_nextActionAt = now + RandomBetweenU64(2500, 6000);

            int side = m_peekPending;
            m_peekPending = 0;
            if (side != 0 && ((side < 0 && m_posX <= m_minX + 8.0f) || (side > 0 && m_posX >= m_maxX - 8.0f))) {
                StartPeek(now, side);
            } else {
                SavePosition();
            }
        }
        UpdateLayout();
        return;
    }

    if (m_state == PetState::SLEEP) {
        if (now >= m_nextZAt) {
            m_nextZAt = now + 1650;
            SpawnParticle(ParticleKind::ZZZ);
        }
        return;
    }

    if (now >= m_nextBlinkAt) {
        m_blinkUntil = now + 130;
        m_nextBlinkAt = now + (RandomBetween(0.0f, 1.0f) < 0.15f ? 450 : RandomBetweenU64(2000, 5500));
    }

    if (s.sleepMinutes > 0 && (now - m_lastInteractionAt > (uint64_t)s.sleepMinutes * 60000) &&
        !m_hopActive && !m_mediaPlaying && m_state == PetState::IDLE) {
        m_state = PetState::SLEEP;
        m_look = 0;
        m_nextZAt = now + 500;
        m_bubble.Hide();
        return;
    }

    if (s.poop && now >= m_nextPoopAt && m_state == PetState::IDLE && !m_hopActive) {
        m_nextPoopAt = now + RandomBetweenU64(150000, 360000);
        PoopNow();
        return;
    }

    if (m_mediaPlaying && s.dance) {
        if (now >= m_nextNoteAt) {
            m_nextNoteAt = now + 1050;
            SpawnParticle(ParticleKind::NOTE);
        }
        return;
    }

    if (now < m_nextActionAt || m_hopActive) return;
    m_nextActionAt = now + RandomBetweenU64(3500, 9000);

    if (!s.wander) {
        int r = rand() % 100;
        if (r < 20) m_look = (rand() % 3) - 1;
        else if (r < 40) Say("idle");
        else if (r < 55) Hop(m_unitPx * 70.0f);
        else if (r < 65) m_waveUntil = now + 1600;
        else if (r < 75) { m_idlePose = PetPose::STRETCH; m_idlePoseStartedAt = now; m_idlePoseUntil = now + 1400; Say("stretch"); }
        else if (r < 85) { m_idlePose = PetPose::YAWN; m_idlePoseStartedAt = now; m_idlePoseUntil = now + 1200; Say("yawn"); }
        else if (r < 93) { m_idlePose = PetPose::GROOM; m_idlePoseStartedAt = now; m_idlePoseUntil = now + 1800; Say("groom"); }
        else { m_idlePose = PetPose::SPIN; m_idlePoseStartedAt = now; m_idlePoseUntil = now + 900; Say("spin"); }
        return;
    }

    if ((now - m_lastInteractionAt > 40000) && now >= m_nextPeekAt && RandomBetween(0.0f, 1.0f) < 0.22f) {
        int side = (m_posX - m_minX < m_maxX - m_posX) ? -1 : 1;
        m_peekPending = side;
        m_walkTarget = (side < 0) ? (float)m_minX : (float)m_maxX;
        m_state = PetState::WALK;
        return;
    }

    int r = rand() % 100;
    if (r < 30) {
        float dist = RandomBetween(m_unitPx * 12.0f, m_unitPx * 45.0f) * ((rand() % 2 == 0) ? 1.0f : -1.0f);
        m_walkTarget = std::max((float)m_minX, std::min((float)m_maxX, m_posX + dist));
        if (std::abs(m_walkTarget - m_posX) > 8.0f) {
            m_state = PetState::WALK;
        }
    } else if (r < 40) {
        m_look = (rand() % 3) - 1;
    } else if (r < 55) {
        Say("idle");
    } else if (r < 68) {
        Hop(m_unitPx * 80.0f);
    } else if (r < 76) {
        m_waveUntil = now + 1600;
    } else if (r < 82) {
        m_idlePose = PetPose::STRETCH; m_idlePoseStartedAt = now; m_idlePoseUntil = now + 1400; Say("stretch");
    } else if (r < 88) {
        m_idlePose = PetPose::YAWN; m_idlePoseStartedAt = now; m_idlePoseUntil = now + 1200; Say("yawn");
    } else if (r < 94) {
        m_idlePose = PetPose::GROOM; m_idlePoseStartedAt = now; m_idlePoseUntil = now + 1800; Say("groom");
    } else {
        m_idlePose = PetPose::SPIN; m_idlePoseStartedAt = now; m_idlePoseUntil = now + 900; Say("spin");
    }
}

void PetOverlay::StepFlight(float dt, uint64_t now) {
    const auto& s = SettingsManager::Instance().GetSettings();
    m_posX += m_velocityX * dt;
    m_posY += m_velocityY * dt;

    if (s.gravity) {
        m_velocityY += m_unitPx * 650.0f * dt;
        m_velocityX *= std::exp(-0.32f * dt);
    } else {
        float friction = std::exp(-1.9f * dt);
        m_velocityX *= friction;
        m_velocityY *= friction;
    }

    if (m_posX < m_minX) {
        m_posX = (float)m_minX;
        m_velocityX = std::abs(m_velocityX) * 0.55f;
    } else if (m_posX > m_maxX) {
        m_posX = (float)m_maxX;
        m_velocityX = -std::abs(m_velocityX) * 0.55f;
    }

    if (m_posY < m_minY) {
        if (m_velocityY < -m_unitPx * 65.0f) {
            StartHang(now);
            return;
        }
        m_posY = (float)m_minY;
        m_velocityY = std::abs(m_velocityY) * 0.55f;
    } else if (m_posY > m_maxY) {
        m_posY = (float)m_maxY;
        if (s.gravity && m_velocityY < m_unitPx * 95.0f) {
            m_state = PetState::IDLE;
            m_velocityX = 0.0f;
            m_velocityY = 0.0f;
            m_squashUntil = now + 140;
            m_nextActionAt = now + RandomBetweenU64(2000, 5000);
            SavePosition();
        } else {
            m_velocityY = -std::abs(m_velocityY) * (s.gravity ? 0.42f : 0.55f);
            m_velocityX *= 0.78f;
            m_squashUntil = now + 120;
        }
    }

    if (m_state == PetState::FLY && !s.gravity && std::hypot(m_velocityX, m_velocityY) < m_unitPx * 10.0f) {
        m_state = PetState::IDLE;
        m_velocityX = 0.0f;
        m_velocityY = 0.0f;
        SavePosition();
    }

    UpdateLayout();
}

void PetOverlay::StartHang(uint64_t now) {
    m_state = PetState::HANG;
    m_velocityX = 0.0f;
    m_velocityY = 0.0f;
    m_posY = m_minY - m_unitPx * 1.5f;
    m_hangStartedAt = now;
    m_hangUntil = now + RandomBetweenU64(2000, 3500);
    UpdateLayout();
    if (RandomBetween(0.0f, 1.0f) < 0.7f) Say("hang");
}

void PetOverlay::StepHang(uint64_t now) {
    m_posY = m_minY - m_unitPx * 1.5f;
    UpdateLayout();
    if (now < m_hangUntil) return;

    m_state = PetState::FLY;
    m_velocityX = 0.0f;
    m_velocityY = m_unitPx * 40.0f;
    SetExpression(PetExpression::SURPRISED, 700);
    if (RandomBetween(0.0f, 1.0f) < 0.6f) Say("hang_fall");
}

void PetOverlay::StartPeek(uint64_t now, int side) {
    m_state = PetState::PEEK;
    m_peekSide = side;
    m_peekOut = false;
    m_peekMoving = true;
    m_peekPhaseUntil = now + RandomBetweenU64(2500, 5000);
    m_peekEndAt = now + RandomBetweenU64(25000, 45000);
    m_look = side;
}

void PetOverlay::EndPeek(uint64_t now) {
    int side = m_peekSide;
    m_peekSide = 0;
    m_peekMoving = false;
    m_nextPeekAt = now + RandomBetweenU64(60000, 120000);
    m_state = PetState::WALK;
    m_walkTarget = (side < 0) ? (m_minX + m_unitPx * 8.0f) : (m_maxX - m_unitPx * 8.0f);
    m_nextActionAt = now + RandomBetweenU64(3000, 6000);
}

void PetOverlay::StepPeek(float dt, uint64_t now) {
    float hiddenUnits = 3.2f;
    float visibleUnits = 7.0f;

    RECT workArea;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);

    float targetX = (m_peekSide > 0)
        ? (workArea.right - m_unitPx * (m_peekOut ? visibleUnits : hiddenUnits))
        : (workArea.left - m_windowW + m_unitPx * (m_peekOut ? visibleUnits : hiddenUnits));

    float delta = targetX - m_posX;
    float step = m_unitPx * 14.0f * dt;
    m_peekMoving = std::abs(delta) > 1.0f;

    if (m_peekMoving) {
        SetPosition(m_posX + std::max(-step, std::min(step, delta)), m_posY, true);
    }
    m_look = (!m_peekOut && m_peekMoving) ? m_peekSide : -m_peekSide;

    if (now < m_peekPhaseUntil || m_peekMoving) return;
    if (now >= m_peekEndAt) {
        EndPeek(now);
        return;
    }

    if (!m_peekOut) {
        m_peekOut = true;
        m_peekPhaseUntil = now + RandomBetweenU64(1400, 2800);
        if (RandomBetween(0.0f, 1.0f) < 0.55f) Say("peek");
    } else {
        m_peekOut = false;
        m_peekPhaseUntil = now + RandomBetweenU64(3000, 7000);
    }
}

void PetOverlay::OnLButtonDown(int x, int y) {
    uint64_t now = GetTickCount64();
    POINT pt;
    GetCursorPos(&pt);

    bool wasSleeping = (m_state == PetState::SLEEP);
    bool wasPeeking = (m_state == PetState::PEEK);

    m_lastInteractionAt = now;
    m_mouseDown = true;
    m_mouseDownAt = now;
    m_dragStartX = pt.x;
    m_dragStartY = pt.y;
    m_dragOffsetX = pt.x - (int)m_posX;
    m_dragOffsetY = pt.y - (int)m_posY;
    m_anchorX = pt.x;
    m_petDirection = 0;
    m_samples.clear();
    m_samples.push_back({now, (float)pt.x, (float)pt.y});

    m_hopActive = false;
    m_hopOffset = 0.0f;
    m_hopVelocity = 0.0f;
    m_velocityX = 0.0f;
    m_velocityY = 0.0f;
    m_peekSide = 0;
    m_peekPending = 0;
    m_state = PetState::PRESS;

    if (wasSleeping) {
        SetExpression(PetExpression::SURPRISED, 800);
        Say("wake");
    } else if (wasPeeking) {
        m_nextPeekAt = now + RandomBetweenU64(60000, 120000);
        SetExpression(PetExpression::SURPRISED, 650);
        Say("found");
    }
}

void PetOverlay::OnMouseMove(int x, int y) {
    if (!m_mouseDown) return;
    uint64_t now = GetTickCount64();
    POINT pt;
    GetCursorPos(&pt);

    m_samples.push_back({now, (float)pt.x, (float)pt.y});
    while (m_samples.size() > 6) m_samples.erase(m_samples.begin());

    if (m_state == PetState::PRESS && std::hypot((float)pt.x - m_dragStartX, (float)pt.y - m_dragStartY) > 8.0f) {
        m_state = PetState::DRAG;
        if (RandomBetween(0.0f, 1.0f) < 0.25f) Say("drag");
    }

    if (m_state == PetState::DRAG) {
        SetPosition((float)pt.x - m_dragOffsetX, (float)pt.y - m_dragOffsetY);
        if (m_samples.size() >= 2) {
            uint64_t dt = now - m_samples.front().timeMs;
            if (dt > 5) {
                m_dragVelX = ((float)pt.x - m_samples.front().x) / (dt / 1000.0f);
                m_dragVelY = ((float)pt.y - m_samples.front().y) / (dt / 1000.0f);
            }
        }
    } else if (m_state == PetState::PET) {
        int dx = pt.x - m_anchorX;
        if (std::abs(dx) > 12) {
            int dir = (dx > 0) ? 1 : -1;
            if (m_petDirection != 0 && dir != m_petDirection) {
                SpawnParticle(ParticleKind::HEART);
            }
            m_petDirection = dir;
            m_anchorX = pt.x;
        }
    }
}

void PetOverlay::OnLButtonUp(int x, int y) {
    if (!m_mouseDown) return;
    m_mouseDown = false;
    uint64_t now = GetTickCount64();
    POINT pt;
    GetCursorPos(&pt);

    if (m_state == PetState::DRAG) {
        float vx = 0.0f, vy = 0.0f;
        if (!m_samples.empty()) {
            uint64_t dt = now - m_samples.front().timeMs;
            if (dt > 5 && dt < 120) {
                vx = ((float)pt.x - m_samples.front().x) / (dt / 1000.0f);
                vy = ((float)pt.y - m_samples.front().y) / (dt / 1000.0f);
            }
        }

        const auto& s = SettingsManager::Instance().GetSettings();
        bool wasFling = (std::hypot(vx, vy) > m_unitPx * 160.0f);
        if (std::abs(vy) > m_unitPx * 170.0f && m_posY <= m_minY + m_unitPx * 2.0f) {
            StartHang(now);
        } else if (wasFling || (s.gravity && m_posY < m_maxY - 4.0f)) {
            m_state = PetState::FLY;
            m_velocityX = std::max(-1800.0f, std::min(1800.0f, vx));
            m_velocityY = std::max(-1800.0f, std::min(1800.0f, vy));
            if (wasFling) Say("throw");
        } else {
            m_state = PetState::IDLE;
            m_squashUntil = now + 130;
            m_nextActionAt = now + RandomBetweenU64(2000, 5000);
            SavePosition();
        }
    } else if (m_state == PetState::PET) {
        EndPet(now);
    } else if (m_state == PetState::PRESS) {
        if (now - m_mouseDownAt >= 450) {
            BeginPet(now);
            EndPet(now);
        } else {
            m_state = PetState::IDLE;
            OnTap(now);
        }
    } else {
        m_state = PetState::IDLE;
    }
}

void PetOverlay::OnTap(uint64_t now) {
    if (now - m_lastTapAt < 380) {
        m_lastTapAt = 0;
        Hop(m_unitPx * 160.0f);
        SetExpression(PetExpression::HAPPY, 1300);
        for (int i = 0; i < 3; i++) SpawnParticle(ParticleKind::SPARK);
        if (RandomBetween(0.0f, 1.0f) < 0.4f) Say("jump");
        return;
    }

    m_lastTapAt = now;
    while (!m_recentTaps.empty() && now - m_recentTaps.front() > 2500) {
        m_recentTaps.erase(m_recentTaps.begin());
    }
    m_recentTaps.push_back(now);

    if (m_recentTaps.size() >= 6) {
        m_recentTaps.clear();
        SetExpression(PetExpression::ANGRY, 2000);
        Say("annoyed", true);
        return;
    }

    m_squashUntil = now + 130;
    SetExpression(PetExpression::SURPRISED, 450);
    Hop(m_unitPx * 75.0f);
    if (RandomBetween(0.0f, 1.0f) < 0.35f) Say("tap");
}

void PetOverlay::BeginPet(uint64_t now) {
    if (m_state != PetState::PRESS && m_state != PetState::PET) return;
    m_state = PetState::PET;
    m_nextHeartAt = now + 100;
}

void PetOverlay::EndPet(uint64_t now) {
    m_state = PetState::IDLE;
    SetExpression(PetExpression::HAPPY, 2200);
    m_nextActionAt = now + RandomBetweenU64(3000, 6000);

    auto& s = SettingsManager::Instance().GetMutableSettings();
    s.petsCount++;
    SettingsManager::Instance().Save();

    if (RandomBetween(0.0f, 1.0f) < 0.65f) Say("pet");
}

void PetOverlay::OnRButtonUp(int x, int y) {
    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, 1, L"Позвать в центр");
    AppendMenuW(hMenu, MF_STRING, 2, L"Сделать кучку");
    AppendMenuW(hMenu, MF_STRING, 3, L"Очистить все кучки");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

    HMENU hSubSpecies = CreatePopupMenu();
    AppendMenuW(hSubSpecies, MF_STRING, 10, L"GUPet");
    AppendMenuW(hSubSpecies, MF_STRING, 11, L"Кошка");
    AppendMenuW(hSubSpecies, MF_STRING, 12, L"Собака");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hSubSpecies, L"Вид питомца");

    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, 20, L"Настройки...");
    AppendMenuW(hMenu, MF_STRING, 21, L"Скрыть питомца");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, 30, L"Выход");

    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(m_hwnd);
    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hwnd, NULL);
    DestroyMenu(hMenu);

    if (cmd == 1) {
        Recenter();
    } else if (cmd == 2) {
        PoopNow();
    } else if (cmd == 3) {
        CleanAllPoops();
    } else if (cmd == 10) {
        auto& s = SettingsManager::Instance().GetMutableSettings();
        s.species = PetSpecies::GUPET;
        s.customSkinId.clear();
        m_customSkin = nullptr;
        SettingsManager::Instance().Save();
    } else if (cmd == 11) {
        auto& s = SettingsManager::Instance().GetMutableSettings();
        s.species = PetSpecies::CAT;
        s.customSkinId.clear();
        m_customSkin = nullptr;
        SettingsManager::Instance().Save();
    } else if (cmd == 12) {
        auto& s = SettingsManager::Instance().GetMutableSettings();
        s.species = PetSpecies::DOG;
        s.customSkinId.clear();
        m_customSkin = nullptr;
        SettingsManager::Instance().Save();
    } else if (cmd == 20) {
        PostMessage(m_hwnd, WM_COMMAND, ID_TRAY_SETTINGS, 0);
    } else if (cmd == 21) {
        SetVisible(false);
    } else if (cmd == 30) {
        PostQuitMessage(0);
    }
}

void PetOverlay::Render(uint64_t now) {
    if (!m_hwnd || m_windowW <= 0 || m_windowH <= 0) return;

    PetPose pose = PetPose::IDLE;
    switch (m_state) {
        case PetState::WALK: pose = PetPose::WALK; break;
        case PetState::PRESS: pose = PetPose::PRESS; break;
        case PetState::DRAG: pose = PetPose::DRAG; break;
        case PetState::PET: pose = PetPose::PET; break;
        case PetState::FLY: pose = PetPose::FLY; break;
        case PetState::SLEEP: pose = PetPose::SLEEP; break;
        case PetState::HANG: pose = PetPose::HANG; break;
        case PetState::PEEK: pose = PetPose::PEEK; break;
        case PetState::IDLE:
            if (m_mediaPlaying && SettingsManager::Instance().GetSettings().dance) {
                switch ((now / 700) % 10) {
                    case 4: pose = PetPose::STRETCH; break;
                    case 7: pose = PetPose::SPIN; break;
                    case 9: pose = PetPose::GROOM; break;
                    default: pose = PetPose::DANCE; break;
                }
            } else if (now < m_idlePoseUntil) {
                pose = m_idlePose;
            } else {
                pose = PetPose::IDLE;
            }
            break;
    }

    PetExpression face = PetExpression::OPEN;
    if (m_state == PetState::SLEEP) {
        face = PetExpression::SLEEP;
    } else if (now < m_expressionUntil) {
        face = m_frame.expression;
    } else if (now < m_blinkUntil && (m_state == PetState::IDLE || m_state == PetState::WALK ||
                                      m_state == PetState::PEEK || m_state == PetState::HANG)) {
        face = PetExpression::BLINK;
    }

    int waveSide = 0;
    if (pose == PetPose::PEEK && m_peekOut && !m_peekMoving && (now / 220) % 2 == 0) {
        waveSide = (m_peekSide < 0) ? -1 : 1;
    }
    if (now < m_waveUntil && (now / 240) % 2 == 0) {
        waveSide = ((now / 240) % 4 == 0) ? -1 : 1;
    }
    if (m_mediaPlaying && SettingsManager::Instance().GetSettings().dance && m_state == PetState::IDLE) {
        face = PetExpression::HAPPY;
        waveSide = ((now / 240) % 2 == 0) ? -1 : 1;
    }

    float rotation = 0.0f;
    if (m_state == PetState::FLY) {
        rotation = std::max(-25.0f, std::min(25.0f, m_velocityX / 15.0f));
    } else if (m_state == PetState::HANG) {
        rotation = std::sin((now - m_hangStartedAt) / 1000.0f * 3.0f) * 6.0f;
    } else if (pose == PetPose::SPIN) {
        rotation = ((now % 900) / 900.0f) * 360.0f;
    } else if (face == PetExpression::DIZZY && now < m_expressionUntil) {
        rotation = std::sin(now / 100.0f) * 7.0f;
    } else if (m_mediaPlaying && m_state == PetState::IDLE) {
        rotation = std::sin(now / 250.0f) * 5.0f;
    } else if (m_state == PetState::DRAG) {
        rotation = std::max(-18.0f, std::min(18.0f, m_dragVelX / 25.0f));
    }

    float squash = 0.0f;
    if (m_hopActive && m_hopVelocity < 0.0f) squash = -1.0f;
    else if (now < m_squashUntil) squash = 1.0f;

    m_frame.pose = pose;
    m_frame.expression = face;
    m_frame.look = m_look;
    m_frame.waveSide = waveSide;
    m_frame.blush = (m_state == PetState::PET || face == PetExpression::HAPPY || pose == PetPose::DANCE);
    m_frame.mouthOpen = (now < m_mouthOpenUntil || (m_state == PetState::PET && (now / 180) % 2 == 0));
    m_frame.squash = squash;
    m_frame.rotation = rotation;
    m_frame.hopOffset = m_hopOffset;
    m_frame.phase = now / 1000.0f;

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = m_windowW;
    bmi.bmiHeader.biHeight = -m_windowH;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    void* pBits = nullptr;
    HBITMAP hBmp = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcMem, hBmp);

    Bitmap bmp(m_windowW, m_windowH, m_windowW * 4, PixelFormat32bppPARGB, (BYTE*)pBits);
    Graphics g(&bmp);
    g.SetSmoothingMode(SmoothingModeNone);
    g.SetInterpolationMode(InterpolationModeNearestNeighbor);

    const auto& s = SettingsManager::Instance().GetSettings();
    PetRenderer::DrawPet(
        g,
        (float)m_windowW,
        (float)m_windowH,
        s.species,
        s.palette,
        m_frame,
        now,
        m_particles,
        m_customSkin
    );

    POINT ptSrc = {0, 0};
    POINT ptDst = {(int)m_posX, (int)m_posY};
    SIZE sizeWnd = {m_windowW, m_windowH};
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};

    UpdateLayeredWindow(m_hwnd, hdcScreen, &ptDst, &sizeWnd, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(hdcMem, hOldBmp);
    DeleteObject(hBmp);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

} // namespace GUPet
