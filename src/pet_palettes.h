#pragma once

#include <cstdint>
#include <algorithm>

namespace GUPet {

class PetPalettes {
public:
    static constexpr int PALETTE_COUNT = 6;

    static const wchar_t* GetName(int index) {
        static const wchar_t* names[] = {
            L"Классический",
            L"Мятный",
            L"Лавандовый",
            L"Небесный",
            L"Розовый",
            L"Графит"
        };
        if (index < 0 || index >= PALETTE_COUNT) index = 0;
        return names[index];
    }

    static uint32_t Body(int index) {
        static const uint32_t body[] = {
            0xFFD97757, 0xFF4FBF98, 0xFF9B8AEE,
            0xFF4FA8E8, 0xFFEE85AE, 0xFF697580
        };
        return body[Clamp(index)];
    }

    static uint32_t Eye(int index) {
        static const uint32_t eye[] = {
            0xFF37221E, 0xFF163328, 0xFF29234A,
            0xFF163248, 0xFF4B1F35, 0xFFF4F1E8
        };
        return eye[Clamp(index)];
    }

    static uint32_t Shade(int index) {
        static const uint32_t shade[] = {
            0xFFB7553F, 0xFF2D9875, 0xFF7866CB,
            0xFF317DAC, 0xFFC95783, 0xFF45505D
        };
        return shade[Clamp(index)];
    }

    static uint32_t Outline(int index) {
        static const uint32_t outline[] = {
            0xFF743927, 0xFF205E4A, 0xFF4B3D8B,
            0xFF20577F, 0xFF873958, 0xFF303842
        };
        return outline[Clamp(index)];
    }

    static uint32_t Highlight(int index) {
        static const uint32_t highlight[] = {
            0xFFFFAC85, 0xFF9CE4C6, 0xFFD3CAFF,
            0xFFB4E2FF, 0xFFFFC4D6, 0xFFAEB8C1
        };
        return highlight[Clamp(index)];
    }

    static uint32_t Blush(int index) {
        return Blend(Body(index), 0xFFFF6F91, 0.58f);
    }

    static uint32_t Blend(uint32_t a, uint32_t b, float amount) {
        float t = std::max(0.0f, std::min(1.0f, amount));
        uint32_t ra = (a >> 16) & 0xFF;
        uint32_t ga = (a >> 8) & 0xFF;
        uint32_t ba = a & 0xFF;

        uint32_t rb = (b >> 16) & 0xFF;
        uint32_t gb = (b >> 8) & 0xFF;
        uint32_t bb = b & 0xFF;

        uint32_t r = static_cast<uint32_t>(ra + (rb - ra) * t);
        uint32_t g = static_cast<uint32_t>(ga + (gb - ga) * t);
        uint32_t blue = static_cast<uint32_t>(ba + (bb - ba) * t);

        return (0xFF << 24) | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (blue & 0xFF);
    }

private:
    static int Clamp(int index) {
        if (index < 0) return 0;
        if (index >= PALETTE_COUNT) return PALETTE_COUNT - 1;
        return index;
    }
};

} // namespace GUPet
