#include "UITabs.h"
#include "widgets/Widget_LayerManager.h"
#include "imgui.h"
#include "FileDialog.h"
#include "../core/TextureLoader.h"
#include "miniz.h"
#include "stb_image.h"
#include <fstream>
#include <GLFW/glfw3.h> // For OpenGL texture generation
#include <stdint.h>

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif

#ifndef GL_TEXTURE_SWIZZLE_RGBA
#define GL_TEXTURE_SWIZZLE_RGBA 0x8E46
#endif

#ifndef GL_COMPRESSED_RGBA_BPTC_UNORM
#define GL_COMPRESSED_RGBA_BPTC_UNORM 0x8E8C
#endif

// Define function pointer for OpenGL extension
typedef void (APIENTRY * PFNGLCOMPRESSEDTEXIMAGE2DPROC)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data);


static unsigned int LoadDDSFromMemory(const unsigned char* buffer, size_t bufferSize, float outColor[4]) {
    if (bufferSize < 128) return 0;
    if (buffer[0] != 'D' || buffer[1] != 'D' || buffer[2] != 'S' || buffer[3] != ' ') return 0;
    
    uint32_t height = *(uint32_t*)&buffer[12];
    uint32_t width = *(uint32_t*)&buffer[16];
    uint32_t mipMapCount = *(uint32_t*)&buffer[28];
    uint32_t fourCC = *(uint32_t*)&buffer[84];
    
    unsigned int format = 0;
    unsigned int blockSize = 16;
    size_t offset = 128;
    
    if (fourCC == 0x31545844) { // DXT1
        format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        blockSize = 8;
    } else if (fourCC == 0x35545844) { // DXT5
        format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        blockSize = 16;
    } else if (fourCC == 0x30315844) { // DX10
        if (bufferSize < 148) return 0;
        uint32_t dxgiFormat = *(uint32_t*)&buffer[128];
        if (dxgiFormat == 98 || dxgiFormat == 99) { // BC7_UNORM or SRGB
            format = GL_COMPRESSED_RGBA_BPTC_UNORM;
            blockSize = 16;
        } else {
            return 0; // Unsupported DX10 format
        }
        offset = 148;
    } else {
        return 0; // Unsupported
    }
    
    unsigned int texID = 0;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    
    if (outColor) {
        int blocksX = (width + 3) / 4;
        int blocksY = (height + 3) / 4;
        int totalBlocks = blocksX * blocksY;
        
        if (totalBlocks > 0 && (fourCC == 0x31545844 || fourCC == 0x35545844)) {
            long long totalR = 0, totalG = 0, totalB = 0;
            int sampleCount = 0;
            int step = std::max(1, totalBlocks / 1000); // Max 1000 samples for performance
            
            for (int i = 0; i < totalBlocks; i += step) {
                size_t blockOffset = offset + i * blockSize;
                if (blockOffset + blockSize > bufferSize) break;
                
                const uint16_t* colorBlock = (const uint16_t*)(buffer + blockOffset + (fourCC == 0x35545844 ? 8 : 0));
                uint16_t c0 = colorBlock[0];
                uint16_t c1 = colorBlock[1];
                
                int r0 = ((c0 >> 11) & 0x1F) * 255 / 31;
                int g0 = ((c0 >> 5) & 0x3F) * 255 / 63;
                int b0 = (c0 & 0x1F) * 255 / 31;
                
                int r1 = ((c1 >> 11) & 0x1F) * 255 / 31;
                int g1 = ((c1 >> 5) & 0x3F) * 255 / 63;
                int b1 = (c1 & 0x1F) * 255 / 31;
                
                totalR += (r0 + r1) / 2;
                totalG += (g0 + g1) / 2;
                totalB += (b0 + b1) / 2;
                sampleCount++;
            }
            
            if (sampleCount > 0) {
                outColor[0] = (totalR / sampleCount) / 255.0f;
                outColor[1] = (totalG / sampleCount) / 255.0f;
                outColor[2] = (totalB / sampleCount) / 255.0f;
                outColor[3] = 1.0f;
            } else {
                outColor[0] = outColor[1] = outColor[2] = 0.5f;
                outColor[3] = 1.0f;
            }
        } else {
            outColor[0] = outColor[1] = outColor[2] = 0.5f;
            outColor[3] = 1.0f;
        }
    }
    
    unsigned int w = width;
    unsigned int h = height;
    if (mipMapCount == 0) mipMapCount = 1;
    
    for (unsigned int level = 0; level < mipMapCount && (w || h); ++level) {
        if (w == 0) w = 1;
        if (h == 0) h = 1;
        
        unsigned int size = ((w + 3) / 4) * ((h + 3) / 4) * blockSize;
        if (offset + size > bufferSize) break;
        
        static PFNGLCOMPRESSEDTEXIMAGE2DPROC glCompressedTexImage2D_ptr = nullptr;
        if (!glCompressedTexImage2D_ptr) {
            glCompressedTexImage2D_ptr = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)glfwGetProcAddress("glCompressedTexImage2D");
        }
        
        if (glCompressedTexImage2D_ptr) {
            glCompressedTexImage2D_ptr(GL_TEXTURE_2D, level, format, w, h, 0, size, buffer + offset);
        }
        
        offset += size;
        w /= 2;
        h /= 2;
    }
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipMapCount > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    return texID;
}

