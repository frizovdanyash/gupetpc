#include "pet_overlay_linux.h"
#include "settings_manager.h"
#include "settings_dialog_linux.h"
#include "skin_store.h"

#include <iostream>
#include <cstring>

int main(int argc, char* argv[]) {
    bool openSettingsOnly = false;
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--settings") == 0 || std::strcmp(argv[i], "-s") == 0) {
            openSettingsOnly = true;
        } else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            std::cout << "GUPet — Пиксельный питомец для рабочего стола\n"
                      << "Использование: gupet [опции]\n\n"
                      << "Опции:\n"
                      << "  -s, --settings    Открыть окно настроек\n"
                      << "  -h, --help        Показать эту справку\n";
            return 0;
        }
    }

    // Initialize core subsystems
    GUPet::SettingsManager::Instance().Init();
    GUPet::SkinStore::Instance().Init(GUPet::SettingsManager::Instance().GetAppDataDirA());

    if (!GUPet::PetOverlayLinux::Instance().Init(argc, argv)) {
        std::cerr << "Не удалось инициализировать окно питомца (проверьте DISPLAY/Wayland)" << std::endl;
        return 1;
    }

    if (openSettingsOnly) {
        GUPet::SettingsDialogLinux::Instance().Show();
    }

    GUPet::PetOverlayLinux::Instance().Run();
    GUPet::PetOverlayLinux::Instance().Shutdown();

    return 0;
}
