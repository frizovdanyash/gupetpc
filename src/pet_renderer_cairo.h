#pragma once

#include "pet_types.h"
#include "pet_palettes.h"
#include "skin_store.h"

#include <cairo.h>
#include <vector>
#include <memory>

namespace GUPet {

class PetRendererCairo {
public:
    static void DrawPet(
        cairo_t* cr,
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
        cairo_t* cr,
        float width,
        float height,
        float scale = 1.0f,
        float alpha = 1.0f
    );

private:
    static void DrawParticles(
        cairo_t* cr,
        float unit,
        uint64_t nowMs,
        const std::vector<PetParticle>& particles
    );

    static void DrawParticle(
        cairo_t* cr,
        ParticleKind kind,
        float x,
        float y,
        float cell,
        float alpha
    );

    static void DrawArm(
        cairo_t* cr,
        int side,
        bool raised,
        uint32_t body,
        uint32_t outline,
        uint32_t highlight,
        float unit,
        float top
    );

    static void DrawEye(
        cairo_t* cr,
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
        cairo_t* cr,
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
