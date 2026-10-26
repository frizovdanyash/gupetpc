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

    std::string GetAppDataDirA() const { return m_appDataDirA; }
#ifdef _WIN32
    std::wstring GetAppDataDir() const { return m_appDataDirW; }
#endif

    bool SetAutostart(bool enable);
    bool IsAutostartEnabled();

private:
    SettingsManager() = default;

    std::string m_appDataDirA;
    std::string m_configFileA;

#ifdef _WIN32
    std::wstring m_appDataDirW;
    std::wstring m_configFileW;
#endif

    PetSettings m_settings;
};

} // namespace GUPet
