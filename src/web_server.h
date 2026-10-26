#pragma once

#include "pet_types.h"
#include <string>
#include <thread>
#include <atomic>
#include <functional>

namespace GUPet {

class WebServer {
public:
    static WebServer& Instance();

    bool Start(int port = 23847);
    void Stop();
    int GetPort() const { return m_port; }

    void SetOnSettingsChanged(std::function<void(const PetSettings&)> cb) {
        m_onSettingsChanged = cb;
    }

    void SetOnAction(std::function<void(const std::string&)> cb) {
        m_onAction = cb;
    }

    void OpenSettingsInBrowser();

private:
    WebServer() = default;
    ~WebServer() { Stop(); }

    void WorkerLoop();
    void HandleClient(uintptr_t clientSock);

    int m_port = 23847;
    std::atomic<bool> m_running{false};
    std::thread m_thread;
    uintptr_t m_serverSock = 0;

    std::function<void(const PetSettings&)> m_onSettingsChanged;
    std::function<void(const std::string&)> m_onAction;
};

} // namespace GUPet
