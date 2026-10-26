#include "pet_renderer_cairo.h"
#include <cmath>
#include <algorithm>

namespace GUPet {

static const int GUPET_L[] = {5, 4, 3, 3, 3, 4, 5};
static const int GUPET_R[] = {13, 14, 15, 15, 15, 14, 13};

static const int CAT_L[] = {5, 4, 3, 2, 2, 2, 2, 2, 3, 4};
static const int CAT_R[] = {13, 14, 15, 16, 16, 16, 16, 16, 15, 14};

void PetRendererCairo::DrawPx(cairo_t* cr, int x, int y, int w, int h, uint32_t color, float unit, float top) {
    double a = ((color >> 24) & 0xFF) / 255.0;
    double r = ((color >> 16) & 0xFF) / 255.0;
    double g = ((color >> 8) & 0xFF) / 255.0;
    double b = (color & 0xFF) / 255.0;
    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_rectangle(cr, std::round(x * unit), std::round(top + y * unit), std::round(w * unit), std::round(h * unit));
    cairo_fill(cr);
}

void PetRendererCairo::DrawArm(
    cairo_t* cr,
    int side,
    bool raised,
    uint32_t body,
    uint32_t outline,
    uint32_t highlight,
    float unit,
    float top
) {
    if (side < 0) {
        if (raised) {
            DrawPx(cr, 1, 4, 2, 2, outline, unit, top);
            DrawPx(cr, 2, 4, 1, 2, body, unit, top);
            DrawPx(cr, 2, 4, 1, 1, highlight, unit, top);
        } else {
            DrawPx(cr, 1, 7, 2, 2, outline, unit, top);
            DrawPx(cr, 2, 7, 1, 2, body, unit, top);
            DrawPx(cr, 2, 7, 1, 1, highlight, unit, top);
        }
    } else {
        if (raised) {
            DrawPx(cr, 16, 4, 2, 2, outline, unit, top);
            DrawPx(cr, 16, 4, 1, 2, body, unit, top);
            DrawPx(cr, 16, 4, 1, 1, highlight, unit, top);
        } else {
            DrawPx(cr, 16, 7, 2, 2, outline, unit, top);
            DrawPx(cr, 16, 7, 1, 2, body, unit, top);
            DrawPx(cr, 16, 7, 1, 1, highlight, unit, top);
        }
    }
}

void PetRendererCairo::DrawEye(
    cairo_t* cr,
    int x,
    int y,
    uint32_t color,
    uint32_t outline,
    PetExpression expression,
    float unit,
    float top,
    PetSpecies species
) {
    switch (expression) {
        case PetExpression::BLINK:
        case PetExpression::SLEEP:
            DrawPx(cr, x - 1, y + 1, 3, 1, color, unit, top);
            break;
        case PetExpression::HAPPY:
            DrawPx(cr, x - 1, y + 1, 1, 1, color, unit, top);
            DrawPx(cr, x, y, 1, 1, color, unit, top);
            DrawPx(cr, x + 1, y + 1, 1, 1, color, unit, top);
            break;
        case PetExpression::SURPRISED:
            DrawPx(cr, x - 1, y, 3, 3, outline, unit, top);
            DrawPx(cr, x, y + 1, 1, 1, 0xFFFFFFFF, unit, top);
            break;
        case PetExpression::DIZZY:
            DrawPx(cr, x - 1, y, 1, 1, color, unit, top);
            DrawPx(cr, x + 1, y, 1, 1, color, unit, top);
            DrawPx(cr, x, y + 1, 1, 1, color, unit, top);
            DrawPx(cr, x - 1, y + 2, 1, 1, color, unit, top);
            DrawPx(cr, x + 1, y + 2, 1, 1, color, unit, top);
            break;
        case PetExpression::ANGRY:
            DrawPx(cr, x - 1, y, 3, 1, outline, unit, top);
            DrawPx(cr, x, y + 1, 1, 2, color, unit, top);
            DrawPx(cr, x, y + 1, 1, 1, 0xFFFFFFFF, unit, top);
            break;
        case PetExpression::LOVE:
            DrawPx(cr, x - 1, y, 1, 1, 0xFFFF4D79, unit, top);
            DrawPx(cr, x + 1, y, 1, 1, 0xFFFF4D79, unit, top);
            DrawPx(cr, x - 1, y + 1, 3, 1, 0xFFFF4D79, unit, top);
            DrawPx(cr, x, y + 2, 1, 1, 0xFFFF4D79, unit, top);
            break;
        case PetExpression::OPEN:
        default:
            DrawPx(cr, x, y, 1, 2, color, unit, top);
            DrawPx(cr, x, y, 1, 1, 0xFFFFFFFF, unit, top);
            break;
    }
}

void PetRendererCairo::DrawParticle(
    cairo_t* cr,
    ParticleKind kind,
    float x,
    float y,
    float cell,
    float alpha
) {
    if (alpha <= 0.0f) return;

    auto pxPart = [&](int px, int py, int w, int h, uint32_t col) {
        double a = (((col >> 24) & 0xFF) / 255.0) * alpha;
        double r = ((col >> 16) & 0xFF) / 255.0;
        double g = ((col >> 8) & 0xFF) / 255.0;
        double b = (col & 0xFF) / 255.0;
        cairo_set_source_rgba(cr, r, g, b, a);
        cairo_rectangle(cr, std::round(x + px * cell), std::round(y + py * cell), std::round(w * cell), std::round(h * cell));
        cairo_fill(cr);
    };

    switch (kind) {
        case ParticleKind::HEART:
            pxPart(0, 0, 2, 2, 0xFFFF4D79);
            pxPart(3, 0, 2, 2, 0xFFFF4D79);
            pxPart(0, 2, 5, 2, 0xFFFF4D79);
            pxPart(1, 4, 3, 1, 0xFFFF4D79);
            pxPart(2, 5, 1, 1, 0xFFFF4D79);
            pxPart(1, 1, 1, 1, 0xFFFFB3C7);
            break;
        case ParticleKind::SPARK:
            pxPart(1, 0, 1, 3, 0xFFFFDF5B);
            pxPart(0, 1, 3, 1, 0xFFFFDF5B);
            pxPart(1, 1, 1, 1, 0xFFFFFFFF);
            break;
        case ParticleKind::ZZZ:
            pxPart(0, 0, 3, 1, 0xFF96B3E8);
            pxPart(1, 1, 1, 1, 0xFF96B3E8);
            pxPart(0, 2, 3, 1, 0xFF96B3E8);
            break;
        case ParticleKind::NOTE:
            pxPart(2, 0, 1, 4, 0xFF6C63FF);
            pxPart(0, 3, 3, 2, 0xFF6C63FF);
            pxPart(1, 3, 1, 1, 0xFFB3ADFF);
            break;
        case ParticleKind::STAR:
            pxPart(1, 0, 1, 1, 0xFFFFDF5B);
            pxPart(0, 1, 3, 1, 0xFFFFDF5B);
            pxPart(1, 2, 1, 1, 0xFFFFDF5B);
            pxPart(1, 1, 1, 1, 0xFFFFFFFF);
            break;
    }
}

void PetRendererCairo::DrawParticles(
    cairo_t* cr,
    float unit,
    uint64_t nowMs,
    const std::vector<PetParticle>& particles
) {
    float cell = unit * 0.72f;
    for (const auto& p : particles) {
        uint64_t age = nowMs - p.bornAtMs;
        if (age >= p.durationMs) continue;
        float progress = static_cast<float>(age) / static_cast<float>(p.durationMs);

        float rise = (p.kind == ParticleKind::ZZZ) ? (unit * 12.0f * progress) : (unit * 8.0f * progress);
        float drift = (p.kind == ParticleKind::ZZZ)
            ? (p.driftX + std::sin(progress * 6.28f) * unit * 2.0f)
            : (p.driftX * progress);

        float alpha = 1.0f;
        if (progress > 0.65f) {
            alpha = (1.0f - (progress - 0.65f) / 0.35f);
        }
        DrawParticle(cr, p.kind, p.x + drift, p.y - rise, cell, alpha);
    }
}

void PetRendererCairo::DrawPet(
    cairo_t* cr,
    float width,
    float height,
    PetSpecies species,
    int palette,
    const PetFrame& frame,
    uint64_t nowMs,
    const std::vector<PetParticle>& particles,
    const std::shared_ptr<GUPetSkin>& customSkin
) {
    if (width <= 0.0f || height <= 0.0f) return;
    float unit = std::min(width / GRID_W, height / (GRID_H + 2.0f));
    if (unit <= 0.0f) return;

    float topPad = unit * 2.0f;
    float centerX = width / 2.0f;
    float centerY = topPad + unit * 6.5f;

    float idleBreath = 0.0f;
    if (frame.pose == PetPose::IDLE || frame.pose == PetPose::PET || frame.pose == PetPose::SLEEP) {
        float freq = (frame.pose == PetPose::SLEEP) ? 2.2f : 3.1f;
        idleBreath = std::sin(frame.phase * freq) * 0.34f * unit;
    }

    float danceBob = 0.0f;
    if (frame.pose == PetPose::DANCE) {
        danceBob = std::abs(std::sin(frame.phase * 10.0f)) * unit * 0.8f;
    }

    float squashX = 1.0f;
    float squashY = 1.0f;
    if (frame.pose == PetPose::STRETCH) {
        squashX = 0.97f;
        squashY = 1.06f;
    } else if (frame.squash > 0.0f) {
        squashX = 1.08f;
        squashY = 0.91f;
    } else if (frame.squash < 0.0f) {
        squashX = 0.93f;
        squashY = 1.08f;
    }

    cairo_save(cr);

    cairo_translate(cr, centerX, centerY + frame.hopOffset);
    cairo_rotate(cr, frame.rotation * M_PI / 180.0);
    cairo_translate(cr, -centerX, -(centerY + frame.hopOffset));

    cairo_translate(cr, 0.0, frame.hopOffset - idleBreath - danceBob);

    cairo_translate(cr, centerX, centerY);
    cairo_scale(cr, squashX, squashY);
    cairo_translate(cr, -centerX, -centerY);

    if (customSkin != nullptr) {
        const auto* pixels = customSkin->GetFramePixels(frame);
        if (pixels && pixels->size() == GRID_W * GRID_H) {
            for (int y = 0; y < GRID_H; y++) {
                for (int x = 0; x < GRID_W; x++) {
                    uint32_t col = (*pixels)[y * GRID_W + x];
                    if ((col >> 24) > 0) {
                        DrawPx(cr, x, y, 1, 1, col, unit, topPad);
                    }
                }
            }
        }
        DrawParticles(cr, unit, nowMs, particles);
        cairo_restore(cr);
        return;
    }

    uint32_t body = PetPalettes::Body(palette);
    uint32_t eye = PetPalettes::Eye(palette);
    uint32_t shade = PetPalettes::Shade(palette);
    uint32_t outline = PetPalettes::Outline(palette);
    uint32_t highlight = PetPalettes::Highlight(palette);
    uint32_t blush = PetPalettes::Blush(palette);

    int alternate = static_cast<int>(frame.phase * (species == PetSpecies::DOG ? 13.0f : 5.0f)) & 1;
    int tailLift = 0;
    if (species == PetSpecies::CAT) {
        tailLift = (alternate == 0) ? 0 : 1;
    } else if (species == PetSpecies::DOG) {
        tailLift = (alternate == 0) ? 1 : -1;
    }

    // Shadow
    DrawPx(cr, 4, 12, 10, 1, 0x48352520, unit, topPad);

    if (species == PetSpecies::GUPET) {
        for (int r = 0; r < 7; r++) {
            int y = 3 + r;
            int l = GUPET_L[r];
            int rgt = GUPET_R[r];
            DrawPx(cr, l - 1, y, rgt - l + 2, 1, outline, unit, topPad);
            DrawPx(cr, l, y, rgt - l, 1, body, unit, topPad);
            if (r >= 2 && r <= 5) DrawPx(cr, rgt - 2, y, 1, 1, shade, unit, topPad);
            if (r >= 1 && r <= 3) DrawPx(cr, l + 1, y, 1, 1, highlight, unit, topPad);
        }

        DrawPx(cr, 4, 10, 2, 1, outline, unit, topPad);
        DrawPx(cr, 4, 10, 1, 1, body, unit, topPad);
        DrawPx(cr, 6, 10, 2, 1, outline, unit, topPad);
        DrawPx(cr, 6, 10, 1, 1, body, unit, topPad);
        DrawPx(cr, 11, 10, 2, 1, outline, unit, topPad);
        DrawPx(cr, 12, 10, 1, 1, body, unit, topPad);
        DrawPx(cr, 13, 10, 2, 1, outline, unit, topPad);
        DrawPx(cr, 14, 10, 1, 1, body, unit, topPad);
    } else if (species == PetSpecies::CAT) {
        // Cat ears
        DrawPx(cr, 4, 2, 4, 4, outline, unit, topPad);
        DrawPx(cr, 5, 1, 3, 4, body, unit, topPad);
        DrawPx(cr, 6, 1, 1, 2, 0xFFFFB2BC, unit, topPad);

        DrawPx(cr, 11, 1, 3, 4, body, unit, topPad);
        DrawPx(cr, 11, 2, 4, 4, outline, unit, topPad);
        DrawPx(cr, 12, 1, 1, 2, 0xFFFFB2BC, unit, topPad);

        for (int r = 0; r < 10; r++) {
            int y = 2 + r;
            int l = CAT_L[r];
            int rgt = CAT_R[r];
            DrawPx(cr, l - 1, y, rgt - l + 2, 1, outline, unit, topPad);
            DrawPx(cr, l, y, rgt - l, 1, body, unit, topPad);
            if (r >= 2 && r <= 8) DrawPx(cr, rgt - 2, y, 1, 1, shade, unit, topPad);
            if (r >= 1 && r <= 4) DrawPx(cr, l + 1, y, 1, 1, highlight, unit, topPad);
        }

        // Tail
        DrawPx(cr, 14, 8 - tailLift, 2, 1, outline, unit, topPad);
        DrawPx(cr, 15, 6 - tailLift, 2, 2, body, unit, topPad);
        DrawPx(cr, 15, 7 - tailLift, 2, 1, outline, unit, topPad);

        // Legs
        DrawPx(cr, 4, 11, 2, 1, outline, unit, topPad);
        DrawPx(cr, 4, 11, 1, 1, body, unit, topPad);
        DrawPx(cr, 7, 11, 2, 1, outline, unit, topPad);
        DrawPx(cr, 8, 11, 1, 1, body, unit, topPad);
        DrawPx(cr, 11, 11, 2, 1, outline, unit, topPad);
        DrawPx(cr, 11, 11, 1, 1, body, unit, topPad);
        DrawPx(cr, 14, 11, 2, 1, outline, unit, topPad);
        DrawPx(cr, 14, 11, 1, 1, body, unit, topPad);
    } else { // DOG
        // Dog ears
        DrawPx(cr, 1, 3, 4, 4, outline, unit, topPad);
        DrawPx(cr, 2, 3, 3, 4, shade, unit, topPad);
        DrawPx(cr, 13, 3, 4, 4, outline, unit, topPad);
        DrawPx(cr, 13, 3, 3, 4, shade, unit, topPad);

        for (int r = 0; r < 10; r++) {
            int y = 2 + r;
            int l = CAT_L[r];
            int rgt = CAT_R[r];
            DrawPx(cr, l - 1, y, rgt - l + 2, 1, outline, unit, topPad);
            DrawPx(cr, l, y, rgt - l, 1, body, unit, topPad);
            if (r >= 2 && r <= 8) DrawPx(cr, rgt - 2, y, 1, 1, shade, unit, topPad);
            if (r >= 1 && r <= 4) DrawPx(cr, l + 1, y, 1, 1, highlight, unit, topPad);
        }

        // Tail
        DrawPx(cr, 14, 6 + tailLift, 2, 2, outline, unit, topPad);
        DrawPx(cr, 15, 5 + tailLift, 2, 2, body, unit, topPad);

        // Legs
        DrawPx(cr, 4, 11, 2, 1, outline, unit, topPad);
        DrawPx(cr, 4, 11, 1, 1, body, unit, topPad);
        DrawPx(cr, 7, 11, 2, 1, outline, unit, topPad);
        DrawPx(cr, 8, 11, 1, 1, body, unit, topPad);
        DrawPx(cr, 11, 11, 2, 1, outline, unit, topPad);
        DrawPx(cr, 11, 11, 1, 1, body, unit, topPad);
        DrawPx(cr, 14, 11, 2, 1, outline, unit, topPad);
        DrawPx(cr, 14, 11, 1, 1, body, unit, topPad);
    }

    // Arms
    bool waveL = (frame.waveSide < 0);
    bool waveR = (frame.waveSide > 0);
    DrawArm(cr, -1, waveL, body, outline, highlight, unit, topPad);
    DrawArm(cr, 1, waveR, body, outline, highlight, unit, topPad);

    // Eyes
    int eyeY = (species == PetSpecies::GUPET) ? 5 : 5;
    int eyeLeft = (species == PetSpecies::GUPET) ? (6 + frame.look) : (5 + frame.look);
    int eyeRight = (species == PetSpecies::GUPET) ? (11 + frame.look) : (11 + frame.look);

    DrawEye(cr, eyeLeft, eyeY, eye, outline, frame.expression, unit, topPad, species);
    DrawEye(cr, eyeRight, eyeY, eye, outline, frame.expression, unit, topPad, species);

    // Blush
    if (frame.blush) {
        DrawPx(cr, eyeLeft - 2, eyeY + 2, 2, 1, blush, unit, topPad);
        DrawPx(cr, eyeRight + 1, eyeY + 2, 2, 1, blush, unit, topPad);
    }

    // Muzzle / Mouth
    if (species == PetSpecies::DOG) {
        DrawPx(cr, 7 + frame.look, 7, 4, 2, 0xFFFFE2BE, unit, topPad);
        DrawPx(cr, 8 + frame.look, 7, 2, 1, outline, unit, topPad);
        if (frame.mouthOpen) {
            DrawPx(cr, 8 + frame.look, 8, 2, 1, 0xFFFF5252, unit, topPad);
        }
    } else if (species == PetSpecies::CAT) {
        DrawPx(cr, 8 + frame.look, 7, 1, 1, 0xFFFFA9B7, unit, topPad);
        DrawPx(cr, 7 + frame.look, 8, 2, 1, outline, unit, topPad);
        if (frame.mouthOpen) {
            DrawPx(cr, 8 + frame.look, 8, 1, 1, 0xFFFF5252, unit, topPad);
        }
    } else {
        if (frame.mouthOpen) {
            DrawPx(cr, 8 + frame.look, 7, 2, 2, outline, unit, topPad);
            DrawPx(cr, 8 + frame.look, 8, 2, 1, 0xFFFF5252, unit, topPad);
        } else {
            DrawPx(cr, 8 + frame.look, 7, 1, 1, outline, unit, topPad);
            DrawPx(cr, 9 + frame.look, 8, 1, 1, outline, unit, topPad);
        }
    }

    DrawParticles(cr, unit, nowMs, particles);
    cairo_restore(cr);
}

void PetRendererCairo::DrawPoop(
    cairo_t* cr,
    float width,
    float height,
    float scale,
    float alpha
) {
    if (alpha <= 0.0f) return;
    float unit = std::min(width / 16.0f, height / 16.0f) * scale;
    float top = (height - unit * 10.0f) / 2.0f;
    float left = (width - unit * 12.0f) / 2.0f;

    auto pxPoop = [&](int x, int y, int w, int h, uint32_t col) {
        double a = (((col >> 24) & 0xFF) / 255.0) * alpha;
        double r = ((col >> 16) & 0xFF) / 255.0;
        double g = ((col >> 8) & 0xFF) / 255.0;
        double b = (col & 0xFF) / 255.0;
        cairo_set_source_rgba(cr, r, g, b, a);
        cairo_rectangle(cr, std::round(left + x * unit), std::round(top + y * unit), std::round(w * unit), std::round(h * unit));
        cairo_fill(cr);
    };

    uint32_t brown = 0xFF795548;
    uint32_t dark = 0xFF4E342E;
    uint32_t light = 0xFF8D6E63;

    // Swirl
    pxPoop(5, 1, 2, 1, dark);
    pxPoop(6, 0, 1, 1, dark);
    pxPoop(4, 2, 4, 1, brown);
    pxPoop(4, 2, 1, 1, light);

    // Mid
    pxPoop(3, 3, 6, 2, brown);
    pxPoop(3, 3, 1, 2, light);
    pxPoop(8, 3, 1, 2, dark);

    // Base
    pxPoop(2, 5, 8, 2, brown);
    pxPoop(2, 5, 1, 2, light);
    pxPoop(9, 5, 1, 2, dark);

    // Bottom
    pxPoop(1, 7, 10, 2, brown);
    pxPoop(1, 7, 1, 2, light);
    pxPoop(10, 7, 1, 2, dark);

    // Eyes
    pxPoop(4, 5, 1, 1, 0xFFFFFFFF);
    pxPoop(7, 5, 1, 1, 0xFFFFFFFF);
    pxPoop(4, 5, 1, 1, 0xFF000000);
    pxPoop(7, 5, 1, 1, 0xFF000000);
}

} // namespace GUPet
