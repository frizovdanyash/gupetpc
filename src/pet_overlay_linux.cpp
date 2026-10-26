#include "pet_overlay_linux.h"
#include "settings_manager.h"
#include "settings_dialog_linux.h"
#include "phrases.h"

#include <chrono>
#include <random>
#include <cmath>
#include <pango/pangocairo.h>
#include <iostream>

namespace GUPet {

static uint64_t GetNowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

static float RandomFloat(float minVal, float maxVal) {
    static std::mt19937 gen(42);
    std::uniform_real_distribution<float> dis(minVal, maxVal);
    return dis(gen);
}

static uint64_t RandomU64(uint64_t minVal, uint64_t maxVal) {
    static std::mt19937_64 gen(42);
    std::uniform_int_distribution<uint64_t> dis(minVal, maxVal);
    return dis(gen);
}

static gboolean OnDrawBubbleStatic(GtkWidget* w, cairo_t* cr, gpointer data) {
    PetOverlayLinux* self = (PetOverlayLinux*)data;
    int width = gtk_widget_get_allocated_width(w);
    int height = gtk_widget_get_allocated_height(w);

    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    // Draw bubble shadow & background
    double r = 12.0;
    double tailH = 6.0;
    double bubbleH = height - tailH;

    cairo_new_sub_path(cr);
    cairo_arc(cr, width - r, r, r, -M_PI/2, 0);
    cairo_arc(cr, width - r, bubbleH - r, r, 0, M_PI/2);
    cairo_arc(cr, r, bubbleH - r, r, M_PI/2, M_PI);
    cairo_arc(cr, r, r, r, M_PI, 3*M_PI/2);
    cairo_close_path(cr);

    // Bubble fill
    cairo_set_source_rgba(cr, 0.98, 0.96, 0.95, 0.95);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0.2, 0.15, 0.13, 0.2);
    cairo_set_line_width(cr, 1.5);
    cairo_stroke(cr);

    // Text
    PangoLayout* layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(layout, self->GetBubbleText().c_str(), -1);
    PangoFontDescription* font = pango_font_description_from_string("Sans Bold 10");
    pango_layout_set_font_description(layout, font);
    pango_font_description_free(font);

    int textW, textH;
    pango_layout_get_pixel_size(layout, &textW, &textH);
    cairo_move_to(cr, (width - textW) / 2.0, (bubbleH - textH) / 2.0);
    cairo_set_source_rgba(cr, 0.15, 0.12, 0.11, 1.0);
    pango_cairo_show_layout(cr, layout);
    g_object_unref(layout);

    return FALSE;
}

static gboolean OnDrawPoopStatic(GtkWidget* w, cairo_t* cr, gpointer data) {
    int width = gtk_widget_get_allocated_width(w);
    int height = gtk_widget_get_allocated_height(w);
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    PetRendererCairo::DrawPoop(cr, (float)width, (float)height, 1.0f, 1.0f);
    return FALSE;
}

static gboolean OnPoopClickStatic(GtkWidget* w, GdkEventButton* event, gpointer data) {
    PetOverlayLinux* self = (PetOverlayLinux*)data;
    self->OnPoopClicked(w);
    return TRUE;
}

PetOverlayLinux& PetOverlayLinux::Instance() {
    static PetOverlayLinux s_instance;
    return s_instance;
}

gboolean PetOverlayLinux::OnTickStatic(gpointer data) {
    PetOverlayLinux* self = (PetOverlayLinux*)data;
    self->OnTick();
    return TRUE;
}

gboolean PetOverlayLinux::OnDrawStatic(GtkWidget* widget, cairo_t* cr, gpointer data) {
    PetOverlayLinux* self = (PetOverlayLinux*)data;
    self->Render(cr, GetNowMs());
    return FALSE;
}

gboolean PetOverlayLinux::OnButtonPressStatic(GtkWidget* widget, GdkEventButton* event, gpointer data) {
    PetOverlayLinux* self = (PetOverlayLinux*)data;
    if (event->button == 1) {
        self->OnButtonDown(event->x_root, event->y_root, event->button);
    } else if (event->button == 3) {
        self->ShowContextMenu(event->button, event->time);
    }
    return TRUE;
}

