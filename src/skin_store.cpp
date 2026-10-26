#include "skin_store.h"
#include "miniz.h"

#include <windows.h>
#include <gdiplus.h>
#include <shlwapi.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace GUPet {

static std::string WStringToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &str[0], len, NULL, NULL);
    return str;
}

static std::wstring Utf8ToWString(const std::string& str) {
    if (str.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstr(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], len);
    return wstr;
}

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

void SkinStore::Init(const std::wstring& baseDir) {
    m_skinsDir = baseDir + L"\\skins";
    CreateDirectoryW(m_skinsDir.c_str(), NULL);
}

bool SkinStore::DecodePngBytes(const uint8_t* data, size_t size, std::vector<uint32_t>& outPixels, int& outW, int& outH) {
    if (!data || size == 0) return false;

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!hMem) return false;
    void* p = GlobalLock(hMem);
    if (!p) { GlobalFree(hMem); return false; }
    memcpy(p, data, size);
    GlobalUnlock(hMem);

    IStream* pStream = NULL;
    if (CreateStreamOnHGlobal(hMem, TRUE, &pStream) != S_OK) {
        GlobalFree(hMem);
        return false;
    }

    Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromStream(pStream);
    pStream->Release();

    if (!bmp || bmp->GetLastStatus() != Gdiplus::Ok) {
        delete bmp;
        return false;
    }

    outW = bmp->GetWidth();
    outH = bmp->GetHeight();
    if (outW <= 0 || outH <= 0 || outW > 256 || outH > 256) {
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
    std::wstring searchPattern = m_skinsDir + L"\\*";
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return result;

    do {
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            wcscmp(findData.cFileName, L".") != 0 &&
            wcscmp(findData.cFileName, L"..") != 0 &&
            findData.cFileName[0] != L'.') {

            std::wstring folder = m_skinsDir + L"\\" + findData.cFileName;
            std::wstring manifestPath = folder + L"\\manifest.json";

            std::ifstream file(manifestPath.c_str(), std::ios::binary);
            if (file.is_open()) {
                std::stringstream ss;
                ss << file.rdbuf();
                std::string content = ss.str();
                GUPetSkinInfo info;
                std::map<std::string, std::string> frames;
                if (ParseManifest(content, info, frames)) {
                    info.folderPath = folder;
                    result.push_back(info);
                }
            }
        }
    } while (FindNextFileW(hFind, &findData));
    FindClose(hFind);

    return result;
}

std::shared_ptr<GUPetSkin> SkinStore::LoadFromFolder(const std::wstring& folderPath) {
    std::wstring manifestPath = folderPath + L"\\manifest.json";
    std::ifstream file(manifestPath.c_str(), std::ios::binary);
    if (!file.is_open()) return nullptr;

    std::stringstream ss;
    ss << file.rdbuf();
    std::string json = ss.str();

    GUPetSkinInfo info;
    std::map<std::string, std::string> frameFiles;
    if (!ParseManifest(json, info, frameFiles)) return nullptr;
    info.folderPath = folderPath;

    std::map<std::string, std::vector<uint32_t>> frames;
    for (const auto& pair : frameFiles) {
        std::wstring imgPath = folderPath + L"\\" + Utf8ToWString(pair.second);
        std::ifstream imgFile(imgPath.c_str(), std::ios::binary | std::ios::ate);
        if (imgFile.is_open()) {
            size_t size = (size_t)imgFile.tellg();
            imgFile.seekg(0, std::ios::beg);
            std::vector<uint8_t> buffer(size);
            imgFile.read((char*)buffer.data(), size);

            std::vector<uint32_t> pixels;
            int w = 0, h = 0;
            if (DecodePngBytes(buffer.data(), buffer.size(), pixels, w, h)) {
                frames[pair.first] = std::move(pixels);
            }
        }
    }

    if (frames.find("idle") == frames.end()) return nullptr;
    return std::make_shared<GUPetSkin>(info, std::move(frames));
}

std::shared_ptr<GUPetSkin> SkinStore::LoadSkin(const std::string& id) {
    if (id.empty()) return nullptr;
    auto it = m_cachedSkins.find(id);
    if (it != m_cachedSkins.end()) return it->second;

    std::wstring folder = m_skinsDir + L"\\" + Utf8ToWString(id);
    auto skin = LoadFromFolder(folder);
    if (skin) {
        m_cachedSkins[id] = skin;
    }
    return skin;
}

bool SkinStore::ImportSkin(const std::wstring& gupetFilePath, std::string& outId, std::string& outError) {
    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    std::string zipPathUtf8 = WStringToUtf8(gupetFilePath);

    if (!mz_zip_reader_init_file(&zip, zipPathUtf8.c_str(), 0)) {
        outError = "Не удалось открыть архив скина";
        return false;
    }

    size_t manifestSize = 0;
    void* manifestData = mz_zip_reader_extract_file_to_heap(&zip, "manifest.json", &manifestSize, 0);
    if (!manifestData) {
        mz_zip_reader_end(&zip);
        outError = "В архиве отсутствует manifest.json";
        return false;
    }

    std::string json((char*)manifestData, manifestSize);
    mz_free(manifestData);

    GUPetSkinInfo info;
    std::map<std::string, std::string> frameFiles;
    if (!ParseManifest(json, info, frameFiles)) {
        mz_zip_reader_end(&zip);
        outError = "Ошибка в содержимом manifest.json";
        return false;
    }

    std::wstring destFolder = m_skinsDir + L"\\" + Utf8ToWString(info.id);
    CreateDirectoryW(destFolder.c_str(), NULL);

    std::wstring destManifest = destFolder + L"\\manifest.json";
    std::ofstream manifestOut(destManifest.c_str(), std::ios::binary);
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
                std::wstring outFilePath = destFolder + L"\\" + Utf8ToWString(filename);
                std::ofstream outFile(outFilePath.c_str(), std::ios::binary);
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

bool SkinStore::DeleteSkin(const std::string& id) {
    if (id.empty()) return false;
    m_cachedSkins.erase(id);

    std::wstring folder = m_skinsDir + L"\\" + Utf8ToWString(id);
    std::wstring searchPattern = folder + L"\\*";
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                std::wstring filePath = folder + L"\\" + findData.cFileName;
                DeleteFileW(filePath.c_str());
            }
        } while (FindNextFileW(hFind, &findData));
        FindClose(hFind);
    }
    return RemoveDirectoryW(folder.c_str()) != 0;
}

} // namespace GUPet
