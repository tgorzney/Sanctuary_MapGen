#include "UITabs.h"
#include "imgui.h"
#include "FileDialog.h"

extern std::vector<std::string> GetEnvironmentsFromSanpack(const std::string& zipPath);
extern std::vector<std::string> GetMaterialsFromSanpack(const std::string& zipPath, const std::string& envName);

#include "miniz.h"
#include "stb_image.h"

extern std::vector<std::string> GetEnvironmentsFromSanpack(const std::string& zipPath);
extern std::vector<std::string> GetMaterialsFromSanpack(const std::string& zipPath, const std::string& envName);

static void GetAverageAlbedoColorFromSanpack(const std::string& zipPath, const std::string& texturePath, float outColor[4]) {
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));
    if (!mz_zip_reader_init_file(&zip_archive, zipPath.c_str(), 0)) return;
    
    size_t uncomp_size;
    void* p = mz_zip_reader_extract_file_to_heap(&zip_archive, texturePath.c_str(), &uncomp_size, 0);
    if (p) {
        int w, h, channels;
        unsigned char* data = stbi_load_from_memory((const stbi_uc*)p, (int)uncomp_size, &w, &h, &channels, 4);
        if (data) {
            long long r = 0, g = 0, b = 0, a = 0;
            int samples = 0;
            // sample 10x10 grid
            for (int y = 0; y < h; y += std::max(1, h / 10)) {
                for (int x = 0; x < w; x += std::max(1, w / 10)) {
                    int idx = (y * w + x) * 4;
                    r += data[idx]; g += data[idx+1]; b += data[idx+2]; a += data[idx+3];
                    samples++;
                }
            }
            if (samples > 0) {
                outColor[0] = (r / samples) / 255.0f;
                outColor[1] = (g / samples) / 255.0f;
                outColor[2] = (b / samples) / 255.0f;
                outColor[3] = (a / samples) / 255.0f;
            }
            stbi_image_free(data);
        }
        mz_free(p);
    }
    mz_zip_reader_end(&zip_archive);
}

namespace SanmapGen {
namespace UI {

    void RenderStratumsTab(GenerationParams& params, bool& bNeedsMapUpdate) {
        ImGui::Checkbox("##showStratums", &params.ShowStratums); ImGui::SameLine();
        ImGui::Text("Material Strata Settings");
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
                        if (ImGui::BeginCombo("Environment", selectedEnv.empty() ? "Select..." : selectedEnv.c_str())) {
                            for (const auto& env : envs) {
                                if (ImGui::Selectable(env.c_str(), selectedEnv == env)) {
                                    selectedEnv = env;
                                }
                            }
                            ImGui::EndCombo();
                        }
                        
                        if (!selectedEnv.empty()) {
                            std::vector<std::string> mats = GetMaterialsFromSanpack(params.GlobalEnvironmentPath, selectedEnv);
                            if (ImGui::BeginCombo("Material", "Select to Auto-fill...")) {
                                for (const auto& mat : mats) {
                                    if (ImGui::Selectable(mat.c_str())) {
                                        params.Stratums[i].AlbedoPath = selectedEnv + "/" + mat + "_Albedo.png";
                                        params.Stratums[i].NormalPath = selectedEnv + "/" + mat + "_Normal.png";
                                        params.Stratums[i].MaskPath = selectedEnv + "/" + mat + "_Mask.png";
                                        bNeedsMapUpdate = true;
                                        
                                        GetAverageAlbedoColorFromSanpack(params.GlobalEnvironmentPath, params.Stratums[i].AlbedoPath, params.Stratums[i].PreviewColor);
                                    }
                                }
                                ImGui::EndCombo();
                            }
                        }
                    }
                }
                ImGui::Spacing();
                
                char albedoBuf[256]; strncpy(albedoBuf, params.Stratums[i].AlbedoPath.c_str(), sizeof(albedoBuf));
                if (ImGui::InputText("Albedo Path", albedoBuf, IM_ARRAYSIZE(albedoBuf))) { params.Stratums[i].AlbedoPath = albedoBuf; bNeedsMapUpdate = true; }
                
                char normalBuf[256]; strncpy(normalBuf, params.Stratums[i].NormalPath.c_str(), sizeof(normalBuf));
                if (ImGui::InputText("Normal Path", normalBuf, IM_ARRAYSIZE(normalBuf))) { params.Stratums[i].NormalPath = normalBuf; bNeedsMapUpdate = true; }
                