static unsigned int LoadTextureFromSanpack(const std::string& zipPath, const std::string& texturePath, const std::string& mapFolderPath, float outAverageColor[4]) {
    if (texturePath.empty()) return 0;
    
    std::vector<uint8_t> fileData;
    bool bFoundFile = false;
    
    if (!zipPath.empty()) {
        mz_zip_archive zip_archive;
        memset(&zip_archive, 0, sizeof(zip_archive));
        if (mz_zip_reader_init_file(&zip_archive, zipPath.c_str(), 0)) {
            std::string targetPath = texturePath;
            size_t lastSlash = targetPath.find_last_of("/\\");
            if (lastSlash != std::string::npos) targetPath = targetPath.substr(lastSlash + 1);
            size_t lastDot = targetPath.find_last_of('.');
            if (lastDot != std::string::npos) targetPath = targetPath.substr(0, lastDot);
            std::transform(targetPath.begin(), targetPath.end(), targetPath.begin(), ::tolower);
            
            int numFiles = (int)mz_zip_reader_get_num_files(&zip_archive);
            int targetIndex = -1;
            for (int i = 0; i < numFiles; ++i) {
                mz_zip_archive_file_stat file_stat;
                if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) continue;
                std::string fname = file_stat.m_filename;
                std::string fnameLower = fname;
                std::transform(fnameLower.begin(), fnameLower.end(), fnameLower.begin(), ::tolower);
                if (fnameLower.find(targetPath) != std::string::npos) {
                    targetIndex = i;
                    break;
                }
            }
            
            if (targetIndex != -1) {
                size_t uncomp_size;
                void* p = mz_zip_reader_extract_to_heap(&zip_archive, targetIndex, &uncomp_size, 0);
                if (p) {
                    fileData.resize(uncomp_size);
                    memcpy(fileData.data(), p, uncomp_size);
                    mz_free(p);
                    bFoundFile = true;
                }
            }
            mz_zip_reader_end(&zip_archive);
        }
    }
    
    // Fallback to MapFolderPath if not found in zip
    if (!bFoundFile && !mapFolderPath.empty()) {
        std::string fallbackPath = mapFolderPath + "/" + texturePath;
        std::ifstream file(fallbackPath, std::ios::binary | std::ios::ate);
        if (file) {
            size_t uncomp_size = file.tellg();
            file.seekg(0, std::ios::beg);
            fileData.resize(uncomp_size);
            if (file.read(reinterpret_cast<char*>(fileData.data()), uncomp_size)) {
                bFoundFile = true;
            }
        }
    }
    
    if (!bFoundFile || fileData.empty()) return 0;
    
    int w, h, channels;
    unsigned char* data = stbi_load_from_memory((const stbi_uc*)fileData.data(), (int)fileData.size(), &w, &h, &channels, 4);
    
    if (!data) {
        // Fallback: Try loading it as a DDS file directly to the GPU!
        unsigned int ddsTex = LoadDDSFromMemory(fileData.data(), fileData.size(), outAverageColor);
        
        if (ddsTex == 0 && outAverageColor) {
            // Unrecognized format
            outAverageColor[0] = 1.0f;
            outAverageColor[1] = 0.0f;
            outAverageColor[2] = 1.0f;
            outAverageColor[3] = 1.0f;
        }
        return ddsTex;
    }
    
    if (data) {
        if (outAverageColor) {
            long long r = 0, g = 0, b = 0, a = 0;
            int samples = 0;
            for (int y = 0; y < h; y += std::max(1, h / 10)) {
                for (int x = 0; x < w; x += std::max(1, w / 10)) {
                    int idx = (y * w + x) * 4;
                    r += data[idx]; g += data[idx+1]; b += data[idx+2]; a += data[idx+3];
                    samples++;
                }
            }
            if (samples > 0) {
                outAverageColor[0] = (r / samples) / 255.0f;
                outAverageColor[1] = (g / samples) / 255.0f;
                outAverageColor[2] = (b / samples) / 255.0f;
                outAverageColor[3] = (a / samples) / 255.0f;
            }
        }
        
        unsigned int texID;
        glGenTextures(1, &texID);
        glBindTexture(GL_TEXTURE_2D, texID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
        return texID;
    }
    return 0;
}



