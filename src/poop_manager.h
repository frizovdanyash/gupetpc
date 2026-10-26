#pragma once

#include <windows.h>
#include <vector>
#include <cstdint>
#include <functional>

namespace GUPet {

struct PoopItem {
    HWND hwnd = NULL;
    int x = 0;
    int y = 0;
    int width = 36;
    int height = 28;
    float scale = 0.1f;
    float alpha = 1.0f;
    bool appearing = true;
    bool cleaning = false;
    uint64_t bornAt = 0;
};

class PoopManager {
public:
    static PoopManager& Instance();

    void Init(std::function<void(int px, int py)> onCleanedCallback);
    void Shutdown();

    void SpawnPoop(int petX, int petY, int petW, int petH, int look);
    void CleanAll(bool count = true);
    void Update();

    int GetCount() const { return (int)m_poops.size(); }

private:
    PoopManager() = default;
    ~PoopManager() { Shutdown(); }

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void RenderPoop(PoopItem& item);
    void RemovePoop(size_t index, bool count = true);

    std::vector<PoopItem> m_poops;
    std::function<void(int px, int py)> m_onCleaned;
    static constexpr size_t MAX_POOPS = 6;
};

} // namespace GUPet