                char maskBuf[256]; strncpy(maskBuf, params.Stratums[i].MaskPath.c_str(), sizeof(maskBuf));
                if (ImGui::InputText("Mask Path", maskBuf, IM_ARRAYSIZE(maskBuf))) { params.Stratums[i].MaskPath = maskBuf; bNeedsMapUpdate = true; }
                
                ImGui::Separator();
                if (ImGui::ColorEdit4("Preview Base Color", params.Stratums[i].PreviewColor)) bNeedsMapUpdate = true;
                
                if (ImGui::ColorEdit4("Diffuse Remap", params.Stratums[i].DiffuseRemap)) bNeedsMapUpdate = true;
                if (ImGui::ColorEdit4("Far Color Remap", params.Stratums[i].FarColorRemap)) bNeedsMapUpdate = true;
                
                ImGui::Separator();
                if (ImGui::DragFloat4("Mask Remap Min", params.Stratums[i].MaskRemapMin, 0.01f, 0.0f, 10.0f)) bNeedsMapUpdate = true;
                if (ImGui::DragFloat4("Mask Remap Max", params.Stratums[i].MaskRemapMax, 0.01f, 0.0f, 10.0f)) bNeedsMapUpdate = true;
                
                ImGui::Separator();
                if (ImGui::DragFloat2("Tile Size", params.Stratums[i].TileSize, 0.1f, 0.1f, 1000.0f)) bNeedsMapUpdate = true;
                if (ImGui::DragFloat2("Tile Size Far", params.Stratums[i].TileSizeFar, 0.1f, 0.1f, 1000.0f)) bNeedsMapUpdate = true;
                if (ImGui::SliderFloat("Triplanar Tile", &params.Stratums[i].TileSizeTriplanar, 0.1f, 100.0f)) bNeedsMapUpdate = true;
                if (ImGui::SliderFloat("Far Triplanar Tile", &params.Stratums[i].TileSizeFarTriplanar, 0.1f, 100.0f)) bNeedsMapUpdate = true;
                
                ImGui::Separator();
                if (ImGui::SliderFloat("Normal Scale", &params.Stratums[i].NormalScale, 0.0f, 5.0f)) bNeedsMapUpdate = true;
                if (ImGui::SliderFloat("Normal Scale Far", &params.Stratums[i].NormalScaleFar, 0.0f, 5.0f)) bNeedsMapUpdate = true;
                if (ImGui::SliderFloat("Normal Far/Near Blend", &params.Stratums[i].NormalFarNearBlend, 0.0f, 1.0f)) bNeedsMapUpdate = true;
                if (ImGui::SliderFloat("Height Far/Near Blend", &params.Stratums[i].HeightFarNearBlend, 0.0f, 1.0f)) bNeedsMapUpdate = true;
                
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

    void RenderDetailNormalTab(GenerationParams& params, bool& bNeedsMapUpdate) {
        ImGui::Checkbox("##showDetailNormal", &params.ShowDetailNormal); ImGui::SameLine();
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
                    bNeedsMapUpdate = true;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::Spacing();
        RenderLayerStack(params, params.DetailNormalLayers, nullptr, false, bNeedsMapUpdate);
    }

    void RenderSmoothnessTab(GenerationParams& params, bool& bNeedsMapUpdate) {
        ImGui::Checkbox("##showSmoothness", &params.ShowSmoothness); ImGui::SameLine();
        ImGui::Text("Smoothness Masking");
        ImGui::Separator();
        RenderLayerStack(params, params.SmoothnessLayers, nullptr, false, bNeedsMapUpdate);
    }

    void RenderTintTab(GenerationParams& params, bool& bNeedsMapUpdate) {
        ImGui::Checkbox("##showTint", &params.ShowTint); ImGui::SameLine();
        ImGui::Text("Procedural Tinting");
        ImGui::Separator();
        RenderLayerStack(params, params.TintLayers, nullptr, false, bNeedsMapUpdate);
    }

    void RenderHolesTab(GenerationParams& params, bool& bNeedsMapUpdate) {
        ImGui::Checkbox("##showHoles", &params.ShowHoles); ImGui::SameLine();
        ImGui::Text("Dyson Sphere Holes");
        ImGui::Separator();
        RenderLayerStack(params, params.HoleLayers, nullptr, false, bNeedsMapUpdate);
    }

} // namespace UI
} // namespace SanmapGen
