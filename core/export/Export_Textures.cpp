#include "Export_Textures.h"
#include "../stb_image_write.h"
#include <algorithm>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace SanmapGen {
namespace TextureExporter {

void ExportHeightmap(const std::string& filePath, const GenerationParams& params, const FloatMask& heightmap) {
    int dim = params.MapSize + 1;
    int hWidth = heightmap.GetWidth();
    std::vector<uint16_t> rawHeightmap(dim * dim);
    for (int y = 0; y < dim; ++y) {
        for (int x = 0; x < dim; ++x) {
            float val = 0.0f;
            if (x < hWidth && y < hWidth) val = heightmap.Get(x, y);
            val = std::clamp(val, 0.0f, 1.0f);
            rawHeightmap[y * dim + x] = static_cast<uint16_t>(val * 65535.0f);
        }
    }
    std::ofstream hmOut(filePath, std::ios::binary);
    if (hmOut) {
        hmOut.write(reinterpret_cast<const char*>(rawHeightmap.data()), rawHeightmap.size() * sizeof(uint16_t));
        hmOut.close();
    }
}

void ExportStratums(const std::string& folderPath, const GenerationParams& params, const GenerationResult& genData) {
    int texSize = params.MapSize;
    int pixelCount = texSize * texSize;
    std::vector<uint8_t> s1_4(pixelCount * 4, 0);
    std::vector<uint8_t> s5_8(pixelCount * 4, 0);

    for (int y = 0; y < texSize; ++y) {
        for (int x = 0; x < texSize; ++x) {
            int idx = (y * texSize + x) * 4;
            for (int i = 0; i < 4; ++i) {
                float val = (i < genData.MaterialMasks.size()) ? genData.MaterialMasks[i].Get(x, y) : 0.0f;
                s1_4[idx + i] = static_cast<uint8_t>(std::clamp(val, 0.0f, 1.0f) * 255.0f);
            }
            for (int i = 0; i < 4; ++i) {
                float val = ((i + 4) < genData.MaterialMasks.size()) ? genData.MaterialMasks[i + 4].Get(x, y) : 0.0f;
                s5_8[idx + i] = static_cast<uint8_t>(std::clamp(val, 0.0f, 1.0f) * 255.0f);
            }
        }
    }
    std::string p1 = folderPath + "/stratums_1_4.tga";
    std::string p2 = folderPath + "/stratums_5_8.tga";

    stbi_write_tga_with_rle = 0; // Disable RLE compression for Sanctuary editor compatibility
    stbi_write_tga(p1.c_str(), texSize, texSize, 4, s1_4.data());
    stbi_write_tga(p2.c_str(), texSize, texSize, 4, s5_8.data());
}

void ExportFlowMap(const std::string& filePath, const GenerationParams& params, const GenerationResult& genData) {
    int texSize = params.MapSize;
    std::vector<uint8_t> pixels(texSize * texSize * 4, 0);
    for (int y = 0; y < texSize; ++y) {
        for (int x = 0; x < texSize; ++x) {
            float val = genData.FlowMap.Get(x, y) * 100.0f; // Scale it a bit for visibility
            uint8_t intensity = static_cast<uint8_t>(std::clamp(val, 0.0f, 1.0f) * 255.0f);
            int idx = (y * texSize + x) * 4;
            pixels[idx] = intensity; // R
            pixels[idx+1] = intensity; // G
            pixels[idx+2] = intensity; // B
            pixels[idx+3] = 255;
        }
    }
    stbi_write_png(filePath.c_str(), texSize, texSize, 4, pixels.data(), texSize * 4);
}

void ExportSlopeMap(const std::string& filePath, const GenerationParams& params, const FloatMask& heightmap) {
    int texSize = params.MapSize;
    std::vector<uint8_t> pixels(texSize * texSize * 4, 0);
    float quadWidth = 1024.0f;
    float cellSize = static_cast<float>(params.MapSize) / quadWidth;
    if (cellSize < 1.0f) cellSize = 1.0f;

    for (int y = 0; y < texSize; ++y) {
        for (int x = 0; x < texSize; ++x) {
            float v00 = heightmap.Get(x, y);
            float v10 = heightmap.Get(std::min(x + 1, texSize - 1), y);
            float v01 = heightmap.Get(x, std::min(y + 1, texSize - 1));
            float v11 = heightmap.Get(std::min(x + 1, texSize - 1), std::min(y + 1, texSize - 1));

            float dx = (((v10 + v11) - (v00 + v01)) * 0.5f * 128.0f) / cellSize;
            float dy = (((v01 + v11) - (v00 + v10)) * 0.5f * 128.0f) / cellSize;
            float slopeDegrees = atan(sqrt(dx*dx + dy*dy)) * (180.0f / 3.14159265f);

            // Normalize slope to 0-90 degrees for export visualization
            float val = slopeDegrees / 90.0f; 
            uint8_t intensity = static_cast<uint8_t>(std::clamp(val, 0.0f, 1.0f) * 255.0f);
            
            int idx = (y * texSize + x) * 4;
            pixels[idx] = intensity;
            pixels[idx+1] = intensity;
            pixels[idx+2] = intensity;
            pixels[idx+3] = 255;
        }
    }
    stbi_write_png(filePath.c_str(), texSize, texSize, 4, pixels.data(), texSize * 4);
}

void ExportTints(const std::string& folderPath, const GenerationParams& params) {
    int texSize = params.MapSize;
    int pixelCount = texSize * texSize;

    // 4. Export tint_colors.tga (RGB = 128 for no tint, A = Smoothness 148 default)
    std::vector<uint8_t> tintColors(pixelCount * 4, 0);
    for (int i = 0; i < pixelCount * 4; i += 4) {
        tintColors[i + 0] = 128; // R
        tintColors[i + 1] = 128; // G
        tintColors[i + 2] = 128; // B
        tintColors[i + 3] = 148; // A
    }
    // TODO: Actually fill Tint/Smoothness based on layers if applicable in future
    std::string pColors = folderPath + "/tint_colors.tga";
    stbi_write_tga_with_rle = 0; // Disable RLE compression
    stbi_write_tga(pColors.c_str(), texSize, texSize, 4, tintColors.data());

    // 5. Export tint_geometry.tga (RG = Normals (128), B = Holes (255 for no hole), A = Padding (255))
    std::vector<uint8_t> tintGeom(pixelCount * 4, 0);
    for (int i = 0; i < pixelCount * 4; i += 4) {
        tintGeom[i + 0] = 128; // R
        tintGeom[i + 1] = 128; // G
        tintGeom[i + 2] = 255; // B
        tintGeom[i + 3] = 255; // A
    }
    // TODO: Calculate real normals or holes from data
    std::string pGeom = folderPath + "/tint_geometry.tga";
    stbi_write_tga_with_rle = 0; // Disable RLE compression
    stbi_write_tga(pGeom.c_str(), texSize, texSize, 4, tintGeom.data());
}

} // namespace TextureExporter
} // namespace SanmapGen