gboolean PetOverlayLinux::OnButtonReleaseStatic(GtkWidget* widget, GdkEventButton* event, gpointer data) {
    PetOverlayLinux* self = (PetOverlayLinux*)data;
    if (event->button == 1) {
        self->OnButtonUp(event->x_root, event->y_root, event->button);
    }
    return TRUE;
}

gboolean PetOverlayLinux::OnMotionNotifyStatic(GtkWidget* widget, GdkEventMotion* event, gpointer data) {
    PetOverlayLinux* self = (PetOverlayLinux*)data;
    self->OnMouseMove(event->x_root, event->y_root);
    return TRUE;
}

bool PetOverlayLinux::Init(int argc, char* argv[]) {
    gtk_init(&argc, &argv);

    m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_app_paintable(m_window, TRUE);
    gtk_window_set_decorated(GTK_WINDOW(m_window), FALSE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(m_window), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(m_window), TRUE);
    gtk_window_set_keep_above(GTK_WINDOW(m_window), TRUE);
    gtk_window_stick(GTK_WINDOW(m_window));

    GdkScreen* screen = gtk_widget_get_screen(m_window);
    GdkVisual* visual = gdk_screen_get_rgba_visual(screen);
    if (visual) {
        gtk_widget_set_visual(m_window, visual);
    }

    gtk_widget_add_events(m_window, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);

    g_signal_connect(m_window, "draw", G_CALLBACK(OnDrawStatic), this);
    g_signal_connect(m_window, "button-press-event", G_CALLBACK(OnButtonPressStatic), this);
    g_signal_connect(m_window, "button-release-event", G_CALLBACK(OnButtonReleaseStatic), this);
    g_signal_connect(m_window, "motion-notify-event", G_CALLBACK(OnMotionNotifyStatic), this);

    CreateBubbleWindow();
    ReloadSettings();
    ResizeWindow();
    RestorePosition();

    uint64_t now = GetNowMs();
    m_lastTickAt = now;
    m_lastInteractionAt = now;
    m_nextActionAt = now + 3500;
    m_nextBlinkAt = now + 2000;
    m_nextPoopAt = now + RandomU64(120000, 300000);
    m_nextPeekAt = now + 45000;

    g_timeout_add(16, (GSourceFunc)OnTickStatic, this);

    if (SettingsManager::Instance().GetSettings().show) {
        gtk_widget_show_all(m_window);
        Say("hello");
    }

    return true;
}

void PetOverlayLinux::Run() {
    gtk_main();
}

void PetOverlayLinux::Shutdown() {
    if (m_window) {
        gtk_widget_destroy(m_window);
        m_window = nullptr;
    }
    if (m_bubbleWindow) {
        gtk_widget_destroy(m_bubbleWindow);
        m_bubbleWindow = nullptr;
    }
    CleanAllPoops();
}

void PetOverlayLinux::CreateBubbleWindow() {
    m_bubbleWindow = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_widget_set_app_paintable(m_bubbleWindow, TRUE);
    gtk_window_set_decorated(GTK_WINDOW(m_bubbleWindow), FALSE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(m_bubbleWindow), TRUE);
    gtk_window_set_keep_above(GTK_WINDOW(m_bubbleWindow), TRUE);

    GdkScreen* screen = gtk_widget_get_screen(m_bubbleWindow);
    GdkVisual* visual = gdk_screen_get_rgba_visual(screen);
    if (visual) gtk_widget_set_visual(m_bubbleWindow, visual);

    g_signal_connect(m_bubbleWindow, "draw", G_CALLBACK(OnDrawBubbleStatic), this);
}

void PetOverlayLinux::ShowBubble(const std::string& text) {
    if (!m_bubbleWindow) return;
    m_bubbleText = text;
    m_bubbleHideAt = GetNowMs() + 3800;

    int w = std::max(100, (int)text.size() * 9 + 28);
    int h = 42;
    gtk_window_resize(GTK_WINDOW(m_bubbleWindow), w, h);
    UpdateBubblePos();
    gtk_widget_show_all(m_bubbleWindow);
    gtk_widget_queue_draw(m_bubbleWindow);
}

