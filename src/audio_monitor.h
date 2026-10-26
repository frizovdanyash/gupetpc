#pragma once

#include <cstdint>

namespace GUPet {

class AudioMonitor {
public:
    static AudioMonitor& Instance();

    void Init();
    void Shutdown();
    void Update();

    bool IsPlaying() const { return m_isPlaying; }

private:
    AudioMonitor() = default;
    ~AudioMonitor() { Shutdown(); }

    bool m_initialized = false;
    bool m_isPlaying = false;
    uint64_t m_lastSoundAt = 0;
    void* m_pMeter = nullptr; // IAudioMeterInformation*
    void* m_pDevice = nullptr;
    void* m_pEnum = nullptr;
};

} // namespace GUPet