namespace SanmapGen {
namespace UI {

    void RenderStratumsTab(GenerationParams& params, bool& bNeedsMapUpdate, bool& bNeedsPreviewRender) {
        ImGui::Text("Material Strata Settings");
        if (ImGui::Checkbox("Show Stratums Overlay", &params.ShowStratums)) bNeedsPreviewRender = true;
        ImGui::Separator();
        
        if (ImGui::Button("Select Environment .sanpack")) {
            std::string path;
            if (FileDialog::OpenFile("Sanctuary Packs\0*.sanpack;*.zip\0All Files\0*.*\0", path)) {
                params.GlobalEnvironmentPath = path;
                ReloadStratumTextures(params);
                bNeedsPreviewRender = true;
            }
        }
        if (!params.GlobalEnvironmentPath.empty()) {
            ImGui::TextWrapped("Env: %s", params.GlobalEnvironmentPath.c_str());
        }
        
        ImGui::Separator();
        for (int i = 0; i < (int)params.Stratums.size(); ++i) {
            ImGui::PushID(i);
            char label[64]; snprintf(label, sizeof(label), "Stratum %d - %s", i, params.Stratums[i].name.c_str());
            if (ImGui::CollapsingHeader(label)) {
                char nameBuf[128]; strncpy(nameBuf, params.Stratums[i].name.c_str(), sizeof(nameBuf));
                if (ImGui::InputText("Name", nameBuf, IM_ARRAYSIZE(nameBuf))) params.Stratums[i].name = nameBuf;
                
                ImGui::Separator();
                
                // Environment Auto-fill
                if (!params.GlobalEnvironmentPath.empty()) {
                    std::vector<std::string> envs = SanmapGen::TextureLoader::GetEnvironmentsFromSanpack(params.GlobalEnvironmentPath);
                    if (!envs.empty()) {
                        static std::string selectedEnv = "";
                        ImGui::PushItemWidth(150.0f);
                        if (ImGui::BeginCombo("Environment", selectedEnv.empty() ? "Select..." : selectedEnv.c_str())) {
                            for (const auto& env : envs) {
                                if (ImGui::Selectable(env.c_str(), selectedEnv == env)) {
                                    selectedEnv = env;
                                }
                            }
                            ImGui::EndCombo();
                        }
                        
                        if (!selectedEnv.empty()) {
                            ImGui::SameLine();
                            std::vector<std::string> mats = SanmapGen::TextureLoader::GetMaterialsFromSanpack(params.GlobalEnvironmentPath, selectedEnv);
                            if (ImGui::BeginCombo("Material", "Auto-fill...")) {
                                for (const auto& mat : mats) {
                                    if (ImGui::Selectable(mat.c_str())) {
                                        params.Stratums[i].albedo.path = selectedEnv + "/" + mat + "_Albedo.png";
                                        params.Stratums[i].normal.path = selectedEnv + "/" + mat + "_Normal.png";
                                        params.Stratums[i].mask.path = selectedEnv + "/" + mat + "_Mask.png";
                                        bNeedsPreviewRender = true;
                                        
                                        if (params.Stratums[i].previewAlbedoTex) glDeleteTextures(1, &params.Stratums[i].previewAlbedoTex);
                                        params.Stratums[i].previewAlbedoTex = LoadTextureFromSanpack(params.GlobalEnvironmentPath, params.Stratums[i].albedo.path, params.MapFolderPath, &params.Stratums[i].previewColor.r);
                                        
                                        if (params.Stratums[i].previewNormalTex) glDeleteTextures(1, &params.Stratums[i].previewNormalTex);
                                        params.Stratums[i].previewNormalTex = LoadTextureFromSanpack(params.GlobalEnvironmentPath, params.Stratums[i].normal.path, params.MapFolderPath, nullptr);
                                        
                                        if (params.Stratums[i].previewMaskTex) glDeleteTextures(1, &params.Stratums[i].previewMaskTex);
                                        params.Stratums[i].previewMaskTex = LoadTextureFromSanpack(params.GlobalEnvironmentPath, params.Stratums[i].mask.path, params.MapFolderPath, nullptr);
                                    }
                                }
                                ImGui::EndCombo();
                            }
                        }
                        ImGui::PopItemWidth();
                    }
                }
                ImGui::Spacing();
                
                // Generate Actual Mask Thumbnail if it exists but hasn't been uploaded to GPU yet
                if (!params.Stratums[i].importedMaskData.empty() && params.Stratums[i].previewActualMaskTex == 0) {
                    int maskSize = static_cast<int>(std::sqrt(params.Stratums[i].importedMaskData.size()));
                    std::vector<uint8_t> thumbData(maskSize * maskSize);
                    for (size_t p = 0; p < thumbData.size(); ++p) {
                        thumbData[p] = static_cast<uint8_t>(std::clamp(params.Stratums[i].importedMaskData[p], 0.0f, 1.0f) * 255.0f);
                    }
                    
                    glGenTextures(1, &params.Stratums[i].previewActualMaskTex);
                    glBindTexture(GL_TEXTURE_2D, params.Stratums[i].previewActualMaskTex);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    
                    // Fix OpenGL row padding skewing for MapSizes that aren't multiples of 4
                    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, maskSize, maskSize, 0, GL_RED, GL_UNSIGNED_BYTE, thumbData.data());
                    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
                    
                    // Tell OpenGL we want to sample RED into RGB (swizzle)
                    GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_ONE};
                    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
                }
                
                if (ImGui::BeginTable("##PathsTable", 2)) {
                    ImGui::TableSetupColumn("Inputs", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Thumbs", ImGuiTableColumnFlags_WidthFixed, 350.0f); // Width for 4 thumbnails
                    ImGui::TableNextRow();
                    
                    ImGui::TableSetColumnIndex(0);
                    auto getShortPath = [](const std::string& path) {
                        if (path.empty()) return std::string("None");
                        size_t lastSlash = path.find_last_of("/\\");
                        if (lastSlash != std::string::npos && lastSlash > 0) {
                            size_t prevSlash = path.find_last_of("/\\", lastSlash - 1);
                            if (prevSlash != std::string::npos) {
                                return path.substr(prevSlash + 1);
                            }
                        }
                        return path;
                    };

                    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 120.0f);
                    
                    if (ImGui::Button(("Albedo##A" + std::to_string(i)).c_str(), ImVec2(80, 0))) {
                        std::string path = params.Stratums[i].albedo.path;
                        if (FileDialog::OpenFile("Images\0*.png;*.dds;*.tga\0All Files\0*.*\0", path)) {
                            params.Stratums[i].albedo.path = path;
                            if (params.Stratums[i].previewAlbedoTex) glDeleteTextures(1, &params.Stratums[i].previewAlbedoTex);
                            params.Stratums[i].previewAlbedoTex = LoadTextureFromSanpack(params.GlobalEnvironmentPath, params.Stratums[i].albedo.path, params.MapFolderPath, &params.Stratums[i].previewColor.r);
                            bNeedsPreviewRender = true;
                        }
                    }
                    ImGui::SameLine();
                    char albedoBuf[256]; strncpy(albedoBuf, getShortPath(params.Stratums[i].albedo.path).c_str(), sizeof(albedoBuf));
                    if (ImGui::InputText("##AlbedoPath", albedoBuf, IM_ARRAYSIZE(albedoBuf), ImGuiInputTextFlags_ReadOnly)) {}
                    if (params.Stratums[i].albedo.path.empty()) {
                        params.Stratums[i].previewColor.r = 1.0f;
                        params.Stratums[i].previewColor.g = 1.0f;
                        params.Stratums[i].previewColor.b = 1.0f;
                        params.Stratums[i].previewColor.a = 1.0f;
                        if (params.Stratums[i].previewAlbedoTex) glDeleteTextures(1, &params.Stratums[i].previewAlbedoTex);
                        params.Stratums[i].previewAlbedoTex = 0;
                    }
                    
                    if (ImGui::Button(("Normal##N" + std::to_string(i)).c_str(), ImVec2(80, 0))) {
                        std::string path = params.Stratums[i].normal.path;
                        if (FileDialog::OpenFile("Images\0*.png;*.dds;*.tga\0All Files\0*.*\0", path)) {
                            params.Stratums[i].normal.path = path;
                            if (params.Stratums[i].previewNormalTex) glDeleteTextures(1, &params.Stratums[i].previewNormalTex);
                            params.Stratums[i].previewNormalTex = LoadTextureFromSanpack(params.GlobalEnvironmentPath, params.Stratums[i].normal.path, params.MapFolderPath, nullptr);
                            bNeedsPreviewRender = true;
                        }
                    }
                    ImGui::SameLine();
                    char normalBuf[256]; strncpy(normalBuf, getShortPath(params.Stratums[i].normal.path).c_str(), sizeof(normalBuf));
                    if (ImGui::InputText("##NormalPath", normalBuf, IM_ARRAYSIZE(normalBuf), ImGuiInputTextFlags_ReadOnly)) {}
                    
                    if (ImGui::Button(("Composite##C" + std::to_string(i)).c_str(), ImVec2(80, 0))) {
                        std::string path = params.Stratums[i].mask.path;
                        if (FileDialog::OpenFile("Images\0*.png;*.dds;*.tga\0All Files\0*.*\0", path)) {
                            params.Stratums[i].mask.path = path;
                            if (params.Stratums[i].previewMaskTex) glDeleteTextures(1, &params.Stratums[i].previewMaskTex);
                            params.Stratums[i].previewMaskTex = LoadTextureFromSanpack(params.GlobalEnvironmentPath, params.Stratums[i].mask.path, params.MapFolderPath, nullptr);
                            bNeedsPreviewRender = true;
                        }
                    }
                    ImGui::SameLine();
                    char maskBuf[256]; strncpy(maskBuf, getShortPath(params.Stratums[i].mask.path).c_str(), sizeof(maskBuf));
                    if (ImGui::InputText("##MaskPath", maskBuf, IM_ARRAYSIZE(maskBuf), ImGuiInputTextFlags_ReadOnly)) {}
                    
                    ImGui::PopItemWidth();
                    
                    ImGui::TableSetColumnIndex(1);
                    float groupHeight = ImGui::GetFrameHeightWithSpacing() * 3 - ImGui::GetStyle().ItemSpacing.y;
                    if (groupHeight < 10) groupHeight = 80.0f;
                    ImVec2 texSize(80.0f, 80.0f);
                    
                    if (params.Stratums[i].previewAlbedoTex) ImGui::Image((void*)(intptr_t)params.Stratums[i].previewAlbedoTex, texSize);
                    else ImGui::Button("No Image\nFound##A", texSize);
                    ImGui::SameLine();
                    
                    if (params.Stratums[i].previewNormalTex) ImGui::Image((void*)(intptr_t)params.Stratums[i].previewNormalTex, texSize);
                    else ImGui::Button("No Image\nFound##N", texSize);
                    ImGui::SameLine();
                    
                    if (params.Stratums[i].previewMaskTex) ImGui::Image((void*)(intptr_t)params.Stratums[i].previewMaskTex, texSize);
                    else ImGui::Button("No Image\nFound##M", texSize);
                    ImGui::SameLine();
                    
                    bool pushedColor = (params.Stratums[i].maskMode != ImportedMaskMode::Disabled);
                    if (pushedColor) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
                    }
                    
                    auto toggleMode = [&]() {
                        if (params.Stratums[i].maskMode == ImportedMaskMode::Disabled) params.Stratums[i].maskMode = ImportedMaskMode::ProceduralStart;
                        else if (params.Stratums[i].maskMode == ImportedMaskMode::ProceduralStart) params.Stratums[i].maskMode = ImportedMaskMode::StaticOverride;
                        else params.Stratums[i].maskMode = ImportedMaskMode::Disabled;
                        bNeedsMapUpdate = true;
                    };
                    
                    if (params.Stratums[i].previewActualMaskTex) {
                        if (ImGui::ImageButton("##ToggleBtn", (ImTextureID)(intptr_t)params.Stratums[i].previewActualMaskTex, texSize)) {
                            toggleMode();
                        }
                    } else {
                        const char* btnText = "Procedural\nMask\n(OFF)##B";
                        if (params.Stratums[i].maskMode == ImportedMaskMode::StaticOverride) btnText = "Static\nOverride\n(ON)##B";
                        else if (params.Stratums[i].maskMode == ImportedMaskMode::ProceduralStart) btnText = "Procedural\nStart\n(ON)##B";
                        
                        if (ImGui::Button(btnText, texSize)) {
                            toggleMode();
                        }
                    }
                    if (pushedColor) {
                        ImGui::PopStyleColor();
                    }
                    
                    ImGui::EndTable();
                }
                
