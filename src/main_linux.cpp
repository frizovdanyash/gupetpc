#include "pet_overlay_linux.h"
#include "settings_manager.h"
#include "skin_store.h"
#include "web_server.h"

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
                      << "  -s, --settings    Открыть панель настроек в браузере\n"
                      << "  -h, --help        Показать эту справку\n";
            return 0;
        }
    }

    // Initialize core subsystems
    GUPet::SettingsManager::Instance().Init();
    GUPet::SkinStore::Instance().Init(GUPet::SettingsManager::Instance().GetAppDataDirA());

    // Start embedded Material 3 UI server
    if (GUPet::WebServer::Instance().Start(23847)) {
        GUPet::WebServer::Instance().SetOnSettingsChanged([](const GUPet::PetSettings&) {
            GUPet::PetOverlayLinux::Instance().ReloadSettings();
        });
        GUPet::WebServer::Instance().SetOnAction([](const std::string& act) {
            if (act == "recenter") GUPet::PetOverlayLinux::Instance().Recenter();
            else if (act == "poop") GUPet::PetOverlayLinux::Instance().PoopNow();
            else if (act == "clean") GUPet::PetOverlayLinux::Instance().CleanAllPoops();
            else if (act == "say") GUPet::PetOverlayLinux::Instance().Say("idle", true);
        });

        if (openSettingsOnly) {
            GUPet::WebServer::Instance().OpenSettingsInBrowser();
        }
    }

    if (!GUPet::PetOverlayLinux::Instance().Init(argc, argv)) {
        std::cerr << "Не удалось инициализировать окно питомца (проверьте DISPLAY/Wayland)" << std::endl;
        return 1;
    }

    GUPet::PetOverlayLinux::Instance().Run();
    GUPet::PetOverlayLinux::Instance().Shutdown();
    GUPet::WebServer::Instance().Stop();

    return 0;
}
