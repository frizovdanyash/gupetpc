#include "skin_store.h"
#include "miniz.h"

#ifdef _WIN32
  #include <windows.h>
  #include <gdiplus.h>
  #include <shlwapi.h>
#else
  #include <cairo.h>
#endif

#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace GUPet {

namespace fs = std::filesystem;

#ifndef _WIN32
struct StreamData {
    const uint8_t* data;
    size_t size;
    size_t pos;
};

static cairo_status_t PngReadFunc(void* closure, unsigned char* data, unsigned int length) {
    StreamData* s = (StreamData*)closure;
    if (s->pos + length > s->size) return CAIRO_STATUS_READ_ERROR;
    std::memcpy(data, s->data + s->pos, length);
    s->pos += length;
    return CAIRO_STATUS_SUCCESS;
}
#endif

static std::string FindJsonString(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos);
    if (pos == std::string::npos) return "";
    pos = json.find('\"', pos);
    if (pos == std::string::npos) return "";
    size_t endPos = json.find('\"', pos + 1);
    if (endPos == std::string::npos) return "";
    return json.substr(pos + 1, endPos - pos - 1);
}

SkinStore& SkinStore::Instance() {
    static SkinStore s_instance;
    return s_instance;
}

void SkinStore::Init(const std::string& baseDir) {
    m_skinsDir = baseDir + "/skins";
    try {
        fs::create_directories(m_skinsDir);
    } catch (...) {}
}

bool SkinStore::DecodePngBytes(const uint8_t* data, size_t size, std::vector<uint32_t>& outPixels, int& outW, int& outH) {
    if (!data || size == 0) return false;

#ifdef _WIN32
    IStream* pStream = SHCreateMemStream(data, (UINT)size);
    if (!pStream) return false;
    Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromStream(pStream);
    pStream->Release();
    if (!bmp || bmp->GetLastStatus() != Gdiplus::Ok) {
        delete bmp;
        return false;
    }

    outW = bmp->GetWidth();
    outH = bmp->GetHeight();
    if (outW <= 0 || outH <= 0 || outW > 2048 || outH > 2048) {
        delete bmp;
        return false;
    }

    outPixels.resize(outW * outH);
    Gdiplus::BitmapData bmpData;
    Gdiplus::Rect rect(0, 0, outW, outH);
    if (bmp->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmpData) == Gdiplus::Ok) {
        for (int y = 0; y < outH; y++) {
            const uint32_t* srcRow = (const uint32_t*)((const uint8_t*)bmpData.Scan0 + y * bmpData.Stride);
            for (int x = 0; x < outW; x++) {
                outPixels[y * outW + x] = srcRow[x];
            }
        }
        bmp->UnlockBits(&bmpData);
    }
    delete bmp;
    return true;
#else
    StreamData s = { data, size, 0 };
    cairo_surface_t* surface = cairo_image_surface_create_from_png_stream(PngReadFunc, &s);
    if (!surface || cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        if (surface) cairo_surface_destroy(surface);
        return false;
    }

    outW = cairo_image_surface_get_width(surface);
    outH = cairo_image_surface_get_height(surface);
    if (outW <= 0 || outH <= 0 || outW > 2048 || outH > 2048) {
        cairo_surface_destroy(surface);
        return false;
    }

    int stride = cairo_image_surface_get_stride(surface);
    unsigned char* src = cairo_image_surface_get_data(surface);
    outPixels.resize(outW * outH);
    for (int y = 0; y < outH; y++) {
        const uint32_t* row = (const uint32_t*)(src + y * stride);
        for (int x = 0; x < outW; x++) {
            outPixels[y * outW + x] = row[x];
        }
    }
    cairo_surface_destroy(surface);
    return true;
#endif
}

