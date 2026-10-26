#include "audio_monitor.h"

#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>

#ifndef __IAudioMeterInformation_INTERFACE_DEFINED__
#define __IAudioMeterInformation_INTERFACE_DEFINED__
static const IID IID_IAudioMeterInformation_Local = {
    0xC02216F6, 0x8C67, 0x4B5B, {0x9D, 0x00, 0xD0, 0x08, 0xE7, 0x3E, 0x00, 0x64}
};

MIDL_INTERFACE("C02216F6-8C67-4B5B-9D00-D008E73E0064")
IAudioMeterInformationLocal : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetPeakValue(float *pfPeak) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetMeteringChannelCount(UINT *pnChannelCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetChannelsPeakValues(UINT32 u32ChannelCount, float *afPeakValues) = 0;
    virtual HRESULT STDMETHODCALLTYPE QueryHardwareSupport(DWORD *pdwHardwareSupportMask) = 0;
};
#endif

namespace GUPet {

AudioMonitor& AudioMonitor::Instance() {
    static AudioMonitor s_instance;
    return s_instance;
}

void AudioMonitor::Init() {
    if (m_initialized) return;

    IMMDeviceEnumerator* pEnum = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
                                  __uuidof(IMMDeviceEnumerator), (void**)&pEnum);
    if (SUCCEEDED(hr) && pEnum) {
        IMMDevice* pDevice = nullptr;
        hr = pEnum->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
        if (SUCCEEDED(hr) && pDevice) {
            IAudioMeterInformationLocal* pMeter = nullptr;
            hr = pDevice->Activate(IID_IAudioMeterInformation_Local, CLSCTX_ALL, NULL, (void**)&pMeter);
            if (SUCCEEDED(hr) && pMeter) {
                m_pEnum = pEnum;
                m_pDevice = pDevice;
                m_pMeter = pMeter;
                m_initialized = true;
                return;
            }
            pDevice->Release();
        }
        pEnum->Release();
    }
}

void AudioMonitor::Shutdown() {
    if (m_pMeter) {
        ((IAudioMeterInformationLocal*)m_pMeter)->Release();
        m_pMeter = nullptr;
    }
    if (m_pDevice) {
        ((IMMDevice*)m_pDevice)->Release();
        m_pDevice = nullptr;
    }
    if (m_pEnum) {
        ((IMMDeviceEnumerator*)m_pEnum)->Release();
        m_pEnum = nullptr;
    }
    m_initialized = false;
    m_isPlaying = false;
}

void AudioMonitor::Update() {
    if (!m_initialized) {
        Init();
        if (!m_initialized) return;
    }

    IAudioMeterInformationLocal* pMeter = (IAudioMeterInformationLocal*)m_pMeter;
    float peak = 0.0f;
    HRESULT hr = pMeter->GetPeakValue(&peak);
    if (FAILED(hr)) {
        Shutdown();
        return;
    }

    uint64_t now = GetTickCount64();
    if (peak > 0.008f) {
        m_lastSoundAt = now;
        m_isPlaying = true;
    } else {
        if (m_isPlaying && (now - m_lastSoundAt > 1200)) {
            m_isPlaying = false;
        }
    }
}

} // namespace GUPet
