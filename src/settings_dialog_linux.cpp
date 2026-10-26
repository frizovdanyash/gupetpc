#include "settings_dialog_linux.h"
#include "settings_manager.h"
#include "pet_overlay_linux.h"
#include "pet_renderer_cairo.h"
#include "pet_palettes.h"
#include "skin_store.h"

#include <cmath>
#include <chrono>
#include <iostream>

namespace GUPet {

static uint64_t GetNowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

static gboolean OnDrawPreviewStatic(GtkWidget* w, cairo_t* cr, gpointer) {
    int width = gtk_widget_get_allocated_width(w);
    int height = gtk_widget_get_allocated_height(w);

    double r = 16.0;
    cairo_new_sub_path(cr);
    cairo_arc(cr, width - r, r, r, -M_PI/2, 0);
    cairo_arc(cr, width - r, height - r, r, 0, M_PI/2);
    cairo_arc(cr, r, height - r, r, M_PI/2, M_PI);
    cairo_arc(cr, r, r, r, M_PI, 3*M_PI/2);
    cairo_close_path(cr);

    cairo_set_source_rgba(cr, 1.0, 0.94, 0.91, 1.0);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0.85, 0.47, 0.34, 0.3);
    cairo_set_line_width(cr, 1.5);
    cairo_stroke(cr);

    const auto& s = SettingsManager::Instance().GetSettings();
    uint64_t now = GetNowMs();

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

    cairo_save(cr);
    cairo_translate(cr, (width - petW) / 2.0, (height - petH) / 2.0);
    PetRendererCairo::DrawPet(cr, petW, petH, s.species, s.palette, frame, now, emptyParticles, skin);
    cairo_restore(cr);

    return FALSE;
}

static void OnImportSkinClicked(GtkButton*, gpointer data) {
    auto self = (SettingsDialogLinux*)data;
    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        "Выберите файл скина GUPet",
        NULL,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "Отмена", GTK_RESPONSE_CANCEL,
        "Открыть", GTK_RESPONSE_ACCEPT,
        NULL
    );
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Файлы скинов (*.gupet, *.zip)");
    gtk_file_filter_add_pattern(filter, "*.gupet");
    gtk_file_filter_add_pattern(filter, "*.zip");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (filename) {
            std::string newId, err;
            if (SkinStore::Instance().ImportSkin(filename, newId, err)) {
                auto& st = SettingsManager::Instance().GetMutableSettings();
                st.customSkinId = newId;
                SettingsManager::Instance().Save();
                PetOverlayLinux::Instance().ReloadSettings();
                self->PopulateSkins();
            } else {
                std::string msg = "Ошибка импорта: " + err;
                GtkWidget* errDialog = gtk_message_dialog_new(
                    NULL,
                    GTK_DIALOG_DESTROY_WITH_PARENT,
                    GTK_MESSAGE_ERROR,
                    GTK_BUTTONS_CLOSE,
                    "%s", msg.c_str()
                );
                gtk_dialog_run(GTK_DIALOG(errDialog));
                gtk_widget_destroy(errDialog);
            }
            g_free(filename);
        }
    }
    gtk_widget_destroy(dialog);
}

SettingsDialogLinux& SettingsDialogLinux::Instance() {
    static SettingsDialogLinux s_instance;
    return s_instance;
}

void SettingsDialogLinux::Show() {
    if (m_window) {
        gtk_window_present(GTK_WINDOW(m_window));
        return;
    }

    BuildUi();
    gtk_widget_show_all(m_window);
    gtk_window_present(GTK_WINDOW(m_window));
}

void SettingsDialogLinux::Close() {
    if (m_animTimerId > 0) {
        g_source_remove(m_animTimerId);
        m_animTimerId = 0;
    }
    if (m_window) {
        gtk_widget_destroy(m_window);
        m_window = nullptr;
    }
}