void PetOverlayLinux::UpdateBubblePos() {
    if (!m_bubbleWindow || !gtk_widget_get_visible(m_bubbleWindow)) return;
    int bw, bh;
    gtk_window_get_size(GTK_WINDOW(m_bubbleWindow), &bw, &bh);
    int x = (int)(m_posX + (m_windowW - bw) / 2.0f);
    int y = (int)(m_posY - bh - 6);
    gtk_window_move(GTK_WINDOW(m_bubbleWindow), x, y);
}

void PetOverlayLinux::HideBubble() {
    if (m_bubbleWindow) {
        gtk_widget_hide(m_bubbleWindow);
    }
}

void PetOverlayLinux::OnPoopClicked(GtkWidget* w) {
    for (auto it = m_poops.begin(); it != m_poops.end(); ++it) {
        if (it->window == w) {
            for (int i = 0; i < 4; i++) {
                SpawnParticle(ParticleKind::SPARK, (float)it->x, (float)it->y);
            }
            gtk_widget_destroy(w);
            m_poops.erase(it);
            auto& s = SettingsManager::Instance().GetMutableSettings();
            s.poopsCleaned++;
            SettingsManager::Instance().Save();
            Say("clean");
            break;
        }
    }
}

void PetOverlayLinux::CreatePoopWindow(int x, int y) {
    GtkWidget* win = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_widget_set_app_paintable(win, TRUE);
    gtk_window_set_decorated(GTK_WINDOW(win), FALSE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(win), TRUE);
    gtk_window_set_keep_above(GTK_WINDOW(win), TRUE);

    GdkScreen* screen = gtk_widget_get_screen(win);
    GdkVisual* visual = gdk_screen_get_rgba_visual(screen);
    if (visual) gtk_widget_set_visual(win, visual);

    gtk_widget_add_events(win, GDK_BUTTON_PRESS_MASK);

    int pSize = (int)(m_unitPx * 8.0f);
    pSize = std::max(28, pSize);
    gtk_window_resize(GTK_WINDOW(win), pSize, pSize);
    gtk_window_move(GTK_WINDOW(win), x, y);

    g_signal_connect(win, "draw", G_CALLBACK(OnDrawPoopStatic), nullptr);
    g_signal_connect(win, "button-press-event", G_CALLBACK(OnPoopClickStatic), this);

    gtk_widget_show_all(win);

    PoopItemLinux item;
    item.x = x;
    item.y = y;
    item.scale = 1.0f;
    item.alpha = 1.0f;
    item.window = win;
    m_poops.push_back(item);
}

void PetOverlayLinux::PoopNow() {
    int px = (int)m_posX + (m_look < 0 ? m_windowW - 10 : 0);
    int py = (int)m_posY + m_windowH - (int)(m_unitPx * 7.0f);
    CreatePoopWindow(px, py);
    Say("poop");
    SetExpression(PetExpression::HAPPY, 1700);
    for (int i = 0; i < 2; i++) SpawnParticle(ParticleKind::SPARK);
}

void PetOverlayLinux::CleanAllPoops() {
    for (auto& p : m_poops) {
        if (p.window) gtk_widget_destroy(p.window);
    }
    m_poops.clear();
}

void PetOverlayLinux::ReloadSettings() {
    const auto& s = SettingsManager::Instance().GetSettings();
    if (!s.customSkinId.empty()) {
        m_customSkin = SkinStore::Instance().LoadSkin(s.customSkinId);
    } else {
        m_customSkin = nullptr;
    }
    ResizeWindow();
}

void PetOverlayLinux::ResizeWindow() {
    const auto& s = SettingsManager::Instance().GetSettings();
    float scale = std::max(0.2f, s.sizeScale);
    m_unitPx = scale * 3.8f;
    m_windowW = std::max(28, (int)(m_unitPx * 18.0f));
    m_windowH = std::max(28, (int)(m_unitPx * 17.0f));

    if (m_window) {
        gtk_window_resize(GTK_WINDOW(m_window), m_windowW, m_windowH);
    }
    RefreshBounds();
    UpdateLayout(true);
}

