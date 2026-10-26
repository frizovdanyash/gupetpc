#include "web_server.h"
#include "web_ui.h"
#include "settings_manager.h"
#include "skin_store.h"

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <windows.h>
  #include <shellapi.h>
  typedef int socklen_t;
  #define CLOSE_SOCKET(s) closesocket(s)
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <fcntl.h>
  #define INVALID_SOCKET (-1)
  #define SOCKET_ERROR (-1)
  #define CLOSE_SOCKET(s) close(s)
#endif

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <algorithm>

namespace GUPet {

WebServer& WebServer::Instance() {
    static WebServer s_instance;
    return s_instance;
}

static std::string FindJsonStr(const std::string& json, const std::string& key) {
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

static bool FindJsonBool(const std::string& json, const std::string& key, bool defaultVal, bool* found = nullptr) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) {
        if (found) *found = false;
        return defaultVal;
    }
    pos = json.find(':', pos);
    if (pos == std::string::npos) {
        if (found) *found = false;
        return defaultVal;
    }
    size_t truePos = json.find("true", pos);
    size_t falsePos = json.find("false", pos);
    size_t commaPos = json.find_first_of(",}\n", pos);
    if (found) *found = true;
    if (truePos < commaPos) return true;
    if (falsePos < commaPos) return false;
    return defaultVal;
}

static int FindJsonInt(const std::string& json, const std::string& key, int defaultVal, bool* found = nullptr) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) {
        if (found) *found = false;
        return defaultVal;
    }
    pos = json.find(':', pos);
    if (pos == std::string::npos) {
        if (found) *found = false;
        return defaultVal;
    }
    pos = json.find_first_of("-0123456789", pos);
    if (pos == std::string::npos) {
        if (found) *found = false;
        return defaultVal;
    }
    size_t endPos = json.find_first_not_of("0123456789", pos + 1);
    std::string num = (endPos == std::string::npos) ? json.substr(pos) : json.substr(pos, endPos - pos);
    if (found) *found = true;
    try { return std::stoi(num); } catch (...) { return defaultVal; }
}

static float FindJsonFloat(const std::string& json, const std::string& key, float defaultVal, bool* found = nullptr) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) {
        if (found) *found = false;
        return defaultVal;
    }
    pos = json.find(':', pos);
    if (pos == std::string::npos) {
        if (found) *found = false;
        return defaultVal;
    }
    pos = json.find_first_of("-0123456789.", pos);
    if (pos == std::string::npos) {
        if (found) *found = false;
        return defaultVal;
    }
    size_t endPos = json.find_first_not_of("0123456789.", pos);
    std::string num = (endPos == std::string::npos) ? json.substr(pos) : json.substr(pos, endPos - pos);
    if (found) *found = true;
    try { return std::stof(num); } catch (...) { return defaultVal; }
}

bool WebServer::Start(int port) {
    if (m_running) return true;
    m_port = port;

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;
#endif

    auto s = socket(AF_INET, SOCK_STREAM, 0);
    if ((intptr_t)s == (intptr_t)INVALID_SOCKET) return false;

    int opt = 1;
#ifdef _WIN32
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons((uint16_t)m_port);

    if (bind(s, (sockaddr*)&addr, sizeof(addr)) != 0) {
        // Try next port if busy
        addr.sin_port = 0;
        if (bind(s, (sockaddr*)&addr, sizeof(addr)) != 0) {
            CLOSE_SOCKET(s);
            return false;
        }
        socklen_t len = sizeof(addr);
        getsockname(s, (sockaddr*)&addr, &len);
        m_port = ntohs(addr.sin_port);
    }

    if (listen(s, 10) != 0) {
        CLOSE_SOCKET(s);
        return false;
    }

    m_serverSock = s;
    m_running = true;
    m_thread = std::thread(&WebServer::WorkerLoop, this);
    return true;
}

void WebServer::Stop() {
    if (!m_running) return;
    m_running = false;
    if (m_serverSock != 0 && m_serverSock != (uintptr_t)INVALID_SOCKET) {
        CLOSE_SOCKET(m_serverSock);
        m_serverSock = 0;
    }
    if (m_thread.joinable()) {
        m_thread.join();
    }
#ifdef _WIN32
    WSACleanup();
#endif
}

void WebServer::WorkerLoop() {
    while (m_running) {
        sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        auto client = accept(m_serverSock, (sockaddr*)&clientAddr, &clientLen);
        if ((intptr_t)client == (intptr_t)INVALID_SOCKET) {
            if (!m_running) break;
            continue;
        }
        HandleClient(client);
    }
}

static void SendHttp(uintptr_t sock, int code, const std::string& status, const std::string& contentType, const std::string& body) {
    std::ostringstream ss;
    ss << "HTTP/1.1 " << code << " " << status << "\r\n";
    ss << "Content-Type: " << contentType << "\r\n";
    ss << "Content-Length: " << body.size() << "\r\n";
    ss << "Access-Control-Allow-Origin: *\r\n";
    ss << "Access-Control-Allow-Methods: GET, POST, DELETE, OPTIONS\r\n";
    ss << "Access-Control-Allow-Headers: Content-Type\r\n";
    ss << "Connection: close\r\n\r\n";
    ss << body;

    std::string resp = ss.str();
    send(sock, resp.c_str(), (int)resp.size(), 0);
    CLOSE_SOCKET(sock);
}

