#pragma once

#include "pet_types.h"
#include "pet_palettes.h"
#include "skin_store.h"

#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <memory>

namespace GUPet {

class PetRenderer {
public:
    static void DrawPet(
        Gdiplus::Graphics& g,
        float width,
        float height,
        PetSpecies species,
        int palette,
        const PetFrame& frame,
        uint64_t nowMs,
        const std::vector<PetParticle>& particles,
        const std::shared_ptr<GUPetSkin>& customSkin = nullptr
    );

    static void DrawPoop(
        Gdiplus::Graphics& g,
        float width,
        float height,
        float scale = 1.0f,
        float alpha = 1.0f
    );

private:
    static void DrawParticles(
        Gdiplus::Graphics& g,
        float unit,
        uint64_t nowMs,
        const std::vector<PetParticle>& particles
    );

    static void DrawParticle(
        Gdiplus::Graphics& g,
        ParticleKind kind,
        float x,
        float y,
        float cell,
        int alpha
    );

    static void DrawArm(
        Gdiplus::Graphics& g,
        int side,
        bool raised,
        uint32_t body,
        uint32_t outline,
        uint32_t highlight,
        float unit,
        float top
    );

    static void DrawEye(
        Gdiplus::Graphics& g,
        int x,
        int y,
        uint32_t color,
        uint32_t outline,
        PetExpression expression,
        float unit,
        float top,
        PetSpecies species
    );

    static void DrawPx(
        Gdiplus::Graphics& g,
        int x,
        int y,
        int w,
        int h,
        uint32_t color,
        float unit,
        float top
    );
};

} // namespace GUPet