                ImGui::Separator();
                if (ImGui::ColorEdit4("Preview Base Color", &params.Stratums[i].previewColor.r)) bNeedsPreviewRender = true;
                
                if (ImGui::ColorEdit4("Diffuse Remap", &params.Stratums[i].diffuseRemap.r)) bNeedsPreviewRender = true;
                if (ImGui::ColorEdit4("Far Color Remap", &params.Stratums[i].farColorRemap.r)) bNeedsPreviewRender = true;
                
                ImGui::Separator();
                if (ImGui::DragFloat4("Mask Remap Min", &params.Stratums[i].maskRemapMin.x, 0.01f, 0.0f, 10.0f)) bNeedsPreviewRender = true;
                if (ImGui::DragFloat4("Mask Remap Max", &params.Stratums[i].maskRemapMax.x, 0.01f, 0.0f, 10.0f)) bNeedsPreviewRender = true;
                
                ImGui::Separator();
                if (ImGui::DragFloat2("Tile Size", &params.Stratums[i].tileSize.x, 0.1f, 0.1f, 1000.0f)) bNeedsPreviewRender = true;
                if (ImGui::DragFloat2("Tile Size Far", &params.Stratums[i].tileSizeFar.x, 0.1f, 0.1f, 1000.0f)) bNeedsPreviewRender = true;
                if (ImGui::SliderFloat("Triplanar Tile", &params.Stratums[i].tileSizeTriplanar, 0.1f, 100.0f)) bNeedsPreviewRender = true;
                if (ImGui::SliderFloat("Far Triplanar Tile", &params.Stratums[i].tileSizeFarTriplanar, 0.1f, 100.0f)) bNeedsPreviewRender = true;
                
