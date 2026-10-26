#include "settings_manager.h"

#ifdef _WIN32
  #include <windows.h>
  #include <shlobj.h>
#else
  #include <unistd.h>
  #include <pwd.h>
  #include <sys/types.h>
#endif

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>

namespace GUPet {

namespace fs = std::filesystem;

static std::string FindJsonString(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos);
    if (pos == std::string::npos) return "";
    pos = json.find('\"', pos);
    if (pos == std::string::npos) return "";
    size_t endPos = json.find('\"', pos + 1);
    if (endPos == std::string::npos) return "";
    return json.substr(pos + 1, endPos - pos - 1);
}

static bool FindJsonBool(const std::string& json, const std::string& key, bool defaultVal) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return defaultVal;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return defaultVal;
    size_t truePos = json.find("true", pos);
    size_t falsePos = json.find("false", pos);
    size_t commaPos = json.find_first_of(",}\n", pos);
    if (truePos < commaPos) return true;
    if (falsePos < commaPos) return false;
    return defaultVal;
}

static int FindJsonInt(const std::string& json, const std::string& key, int defaultVal) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return defaultVal;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return defaultVal;
    pos = json.find_first_of("-0123456789", pos);
    if (pos == std::string::npos) return defaultVal;
    size_t endPos = json.find_first_not_of("0123456789", pos + 1);
    std::string num = (endPos == std::string::npos) ? json.substr(pos) : json.substr(pos, endPos - pos);
    try {
        return std::stoi(num);
    } catch (...) {
        return defaultVal;
    }
}

static float FindJsonFloat(const std::string& json, const std::string& key, float defaultVal) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return defaultVal;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return defaultVal;
    pos = json.find_first_of("-0123456789.", pos);
    if (pos == std::string::npos) return defaultVal;
    size_t endPos = json.find_first_not_of("0123456789.", pos);
    std::string num = (endPos == std::string::npos) ? json.substr(pos) : json.substr(pos, endPos - pos);
    try {
        return std::stof(num);
    } catch (...) {
        return defaultVal;
    }
}

SettingsManager& SettingsManager::Instance() {
    static SettingsManager s_instance;
    return s_instance;
}

void SettingsManager::Init() {
#ifdef _WIN32
    wchar_t appData[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appData))) {
        m_appDataDirW = std::wstring(appData) + L"\\GUPet";
    } else {
        m_appDataDirW = L".";
    }
    CreateDirectoryW(m_appDataDirW.c_str(), NULL);
    m_configFileW = m_appDataDirW + L"\\settings.json";

    fs::path p(m_appDataDirW);
    m_appDataDirA = p.string();
    m_configFileA = (p / "settings.json").string();
#else
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        home = pw ? pw->pw_dir : ".";
    }
    const char* xdgConfig = getenv("XDG_CONFIG_HOME");
    if (xdgConfig && xdgConfig[0]) {
        m_appDataDirA = std::string(xdgConfig) + "/gupet";
    } else {
        m_appDataDirA = std::string(home) + "/.config/gupet";
    }
    try {
        fs::create_directories(m_appDataDirA);
    } catch (...) {}
    m_configFileA = m_appDataDirA + "/settings.json";
#endif

    Load();
}

void SettingsManager::Load() {
    std::ifstream file(m_configFileA, std::ios::binary);
    if (!file.is_open()) return;

    std::stringstream ss;
    ss << file.rdbuf();
    std::string json = ss.str();

    m_settings.show = FindJsonBool(json, "show", true);
    m_settings.species = KeyToSpecies(FindJsonString(json, "species"));
    m_settings.palette = FindJsonInt(json, "palette", 0);
    m_settings.sizeScale = FindJsonFloat(json, "sizeScale", 4.0f);
    m_settings.wander = FindJsonBool(json, "wander", true);
    m_settings.phrases = FindJsonBool(json, "phrases", true);
    m_settings.gravity = FindJsonBool(json, "gravity", false);
    m_settings.shake = FindJsonBool(json, "shake", true);
    m_settings.haptics = FindJsonBool(json, "haptics", true);
    m_settings.poop = FindJsonBool(json, "poop", true);
    m_settings.sleepMinutes = FindJsonInt(json, "sleepMinutes", 3);
    m_settings.dance = FindJsonBool(json, "dance", true);
    m_settings.alwaysOnTop = FindJsonBool(json, "alwaysOnTop", true);
    m_settings.ghostMode = FindJsonBool(json, "ghostMode", false);
    m_settings.autostart = FindJsonBool(json, "autostart", false);
    m_settings.customSkinId = FindJsonString(json, "customSkinId");

    m_settings.posX = FindJsonFloat(json, "posX", 0.0f);
    m_settings.posY = FindJsonFloat(json, "posY", 0.0f);
    m_settings.positionSaved = FindJsonBool(json, "positionSaved", false);

    m_settings.petsCount = FindJsonInt(json, "petsCount", 0);
    m_settings.poopsCleaned = FindJsonInt(json, "poopsCleaned", 0);
}