bool SkinStore::ParseManifest(const std::string& json, GUPetSkinInfo& outInfo, std::map<std::string, std::string>& outFrameFiles) {
    outInfo.id = FindJsonString(json, "id");
    outInfo.name = FindJsonString(json, "name");
    outInfo.author = FindJsonString(json, "author");

    if (outInfo.id.empty()) return false;
    if (outInfo.name.empty()) outInfo.name = outInfo.id;

    static const char* frameNames[] = {
        "idle", "blink", "happy", "sleep", "dance", "surprised", "dizzy", "angry", "love"
    };

    size_t framesPos = json.find("\"frames\"");
    if (framesPos != std::string::npos) {
        size_t blockEnd = json.find('}', framesPos);
        std::string framesBlock = (blockEnd != std::string::npos) ? json.substr(framesPos, blockEnd - framesPos + 1) : json.substr(framesPos);
        for (const char* name : frameNames) {
            std::string file = FindJsonString(framesBlock, name);
            if (!file.empty()) {
                outFrameFiles[name] = file;
            }
        }
    }

    if (outFrameFiles.find("idle") == outFrameFiles.end()) {
        outFrameFiles["idle"] = "idle.png";
    }

    return true;
}

std::vector<GUPetSkinInfo> SkinStore::ListSkins() {
    std::vector<GUPetSkinInfo> result;
    try {
        if (!fs::exists(m_skinsDir)) return result;
        for (const auto& entry : fs::directory_iterator(m_skinsDir)) {
            if (entry.is_directory()) {
                fs::path manifestPath = entry.path() / "manifest.json";
                if (fs::exists(manifestPath)) {
                    std::ifstream file(manifestPath, std::ios::binary);
                    if (file.is_open()) {
                        std::stringstream ss;
                        ss << file.rdbuf();
                        std::string json = ss.str();
                        GUPetSkinInfo info;
                        std::map<std::string, std::string> frames;
                        if (ParseManifest(json, info, frames)) {
                            info.folderPath = entry.path().string();
                            result.push_back(info);
                        }
                    }
                }
            }
        }
    } catch (...) {}
    return result;
}

std::shared_ptr<GUPetSkin> SkinStore::LoadFromFolder(const std::string& folderPath) {
    fs::path manifestPath = fs::path(folderPath) / "manifest.json";
    std::ifstream file(manifestPath, std::ios::binary);
    if (!file.is_open()) return nullptr;

    std::stringstream ss;
    ss << file.rdbuf();
    std::string json = ss.str();

    GUPetSkinInfo info;
    std::map<std::string, std::string> frameFiles;
    if (!ParseManifest(json, info, frameFiles)) return nullptr;
    info.folderPath = folderPath;

    std::map<std::string, std::vector<uint32_t>> loadedFrames;

    for (const auto& pair : frameFiles) {
        fs::path imgPath = fs::path(folderPath) / pair.second;
        // Case-insensitive search if needed
        if (!fs::exists(imgPath)) {
            try {
                for (const auto& e : fs::directory_iterator(folderPath)) {
                    std::string f1 = e.path().filename().string();
                    std::string f2 = pair.second;
                    std::transform(f1.begin(), f1.end(), f1.begin(), ::tolower);
                    std::transform(f2.begin(), f2.end(), f2.begin(), ::tolower);
                    if (f1 == f2) {
                        imgPath = e.path();
                        break;
                    }
                }
            } catch (...) {}
        }

        std::ifstream imgFile(imgPath, std::ios::binary);
        if (imgFile.is_open()) {
            std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(imgFile)), std::istreambuf_iterator<char>());
            std::vector<uint32_t> pixels;
            int w = 0, h = 0;
            if (DecodePngBytes(bytes.data(), bytes.size(), pixels, w, h)) {
                loadedFrames[pair.first] = std::move(pixels);
            }
        }
    }

    if (loadedFrames.empty()) return nullptr;
    return std::make_shared<GUPetSkin>(info, std::move(loadedFrames));
}

std::shared_ptr<GUPetSkin> SkinStore::LoadSkin(const std::string& id) {
    if (id.empty()) return nullptr;

    auto it = m_cachedSkins.find(id);
    if (it != m_cachedSkins.end()) return it->second;

    fs::path folder = fs::path(m_skinsDir) / id;
    if (fs::exists(folder)) {
        auto skin = LoadFromFolder(folder.string());
        if (skin) {
            m_cachedSkins[id] = skin;
            return skin;
        }
    }

    // Try finding by ID inside any skin manifest
    auto list = ListSkins();
    for (const auto& info : list) {
        if (info.id == id) {
            auto skin = LoadFromFolder(info.folderPath);
            if (skin) {
                m_cachedSkins[id] = skin;
                return skin;
            }
        }
    }

    return nullptr;
}