void WebServer::HandleClient(uintptr_t clientSock) {
    std::vector<char> buffer(65536);
    int received = recv(clientSock, buffer.data(), (int)buffer.size() - 1, 0);
    if (received <= 0) {
        CLOSE_SOCKET(clientSock);
        return;
    }
    buffer[received] = '\0';
    std::string req(buffer.data(), received);

    // Parse method and path
    size_t space1 = req.find(' ');
    if (space1 == std::string::npos) {
        CLOSE_SOCKET(clientSock);
        return;
    }
    std::string method = req.substr(0, space1);
    size_t space2 = req.find(' ', space1 + 1);
    if (space2 == std::string::npos) {
        CLOSE_SOCKET(clientSock);
        return;
    }
    std::string path = req.substr(space1 + 1, space2 - space1 - 1);

    if (method == "OPTIONS") {
        SendHttp(clientSock, 204, "No Content", "text/plain", "");
        return;
    }

    // Find body
    std::string body;
    size_t headerEnd = req.find("\r\n\r\n");
    if (headerEnd != std::string::npos) {
        body = req.substr(headerEnd + 4);
    }

    if (method == "GET") {
        if (path == "/" || path == "/index.html" || path == "/settings") {
            SendHttp(clientSock, 200, "OK", "text/html; charset=utf-8", INDEX_HTML);
            return;
        } else if (path == "/api/settings") {
            const auto& s = SettingsManager::Instance().GetSettings();
            std::ostringstream json;
            json << "{\n";
            json << "  \"show\": " << (s.show ? "true" : "false") << ",\n";
            json << "  \"species\": \"" << SpeciesToKey(s.species) << "\",\n";
            json << "  \"palette\": " << s.palette << ",\n";
            json << "  \"sizeScale\": " << s.sizeScale << ",\n";
            json << "  \"wander\": " << (s.wander ? "true" : "false") << ",\n";
            json << "  \"phrases\": " << (s.phrases ? "true" : "false") << ",\n";
            json << "  \"gravity\": " << (s.gravity ? "true" : "false") << ",\n";
            json << "  \"shake\": " << (s.shake ? "true" : "false") << ",\n";
            json << "  \"haptics\": " << (s.haptics ? "true" : "false") << ",\n";
            json << "  \"poop\": " << (s.poop ? "true" : "false") << ",\n";
            json << "  \"sleepMinutes\": " << s.sleepMinutes << ",\n";
            json << "  \"dance\": " << (s.dance ? "true" : "false") << ",\n";
            json << "  \"alwaysOnTop\": " << (s.alwaysOnTop ? "true" : "false") << ",\n";
            json << "  \"ghostMode\": " << (s.ghostMode ? "true" : "false") << ",\n";
            json << "  \"autostart\": " << (s.autostart ? "true" : "false") << ",\n";
            json << "  \"customSkinId\": \"" << s.customSkinId << "\",\n";
            json << "  \"petsCount\": " << s.petsCount << ",\n";
            json << "  \"poopsCleaned\": " << s.poopsCleaned << "\n";
            json << "}";
            SendHttp(clientSock, 200, "OK", "application/json", json.str());
            return;
        } else if (path == "/api/skins") {
            auto skins = SkinStore::Instance().ListSkins();
            std::ostringstream json;
            json << "[\n";
            for (size_t i = 0; i < skins.size(); i++) {
                json << "  {\"id\":\"" << skins[i].id << "\", \"name\":\"" << skins[i].name << "\", \"author\":\"" << skins[i].author << "\"}";
                if (i + 1 < skins.size()) json << ",";
                json << "\n";
            }
            json << "]";
            SendHttp(clientSock, 200, "OK", "application/json", json.str());
            return;
        }
    } else if (method == "POST") {
        if (path == "/api/settings") {
            auto& s = SettingsManager::Instance().GetMutableSettings();
            bool f = false;

            bool b = FindJsonBool(body, "show", s.show, &f);
            if (f) s.show = b;

            std::string sp = FindJsonStr(body, "species");
            if (!sp.empty()) s.species = KeyToSpecies(sp);

            int p = FindJsonInt(body, "palette", s.palette, &f);
            if (f) s.palette = p;

            float sz = FindJsonFloat(body, "sizeScale", s.sizeScale, &f);
            if (f && sz > 0.05f) s.sizeScale = sz;

            b = FindJsonBool(body, "wander", s.wander, &f);
            if (f) s.wander = b;

            b = FindJsonBool(body, "phrases", s.phrases, &f);
            if (f) s.phrases = b;

            b = FindJsonBool(body, "gravity", s.gravity, &f);
            if (f) s.gravity = b;

            b = FindJsonBool(body, "shake", s.shake, &f);
            if (f) s.shake = b;

            b = FindJsonBool(body, "haptics", s.haptics, &f);
            if (f) s.haptics = b;

            b = FindJsonBool(body, "poop", s.poop, &f);
            if (f) s.poop = b;

            int sl = FindJsonInt(body, "sleepMinutes", s.sleepMinutes, &f);
            if (f) s.sleepMinutes = sl;

            b = FindJsonBool(body, "dance", s.dance, &f);
            if (f) s.dance = b;

            b = FindJsonBool(body, "alwaysOnTop", s.alwaysOnTop, &f);
            if (f) s.alwaysOnTop = b;

            b = FindJsonBool(body, "ghostMode", s.ghostMode, &f);
            if (f) s.ghostMode = b;

            b = FindJsonBool(body, "autostart", s.autostart, &f);
            if (f) s.autostart = b;

            if (body.find("\"customSkinId\"") != std::string::npos) {
                s.customSkinId = FindJsonStr(body, "customSkinId");
                s.customSkinRevision++;
            }

            SettingsManager::Instance().Save();

            if (m_onSettingsChanged) {
                m_onSettingsChanged(s);
            }

            SendHttp(clientSock, 200, "OK", "application/json", "{\"ok\":true}");
            return;
        } else if (path == "/api/action") {
            std::string act = FindJsonStr(body, "action");
            if (m_onAction) {
                m_onAction(act);
            }
            SendHttp(clientSock, 200, "OK", "application/json", "{\"ok\":true}");
            return;
        } else if (path == "/api/import_skin") {
            // Read more data if multipart
            size_t contLen = 0;
            size_t clPos = req.find("Content-Length:");
            if (clPos != std::string::npos) {
                contLen = (size_t)std::strtoul(req.c_str() + clPos + 15, nullptr, 10);
            }

            std::vector<char> fullData(buffer.begin(), buffer.begin() + received);
            while (fullData.size() < headerEnd + 4 + contLen) {
                int more = recv(clientSock, buffer.data(), (int)buffer.size(), 0);
                if (more <= 0) break;
                fullData.insert(fullData.end(), buffer.begin(), buffer.begin() + more);
            }

            // Extract file from multipart
            std::string tempFile;
#ifdef _WIN32
            tempFile = "C:\\Windows\\Temp\\gupet_upload.gupet";
#else
            tempFile = "/tmp/gupet_upload.gupet";
#endif

            // Find zip magic PK\x03\x04
            const char pkMagic[] = "\x50\x4B\x03\x04";
            auto it = std::search(fullData.begin() + headerEnd, fullData.end(), pkMagic, pkMagic + 4);
            if (it != fullData.end()) {
                // Find end of file boundary or take remainder
                std::ofstream out(tempFile, std::ios::binary);
                out.write(&*it, std::distance(it, fullData.end()));
                out.close();

                std::string newId, err;
#ifdef _WIN32
                std::wstring wTemp(tempFile.begin(), tempFile.end());
                bool ok = SkinStore::Instance().ImportSkin(wTemp, newId, err);
#else
                bool ok = SkinStore::Instance().ImportSkin(tempFile, newId, err);
#endif
                if (ok) {
                    auto skin = SkinStore::Instance().LoadSkin(newId);
                    std::string skinName = skin ? skin->GetInfo().name : newId;
                    auto& s = SettingsManager::Instance().GetMutableSettings();
                    s.customSkinId = newId;
                    s.customSkinRevision++;
                    SettingsManager::Instance().Save();
                    if (m_onSettingsChanged) m_onSettingsChanged(s);

                    SendHttp(clientSock, 200, "OK", "application/json", "{\"ok\":true, \"id\":\"" + newId + "\", \"name\":\"" + skinName + "\"}");
                    return;
                } else {
                    SendHttp(clientSock, 400, "Bad Request", "application/json", "{\"ok\":false, \"error\":\"" + err + "\"}");
                    return;
                }
            }
            SendHttp(clientSock, 400, "Bad Request", "application/json", "{\"ok\":false, \"error\":\"Не найден файл .gupet\"}");
            return;
        }
    } else if (method == "DELETE") {
        if (path.rfind("/api/skins/", 0) == 0) {
            std::string skinId = path.substr(11);
            bool ok = SkinStore::Instance().DeleteSkin(skinId);
            auto& s = SettingsManager::Instance().GetMutableSettings();
            if (s.customSkinId == skinId) {
                s.customSkinId = "";
                s.customSkinRevision++;
                SettingsManager::Instance().Save();
                if (m_onSettingsChanged) m_onSettingsChanged(s);
            }
            SendHttp(clientSock, ok ? 200 : 404, ok ? "OK" : "Not Found", "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");
            return;
        }
    }

    SendHttp(clientSock, 404, "Not Found", "text/plain", "Not Found");
}

void WebServer::OpenSettingsInBrowser() {
    std::string url = "http://127.0.0.1:" + std::to_string(m_port) + "/";
#ifdef _WIN32
    std::wstring wUrl(url.begin(), url.end());
    ShellExecuteW(NULL, L"open", wUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);
#else
    std::string cmd = "xdg-open " + url + " >/dev/null 2>&1 &";
    std::system(cmd.c_str());
#endif
}

} // namespace GUPet
