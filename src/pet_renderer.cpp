#include "pet_renderer.h"

#include <cmath>
#include <algorithm>

namespace GUPet {

using namespace Gdiplus;

static const int BODY_LEFT[] = { 5, 4, 3, 2, 2, 2, 2, 2, 3, 4 };
static const int BODY_RIGHT[] = { 13, 14, 15, 16, 16, 16, 16, 16, 15, 14 };
static const int GUPET_BODY_LEFT[] = { 5, 4, 3, 3, 3, 4, 5 };
static const int GUPET_BODY_RIGHT[] = { 13, 14, 15, 15, 15, 14, 13 };
static const int LEG_X[] = { 4, 7, 11, 14 };
static const int GUPET_LEG_X[] = { 4, 6, 11, 13 };

static const char* PARTICLE_HEART[] = {
    ".XX.XX.", "XXXXXXX", "XXXXXXX", ".XXXXX.", "..XXX..", "...X..."
};
static const char* PARTICLE_SPARK[] = {
    "..X..", "..X..", "XX.XX", "..X..", "..X.."
};
static const char* PARTICLE_STAR[] = {
    "..X..", ".XXX.", "XXXXX", ".XXX.", "..X.."
};
static const char* PARTICLE_ZZZ[] = {
    "XXXXX", "...X.", "..X..", ".X...", "XXXXX"
};
static const char* PARTICLE_NOTE[] = {
    "..XXXXX", "..X...X", "..X...X", "..X...X", "XXX.XXX", "XXX.XXX"
};

static const char* POOP_PATTERN[] = {
    "....X....",
    "...XXo...",
    "..XXXXX..",
    ".XXXXXXo.",
    ".XwkXwkX.",
    "XXXXXXXXo",
    "XXXXXXXXX"
};

void PetRenderer::DrawPet(
    Graphics& g,
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

    GraphicsState state = g.Save();

    g.TranslateTransform(centerX, centerY + frame.hopOffset);
    g.RotateTransform(frame.rotation);
    g.TranslateTransform(-centerX, -(centerY + frame.hopOffset));

    g.TranslateTransform(0.0f, frame.hopOffset - idleBreath - danceBob);

    g.TranslateTransform(centerX, centerY);
    g.ScaleTransform(squashX, squashY);
    g.TranslateTransform(-centerX, -centerY);

    if (customSkin != nullptr) {
        const auto* pixels = customSkin->GetFramePixels(frame);
        if (pixels && pixels->size() == GRID_W * GRID_H) {
            for (int y = 0; y < GRID_H; y++) {
                for (int x = 0; x < GRID_W; x++) {
                    uint32_t col = (*pixels)[y * GRID_W + x];
                    if ((col >> 24) > 0) {
                        DrawPx(g, x, y, 1, 1, col, unit, topPad);
                    }
                }
            }
        }
        DrawParticles(g, unit, nowMs, particles);
        g.Restore(state);
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

    if (species == PetSpecies::CAT) {
        DrawPx(g, 14, 8, 2, 1, outline, unit, topPad);
        DrawPx(g, 15, 7 - tailLift, 2, 1, shade, unit, topPad);
        DrawPx(g, 16, 5 - tailLift, 1, 3, body, unit, topPad);
        DrawPx(g, 15, 4 - tailLift, 1, 1, highlight, unit, topPad);
        DrawPx(g, 4, 2, 4, 4, outline, unit, topPad);
        DrawPx(g, 5, 1, 3, 4, body, unit, topPad);
        DrawPx(g, 6, 1, 1, 2, 0xFFFFB2BC, unit, topPad);
        DrawPx(g, 11, 1, 3, 4, body, unit, topPad);
        DrawPx(g, 11, 2, 4, 4, outline, unit, topPad);
        DrawPx(g, 12, 1, 1, 2, 0xFFFFB2BC, unit, topPad);
    } else if (species == PetSpecies::DOG) {
        DrawPx(g, 14, 6 + tailLift, 2, 2, outline, unit, topPad);
        DrawPx(g, 15, 5 + tailLift, 2, 2, body, unit, topPad);
        DrawPx(g, 16, 5 + tailLift, 1, 1, highlight, unit, topPad);
        DrawPx(g, 1, 3, 4, 4, outline, unit, topPad);
        DrawPx(g, 2, 3, 3, 4, shade, unit, topPad);
        DrawPx(g, 13, 3, 4, 4, outline, unit, topPad);
        DrawPx(g, 13, 3, 3, 4, shade, unit, topPad);
        DrawPx(g, 3, 4, 1, 2, highlight, unit, topPad);
    }

    // Shadow
    SolidBrush shadowBrush(Color(0x55, 0x35, 0x25, 0x20));
    g.FillRectangle(&shadowBrush, unit * 4.0f, topPad + unit * 12.5f, unit * 10.0f, unit * 0.5f);

    int outlineExpansion = (frame.pose == PetPose::PET || frame.pose == PetPose::STRETCH) ? 1 : 0;
    const int* activeLeft = (species == PetSpecies::GUPET) ? GUPET_BODY_LEFT : BODY_LEFT;
    const int* activeRight = (species == PetSpecies::GUPET) ? GUPET_BODY_RIGHT : BODY_RIGHT;
    int bodyRows = (species == PetSpecies::GUPET) ? 7 : 10;
    int bodyStartY = (species == PetSpecies::GUPET) ? 3 : 2;

    for (int row = 0; row < bodyRows; row++) {
        int y = row + bodyStartY;
        int left = activeLeft[row] - outlineExpansion;
        int right = activeRight[row] + outlineExpansion;

        DrawPx(g, left - 1, y, right - left + 2, 1, outline, unit, topPad);
        DrawPx(g, left, y, right - left, 1, body, unit, topPad);

        if (row >= 2 && row <= 7) {
            DrawPx(g, right - 2, y, 1, 1, shade, unit, topPad);
        }
        if (row >= 1 && row <= 4) {
            DrawPx(g, left + 1, y, 1, 1, highlight, unit, topPad);
        }
    }

    bool walk = (frame.pose == PetPose::WALK || frame.pose == PetPose::PEEK || frame.pose == PetPose::DRAG);
    int step = static_cast<int>(frame.phase * 8.0f) & 1;
    const int* activeLegs = (species == PetSpecies::GUPET) ? GUPET_LEG_X : LEG_X;
    int legY = (species == PetSpecies::GUPET) ? 10 : 11;

    for (int i = 0; i < 4; i++) {
        int footHeight = (frame.pose == PetPose::SLEEP) ? 1 : (walk && ((i + step) & 1) == 0 ? 2 : 1);
        DrawPx(g, activeLegs[i], legY, 2, footHeight, outline, unit, topPad);
        DrawPx(g, activeLegs[i], legY, 1, footHeight, body, unit, topPad);
    }

    bool armsUp = (frame.pose == PetPose::DRAG || frame.pose == PetPose::FLY || frame.pose == PetPose::HANG ||
                   frame.pose == PetPose::STRETCH || frame.pose == PetPose::DANCE);
    bool groomLeft = (frame.pose == PetPose::GROOM && alternate == 0);
    bool groomRight = (frame.pose == PetPose::GROOM && alternate != 0);

    DrawArm(g, -1, armsUp || frame.waveSide < 0 || groomLeft, body, outline, highlight, unit, topPad);
    DrawArm(g, 1, armsUp || frame.waveSide > 0 || groomRight, body, outline, highlight, unit, topPad);

    int lookOffset = std::max(-1, std::min(1, frame.look));
    int leftEye = (species == PetSpecies::GUPET ? 6 : 5) + lookOffset;
    int rightEye = 11 + lookOffset;
    int eyeY = 5;

    PetExpression expr = (frame.pose == PetPose::SLEEP || frame.pose == PetPose::YAWN) ? PetExpression::SLEEP : frame.expression;
    DrawEye(g, leftEye, eyeY, eye, outline, expr, unit, topPad, species);
    DrawEye(g, rightEye, eyeY, eye, outline, expr, unit, topPad, species);

    if (frame.blush || frame.pose == PetPose::PET || frame.expression == PetExpression::LOVE) {
        DrawPx(g, 4, 7, 2, 1, blush, unit, topPad);
        DrawPx(g, 13, 7, 2, 1, blush, unit, topPad);
    }

    if (species == PetSpecies::DOG) {
        DrawPx(g, 7, 7, 4, 2, 0xFFFFE2BE, unit, topPad);
        DrawPx(g, 8, 7, 2, 1, outline, unit, topPad);
        if (frame.mouthOpen || frame.pose == PetPose::YAWN) {
            DrawPx(g, 8, 8, 2, 1, 0xFFEA6688, unit, topPad);
        }
    } else if (species == PetSpecies::CAT) {
        DrawPx(g, 8, 7, 1, 1, 0xFFFFA9B7, unit, topPad);
        DrawPx(g, 7, 8, 2, 1, outline, unit, topPad);
        if (frame.mouthOpen || frame.pose == PetPose::YAWN) {
            DrawPx(g, 8, 8, 1, 1, 0xFFEA6688, unit, topPad);
        }
    } else {
        if (frame.mouthOpen || frame.pose == PetPose::YAWN) {
            DrawPx(g, 8, 7, 2, 2, outline, unit, topPad);
            DrawPx(g, 8, 8, 2, 1, 0xFFEA6688, unit, topPad);
        } else {
            DrawPx(g, 8, 7, 1, 1, outline, unit, topPad);
            DrawPx(g, 9, 8, 1, 1, outline, unit, topPad);
        }
    }

    DrawParticles(g, unit, nowMs, particles);

    g.Restore(state);
}

void PetRenderer::DrawArm(
    Graphics& g,
    int side,
    bool raised,
    uint32_t body,
    uint32_t outline,
    uint32_t highlight,
    float unit,
    float top
) {
    int baseX = (side < 0) ? 1 : 16;
    if (raised) {
        DrawPx(g, baseX, 5, 2, 3, outline, unit, top);
        DrawPx(g, baseX + (side < 0 ? 1 : 0), 5, 1, 2, body, unit, top);
        DrawPx(g, (side < 0 ? 0 : 17), 4, 1, 2, body, unit, top);
    } else {
        DrawPx(g, baseX, 7, 2, 2, outline, unit, top);
        DrawPx(g, baseX + (side < 0 ? 1 : 0), 7, 1, 2, body, unit, top);
        DrawPx(g, (side < 0 ? 0 : 17), 8, 1, 1, highlight, unit, top);
    }
}

void PetRenderer::DrawEye(
    Graphics& g,
    int x,
    int y,
    uint32_t color,
    uint32_t outline,
    PetExpression expression,
    float unit,
    float top,
    PetSpecies species
) {
    if (species == PetSpecies::GUPET) {
        switch (expression) {
            case PetExpression::BLINK:
            case PetExpression::SLEEP:
                DrawPx(g, x - 1, y + 1, 3, 1, color, unit, top);
                break;
            case PetExpression::HAPPY:
                DrawPx(g, x - 1, y + 1, 1, 1, color, unit, top);
                DrawPx(g, x, y, 1, 1, color, unit, top);
                DrawPx(g, x + 1, y + 1, 1, 1, color, unit, top);
                break;
            case PetExpression::SURPRISED:
                DrawPx(g, x, y, 1, 2, color, unit, top);
                DrawPx(g, x, y, 1, 1, 0xFFFFFFFF, unit, top);
                break;
            case PetExpression::DIZZY:
                DrawPx(g, x - 1, y, 1, 1, color, unit, top);
                DrawPx(g, x + 1, y, 1, 1, color, unit, top);
                DrawPx(g, x, y + 1, 1, 1, color, unit, top);
                DrawPx(g, x - 1, y + 2, 1, 1, color, unit, top);
                DrawPx(g, x + 1, y + 2, 1, 1, color, unit, top);
                break;
            case PetExpression::ANGRY:
                DrawPx(g, x, y, 1, 2, color, unit, top);
                DrawPx(g, x - 1, y - 1, 2, 1, color, unit, top);
                break;
            case PetExpression::LOVE:
                DrawPx(g, x - 1, y, 1, 1, 0xFFFF6485, unit, top);
                DrawPx(g, x + 1, y, 1, 1, 0xFFFF6485, unit, top);
                DrawPx(g, x, y + 1, 1, 1, 0xFFFF6485, unit, top);
                break;
            case PetExpression::OPEN:
            default:
                DrawPx(g, x, y, 1, 2, color, unit, top);
                DrawPx(g, x, y, 1, 1, 0xFFFFFFFF, unit, top);
                break;
        }
        return;
    }

    switch (expression) {
        case PetExpression::BLINK:
        case PetExpression::SLEEP:
            DrawPx(g, x - 1, y + 1, 3, 1, color, unit, top);
            DrawPx(g, x, y, 1, 1, outline, unit, top);
            break;
        case PetExpression::HAPPY:
            DrawPx(g, x - 1, y + 1, 1, 1, color, unit, top);
            DrawPx(g, x, y, 1, 1, color, unit, top);
            DrawPx(g, x + 1, y + 1, 1, 1, color, unit, top);
            break;
        case PetExpression::SURPRISED:
            DrawPx(g, x, y, 2, 3, 0xFFFFF5E9, unit, top);
            DrawPx(g, x, y + 1, 1, 1, color, unit, top);
            DrawPx(g, x + 1, y, 1, 1, 0xFFFFFFFF, unit, top);
            break;
        case PetExpression::DIZZY:
            DrawPx(g, x - 1, y, 1, 1, color, unit, top);
            DrawPx(g, x + 1, y, 1, 1, color, unit, top);
            DrawPx(g, x, y + 1, 1, 1, color, unit, top);
            DrawPx(g, x - 1, y + 2, 1, 1, color, unit, top);
            DrawPx(g, x + 1, y + 2, 1, 1, color, unit, top);
            break;
        case PetExpression::ANGRY:
            DrawPx(g, x - 1, y, 3, 1, color, unit, top);
            DrawPx(g, x, y + 1, 1, 2, color, unit, top);
            break;
        case PetExpression::LOVE:
            DrawPx(g, x - 1, y, 1, 1, 0xFFFF6485, unit, top);
            DrawPx(g, x + 1, y, 1, 1, 0xFFFF6485, unit, top);
            DrawPx(g, x, y + 1, 1, 1, 0xFFFF6485, unit, top);
            DrawPx(g, x, y + 2, 1, 1, 0xFFFF6485, unit, top);
            break;
        case PetExpression::OPEN:
        default:
            DrawPx(g, x - 1, y, 3, 3, 0xFFFFF7EC, unit, top);
            DrawPx(g, x, y + 1, 1, 2, color, unit, top);
            DrawPx(g, x - 1, y, 1, 1, 0xFFFFFFFF, unit, top);
            break;
    }
}

void PetRenderer::DrawParticles(
    Graphics& g,
    float unit,
    uint64_t nowMs,
    const std::vector<PetParticle>& particles
) {
    for (const auto& particle : particles) {
        uint64_t elapsed = (nowMs >= particle.bornAtMs) ? (nowMs - particle.bornAtMs) : 0;
        float progress = std::max(0.0f, std::min(1.0f, (float)elapsed / (float)std::max((uint64_t)1, particle.durationMs)));
        int alpha = static_cast<int>(255.0f * (1.0f - progress));
        float cell = unit * 0.72f;
        float x = particle.x + particle.driftX * progress;
        float y = particle.y - unit * (2.5f * progress);
        DrawParticle(g, particle.kind, x, y, cell, alpha);
    }
}

void PetRenderer::DrawParticle(
    Graphics& g,
    ParticleKind kind,
    float x,
    float y,
    float cell,
    int alpha
) {
    const char** rows = nullptr;
    int rowCount = 0;
    uint32_t baseColor = 0;

    switch (kind) {
        case ParticleKind::HEART:
            rows = PARTICLE_HEART;
            rowCount = 6;
            baseColor = 0xFFFF5C7A;
            break;
        case ParticleKind::SPARK:
            rows = PARTICLE_SPARK;
            rowCount = 5;
            baseColor = 0xFFFFC94D;
            break;
        case ParticleKind::STAR:
            rows = PARTICLE_STAR;
            rowCount = 5;
            baseColor = 0xFFFFE07A;
            break;
        case ParticleKind::ZZZ:
            rows = PARTICLE_ZZZ;
            rowCount = 5;
            baseColor = 0xFF9EAEE5;
            break;
        case ParticleKind::NOTE:
            rows = PARTICLE_NOTE;
            rowCount = 6;
            baseColor = 0xFF64B5F6;
            break;
    }

    if (!rows) return;

    BYTE r = (baseColor >> 16) & 0xFF;
    BYTE gr = (baseColor >> 8) & 0xFF;
    BYTE b = baseColor & 0xFF;
    BYTE a = std::max(0, std::min(255, alpha));

    SolidBrush brush(Color(a, r, gr, b));

    for (int row = 0; row < rowCount; row++) {
        for (int col = 0; rows[row][col] != '\0'; col++) {
            if (rows[row][col] == 'X') {
                g.FillRectangle(&brush, x + col * cell, y + row * cell, cell, cell);
            }
        }
    }
}

void PetRenderer::DrawPx(
    Graphics& g,
    int x,
    int y,
    int w,
    int h,
    uint32_t color,
    float unit,
    float top
) {
    BYTE a = (color >> 24) & 0xFF;
    BYTE r = (color >> 16) & 0xFF;
    BYTE gr = (color >> 8) & 0xFF;
    BYTE b = color & 0xFF;

    SolidBrush brush(Color(a, r, gr, b));
    g.FillRectangle(&brush, x * unit, top + y * unit, w * unit, h * unit);
}

void PetRenderer::DrawPoop(
    Graphics& g,
    float width,
    float height,
    float scale,
    float alpha
) {
    float cell = std::min(width / 9.0f, height / 7.0f);
    float ox = (width - cell * 9.0f) / 2.0f;
    float oy = (height - cell * 7.0f) / 2.0f;

    GraphicsState state = g.Save();
    if (scale != 1.0f) {
        float cx = width / 2.0f;
        float cy = height / 2.0f;
        g.TranslateTransform(cx, cy);
        g.ScaleTransform(scale, scale);
        g.TranslateTransform(-cx, -cy);
    }

    int intAlpha = static_cast<int>(std::max(0.0f, std::min(1.0f, alpha)) * 255.0f);

    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < 9; col++) {
            char ch = POOP_PATTERN[row][col];
            uint32_t color = 0;
            switch (ch) {
                case 'X': color = 0xFF8A5A33; break;
                case 'o': color = 0xFFB27D4E; break;
                case 'w': color = 0xFFFFFFFF; break;
                case 'k': color = 0xFF1A1A1A; break;
                default: break;
            }
            if (color != 0) {
                BYTE r = (color >> 16) & 0xFF;
                BYTE gr = (color >> 8) & 0xFF;
                BYTE b = color & 0xFF;
                SolidBrush brush(Color(intAlpha, r, gr, b));
                g.FillRectangle(&brush, ox + col * cell, oy + row * cell, cell, cell);
            }
        }
    }

    g.Restore(state);
}

} // namespace GUPet
