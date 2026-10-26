#pragma once

#include "pet_types.h"
#include "speech_bubble.h"
#include "skin_store.h"

#include <windows.h>
#include <vector>
#include <memory>
#include <string>

namespace GUPet {

class PetOverlay {
public:
    static PetOverlay& Instance();

    bool Init();
    void Shutdown();

    void Recenter();
    void PoopNow();
    void CleanAllPoops();
    void SetVisible(bool visible);
    void ReloadSettings();
    void Say(const std::string& category, bool force = false);

    HWND GetHwnd() const { return m_hwnd; }

private:
    PetOverlay() = default;
    ~PetOverlay() { Shutdown(); }

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void OnTick();
    void Step();
    void StepGround(float dt, uint64_t now);
    void StepFlight(float dt, uint64_t now);
    void StepHang(uint64_t now);
    void StepPeek(float dt, uint64_t now);

    void Render(uint64_t now);
    void RefreshBounds();
    void UpdateLayout(bool force = false);
    void SetPosition(float x, float y, bool allowOffscreen = false);
    void SavePosition();
    void RestorePosition();
    void ResizeWindow();

    void Hop(float strength);
    void SetExpression(PetExpression expr, uint64_t durationMs);
    void SpawnParticle(ParticleKind kind, float originX = -1.0f, float originY = -1.0f);
    void UpdateParticles(uint64_t now);

    void StartHang(uint64_t now);
    void StartPeek(uint64_t now, int side);
    void EndPeek(uint64_t now);

    void OnLButtonDown(int x, int y);
    void OnMouseMove(int x, int y);
    void OnLButtonUp(int x, int y);
    void OnRButtonUp(int x, int y);
    void OnTap(uint64_t now);
    void BeginPet(uint64_t now);
    void EndPet(uint64_t now);

    HWND m_hwnd = NULL;
    PetState m_state = PetState::IDLE;
    PetFrame m_frame;
    std::vector<PetParticle> m_particles;
    SpeechBubble m_bubble;
    std::shared_ptr<GUPetSkin> m_customSkin;

    float m_posX = 100.0f;
    float m_posY = 100.0f;
    float m_velocityX = 0.0f;
    float m_velocityY = 0.0f;
    float m_unitPx = 4.0f;
    int m_windowW = 72;
    int m_windowH = 68;

    int m_minX = 0;
    int m_maxX = 1920;
    int m_minY = 0;
    int m_maxY = 1080;

    int m_look = 0;
    int m_peekSide = 0;
    int m_peekPending = 0;
    bool m_peekOut = false;
    bool m_peekMoving = false;
    uint64_t m_peekPhaseUntil = 0;
    uint64_t m_peekEndAt = 0;
    uint64_t m_nextPeekAt = 0;

    uint64_t m_lastTickAt = 0;
    uint64_t m_lastInteractionAt = 0;
    uint64_t m_nextActionAt = 0;
    uint64_t m_nextBlinkAt = 0;
    uint64_t m_blinkUntil = 0;
    uint64_t m_expressionUntil = 0;
    uint64_t m_squashUntil = 0;
    uint64_t m_mouthOpenUntil = 0;
    uint64_t m_waveUntil = 0;
    uint64_t m_hangUntil = 0;
    uint64_t m_hangStartedAt = 0;
    uint64_t m_nextHeartAt = 0;
    uint64_t m_nextZAt = 0;
    uint64_t m_nextNoteAt = 0;
    uint64_t m_nextPoopAt = 0;

    PetPose m_idlePose = PetPose::IDLE;
    uint64_t m_idlePoseStartedAt = 0;
    uint64_t m_idlePoseUntil = 0;

    bool m_hopActive = false;
    float m_hopOffset = 0.0f;
    float m_hopVelocity = 0.0f;

    // Mouse tracking
    bool m_mouseDown = false;
    int m_dragStartX = 0;
    int m_dragStartY = 0;
    int m_dragOffsetX = 0;
    int m_dragOffsetY = 0;
    int m_anchorX = 0;
    int m_petDirection = 0;
    uint64_t m_mouseDownAt = 0;
    float m_dragVelX = 0.0f;
    float m_dragVelY = 0.0f;

    struct MouseSample {
        uint64_t timeMs;
        float x;
        float y;
    };
    std::vector<MouseSample> m_samples;

    std::vector<uint64_t> m_recentTaps;
    uint64_t m_lastTapAt = 0;
    std::wstring m_lastPhrase;
    float m_walkTarget = 0.0f;
    bool m_mediaPlaying = false;
};

} // namespace GUPet
