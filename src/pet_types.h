#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace GUPet {

constexpr int GRID_W = 18;
constexpr int GRID_H = 13;

enum class PetSpecies {
    GUPET = 0,
    CAT = 1,
    DOG = 2
};

enum class PetPose {
    IDLE = 0,
    WALK,
    PRESS,
    DRAG,
    FLY,
    PET,
    SLEEP,
    HANG,
    PEEK,
    STRETCH,
    YAWN,
    GROOM,
    SPIN,
    DANCE
};

enum class PetExpression {
    OPEN = 0,
    BLINK,
    HAPPY,
    SURPRISED,
    DIZZY,
    ANGRY,
    SLEEP,
    LOVE
};

enum class ParticleKind {
    HEART = 0,
    SPARK,
    ZZZ,
    NOTE,
    STAR
};

enum class PetState {
    IDLE = 0,
    WALK,
    PRESS,
    DRAG,
    PET,
    FLY,
    SLEEP,
    HANG,
    PEEK
};

struct PetFrame {
    PetPose pose = PetPose::IDLE;
    PetExpression expression = PetExpression::OPEN;
    int look = 0;
    int waveSide = 0;
    bool blush = false;
    bool mouthOpen = false;
    float squash = 0.0f;
    float rotation = 0.0f;
    float hopOffset = 0.0f;
    float phase = 0.0f;
};

struct PetParticle {
    ParticleKind kind = ParticleKind::HEART;
    float x = 0.0f;
    float y = 0.0f;
    float driftX = 0.0f;
    uint64_t bornAtMs = 0;
    uint64_t durationMs = 1100;
};

struct PetSettings {
    bool show = true;
    PetSpecies species = PetSpecies::GUPET;
    int palette = 0;
    float sizeScale = 4.0f; // 3.0 .. 5.0
    bool wander = true;
    bool phrases = true;
    bool gravity = false;
    bool shake = true;
    bool haptics = true;
    bool poop = true;
    int sleepMinutes = 3;
    bool reactNotifications = false;
    bool dance = true;
    std::string customSkinId = "";
    uint64_t customSkinRevision = 0;

    float posX = 0.0f;
    float posY = 0.0f;
    bool positionSaved = false;

    int petsCount = 0;
    int poopsCleaned = 0;
};

inline const char* SpeciesToKey(PetSpecies species) {
    switch (species) {
        case PetSpecies::CAT: return "cat";
        case PetSpecies::DOG: return "dog";
        default: return "gupet";
    }
}

inline const wchar_t* SpeciesToTitle(PetSpecies species) {
    switch (species) {
        case PetSpecies::CAT: return L"Кошка";
        case PetSpecies::DOG: return L"Собака";
        default: return L"GUPet";
    }
}

inline PetSpecies KeyToSpecies(const std::string& key) {
    if (key == "cat") return PetSpecies::CAT;
    if (key == "dog") return PetSpecies::DOG;
    return PetSpecies::GUPET;
}

} // namespace GUPet