bool SkinStore::ImportSkin(const std::string& gupetFilePath, std::string& outId, std::string& outError) {
    std::ifstream srcFile(gupetFilePath, std::ios::binary);
    if (!srcFile.is_open()) {
        outError = "Не удалось открыть файл пакета";
        return false;
    }
    std::vector<uint8_t> zipBytes((std::istreambuf_iterator<char>(srcFile)), std::istreambuf_iterator<char>());
    srcFile.close();

    if (zipBytes.empty()) {
        outError = "Файл пуст";
        return false;
    }

    mz_zip_archive zip;
    std::memset(&zip, 0, sizeof(zip));
    if (!mz_zip_reader_init_mem(&zip, zipBytes.data(), zipBytes.size(), 0)) {
        outError = "Неверный формат архива (.gupet / .zip)";
        return false;
    }

    int manifestIndex = mz_zip_reader_locate_file(&zip, "manifest.json", NULL, 0);
    if (manifestIndex < 0) {
        mz_zip_reader_end(&zip);
        outError = "Внутри архива отсутствует файл manifest.json";
        return false;
    }

    size_t manifestSize = 0;
    char* manifestData = (char*)mz_zip_reader_extract_to_heap(&zip, manifestIndex, &manifestSize, 0);
    if (!manifestData) {
        mz_zip_reader_end(&zip);
        outError = "Не удалось прочитать manifest.json";
        return false;
    }

    std::string json(manifestData, manifestSize);
    mz_free(manifestData);

    GUPetSkinInfo info;
    std::map<std::string, std::string> frameFiles;
    if (!ParseManifest(json, info, frameFiles)) {
        mz_zip_reader_end(&zip);
        outError = "Ошибка в содержимом manifest.json";
        return false;
    }

    fs::path destFolder = fs::path(m_skinsDir) / info.id;
    try {
        fs::create_directories(destFolder);
    } catch (...) {
        mz_zip_reader_end(&zip);
        outError = "Не удалось создать папку для скина";
        return false;
    }

    fs::path destManifest = destFolder / "manifest.json";
    std::ofstream manifestOut(destManifest, std::ios::binary);
    if (manifestOut.is_open()) {
        manifestOut.write(json.data(), json.size());
        manifestOut.close();
    }

    int fileCount = (int)mz_zip_reader_get_num_files(&zip);
    for (int i = 0; i < fileCount; i++) {
        mz_zip_archive_file_stat stat;
        if (mz_zip_reader_file_stat(&zip, i, &stat)) {
            if (stat.m_is_directory) continue;
            std::string filename = stat.m_filename;
            if (filename.find("..") != std::string::npos) continue;

            size_t fileSize = 0;
            void* fileData = mz_zip_reader_extract_to_heap(&zip, i, &fileSize, 0);
            if (fileData) {
                fs::path outFilePath = destFolder / filename;
                std::ofstream outFile(outFilePath, std::ios::binary);
                if (outFile.is_open()) {
                    outFile.write((char*)fileData, fileSize);
                    outFile.close();
                }
                mz_free(fileData);
            }
        }
    }
    mz_zip_reader_end(&zip);

    m_cachedSkins.erase(info.id);
    outId = info.id;
    return true;
}

#ifdef _WIN32
bool SkinStore::ImportSkin(const std::wstring& gupetFilePath, std::string& outId, std::string& outError) {
    fs::path p(gupetFilePath);
    return ImportSkin(p.string(), outId, outError);
}
#endif

bool SkinStore::DeleteSkin(const std::string& id) {
    if (id.empty()) return false;
    m_cachedSkins.erase(id);

    try {
        fs::path folder = fs::path(m_skinsDir) / id;
        if (fs::exists(folder)) {
            fs::remove_all(folder);
            return true;
        }
    } catch (...) {}
    return false;
}

} // namespace GUPet