void SettingsDialogLinux::BuildUi() {
    m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(m_window), "GUPet — Настройки");
    gtk_window_set_default_size(GTK_WINDOW(m_window), 640, 580);
    gtk_window_set_position(GTK_WINDOW(m_window), GTK_WIN_POS_CENTER);

    g_signal_connect(m_window, "destroy", G_CALLBACK(+[](GtkWidget*, gpointer data) {
        ((SettingsDialogLinux*)data)->Close();
    }), this);

    GtkWidget* mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(mainBox), 14);
    gtk_container_add(GTK_CONTAINER(m_window), mainBox);

    GtkWidget* hGrid = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 14);
    gtk_box_pack_start(GTK_BOX(mainBox), hGrid, TRUE, TRUE, 0);

    // Left Column: Appearance
    GtkWidget* leftFrame = gtk_frame_new("Внешний вид и размер");
    gtk_box_pack_start(GTK_BOX(hGrid), leftFrame, TRUE, TRUE, 0);

    GtkWidget* leftBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(leftBox), 10);
    gtk_container_add(GTK_CONTAINER(leftFrame), leftBox);

    // Preview area
    m_previewArea = gtk_drawing_area_new();
    gtk_widget_set_size_request(m_previewArea, 220, 110);
    gtk_box_pack_start(GTK_BOX(leftBox), m_previewArea, FALSE, FALSE, 4);

    g_signal_connect(m_previewArea, "draw", G_CALLBACK(OnDrawPreviewStatic), nullptr);

    // 30 FPS animation timer
    m_animTimerId = g_timeout_add(33, +[](gpointer data) -> gboolean {
        GtkWidget* prev = (GtkWidget*)data;
        if (GTK_IS_WIDGET(prev)) {
            gtk_widget_queue_draw(prev);
            return TRUE;
        }
        return FALSE;
    }, m_previewArea);

    const auto& s = SettingsManager::Instance().GetSettings();

    // Species
    GtkWidget* lblSpecies = gtk_label_new("Вид питомца:");
    gtk_label_set_xalign(GTK_LABEL(lblSpecies), 0.0);
    gtk_box_pack_start(GTK_BOX(leftBox), lblSpecies, FALSE, FALSE, 0);

    GtkWidget* radioGupet = gtk_radio_button_new_with_label(NULL, "GUPet");
    GtkWidget* radioCat = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radioGupet), "Кошка");
    GtkWidget* radioDog = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radioGupet), "Собака");

    if (s.species == PetSpecies::CAT) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radioCat), TRUE);
    else if (s.species == PetSpecies::DOG) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radioDog), TRUE);
    else gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radioGupet), TRUE);

    GtkWidget* speciesBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_start(GTK_BOX(speciesBox), radioGupet, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(speciesBox), radioCat, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(speciesBox), radioDog, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(leftBox), speciesBox, FALSE, FALSE, 0);

    g_signal_connect(radioGupet, "toggled", G_CALLBACK(+[](GtkToggleButton* b, gpointer data) {
        if (gtk_toggle_button_get_active(b)) {
            auto self = (SettingsDialogLinux*)data;
            auto& st = SettingsManager::Instance().GetMutableSettings();
            st.species = PetSpecies::GUPET;
            st.customSkinId.clear();
            SettingsManager::Instance().Save();
            PetOverlayLinux::Instance().ReloadSettings();
            self->PopulateSkins();
            gtk_widget_queue_draw(self->m_previewArea);
        }
    }), this);

    g_signal_connect(radioCat, "toggled", G_CALLBACK(+[](GtkToggleButton* b, gpointer data) {
        if (gtk_toggle_button_get_active(b)) {
            auto self = (SettingsDialogLinux*)data;
            auto& st = SettingsManager::Instance().GetMutableSettings();
            st.species = PetSpecies::CAT;
            st.customSkinId.clear();
            SettingsManager::Instance().Save();
            PetOverlayLinux::Instance().ReloadSettings();
            self->PopulateSkins();
            gtk_widget_queue_draw(self->m_previewArea);
        }
    }), this);

    g_signal_connect(radioDog, "toggled", G_CALLBACK(+[](GtkToggleButton* b, gpointer data) {
        if (gtk_toggle_button_get_active(b)) {
            auto self = (SettingsDialogLinux*)data;
            auto& st = SettingsManager::Instance().GetMutableSettings();
            st.species = PetSpecies::DOG;
            st.customSkinId.clear();
            SettingsManager::Instance().Save();
            PetOverlayLinux::Instance().ReloadSettings();
            self->PopulateSkins();
            gtk_widget_queue_draw(self->m_previewArea);
        }
    }), this);

    // Palette
    GtkWidget* lblPalette = gtk_label_new("Цветовая палитра:");
    gtk_label_set_xalign(GTK_LABEL(lblPalette), 0.0);
    gtk_box_pack_start(GTK_BOX(leftBox), lblPalette, FALSE, FALSE, 0);

    GtkWidget* palCombo = gtk_combo_box_text_new();
    for (int i = 0; i < PetPalettes::PALETTE_COUNT; i++) {
        std::wstring wName = PetPalettes::GetName(i);
        std::string name(wName.begin(), wName.end());
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(palCombo), name.c_str());
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(palCombo), s.palette);
    gtk_box_pack_start(GTK_BOX(leftBox), palCombo, FALSE, FALSE, 0);

    g_signal_connect(palCombo, "changed", G_CALLBACK(+[](GtkComboBox* cb, gpointer data) {
        auto self = (SettingsDialogLinux*)data;
        int active = gtk_combo_box_get_active(cb);
        if (active >= 0) {
            auto& st = SettingsManager::Instance().GetMutableSettings();
            st.palette = active;
            SettingsManager::Instance().Save();
            PetOverlayLinux::Instance().ReloadSettings();
            gtk_widget_queue_draw(self->m_previewArea);
        }
    }), this);

    // Size scale & Spin
    GtkWidget* lblScale = gtk_label_new("Масштаб (любой размер):");
    gtk_label_set_xalign(GTK_LABEL(lblScale), 0.0);
    gtk_box_pack_start(GTK_BOX(leftBox), lblScale, FALSE, FALSE, 0);

    GtkWidget* scaleBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    m_sizeScale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 1.0, 15.0, 0.1);
    gtk_range_set_value(GTK_RANGE(m_sizeScale), s.sizeScale);
    gtk_box_pack_start(GTK_BOX(scaleBox), m_sizeScale, TRUE, TRUE, 0);

    m_sizeSpin = gtk_spin_button_new_with_range(0.5, 30.0, 0.1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(m_sizeSpin), s.sizeScale);
    gtk_box_pack_start(GTK_BOX(scaleBox), m_sizeSpin, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(leftBox), scaleBox, FALSE, FALSE, 0);

    g_signal_connect(m_sizeScale, "value-changed", G_CALLBACK(+[](GtkRange* r, gpointer data) {
        auto self = (SettingsDialogLinux*)data;
        double val = gtk_range_get_value(r);
        auto& st = SettingsManager::Instance().GetMutableSettings();
        st.sizeScale = (float)val;
        SettingsManager::Instance().Save();
        PetOverlayLinux::Instance().ReloadSettings();
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(self->m_sizeSpin), val);
    }), this);

    g_signal_connect(m_sizeSpin, "value-changed", G_CALLBACK(+[](GtkSpinButton* sp, gpointer data) {
        auto self = (SettingsDialogLinux*)data;
        double val = gtk_spin_button_get_value(sp);
        auto& st = SettingsManager::Instance().GetMutableSettings();
        st.sizeScale = (float)val;
        SettingsManager::Instance().Save();
        PetOverlayLinux::Instance().ReloadSettings();
        gtk_range_set_value(GTK_RANGE(self->m_sizeScale), val);
    }), this);

    // Sleep
    GtkWidget* lblSleep = gtk_label_new("Когда засыпать:");
    gtk_label_set_xalign(GTK_LABEL(lblSleep), 0.0);
    gtk_box_pack_start(GTK_BOX(leftBox), lblSleep, FALSE, FALSE, 0);

    GtkWidget* sleepCombo = gtk_combo_box_text_new();
    static const struct { int mins; const char* label; } sleepOpts[] = {
        {0, "Отключено"}, {1, "1 мин"}, {2, "2 мин"},
        {3, "3 мин"}, {5, "5 мин"}, {10, "10 мин"}
    };
    int selSleep = 0;
    for (int i = 0; i < 6; i++) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sleepCombo), sleepOpts[i].label);
        if (s.sleepMinutes == sleepOpts[i].mins) selSleep = i;
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(sleepCombo), selSleep);
    gtk_box_pack_start(GTK_BOX(leftBox), sleepCombo, FALSE, FALSE, 0);

    g_signal_connect(sleepCombo, "changed", G_CALLBACK(+[](GtkComboBox* cb, gpointer) {
        int act = gtk_combo_box_get_active(cb);
        if (act >= 0 && act < 6) {
            auto& st = SettingsManager::Instance().GetMutableSettings();
            st.sleepMinutes = sleepOpts[act].mins;
            SettingsManager::Instance().Save();
        }
    }), nullptr);

    // Right Column: Behaviors & Actions
    GtkWidget* rightFrame = gtk_frame_new("Поведение и команды");
    gtk_box_pack_start(GTK_BOX(hGrid), rightFrame, TRUE, TRUE, 0);

    GtkWidget* rightBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(rightBox), 10);
    gtk_container_add(GTK_CONTAINER(rightFrame), rightBox);

    auto addSwitchRow = [&](const char* label, bool active, auto onToggle) {
        GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        GtkWidget* lbl = gtk_label_new(label);
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
        GtkWidget* sw = gtk_switch_new();
        gtk_switch_set_active(GTK_SWITCH(sw), active);
        gtk_box_pack_start(GTK_BOX(row), lbl, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(row), sw, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(rightBox), row, FALSE, FALSE, 0);

        g_signal_connect(sw, "state-set", G_CALLBACK(+[](GtkSwitch*, gboolean state, gpointer d) -> gboolean {
            auto fn = (decltype(onToggle)*)d;
            (*fn)(state);
            return FALSE;
        }), new auto(onToggle));
    };

    addSwitchRow("Самостоятельно гулять", s.wander, [](bool v) {
        auto& st = SettingsManager::Instance().GetMutableSettings(); st.wander = v; SettingsManager::Instance().Save();
    });
    addSwitchRow("Реплики и облачка фраз", s.phrases, [](bool v) {
        auto& st = SettingsManager::Instance().GetMutableSettings(); st.phrases = v; SettingsManager::Instance().Save();
    });
    addSwitchRow("Гравитация и физика", s.gravity, [](bool v) {
        auto& st = SettingsManager::Instance().GetMutableSettings(); st.gravity = v; SettingsManager::Instance().Save();
    });
    addSwitchRow("Реакция на встряхивание", s.shake, [](bool v) {
        auto& st = SettingsManager::Instance().GetMutableSettings(); st.shake = v; SettingsManager::Instance().Save();
    });
    addSwitchRow("Звуковые отклики", s.haptics, [](bool v) {
        auto& st = SettingsManager::Instance().GetMutableSettings(); st.haptics = v; SettingsManager::Instance().Save();
    });
    addSwitchRow("Оставлять кучки", s.poop, [](bool v) {
        auto& st = SettingsManager::Instance().GetMutableSettings(); st.poop = v; SettingsManager::Instance().Save();
    });
    addSwitchRow("Всегда поверх всех окон", s.alwaysOnTop, [](bool v) {
        auto& st = SettingsManager::Instance().GetMutableSettings(); st.alwaysOnTop = v; SettingsManager::Instance().Save();
        PetOverlayLinux::Instance().ReloadSettings();
    });
    addSwitchRow("Автозапуск при входе", s.autostart, [](bool v) {
        auto& st = SettingsManager::Instance().GetMutableSettings(); st.autostart = v; SettingsManager::Instance().Save();
    });

    // Action buttons
    GtkWidget* actBox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget* btnRecenter = gtk_button_new_with_label("Позвать в центр");
    GtkWidget* btnSay = gtk_button_new_with_label("Сказать фразу");
    gtk_box_pack_start(GTK_BOX(actBox1), btnRecenter, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(actBox1), btnSay, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(rightBox), actBox1, FALSE, FALSE, 2);

    g_signal_connect(btnRecenter, "clicked", G_CALLBACK(+[](GtkButton*, gpointer) {
        PetOverlayLinux::Instance().Recenter();
    }), nullptr);

    g_signal_connect(btnSay, "clicked", G_CALLBACK(+[](GtkButton*, gpointer) {
        PetOverlayLinux::Instance().Say("idle", true);
    }), nullptr);

    GtkWidget* actBox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget* btnPoop = gtk_button_new_with_label("Сделать кучку");
    GtkWidget* btnClean = gtk_button_new_with_label("Очистить кучки");
    gtk_box_pack_start(GTK_BOX(actBox2), btnPoop, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(actBox2), btnClean, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(rightBox), actBox2, FALSE, FALSE, 2);

    g_signal_connect(btnPoop, "clicked", G_CALLBACK(+[](GtkButton*, gpointer) {
        PetOverlayLinux::Instance().PoopNow();
    }), nullptr);

    g_signal_connect(btnClean, "clicked", G_CALLBACK(+[](GtkButton*, gpointer data) {
        PetOverlayLinux::Instance().CleanAllPoops();
        ((SettingsDialogLinux*)data)->UpdateStats();
    }), this);

    // Bottom: Custom Skins
    GtkWidget* skinFrame = gtk_frame_new("Пользовательские скины (.gupet)");
    gtk_box_pack_start(GTK_BOX(mainBox), skinFrame, FALSE, FALSE, 0);

    GtkWidget* skinBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(skinBox), 8);
    gtk_container_add(GTK_CONTAINER(skinFrame), skinBox);

    m_skinCombo = gtk_combo_box_text_new();
    gtk_box_pack_start(GTK_BOX(skinBox), m_skinCombo, TRUE, TRUE, 0);

    GtkWidget* btnResetSkin = gtk_button_new_with_label("Сбросить");
    GtkWidget* btnImportSkin = gtk_button_new_with_label("+ Импорт .gupet");
    gtk_box_pack_start(GTK_BOX(skinBox), btnResetSkin, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(skinBox), btnImportSkin, FALSE, FALSE, 0);

    PopulateSkins();

    g_signal_connect(m_skinCombo, "changed", G_CALLBACK(+[](GtkComboBox* cb, gpointer data) {
        auto self = (SettingsDialogLinux*)data;
        int act = gtk_combo_box_get_active(cb);
        auto& st = SettingsManager::Instance().GetMutableSettings();
        if (act == 0) {
            st.customSkinId.clear();
        } else {
            auto skins = SkinStore::Instance().ListSkins();
            if (act - 1 < (int)skins.size()) {
                st.customSkinId = skins[act - 1].id;
            }
        }
        SettingsManager::Instance().Save();
        PetOverlayLinux::Instance().ReloadSettings();
        gtk_widget_queue_draw(self->m_previewArea);
    }), this);

    g_signal_connect(btnResetSkin, "clicked", G_CALLBACK(+[](GtkButton*, gpointer data) {
        auto self = (SettingsDialogLinux*)data;
        auto& st = SettingsManager::Instance().GetMutableSettings();
        st.customSkinId.clear();
        SettingsManager::Instance().Save();
        PetOverlayLinux::Instance().ReloadSettings();
        self->PopulateSkins();
        gtk_widget_queue_draw(self->m_previewArea);
    }), this);

    g_signal_connect(btnImportSkin, "clicked", G_CALLBACK(OnImportSkinClicked), this);

    // Bottom stats & Done button
    GtkWidget* bottomBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(mainBox), bottomBox, FALSE, FALSE, 4);

    m_statsLabel = gtk_label_new("");
    UpdateStats();
    gtk_box_pack_start(GTK_BOX(bottomBox), m_statsLabel, TRUE, TRUE, 0);

    GtkWidget* btnClose = gtk_button_new_with_label("Готово");
    gtk_box_pack_start(GTK_BOX(bottomBox), btnClose, FALSE, FALSE, 0);

    g_signal_connect(btnClose, "clicked", G_CALLBACK(+[](GtkButton*, gpointer data) {
        ((SettingsDialogLinux*)data)->Close();
    }), this);
}

void SettingsDialogLinux::PopulateSkins() {
    if (!m_skinCombo) return;
    gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(m_skinCombo));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(m_skinCombo), "— Встроенный вид —");

    auto skins = SkinStore::Instance().ListSkins();
    const auto& s = SettingsManager::Instance().GetSettings();
    int selIndex = 0;

    for (size_t i = 0; i < skins.size(); i++) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(m_skinCombo), skins[i].name.c_str());
        if (s.customSkinId == skins[i].id) {
            selIndex = (int)i + 1;
        }
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(m_skinCombo), selIndex);
}

void SettingsDialogLinux::UpdateStats() {
    if (!m_statsLabel) return;
    const auto& s = SettingsManager::Instance().GetSettings();
    std::string text = "Поглаживаний: " + std::to_string(s.petsCount) +
                       "   |   Кучек убрано: " + std::to_string(s.poopsCleaned);
    gtk_label_set_text(GTK_LABEL(m_statsLabel), text.c_str());
}

} // namespace GUPet