void PetOverlayLinux::RefreshBounds() {
    GdkDisplay* display = gdk_display_get_default();
    if (!display) return;

    GdkMonitor* monitor = nullptr;
    if (m_window && gtk_widget_get_window(m_window)) {
        monitor = gdk_display_get_monitor_at_window(display, gtk_widget_get_window(m_window));
    }
    if (!monitor) {
        monitor = gdk_display_get_primary_monitor(display);
    }
    if (!monitor && gdk_display_get_n_monitors(display) > 0) {
        monitor = gdk_display_get_monitor(display, 0);
    }

    if (monitor) {
        GdkRectangle wa;
        gdk_monitor_get_workarea(monitor, &wa);
        m_minX = wa.x;
        m_maxX = std::max(m_minX, wa.x + wa.width - m_windowW);
        m_minY = wa.y;
        m_maxY = std::max(m_minY, wa.y + wa.height - m_windowH);
    } else {
        m_minX = 0;
        m_maxX = 1920 - m_windowW;
        m_minY = 0;
        m_maxY = 1080 - m_windowH;
    }

    if (m_state != PetState::PEEK) {
        m_posX = std::max((float)m_minX, std::min((float)m_maxX, m_posX));
        m_posY = std::max((float)m_minY, std::min((float)m_maxY, m_posY));
    }
}

