#pragma once

#include <gtk/gtk.h>
#include <string>

namespace GUPet {

class SettingsDialogLinux {
public:
    static SettingsDialogLinux& Instance();

    void Show();
    void Close();
    bool IsOpen() const { return m_window != nullptr; }

    void PopulateSkins();
    void UpdateStats();

private:
    SettingsDialogLinux() = default;

    GtkWidget* m_window = nullptr;
    GtkWidget* m_previewArea = nullptr;
    GtkWidget* m_skinCombo = nullptr;
    GtkWidget* m_statsLabel = nullptr;
    GtkWidget* m_sizeSpin = nullptr;
    GtkWidget* m_sizeScale = nullptr;

    guint m_animTimerId = 0;

    void BuildUi();
};

} // namespace GUPet