                ImGui::Separator();
                if (ImGui::SliderFloat("Normal Scale", &params.Stratums[i].normalScale, 0.0f, 5.0f)) bNeedsPreviewRender = true;
                if (ImGui::SliderFloat("Normal Scale Far", &params.Stratums[i].normalScaleFar, 0.0f, 5.0f)) bNeedsPreviewRender = true;
                if (ImGui::SliderFloat("Normal Far/Near Blend", &params.Stratums[i].normalFarNearBlend, 0.0f, 1.0f)) bNeedsPreviewRender = true;
                if (ImGui::SliderFloat("Height Far/Near Blend", &params.Stratums[i].heightFarNearBlend, 0.0f, 1.0f)) bNeedsPreviewRender = true;
                
                ImGui::Separator();
                ImGui::Text("Base Soil Physics");
                
                ImGui::PushID("SoilPresetsPopup");
                if (ImGui::Button("Presets \xEF\x83\x97")) {
                    ImGui::OpenPopup("SoilPresetsPopup");
                }
                if (ImGui::BeginPopup("SoilPresetsPopup")) {
                    if (ImGui::MenuItem("Bedrock")) { params.Stratums[i].hardness = 1.0f; params.Stratums[i].friction = 0.8f; params.Stratums[i].cohesion = 1.0f; params.Stratums[i].capacityMult = 0.1f; bNeedsMapUpdate = true; }
                    if (ImGui::MenuItem("Rock")) { params.Stratums[i].hardness = 0.8f; params.Stratums[i].friction = 0.7f; params.Stratums[i].cohesion = 0.8f; params.Stratums[i].capacityMult = 0.5f; bNeedsMapUpdate = true; }
                    if (ImGui::MenuItem("Clay")) { params.Stratums[i].hardness = 0.5f; params.Stratums[i].friction = 0.4f; params.Stratums[i].cohesion = 0.9f; params.Stratums[i].capacityMult = 1.0f; bNeedsMapUpdate = true; }
                    if (ImGui::MenuItem("Dirt")) { params.Stratums[i].hardness = 0.4f; params.Stratums[i].friction = 0.5f; params.Stratums[i].cohesion = 0.5f; params.Stratums[i].capacityMult = 1.5f; bNeedsMapUpdate = true; }
                    if (ImGui::MenuItem("Mud")) { params.Stratums[i].hardness = 0.2f; params.Stratums[i].friction = 0.2f; params.Stratums[i].cohesion = 0.7f; params.Stratums[i].capacityMult = 2.0f; bNeedsMapUpdate = true; }
                    if (ImGui::MenuItem("Sand")) { params.Stratums[i].hardness = 0.1f; params.Stratums[i].friction = 0.6f; params.Stratums[i].cohesion = 0.1f; params.Stratums[i].capacityMult = 2.5f; bNeedsMapUpdate = true; }
                    ImGui::EndPopup();
                }
                ImGui::PopID();
                