void SettingsManager::Save() {
    std::ofstream file(m_configFileA, std::ios::binary);
    if (!file.is_open()) return;

    file << "{\n";
    file << "  \"show\": " << (m_settings.show ? "true" : "false") << ",\n";
    file << "  \"species\": \"" << SpeciesToKey(m_settings.species) << "\",\n";
    file << "  \"palette\": " << m_settings.palette << ",\n";
    file << "  \"sizeScale\": " << m_settings.sizeScale << ",\n";
    file << "  \"wander\": " << (m_settings.wander ? "true" : "false") << ",\n";
    file << "  \"phrases\": " << (m_settings.phrases ? "true" : "false") << ",\n";
    file << "  \"gravity\": " << (m_settings.gravity ? "true" : "false") << ",\n";
    file << "  \"shake\": " << (m_settings.shake ? "true" : "false") << ",\n";
    file << "  \"haptics\": " << (m_settings.haptics ? "true" : "false") << ",\n";
    file << "  \"poop\": " << (m_settings.poop ? "true" : "false") << ",\n";
    file << "  \"sleepMinutes\": " << m_settings.sleepMinutes << ",\n";
    file << "  \"dance\": " << (m_settings.dance ? "true" : "false") << ",\n";
    file << "  \"alwaysOnTop\": " << (m_settings.alwaysOnTop ? "true" : "false") << ",\n";
    file << "  \"ghostMode\": " << (m_settings.ghostMode ? "true" : "false") << ",\n";
    file << "  \"autostart\": " << (m_settings.autostart ? "true" : "false") << ",\n";
    file << "  \"customSkinId\": \"" << m_settings.customSkinId << "\",\n";
    file << "  \"posX\": " << m_settings.posX << ",\n";
    file << "  \"posY\": " << m_settings.posY << ",\n";
    file << "  \"positionSaved\": " << (m_settings.positionSaved ? "true" : "false") << ",\n";
    file << "  \"petsCount\": " << m_settings.petsCount << ",\n";
    file << "  \"poopsCleaned\": " << m_settings.poopsCleaned << "\n";
    file << "}\n";

    SetAutostart(m_settings.autostart);
}

bool SettingsManager::SetAutostart(bool enable) {
#ifdef _WIN32
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        if (enable) {
            wchar_t exePath[MAX_PATH] = {0};
            GetModuleFileNameW(NULL, exePath, MAX_PATH);
            std::wstring quoted = L"\"" + std::wstring(exePath) + L"\"";
            RegSetValueExW(hKey, L"GUPet", 0, REG_SZ, (const BYTE*)quoted.c_str(), (DWORD)((quoted.size() + 1) * sizeof(wchar_t)));
        } else {
            RegDeleteValueW(hKey, L"GUPet");
        }
        RegCloseKey(hKey);
        return true;
    }
    return false;
#else
    const char* home = getenv("HOME");
    if (!home) return false;
    std::string autostartDir = std::string(home) + "/.config/autostart";
    try {
        fs::create_directories(autostartDir);
    } catch (...) {}
    std::string desktopPath = autostartDir + "/gupet.desktop";

    if (enable) {
        std::ofstream f(desktopPath);
        if (f.is_open()) {
            f << "[Desktop Entry]\n"
              << "Type=Application\n"
              << "Name=GUPet\n"
              << "Comment=Pixel Desktop Pet\n"
              << "Exec=gupet\n"
              << "Icon=gupet\n"
              << "Terminal=false\n"
              << "Categories=Utility;\n";
            f.close();
            return true;
        }
    } else {
        try {
            if (fs::exists(desktopPath)) fs::remove(desktopPath);
            return true;
        } catch (...) {}
    }
    return false;
#endif
}

bool SettingsManager::IsAutostartEnabled() {
#ifdef _WIN32
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD type = 0;
        LONG res = RegQueryValueExW(hKey, L"GUPet", NULL, &type, NULL, NULL);
        RegCloseKey(hKey);
        return (res == ERROR_SUCCESS);
    }
    return false;
#else
    const char* home = getenv("HOME");
    if (!home) return false;
    std::string desktopPath = std::string(home) + "/.config/autostart/gupet.desktop";
    return fs::exists(desktopPath);
#endif
}

} // namespace GUPet
