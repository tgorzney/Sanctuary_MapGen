#include "UITabs.h"
#include "imgui.h"
#include "FileDialog.h"

extern std::vector<std::string> GetEnvironmentsFromSanpack(const std::string& zipPath);
extern std::vector<std::string> GetMaterialsFromSanpack(const std::string& zipPath, const std::string& envName);

#include "miniz.h"
#include "stb_image.h"
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

extern std::vector<std::string> GetEnvironmentsFromSanpack(const std::string& zipPath);
extern std::vector<std::string> GetMaterialsFromSanpack(const std::string& zipPath, const std::string& envName);

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
        
        if (w == 1 && h == 1 && outColor) {
            if (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) {
                uint16_t color0 = *(uint16_t*)(buffer + offset);
                outColor[0] = ((color0 >> 11) & 31) / 31.0f;
                outColor[1] = ((color0 >> 5) & 63) / 63.0f;
                outColor[2] = (color0 & 31) / 31.0f;
                outColor[3] = 1.0f;
            } else {
                outColor[0] = 0.5f; outColor[1] = 0.5f; outColor[2] = 0.5f; outColor[3] = 1.0f;
            }
        }
        
        offset += size;
        w /= 2;
        h /= 2;
    }
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipMapCount > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    return texID;
}

static unsigned int LoadTextureFromSanpack(const std::string& zipPath, const std::string& texturePath, float outAverageColor[4]) {
    if (zipPath.empty() || texturePath.empty()) return 0;
    
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));
    if (!mz_zip_reader_init_file(&zip_archive, zipPath.c_str(), 0)) return 0;
    
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
    
    if (targetIndex == -1) {
        mz_zip_reader_end(&zip_archive);
        return 0;
    }
    
    size_t uncomp_size;
    void* p = mz_zip_reader_extract_to_heap(&zip_archive, targetIndex, &uncomp_size, 0);
    mz_zip_reader_end(&zip_archive);
    if (!p) return 0;
    
    int w, h, channels;
    unsigned char* data = stbi_load_from_memory((const stbi_uc*)p, (int)uncomp_size, &w, &h, &channels, 4);
    
    if (!data) {
        // Fallback: Try loading it as a DDS file directly to the GPU!
        unsigned int ddsTex = LoadDDSFromMemory((const unsigned char*)p, uncomp_size, outAverageColor);
        mz_free(p);
        
        if (ddsTex == 0 && outAverageColor) {
            // Unrecognized format
            outAverageColor[0] = 1.0f;
            outAverageColor[1] = 0.0f;
            outAverageColor[2] = 1.0f;
            outAverageColor[3] = 1.0f;
        }
        return ddsTex;
    }
    
    mz_free(p);
    
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
            }
        }
        if (!params.GlobalEnvironmentPath.empty()) {
            ImGui::TextWrapped("Env: %s", params.GlobalEnvironmentPath.c_str());
        }
        
        ImGui::Separator();
        for (int i = 0; i < (int)params.Stratums.size(); ++i) {
            ImGui::PushID(i);
            char label[64]; snprintf(label, sizeof(label), "Stratum %d - %s", i, params.Stratums[i].Name.c_str());
            if (ImGui::CollapsingHeader(label)) {
                char nameBuf[128]; strncpy(nameBuf, params.Stratums[i].Name.c_str(), sizeof(nameBuf));
                if (ImGui::InputText("Name", nameBuf, IM_ARRAYSIZE(nameBuf))) params.Stratums[i].Name = nameBuf;
                
                ImGui::Separator();
                
                // Environment Auto-fill
                if (!params.GlobalEnvironmentPath.empty()) {
                    std::vector<std::string> envs = GetEnvironmentsFromSanpack(params.GlobalEnvironmentPath);
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
                            std::vector<std::string> mats = GetMaterialsFromSanpack(params.GlobalEnvironmentPath, selectedEnv);
                            if (ImGui::BeginCombo("Material", "Auto-fill...")) {
                                for (const auto& mat : mats) {
                                    if (ImGui::Selectable(mat.c_str())) {
                                        params.Stratums[i].AlbedoPath = selectedEnv + "/" + mat + "_Albedo.png";
                                        params.Stratums[i].NormalPath = selectedEnv + "/" + mat + "_Normal.png";
                                        params.Stratums[i].MaskPath = selectedEnv + "/" + mat + "_Mask.png";
                                        bNeedsPreviewRender = true;
                                        
                                        if (params.Stratums[i].PreviewAlbedoTex) glDeleteTextures(1, &params.Stratums[i].PreviewAlbedoTex);
                                        params.Stratums[i].PreviewAlbedoTex = LoadTextureFromSanpack(params.GlobalEnvironmentPath, params.Stratums[i].AlbedoPath, params.Stratums[i].PreviewColor);
                                        
                                        if (params.Stratums[i].PreviewNormalTex) glDeleteTextures(1, &params.Stratums[i].PreviewNormalTex);
                                        params.Stratums[i].PreviewNormalTex = LoadTextureFromSanpack(params.GlobalEnvironmentPath, params.Stratums[i].NormalPath, nullptr);
                                        
                                        if (params.Stratums[i].PreviewMaskTex) glDeleteTextures(1, &params.Stratums[i].PreviewMaskTex);
                                        params.Stratums[i].PreviewMaskTex = LoadTextureFromSanpack(params.GlobalEnvironmentPath, params.Stratums[i].MaskPath, nullptr);
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
                if (!params.Stratums[i].ImportedMaskData.empty() && params.Stratums[i].PreviewActualMaskTex == 0) {
                    int texSize = params.MapSize;
                    std::vector<uint8_t> thumbData(texSize * texSize);
                    for (size_t p = 0; p < thumbData.size(); ++p) {
                        thumbData[p] = static_cast<uint8_t>(std::clamp(params.Stratums[i].ImportedMaskData[p], 0.0f, 1.0f) * 255.0f);
                    }
                    
                    glGenTextures(1, &params.Stratums[i].PreviewActualMaskTex);
                    glBindTexture(GL_TEXTURE_2D, params.Stratums[i].PreviewActualMaskTex);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, texSize, texSize, 0, GL_RED, GL_UNSIGNED_BYTE, thumbData.data());
                    
                    // Tell OpenGL we want to sample RED into RGB (swizzle)
                    GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_ONE};
                    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
                }
                
                if (ImGui::BeginTable("##PathsTable", 2)) {
                    ImGui::TableSetupColumn("Inputs", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Thumbs", ImGuiTableColumnFlags_WidthFixed, 350.0f); // Width for 4 thumbnails
                    ImGui::TableNextRow();
                    
                    ImGui::TableSetColumnIndex(0);
                    char albedoBuf[256]; strncpy(albedoBuf, params.Stratums[i].AlbedoPath.c_str(), sizeof(albedoBuf));
                    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 100.0f);
                    if (ImGui::InputText("Albedo Path", albedoBuf, IM_ARRAYSIZE(albedoBuf))) { 
                        params.Stratums[i].AlbedoPath = albedoBuf; 
                        if (params.Stratums[i].AlbedoPath.empty()) {
                            params.Stratums[i].PreviewColor[0] = 1.0f;
                            params.Stratums[i].PreviewColor[1] = 1.0f;
                            params.Stratums[i].PreviewColor[2] = 1.0f;
                            params.Stratums[i].PreviewColor[3] = 1.0f;
                            if (params.Stratums[i].PreviewAlbedoTex) glDeleteTextures(1, &params.Stratums[i].PreviewAlbedoTex);
                            params.Stratums[i].PreviewAlbedoTex = 0;
                        } else {
                            if (params.Stratums[i].PreviewAlbedoTex) glDeleteTextures(1, &params.Stratums[i].PreviewAlbedoTex);
                            params.Stratums[i].PreviewAlbedoTex = LoadTextureFromSanpack(params.GlobalEnvironmentPath, params.Stratums[i].AlbedoPath, params.Stratums[i].PreviewColor);
                        }
                        bNeedsPreviewRender = true; 
                    }
                    
                    char normalBuf[256]; strncpy(normalBuf, params.Stratums[i].NormalPath.c_str(), sizeof(normalBuf));
                    if (ImGui::InputText("Normal Path", normalBuf, IM_ARRAYSIZE(normalBuf))) { 
                        params.Stratums[i].NormalPath = normalBuf; 
                        if (params.Stratums[i].PreviewNormalTex) glDeleteTextures(1, &params.Stratums[i].PreviewNormalTex);
                        if (!params.Stratums[i].NormalPath.empty()) {
                            params.Stratums[i].PreviewNormalTex = LoadTextureFromSanpack(params.GlobalEnvironmentPath, params.Stratums[i].NormalPath, nullptr);
                        } else params.Stratums[i].PreviewNormalTex = 0;
                        bNeedsPreviewRender = true; 
                    }
                    
                    char maskBuf[256]; strncpy(maskBuf, params.Stratums[i].MaskPath.c_str(), sizeof(maskBuf));
                    if (ImGui::InputText("Composite Path", maskBuf, IM_ARRAYSIZE(maskBuf))) { 
                        params.Stratums[i].MaskPath = maskBuf; 
                        if (params.Stratums[i].PreviewMaskTex) glDeleteTextures(1, &params.Stratums[i].PreviewMaskTex);
                        if (!params.Stratums[i].MaskPath.empty()) {
                            params.Stratums[i].PreviewMaskTex = LoadTextureFromSanpack(params.GlobalEnvironmentPath, params.Stratums[i].MaskPath, nullptr);
                        } else params.Stratums[i].PreviewMaskTex = 0;
                        bNeedsPreviewRender = true; 
                    }
                    ImGui::PopItemWidth();
                    
                    ImGui::TableSetColumnIndex(1);
                    float groupHeight = ImGui::GetFrameHeightWithSpacing() * 3 - ImGui::GetStyle().ItemSpacing.y;
                    if (groupHeight < 10) groupHeight = 80.0f;
                    ImVec2 texSize(80.0f, 80.0f);
                    
                    if (params.Stratums[i].PreviewAlbedoTex) ImGui::Image((void*)(intptr_t)params.Stratums[i].PreviewAlbedoTex, texSize);
                    else ImGui::Button("No Image\nFound##A", texSize);
                    ImGui::SameLine();
                    
                    if (params.Stratums[i].PreviewNormalTex) ImGui::Image((void*)(intptr_t)params.Stratums[i].PreviewNormalTex, texSize);
                    else ImGui::Button("No Image\nFound##N", texSize);
                    ImGui::SameLine();
                    
                    if (params.Stratums[i].PreviewMaskTex) ImGui::Image((void*)(intptr_t)params.Stratums[i].PreviewMaskTex, texSize);
                    else ImGui::Button("No Image\nFound##M", texSize);
                    ImGui::SameLine();
                    
                    bool pushedColor = params.Stratums[i].UseImportedMask;
                    if (pushedColor) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
                    }
                    if (params.Stratums[i].PreviewActualMaskTex) {
                        if (ImGui::ImageButton("##ToggleBtn", (ImTextureID)(intptr_t)params.Stratums[i].PreviewActualMaskTex, texSize)) {
                            params.Stratums[i].UseImportedMask = !params.Stratums[i].UseImportedMask;
                            bNeedsMapUpdate = true;
                        }
                    } else {
                        if (ImGui::Button(params.Stratums[i].UseImportedMask ? "Baked\nMask\n(ON)##B" : "Procedural\nMask\n(OFF)##B", texSize)) {
                            params.Stratums[i].UseImportedMask = !params.Stratums[i].UseImportedMask;
                            bNeedsMapUpdate = true;
                        }
                    }
                    if (pushedColor) {
                        ImGui::PopStyleColor();
                    }
                    
                    ImGui::EndTable();
                }
                
                ImGui::Separator();
                if (ImGui::ColorEdit4("Preview Base Color", params.Stratums[i].PreviewColor)) bNeedsPreviewRender = true;
                
                if (ImGui::ColorEdit4("Diffuse Remap", params.Stratums[i].DiffuseRemap)) bNeedsPreviewRender = true;
                if (ImGui::ColorEdit4("Far Color Remap", params.Stratums[i].FarColorRemap)) bNeedsPreviewRender = true;
                
                ImGui::Separator();
                if (ImGui::DragFloat4("Mask Remap Min", params.Stratums[i].MaskRemapMin, 0.01f, 0.0f, 10.0f)) bNeedsPreviewRender = true;
                if (ImGui::DragFloat4("Mask Remap Max", params.Stratums[i].MaskRemapMax, 0.01f, 0.0f, 10.0f)) bNeedsPreviewRender = true;
                
                ImGui::Separator();
                if (ImGui::DragFloat2("Tile Size", params.Stratums[i].TileSize, 0.1f, 0.1f, 1000.0f)) bNeedsPreviewRender = true;
                if (ImGui::DragFloat2("Tile Size Far", params.Stratums[i].TileSizeFar, 0.1f, 0.1f, 1000.0f)) bNeedsPreviewRender = true;
                if (ImGui::SliderFloat("Triplanar Tile", &params.Stratums[i].TileSizeTriplanar, 0.1f, 100.0f)) bNeedsPreviewRender = true;
                if (ImGui::SliderFloat("Far Triplanar Tile", &params.Stratums[i].TileSizeFarTriplanar, 0.1f, 100.0f)) bNeedsPreviewRender = true;
                
                ImGui::Separator();
                if (ImGui::SliderFloat("Normal Scale", &params.Stratums[i].NormalScale, 0.0f, 5.0f)) bNeedsPreviewRender = true;
                if (ImGui::SliderFloat("Normal Scale Far", &params.Stratums[i].NormalScaleFar, 0.0f, 5.0f)) bNeedsPreviewRender = true;
                if (ImGui::SliderFloat("Normal Far/Near Blend", &params.Stratums[i].NormalFarNearBlend, 0.0f, 1.0f)) bNeedsPreviewRender = true;
                if (ImGui::SliderFloat("Height Far/Near Blend", &params.Stratums[i].HeightFarNearBlend, 0.0f, 1.0f)) bNeedsPreviewRender = true;
                
                ImGui::Separator();
                ImGui::Text("Base Soil Physics");
                
                ImGui::PushID("SoilPresetsPopup");
                if (ImGui::Button("Presets \xEF\x83\x97")) {
                    ImGui::OpenPopup("SoilPresetsPopup");
                }
                if (ImGui::BeginPopup("SoilPresetsPopup")) {
                    if (ImGui::MenuItem("Bedrock")) { params.Stratums[i].Hardness = 1.0f; params.Stratums[i].Friction = 0.8f; params.Stratums[i].Cohesion = 1.0f; params.Stratums[i].CapacityMult = 0.1f; bNeedsMapUpdate = true; }
                    if (ImGui::MenuItem("Rock")) { params.Stratums[i].Hardness = 0.8f; params.Stratums[i].Friction = 0.7f; params.Stratums[i].Cohesion = 0.8f; params.Stratums[i].CapacityMult = 0.5f; bNeedsMapUpdate = true; }
                    if (ImGui::MenuItem("Clay")) { params.Stratums[i].Hardness = 0.5f; params.Stratums[i].Friction = 0.4f; params.Stratums[i].Cohesion = 0.9f; params.Stratums[i].CapacityMult = 1.0f; bNeedsMapUpdate = true; }
                    if (ImGui::MenuItem("Dirt")) { params.Stratums[i].Hardness = 0.4f; params.Stratums[i].Friction = 0.5f; params.Stratums[i].Cohesion = 0.5f; params.Stratums[i].CapacityMult = 1.5f; bNeedsMapUpdate = true; }
                    if (ImGui::MenuItem("Mud")) { params.Stratums[i].Hardness = 0.2f; params.Stratums[i].Friction = 0.2f; params.Stratums[i].Cohesion = 0.7f; params.Stratums[i].CapacityMult = 2.0f; bNeedsMapUpdate = true; }
                    if (ImGui::MenuItem("Sand")) { params.Stratums[i].Hardness = 0.1f; params.Stratums[i].Friction = 0.6f; params.Stratums[i].Cohesion = 0.1f; params.Stratums[i].CapacityMult = 2.5f; bNeedsMapUpdate = true; }
                    ImGui::EndPopup();
                }
                ImGui::PopID();
                
                if (ImGui::SliderFloat("Hardness", &params.Stratums[i].Hardness, 0.01f, 1.0f)) bNeedsMapUpdate = true;
                if (ImGui::SliderFloat("Friction", &params.Stratums[i].Friction, 0.01f, 1.0f)) bNeedsMapUpdate = true;
                if (ImGui::SliderFloat("Cohesion", &params.Stratums[i].Cohesion, 0.01f, 1.0f)) bNeedsMapUpdate = true;
                if (ImGui::SliderFloat("Capacity Mult", &params.Stratums[i].CapacityMult, 0.1f, 5.0f)) bNeedsMapUpdate = true;
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
        RenderLayerStack(params, params.DetailNormalLayers, nullptr, false, bNeedsPreviewRender);
    }

    void RenderSmoothnessTab(GenerationParams& params, bool& bNeedsMapUpdate, bool& bNeedsPreviewRender) {
        if (ImGui::Checkbox("##showSmoothness", &params.ShowSmoothness)) bNeedsPreviewRender = true; ImGui::SameLine();
        ImGui::Text("Smoothness Masking");
        ImGui::Separator();
        RenderLayerStack(params, params.SmoothnessLayers, nullptr, false, bNeedsPreviewRender);
    }

    void RenderTintTab(GenerationParams& params, bool& bNeedsMapUpdate, bool& bNeedsPreviewRender) {
        if (ImGui::Checkbox("##showTint", &params.ShowTint)) bNeedsPreviewRender = true; ImGui::SameLine();
        ImGui::Text("Procedural Tinting");
        ImGui::Separator();
        RenderLayerStack(params, params.TintLayers, nullptr, false, bNeedsPreviewRender);
    }

    void RenderHolesTab(GenerationParams& params, bool& bNeedsMapUpdate, bool& bNeedsPreviewRender) {
        if (ImGui::Checkbox("##showHoles", &params.ShowHoles)) bNeedsPreviewRender = true; ImGui::SameLine();
        ImGui::Text("Dyson Sphere Holes");
        ImGui::Separator();
        RenderLayerStack(params, params.HoleLayers, nullptr, false, bNeedsPreviewRender);
    }

} // namespace UI
} // namespace SanmapGen