void PetOverlayLinux::RestorePosition() {
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

void PetOverlayLinux::SavePosition() {
    auto& s = SettingsManager::Instance().GetMutableSettings();
    s.posX = m_posX;
    s.posY = m_posY;
    s.positionSaved = true;
    SettingsManager::Instance().Save();
}

void PetOverlayLinux::SetPosition(float x, float y, bool allowOffscreen) {
    m_posX = allowOffscreen ? x : std::max((float)m_minX, std::min((float)m_maxX, x));
    m_posY = std::max((float)m_minY, std::min((float)m_maxY, y));
    UpdateLayout();
}

void PetOverlayLinux::UpdateLayout(bool force) {
    if (!m_window) return;
    gtk_window_move(GTK_WINDOW(m_window), (int)m_posX, (int)m_posY);
    UpdateBubblePos();
}

void PetOverlayLinux::SetVisible(bool visible) {
    auto& s = SettingsManager::Instance().GetMutableSettings();
    s.show = visible;
    SettingsManager::Instance().Save();

    if (visible) {
        gtk_widget_show_all(m_window);
        Say("hello");
    } else {
        gtk_widget_hide(m_window);
        HideBubble();
        CleanAllPoops();
    }
}

void PetOverlayLinux::Recenter() {
    RefreshBounds();
    m_state = PetState::IDLE;
    m_velocityX = 0.0f;
    m_velocityY = 0.0f;
    m_posX = (m_minX + m_maxX) / 2.0f;
    m_posY = (float)m_maxY;
    UpdateLayout(true);
    Say("hello");
    SetExpression(PetExpression::HAPPY, 1500);
    SavePosition();
}

void PetOverlayLinux::Say(const std::string& category, bool force) {
    const auto& s = SettingsManager::Instance().GetSettings();
    if (!force && !s.phrases) return;

    std::string phrase = PetPhrases::GetRandomUtf8(category, m_lastPhrase);
    if (!phrase.empty()) {
        m_lastPhrase = phrase;
        ShowBubble(phrase);
    }
}

void PetOverlayLinux::Hop(float strength) {
    if (m_state != PetState::IDLE && m_state != PetState::WALK) return;
    m_hopActive = true;
    m_hopVelocity = -strength;
    if (m_hopOffset > 0.0f) m_hopOffset = 0.0f;
}

void PetOverlayLinux::SetExpression(PetExpression expr, uint64_t durationMs) {
    m_frame.expression = expr;
    m_expressionUntil = GetNowMs() + durationMs;
}

void PetOverlayLinux::SpawnParticle(ParticleKind kind, float originX, float originY) {
    if (m_particles.size() >= 12) {
        m_particles.erase(m_particles.begin());
    }

    PetParticle p;
    p.kind = kind;
    p.bornAtMs = GetNowMs();
    p.durationMs = (kind == ParticleKind::ZZZ) ? 1600 : 1100;
    p.driftX = RandomFloat(-m_unitPx * 5.0f, m_unitPx * 5.0f);

    float cell = m_unitPx * 0.72f;
    if (originX >= 0.0f) {
        p.x = originX - m_posX;
        p.y = originY - m_posY;
    } else {
        p.x = m_windowW / 2.0f - cell * 2.5f + RandomFloat(-cell * 2.0f, cell * 2.0f);
        p.y = (kind == ParticleKind::ZZZ) ? (m_unitPx * 4.2f) : (m_unitPx * 3.1f);
    }

    m_particles.push_back(p);
}

void PetOverlayLinux::UpdateParticles(uint64_t now) {
    for (size_t i = 0; i < m_particles.size(); ) {
        if (now - m_particles[i].bornAtMs >= m_particles[i].durationMs) {
            m_particles.erase(m_particles.begin() + i);
        } else {
            i++;
        }
    }
}

void PetOverlayLinux::OnTick() {
    if (!SettingsManager::Instance().GetSettings().show) return;

    Step();
    if (m_bubbleHideAt > 0 && GetNowMs() >= m_bubbleHideAt) {
        HideBubble();
        m_bubbleHideAt = 0;
    }
    if (m_window) {
        gtk_widget_queue_draw(m_window);
    }
}

void PetOverlayLinux::Step() {
    uint64_t now = GetNowMs();
    RefreshBounds();

    float dt = std::max(0.0f, std::min(0.05f, (now - m_lastTickAt) / 1000.0f));
    m_lastTickAt = now;

    if (m_state == PetState::PRESS && (now - m_mouseDownAt >= 450)) {
        BeginPet(now);
    }

    switch (m_state) {
        case PetState::FLY: StepFlight(dt, now); break;
        case PetState::HANG: StepHang(now); break;
        case PetState::PEEK: StepPeek(dt, now); break;
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

    const auto& s = SettingsManager::Instance().GetSettings();
    if (s.poop && now >= m_nextPoopAt && m_state != PetState::SLEEP && m_state != PetState::PEEK && m_state != PetState::HANG) {
        m_nextPoopAt = now + RandomU64(180000, 480000);
        PoopNow();
    }
}

void PetOverlayLinux::StepGround(float dt, uint64_t now) {
    const auto& s = SettingsManager::Instance().GetSettings();

    if (s.sleepMinutes > 0) {
        uint64_t idleMs = now - m_lastInteractionAt;
        if (idleMs >= (uint64_t)s.sleepMinutes * 60000) {
            if (m_state != PetState::SLEEP) {
                m_state = PetState::SLEEP;
                m_frame.expression = PetExpression::SLEEP;
                m_idlePose = PetPose::SLEEP;
                Say("sleep");
            }
            if (now >= m_nextZAt) {
                m_nextZAt = now + 1400;
                SpawnParticle(ParticleKind::ZZZ);
            }
            return;
        }
    }

    if (m_state == PetState::SLEEP) {
        m_state = PetState::IDLE;
        m_frame.expression = PetExpression::OPEN;
    }

    if (m_state == PetState::WALK) {
        float speed = m_unitPx * 18.0f * s.walkSpeed;
        float delta = m_walkTarget - m_posX;
        m_look = (delta > 0) ? 1 : -1;

        if (std::abs(delta) <= speed * dt) {
            m_posX = m_walkTarget;
            m_state = PetState::IDLE;
            m_nextActionAt = now + RandomU64(2500, 6000);
            SavePosition();
        } else {
            m_posX += (delta > 0 ? 1 : -1) * speed * dt;
        }
        UpdateLayout();
        return;
    }

    if (!s.wander) return;

    if (now >= m_nextActionAt) {
        float r = RandomFloat(0.0f, 1.0f);
        if (r < 0.45f) {
            // Walk to random target
            m_state = PetState::WALK;
            m_walkTarget = RandomFloat((float)m_minX + m_unitPx * 6.0f, (float)m_maxX - m_unitPx * 6.0f);
            m_nextActionAt = now + RandomU64(4000, 9000);
            if (RandomFloat(0.0f, 1.0f) < 0.25f) Say("idle");
        } else if (r < 0.70f) {
            // Hop
            Hop(m_unitPx * RandomFloat(90.0f, 140.0f));
            m_nextActionAt = now + RandomU64(2000, 4500);
        } else if (r < 0.85f) {
            // Stretch or yawn
            m_idlePose = (RandomFloat(0.0f, 1.0f) < 0.5f) ? PetPose::STRETCH : PetPose::YAWN;
            m_idlePoseUntil = now + RandomU64(1500, 2500);
            m_nextActionAt = m_idlePoseUntil + RandomU64(1500, 3000);
        } else {
            // Blink / expression
            SetExpression(PetExpression::HAPPY, 1200);
            m_nextActionAt = now + RandomU64(2000, 4000);
        }
    }
}

void PetOverlayLinux::StepFlight(float dt, uint64_t now) {
    const auto& s = SettingsManager::Instance().GetSettings();
    float grav = s.gravity ? (m_unitPx * 380.0f) : 0.0f;
    m_velocityY += grav * dt;

    float drag = std::pow(0.92f, dt * 60.0f);
    m_velocityX *= drag;
    if (!s.gravity) m_velocityY *= drag;

    m_posX += m_velocityX * dt;
    m_posY += m_velocityY * dt;

    // Bounce on floor
    if (m_posY >= (float)m_maxY) {
        m_posY = (float)m_maxY;
        if (std::abs(m_velocityY) > m_unitPx * 40.0f) {
            m_velocityY = -m_velocityY * 0.45f;
            m_squashUntil = now + 120;
        } else {
            m_velocityY = 0.0f;
            m_state = PetState::IDLE;
            SavePosition();
        }
    }

    // Bounce on sides
    if (m_posX <= (float)m_minX) {
        m_posX = (float)m_minX;
        m_velocityX = -m_velocityX * 0.55f;
    } else if (m_posX >= (float)m_maxX) {
        m_posX = (float)m_maxX;
        m_velocityX = -m_velocityX * 0.55f;
    }

    // Hang on top
    if (m_posY <= (float)m_minY && m_velocityY < 0.0f) {
        StartHang(now);
        return;
    }

    if (m_state == PetState::FLY && !s.gravity && std::hypot(m_velocityX, m_velocityY) < m_unitPx * 10.0f) {
        m_state = PetState::IDLE;
        m_velocityX = 0.0f;
        m_velocityY = 0.0f;
        SavePosition();
    }

    UpdateLayout();
}

void PetOverlayLinux::StartHang(uint64_t now) {
    m_state = PetState::HANG;
    m_velocityX = 0.0f;
    m_velocityY = 0.0f;
    m_posY = (float)m_minY - m_unitPx * 1.5f;
    m_hangStartedAt = now;
    m_hangUntil = now + RandomU64(2000, 3500);
    UpdateLayout();
    if (RandomFloat(0.0f, 1.0f) < 0.7f) Say("hang");
}

void PetOverlayLinux::StepHang(uint64_t now) {
    m_posY = (float)m_minY - m_unitPx * 1.5f;
    UpdateLayout();
    if (now < m_hangUntil) return;

    m_state = PetState::FLY;
    m_velocityX = 0.0f;
    m_velocityY = m_unitPx * 40.0f;
    SetExpression(PetExpression::SURPRISED, 700);
    if (RandomFloat(0.0f, 1.0f) < 0.6f) Say("hang_fall");
}

void PetOverlayLinux::StartPeek(uint64_t now, int side) {
    m_state = PetState::PEEK;
    m_peekSide = side;
    m_peekOut = false;
    m_peekMoving = true;
    m_peekPhaseUntil = now + RandomU64(2500, 5000);
    m_peekEndAt = now + RandomU64(25000, 45000);
    m_look = side;
}

void PetOverlayLinux::EndPeek(uint64_t now) {
    int side = m_peekSide;
    m_peekSide = 0;
    m_peekMoving = false;
    m_nextPeekAt = now + RandomU64(60000, 120000);
    m_state = PetState::WALK;
    m_walkTarget = (side < 0) ? (m_minX + m_unitPx * 8.0f) : (m_maxX - m_unitPx * 8.0f);
    m_nextActionAt = now + RandomU64(3000, 6000);
}

void PetOverlayLinux::StepPeek(float dt, uint64_t now) {
    float hiddenUnits = 3.2f;
    float visibleUnits = 7.0f;

    float targetX = (m_peekSide > 0)
        ? (m_maxX + m_windowW - m_unitPx * (m_peekOut ? visibleUnits : hiddenUnits))
        : (m_minX - m_windowW + m_unitPx * (m_peekOut ? visibleUnits : hiddenUnits));

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
        m_peekPhaseUntil = now + RandomU64(1400, 2800);
        if (RandomFloat(0.0f, 1.0f) < 0.55f) Say("peek");
    } else {
        m_peekOut = false;
        m_peekPhaseUntil = now + RandomU64(3000, 7000);
    }
}

void PetOverlayLinux::OnButtonDown(double rootX, double rootY, guint button) {
    uint64_t now = GetNowMs();
    m_lastInteractionAt = now;
    m_mouseDown = true;
    m_mouseDownAt = now;
    m_dragStartX = rootX;
    m_dragStartY = rootY;
    m_dragOffsetX = rootX - m_posX;
    m_dragOffsetY = rootY - m_posY;
    m_samples.clear();
    m_samples.push_back({now, (float)rootX, (float)rootY});

    m_hopActive = false;
    m_hopOffset = 0.0f;
    m_hopVelocity = 0.0f;
    m_velocityX = 0.0f;
    m_velocityY = 0.0f;
    m_state = PetState::PRESS;
}

void PetOverlayLinux::OnMouseMove(double rootX, double rootY) {
    if (!m_mouseDown) return;
    uint64_t now = GetNowMs();
    m_samples.push_back({now, (float)rootX, (float)rootY});
    while (m_samples.size() > 6) m_samples.erase(m_samples.begin());

    if (m_state == PetState::PRESS && std::hypot(rootX - m_dragStartX, rootY - m_dragStartY) > 8.0) {
        m_state = PetState::DRAG;
    }

    if (m_state == PetState::DRAG) {
        m_posX = (float)(rootX - m_dragOffsetX);
        m_posY = (float)(rootY - m_dragOffsetY);
        UpdateLayout();
    }
}

void PetOverlayLinux::OnButtonUp(double rootX, double rootY, guint button) {
    if (!m_mouseDown) return;
    m_mouseDown = false;
    uint64_t now = GetNowMs();

    if (m_state == PetState::DRAG) {
        float vx = 0.0f, vy = 0.0f;
        if (m_samples.size() >= 2) {
            const auto& first = m_samples.front();
            const auto& last = m_samples.back();
            float dt = (last.timeMs - first.timeMs) / 1000.0f;
            if (dt > 0.01f && dt < 0.4f) {
                vx = (last.x - first.x) / dt;
                vy = (last.y - first.y) / dt;
            }
        }
        m_dragVelX = vx;
        m_dragVelY = vy;

        const auto& s = SettingsManager::Instance().GetSettings();
        bool wasFling = (std::hypot(vx, vy) > m_unitPx * 160.0f);
        if (wasFling || (s.gravity && m_posY < m_maxY - 4.0f)) {
            m_state = PetState::FLY;
            m_velocityX = vx;
            m_velocityY = vy;
            if (wasFling) {
                SetExpression(PetExpression::SURPRISED, 1200);
                Say("fling");
            }
        } else {
            m_state = PetState::IDLE;
            SavePosition();
        }
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

void PetOverlayLinux::OnTap(uint64_t now) {
    if (now - m_lastTapAt < 380) {
        m_lastTapAt = 0;
        Hop(m_unitPx * 160.0f);
        SetExpression(PetExpression::HAPPY, 1300);
        for (int i = 0; i < 3; i++) SpawnParticle(ParticleKind::SPARK);
        if (RandomFloat(0.0f, 1.0f) < 0.4f) Say("jump");
        return;
    }

    m_lastTapAt = now;
    m_squashUntil = now + 130;
    SetExpression(PetExpression::SURPRISED, 450);
    Hop(m_unitPx * 75.0f);
    if (RandomFloat(0.0f, 1.0f) < 0.35f) Say("tap");
}

void PetOverlayLinux::BeginPet(uint64_t now) {
    m_state = PetState::PET;
    m_nextHeartAt = now + 100;
}

void PetOverlayLinux::EndPet(uint64_t now) {
    m_state = PetState::IDLE;
    SetExpression(PetExpression::HAPPY, 2200);
    m_nextActionAt = now + RandomU64(3000, 6000);

    auto& s = SettingsManager::Instance().GetMutableSettings();
    s.petsCount++;
    SettingsManager::Instance().Save();

    if (RandomFloat(0.0f, 1.0f) < 0.65f) Say("pet");
}

void PetOverlayLinux::ShowContextMenu(guint button, guint32 time) {
    GtkWidget* menu = gtk_menu_new();

    auto addMenuItem = [&](const char* label, GCallback cb) {
        GtkWidget* item = gtk_menu_item_new_with_label(label);
        g_signal_connect(item, "activate", cb, this);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    };

    addMenuItem("Позвать в центр", G_CALLBACK(+[](GtkMenuItem*, gpointer d) { ((PetOverlayLinux*)d)->Recenter(); }));
    addMenuItem("Сделать кучку", G_CALLBACK(+[](GtkMenuItem*, gpointer d) { ((PetOverlayLinux*)d)->PoopNow(); }));
    addMenuItem("Очистить все кучки", G_CALLBACK(+[](GtkMenuItem*, gpointer d) { ((PetOverlayLinux*)d)->CleanAllPoops(); }));
    addMenuItem("Сказать фразу", G_CALLBACK(+[](GtkMenuItem*, gpointer d) { ((PetOverlayLinux*)d)->Say("idle", true); }));

    GtkWidget* sep1 = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), sep1);

    addMenuItem("Настройки...", G_CALLBACK(+[](GtkMenuItem*, gpointer) {
        SettingsDialogLinux::Instance().Show();
    }));

    addMenuItem("Скрыть питомца", G_CALLBACK(+[](GtkMenuItem*, gpointer d) {
        ((PetOverlayLinux*)d)->SetVisible(false);
    }));

    GtkWidget* sep2 = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), sep2);

    addMenuItem("Выход", G_CALLBACK(+[](GtkMenuItem*, gpointer) {
        gtk_main_quit();
    }));

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_pointer(GTK_MENU(menu), nullptr);
}

