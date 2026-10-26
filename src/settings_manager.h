#pragma once

#include "pet_types.h"
#include <string>

namespace GUPet {

class SettingsManager {
public:
    static SettingsManager& Instance();

    void Init();
    const PetSettings& GetSettings() const { return m_settings; }
    PetSettings& GetMutableSettings() { return m_settings; }

    void Save();
    void Load();

    std::wstring GetAppDataDir() const { return m_appDataDir; }

private:
    SettingsManager() = default;

    std::wstring m_appDataDir;
    std::wstring m_configFile;
    PetSettings m_settings;
};

} // namespace GUPet
