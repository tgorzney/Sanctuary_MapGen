#include "UITabs.h"
#include "imgui.h"
#include "FileDialog.h"
#include "export/Export_Metadata.h"
#include "MapImporter.h"
#include "SupComImporter.h"
#include "UIHelpers.h"
#include "export/Export_Textures.h"
#include <filesystem>
#include <GLFW/glfw3.h>

namespace SanmapGen {
namespace UI {

    void RenderPerformanceTab(GenerationParams& params, bool& bNeedsMapUpdate) {
        ImGui::Text("Hardware Acceleration Options");
        ImGui::Separator();
        if (ImGui::Checkbox("Use GPU Terrain Generation", &params.UseGPUTerrain)) bNeedsMapUpdate = true;
        if (ImGui::Checkbox("Use GPU Flow Accumulation", &params.UseGPUFlowMap)) bNeedsMapUpdate = true;
        
        ImGui::Spacing();
        ImGui::Text("TG_UE Execution Tiers");
        ImGui::Separator();
        if (ImGui::Checkbox("WYSIWYG Baking (Override CPU High-Accuracy)", &params.WYSIWYGBaking)) bNeedsMapUpdate = true;
        if (ImGui::SliderInt("GPU Preview Iterations", &params.GPUPreviewIterations, 1, 100)) bNeedsMapUpdate = true;
    }

    void RenderSaveExportTab(GenerationParams& params, const FloatMask& heightmap, const GenerationResult& genData, bool& bNeedsMapUpdate, bool& bResetPreviewTransform) {
        static std::string importDebugLog = "";
        static bool showMismatchPopup = false;
        static int mismatchDiscoveredDim = 0;

        ImGui::Text("Import & Open Maps");
        ImGui::Separator();
        
        if (ImGui::Button("Open Generator File", ImVec2(-1, 30))) {
            std::string path;
            if (FileDialog::OpenFile("Generator Settings\0*.json\0All Files\0*.*\0", path)) {
                MetadataExporter::LoadSettings(path, params);
                bNeedsMapUpdate = true;
            }
        }
        
        static bool bShowFallbackPopup = false;
        
        if (ImGui::Button("Open Sanmap File", ImVec2(-1, 30))) {
            std::string path;
            if (FileDialog::OpenFile("Sanmap Files\0*.sanmap\0All Files\0*.*\0", path)) {
                importDebugLog.clear();
                
                // Clear all generated GL textures before loading the new map
                for (auto& s : params.Stratums) {
                    if (s.previewActualMaskTex) glDeleteTextures(1, &s.previewActualMaskTex);
                    s.previewActualMaskTex = 0;
                }
                
                if (MapImporter::LoadSanmap(path, params, importDebugLog)) {
                    SanmapGen::UI::ReloadStratumTextures(params);
                    bNeedsMapUpdate = true;
                    bResetPreviewTransform = true; 
                    
                    if (!params.PendingSplat14Path.empty() || !params.PendingSplat58Path.empty() || !params.PendingHeightmapPath.empty()) {
                        bShowFallbackPopup = true;
                    }
                } else {
                    importDebugLog += "\nFailed to load Sanmap.\n";
                }
            }
        }
        
        if (bShowFallbackPopup) {
            ImGui::OpenPopup("Found Imported Textures");
        }
        
        if (ImGui::BeginPopupModal("Found Imported Textures", &bShowFallbackPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("A Textures folder with baked splatmaps/heightmap was found.");
            ImGui::Text("Do you want to load these textures to update the Map Generator's imported data?");
            ImGui::Separator();
            
            if (ImGui::Button("Yes, Load Textures", ImVec2(150, 0))) {
                MapImporter::LoadPendingTextures(params, importDebugLog);
                bNeedsMapUpdate = true;
                bShowFallbackPopup = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("No, Ignore", ImVec2(120, 0))) {
                params.PendingSplat14Path = "";
                params.PendingSplat58Path = "";
                params.PendingHeightmapPath = "";
                bShowFallbackPopup = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        
        if (ImGui::Button("Import SupCom Lua", ImVec2(-1, 30))) {
            std::string path;
            if (FileDialog::OpenFile("Supreme Commander Lua\0*.lua\0All Files\0*.*\0", path)) {
                importDebugLog.clear();
                if (SupComImporter::LoadLua(path, params, importDebugLog)) {
                    bNeedsMapUpdate = true;
                    bResetPreviewTransform = true; 
                } else {
                    importDebugLog += "\nFailed to load SupCom Lua.\n";
                }
            }
        }

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Text("Save & Export");
        ImGui::Separator();

        if (ImGui::Button("Save Generator File", ImVec2(-1, 30))) {
            std::string path;
            if (FileDialog::SaveFile("Generator Settings\0*.json\0All Files\0*.*\0", "json", path)) {
                if(path.find(".json") == std::string::npos) path += ".json";
                MetadataExporter::SaveSettings(path, params);
            }
        }
        
        if (ImGui::Button("Export Sanmap File Only (.sanmap)", ImVec2(-1, 30))) {
            std::string path;
            if (FileDialog::SelectFolder(path)) {
                MetadataExporter::ExportSanmap(path, params, heightmap, genData, false);
            }
        }
        
        ImGui::Spacing();
        if (ImGui::Button("Export All (Project + Textures)", ImVec2(-1, 30))) {
            std::string path;
            if (FileDialog::SelectFolder(path)) {
                MetadataExporter::ExportSanmap(path, params, heightmap, genData, true);
            }
        }
        
        ImGui::Spacing();
        ImGui::Text("Individual Layer Exports");
        ImGui::Separator();
        
        if (ImGui::Button("Export Heightmap (RAW)")) {
            std::string path;
            if (FileDialog::SaveFile("RAW Heightmap\0*.raw\0All Files\0*.*\0", "raw", path)) {
                if(path.find(".raw") == std::string::npos) path += ".raw";
                TextureExporter::ExportHeightmap(path, params, heightmap);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Export Slope Map (PNG)")) {
            std::string path;
            if (FileDialog::SaveFile("PNG Image\0*.png\0All Files\0*.*\0", "png", path)) {
                if(path.find(".png") == std::string::npos) path += ".png";
                TextureExporter::ExportSlopeMap(path, params, heightmap);
            }
        }
        
        if (ImGui::Button("Export Flow Map (PNG)")) {
            std::string path;
            if (FileDialog::SaveFile("PNG Image\0*.png\0All Files\0*.*\0", "png", path)) {
                if(path.find(".png") == std::string::npos) path += ".png";
                TextureExporter::ExportFlowMap(path, params, genData);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Export Stratums (TGA)")) {
            std::string path;
            if (FileDialog::SelectFolder(path)) {
                TextureExporter::ExportStratums(path, params, genData);
            }
        }
        
        ImGui::Spacing();
        ImGui::Text("Legacy");
        if (ImGui::Button("Import Supcom FAF Map", ImVec2(-1, 30))) {
            // Placeholder
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Import Debug Log");
        ImGui::BeginChild("DebugLog", ImVec2(0, 0), true);
        ImGui::TextWrapped("%s", importDebugLog.c_str());
        ImGui::EndChild();
    }

} // namespace UI
} // namespace SanmapGen