void PetOverlayLinux::Render(cairo_t* cr, uint64_t now) {
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    PetPose pose = PetPose::IDLE;
    switch (m_state) {
        case PetState::WALK: pose = PetPose::WALK; break;
        case PetState::DRAG:
        case PetState::PRESS: pose = PetPose::IDLE; break;
        case PetState::FLY: pose = PetPose::FLY; break;
        case PetState::HANG: pose = PetPose::HANG; break;
        case PetState::PEEK: pose = PetPose::PEEK; break;
        case PetState::PET: pose = PetPose::PET; break;
        case PetState::SLEEP: pose = PetPose::SLEEP; break;
        case PetState::IDLE:
            if (now < m_idlePoseUntil) pose = m_idlePose;
            else pose = PetPose::IDLE;
            break;
    }

    PetExpression face = PetExpression::OPEN;
    if (m_state == PetState::SLEEP) face = PetExpression::SLEEP;
    else if (now < m_expressionUntil) face = m_frame.expression;
    else if (now < m_blinkUntil) face = PetExpression::BLINK;

    float rotation = 0.0f;
    if (m_state == PetState::FLY) {
        rotation = std::max(-25.0f, std::min(25.0f, m_velocityX / 15.0f));
    } else if (m_state == PetState::DRAG) {
        rotation = std::max(-18.0f, std::min(18.0f, m_dragVelX / 25.0f));
    }

    float squash = 0.0f;
    if (m_hopActive && m_hopVelocity < 0.0f) squash = -1.0f;
    else if (now < m_squashUntil) squash = 1.0f;

    m_frame.pose = pose;
    m_frame.expression = face;
    m_frame.look = m_look;
    m_frame.waveSide = (now < m_waveUntil && (now / 240) % 2 == 0) ? (((now / 240) % 4 == 0) ? -1 : 1) : 0;
    m_frame.blush = (m_state == PetState::PET || face == PetExpression::HAPPY);
    m_frame.mouthOpen = (now < m_mouthOpenUntil || (m_state == PetState::PET && (now / 180) % 2 == 0));
    m_frame.squash = squash;
    m_frame.rotation = rotation;
    m_frame.hopOffset = m_hopOffset;
    m_frame.phase = now / 1000.0f;

    const auto& s = SettingsManager::Instance().GetSettings();
    PetRendererCairo::DrawPet(
        cr,
        (float)m_windowW,
        (float)m_windowH,
        s.species,
        s.palette,
        m_frame,
        now,
        m_particles,
        m_customSkin
    );
}

} // namespace GUPet