                if (ImGui::SliderFloat("Hardness", &params.Stratums[i].hardness, 0.01f, 1.0f)) bNeedsMapUpdate = true;
                if (ImGui::SliderFloat("Friction", &params.Stratums[i].friction, 0.01f, 1.0f)) bNeedsMapUpdate = true;
                if (ImGui::SliderFloat("Cohesion", &params.Stratums[i].cohesion, 0.01f, 1.0f)) bNeedsMapUpdate = true;
                if (ImGui::SliderFloat("Capacity Mult", &params.Stratums[i].capacityMult, 0.1f, 5.0f)) bNeedsMapUpdate = true;
            }
            ImGui::PopID();
        }
    }

    void RenderDetailNormalTab(GenerationParams& params, bool& bNeedsMapUpdate, bool& bNeedsPreviewRender) {
        if (ImGui::Checkbox("##showDetailNormal", &params.ShowDetailNormal)) bNeedsPreviewRender = true; ImGui::SameLine();
        ImGui::Text("Detail Normal Settings");
        ImGui::Separator();
        
        const int sizes[] = { 256, 512, 1024, 2048, 4096 };
        const char* size_labels[] = { "256", "512", "1024", "2048", "4096" };
        int size_idx = 2; // Default 1024
        for (int i = 0; i < IM_ARRAYSIZE(sizes); ++i) {
            if (sizes[i] == params.DetailNormalMapSize) size_idx = i;
        }
        if (ImGui::BeginCombo("Detail Normal Size", size_labels[size_idx])) {
            for (int n = 0; n < IM_ARRAYSIZE(sizes); n++) {
                if (ImGui::Selectable(size_labels[n], size_idx == n)) {
                    params.DetailNormalMapSize = sizes[n];
                    bNeedsPreviewRender = true;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::Spacing();
        Widget_LayerManager::RenderLayerStack(params, params.DetailNormalLayers, nullptr, false, bNeedsPreviewRender);
    }

    void RenderSmoothnessTab(GenerationParams& params, bool& bNeedsMapUpdate, bool& bNeedsPreviewRender) {
        if (ImGui::Checkbox("##showSmoothness", &params.ShowSmoothness)) bNeedsPreviewRender = true; ImGui::SameLine();
        ImGui::Text("Smoothness Masking");
        ImGui::Separator();
        Widget_LayerManager::RenderLayerStack(params, params.SmoothnessLayers, nullptr, false, bNeedsPreviewRender);
    }

    void RenderTintTab(GenerationParams& params, bool& bNeedsMapUpdate, bool& bNeedsPreviewRender) {
        if (ImGui::Checkbox("##showTint", &params.ShowTint)) bNeedsPreviewRender = true; ImGui::SameLine();
        ImGui::Text("Procedural Tinting");
        ImGui::Separator();
        Widget_LayerManager::RenderLayerStack(params, params.TintLayers, nullptr, false, bNeedsPreviewRender);
    }

    void RenderHolesTab(GenerationParams& params, bool& bNeedsMapUpdate, bool& bNeedsPreviewRender) {
        if (ImGui::Checkbox("##showHoles", &params.ShowHoles)) bNeedsPreviewRender = true; ImGui::SameLine();
        ImGui::Text("Dyson Sphere Holes");
        ImGui::Separator();
        Widget_LayerManager::RenderLayerStack(params, params.HoleLayers, nullptr, false, bNeedsPreviewRender);
    }

    void ReloadStratumTextures(GenerationParams& params) {
        for (size_t i = 0; i < params.Stratums.size(); ++i) {
            float avg[4] = {1.0f, 1.0f, 1.0f, 1.0f};
            
            if (params.Stratums[i].previewAlbedoTex) glDeleteTextures(1, &params.Stratums[i].previewAlbedoTex);
            params.Stratums[i].previewAlbedoTex = 0;
            if (!params.Stratums[i].albedo.path.empty()) {
                params.Stratums[i].previewAlbedoTex = LoadTextureFromSanpack(params.GlobalEnvironmentPath, params.Stratums[i].albedo.path, params.MapFolderPath, avg);
                params.Stratums[i].previewColor[0] = avg[0];
                params.Stratums[i].previewColor[1] = avg[1];
                params.Stratums[i].previewColor[2] = avg[2];
            }
            
            if (params.Stratums[i].previewNormalTex) glDeleteTextures(1, &params.Stratums[i].previewNormalTex);
            params.Stratums[i].previewNormalTex = 0;
            if (!params.Stratums[i].normal.path.empty()) {
                params.Stratums[i].previewNormalTex = LoadTextureFromSanpack(params.GlobalEnvironmentPath, params.Stratums[i].normal.path, params.MapFolderPath, nullptr);
            }
            
            if (params.Stratums[i].previewMaskTex) glDeleteTextures(1, &params.Stratums[i].previewMaskTex);
            params.Stratums[i].previewMaskTex = 0;
            if (!params.Stratums[i].mask.path.empty()) {
                params.Stratums[i].previewMaskTex = LoadTextureFromSanpack(params.GlobalEnvironmentPath, params.Stratums[i].mask.path, params.MapFolderPath, nullptr);
            }
        }
    }

} // namespace UI
} // namespace SanmapGen
