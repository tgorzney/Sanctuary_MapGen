#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "Parameters.h"

// Forward declaration of OpenGL texture ID type
typedef unsigned int GLuint;

namespace SanmapGen {

class TextureLoader {
public:
    // Loads a DDS file directly from a zip/sanpack archive and uploads it to OpenGL.
    // Returns the OpenGL texture ID, or 0 if it failed.
    static GLuint LoadDDSFromArchive(const std::string& archivePath, const std::string& fileInsideArchive, std::string* debugOut = nullptr, void* openZipArchive = nullptr, int exactFileIndex = -1);
    
    // Loads a generic image file (png, jpg, tga) directly from a zip/sanpack archive and uploads it to OpenGL.
    // Useful if icons are ever provided as PNGs.
    static GLuint LoadImageFromArchive(const std::string& archivePath, const std::string& fileInsideArchive, std::string* debugOut = nullptr, void* openZipArchive = nullptr, int exactFileIndex = -1);
    
    // Loads an image file directly from the filesystem
    static GLuint LoadImageFromFile(const std::string& filePath);
    
    // Loads a DDS file directly from the filesystem
    static GLuint LoadDDSFromFile(const std::string& filePath, std::string* debugOut = nullptr);
    
    // Scans UI.sanpack for valid marker icons (ending in _icon.dds)
    static std::vector<std::string> ScanSanpackForMarkers(const std::string& archivePath, std::string* debugOut = nullptr, void* openZipArchive = nullptr);
    
    // Scans the .sanpack for environments (subfolders in the root)
    static std::vector<std::string> GetEnvironmentsFromSanpack(const std::string& zipPath);
    
    // Scans the .sanpack for materials within an environment
    static std::vector<std::string> GetMaterialsFromSanpack(const std::string& zipPath, const std::string& env);
    
    // Scans the .sanpack for a given material and sets the Albedo/Normal/Composite paths
    static void ScanSanpackForMaterial(const std::string& zipPath, const std::string& environmentTheme, const std::string& materialName, SanmapGen::StratumSettings& stratum);
    
    // Atlas Generation
    static void GenerateUnitAtlas(SanmapGen::GenerationParams& params, void* openZipArchive = nullptr);
};

struct AsyncTextureRequest {
    std::string cacheKey;
    std::string typeName;
};

struct AsyncTextureResult {
    std::string cacheKey;
    bool success;
    uint32_t width;
    uint32_t height;
    uint32_t format;
    uint32_t pixelFormat;
    bool bIsCompressed;
    std::vector<uint8_t> buffer;
};

class AsyncTextureManager {
public:
    static void Init(const std::string& gamedataPath);
    static void Shutdown();
    static void RequestUnitIcon(const std::string& typeName, const std::string& cacheKey);
    static void ProcessReadyQueue(SanmapGen::GenerationParams& params);
};

} // namespace SanmapGen
