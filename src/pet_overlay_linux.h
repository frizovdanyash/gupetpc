#pragma once

#include "pet_types.h"
#include "skin_store.h"
#include "pet_renderer_cairo.h"

#include <gtk/gtk.h>
#include <vector>
#include <memory>
#include <string>

namespace GUPet {

struct PoopItemLinux {
    int x;
    int y;
    float scale;
    float alpha;
    GtkWidget* window;
};

class PetOverlayLinux {
public:
    static PetOverlayLinux& Instance();

    bool Init(int argc, char* argv[]);
    void Run();
    void Shutdown();

    void Recenter();
    void PoopNow();
    void CleanAllPoops();
    void SetVisible(bool visible);
    void ReloadSettings();
    void Say(const std::string& category, bool force = false);

    const std::string& GetBubbleText() const { return m_bubbleText; }
    void OnPoopClicked(GtkWidget* w);

private:
    PetOverlayLinux() = default;
    ~PetOverlayLinux() { Shutdown(); }

    static gboolean OnTickStatic(gpointer data);
    static gboolean OnDrawStatic(GtkWidget* widget, cairo_t* cr, gpointer data);
    static gboolean OnButtonPressStatic(GtkWidget* widget, GdkEventButton* event, gpointer data);
    static gboolean OnButtonReleaseStatic(GtkWidget* widget, GdkEventButton* event, gpointer data);
    static gboolean OnMotionNotifyStatic(GtkWidget* widget, GdkEventMotion* event, gpointer data);

    void OnTick();
    void Step();
    void StepGround(float dt, uint64_t now);
    void StepFlight(float dt, uint64_t now);
    void StepHang(uint64_t now);
    void StepPeek(float dt, uint64_t now);

    void Render(cairo_t* cr, uint64_t now);
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

    void OnButtonDown(double x, double y, guint button);
    void OnButtonUp(double x, double y, guint button);
    void OnMouseMove(double x, double y);
    void OnTap(uint64_t now);
    void BeginPet(uint64_t now);
    void EndPet(uint64_t now);
    void ShowContextMenu(guint button, guint32 time);

    void CreateBubbleWindow();
    void ShowBubble(const std::string& text);
    void UpdateBubblePos();
    void HideBubble();

    void CreatePoopWindow(int x, int y);

    GtkWidget* m_window = nullptr;
    GtkWidget* m_bubbleWindow = nullptr;
    std::string m_bubbleText;
    uint64_t m_bubbleHideAt = 0;

    std::vector<PoopItemLinux> m_poops;

    PetState m_state = PetState::IDLE;
    PetFrame m_frame;
    std::vector<PetParticle> m_particles;
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
    uint64_t m_nextPoopAt = 0;

    PetPose m_idlePose = PetPose::IDLE;
    uint64_t m_idlePoseUntil = 0;

    bool m_hopActive = false;
    float m_hopOffset = 0.0f;
    float m_hopVelocity = 0.0f;

    bool m_mouseDown = false;
    double m_dragStartX = 0;
    double m_dragStartY = 0;
    double m_dragOffsetX = 0;
    double m_dragOffsetY = 0;
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
    std::string m_lastPhrase;
    float m_walkTarget = 0.0f;
};

} // namespace GUPet
