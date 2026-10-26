#pragma once

#include "pet_types.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace GUPet {

struct GUPetSkinInfo {
    std::string id;
    std::string name;
    std::string author;
    std::wstring folderPath;
};

class GUPetSkin {
public:
    GUPetSkin(const GUPetSkinInfo& info, std::map<std::string, std::vector<uint32_t>> frames)
        : m_info(info), m_frames(std::move(frames)) {}

    const GUPetSkinInfo& GetInfo() const { return m_info; }
    
    const std::vector<uint32_t>* GetFramePixels(const PetFrame& frame) const {
        std::string name = "idle";
        if (frame.pose == PetPose::SLEEP || frame.pose == PetPose::YAWN) name = "sleep";
        else if (frame.pose == PetPose::DANCE || frame.pose == PetPose::SPIN) name = "dance";
        else if (frame.expression == PetExpression::BLINK) name = "blink";
        else if (frame.expression == PetExpression::HAPPY) name = "happy";
        else if (frame.expression == PetExpression::SURPRISED) name = "surprised";
        else if (frame.expression == PetExpression::DIZZY) name = "dizzy";
        else if (frame.expression == PetExpression::ANGRY) name = "angry";
        else if (frame.expression == PetExpression::LOVE) name = "love";

        auto it = m_frames.find(name);
        if (it != m_frames.end()) return &it->second;

        auto idleIt = m_frames.find("idle");
        if (idleIt != m_frames.end()) return &idleIt->second;

        return nullptr;
    }

private:
    GUPetSkinInfo m_info;
    std::map<std::string, std::vector<uint32_t>> m_frames;
};

class SkinStore {
public:
    static SkinStore& Instance();

    void Init(const std::wstring& baseDir);
    std::vector<GUPetSkinInfo> ListSkins();
    std::shared_ptr<GUPetSkin> LoadSkin(const std::string& id);
    bool ImportSkin(const std::wstring& gupetFilePath, std::string& outId, std::string& outError);
    bool DeleteSkin(const std::string& id);

    std::wstring GetSkinsDir() const { return m_skinsDir; }

private:
    SkinStore() = default;
    std::shared_ptr<GUPetSkin> LoadFromFolder(const std::wstring& folderPath);
    bool ParseManifest(const std::string& json, GUPetSkinInfo& outInfo, std::map<std::string, std::string>& outFrameFiles);
    bool DecodePngBytes(const uint8_t* data, size_t size, std::vector<uint32_t>& outPixels, int& outW, int& outH);

    std::wstring m_skinsDir;
    std::map<std::string, std::shared_ptr<GUPetSkin>> m_cachedSkins;
};

} // namespace GUPet
