#include "UITabs.h"
#include "imgui.h"
#include "FileDialog.h"
#include "MapExporter.h"
#include "MapImporter.h"
#include "SupComImporter.h"
#include "UIHelpers.h"
#include <filesystem>

namespace SanmapGen {
namespace UI {

    void RenderPerformanceTab(GenerationParams& params, bool& bNeedsMapUpdate) {
        ImGui::Text("Hardware Acceleration Options");
        ImGui::Separator();
        if (ImGui::Checkbox("Use GPU Terrain Generation", &params.UseGPUTerrain)) bNeedsMapUpdate = true;
        if (ImGui::Checkbox("Use GPU Hydraulic Erosion", &params.UseGPUHydraulic)) bNeedsMapUpdate = true;
        if (ImGui::Checkbox("Use GPU Soil Deposition", &params.UseGPUDeposition)) bNeedsMapUpdate = true;
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
                MapExporter::LoadSettings(path, params);
                bNeedsMapUpdate = true;
            }
        }
        
        if (ImGui::Button("Open Sanmap File", ImVec2(-1, 30))) {
            std::string path;
            if (FileDialog::OpenFile("Sanmap Files\0*.sanmap\0All Files\0*.*\0", path)) {
                importDebugLog.clear();
                bool bMismatch = false;
                int discoveredDim = 0;
                if (MapImporter::LoadSanmap(path, params, importDebugLog, bMismatch, discoveredDim)) {
                    SanmapGen::UI::ReloadStratumTextures(params);
                    bNeedsMapUpdate = true;
                    bResetPreviewTransform = true; 
                    if (bMismatch) {
                        mismatchDiscoveredDim = discoveredDim;
                        ImGui::OpenPopup("Heightmap Size Mismatch");
                    }
                } else {
                    importDebugLog += "\nFailed to load Sanmap.\n";
                }
            }
        }

        if (ImGui::BeginPopupModal("Heightmap Size Mismatch", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("The loaded .sanmap settings specify a Map Size of %d.", params.MapSize);
            ImGui::Text("However, the discovered heightmap.raw file corresponds to a Map Size of %d.", mismatchDiscoveredDim - 1);
            ImGui::Spacing();
            ImGui::Text("How would you like to proceed?");
            ImGui::Spacing();
            
            if (ImGui::Button("Update Map Size", ImVec2(-1, 0))) {
                params.MapSize = mismatchDiscoveredDim - 1;
                bNeedsMapUpdate = true;
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Changes the Map Size setting to match the heightmap file.");
            
            if (ImGui::Button("Scale Heightmap", ImVec2(-1, 0))) {
                bNeedsMapUpdate = true;
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Keeps the current Map Size and scales the heightmap data to fit.");
            
            if (ImGui::Button("Ignore Heightmap", ImVec2(-1, 0))) {
                params.GeoLayers.clear(); // Discard the mismatched heightmap
                bNeedsMapUpdate = true;
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Loads the settings only and discards the heightmap data.");
            
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
                MapExporter::SaveSettings(path, params);
            }
        }
        
        if (ImGui::Button("Export Sanmap File Only (.sanmap)", ImVec2(-1, 30))) {
            std::string path;
            if (FileDialog::SelectFolder(path)) {
                MapExporter::ExportSanmap(path, params, heightmap, genData, false);
            }
        }
        
        ImGui::Spacing();
        if (ImGui::Button("Export All (Project + Textures)", ImVec2(-1, 30))) {
            std::string path;
            if (FileDialog::SelectFolder(path)) {
                MapExporter::ExportSanmap(path, params, heightmap, genData, true);
            }
        }
        
        ImGui::Spacing();
        ImGui::Text("Individual Layer Exports");
        ImGui::Separator();
        
        if (ImGui::Button("Export Heightmap (.raw)", ImVec2(-1, 24))) {
            std::string path;
            if (FileDialog::SaveFile("RAW Heightmap\0*.raw\0All Files\0*.*\0", "raw", path)) {
                if(path.find(".raw") == std::string::npos) path += ".raw";
                MapExporter::ExportHeightmap(path, params, heightmap);
            }
        }
        if (ImGui::Button("Export Slope Map (.png)", ImVec2(-1, 24))) {
            std::string path;
            if (FileDialog::SaveFile("PNG Image\0*.png\0All Files\0*.*\0", "png", path)) {
                if(path.find(".png") == std::string::npos) path += ".png";
                MapExporter::ExportSlopeMap(path, params, heightmap);
            }
        }
        if (ImGui::Button("Export Flow Map (.png)", ImVec2(-1, 24))) {
            std::string path;
            if (FileDialog::SaveFile("PNG Image\0*.png\0All Files\0*.*\0", "png", path)) {
                if(path.find(".png") == std::string::npos) path += ".png";
                MapExporter::ExportFlowMap(path, params, genData);
            }
        }
        if (ImGui::Button("Export Stratums (.tga masks)", ImVec2(-1, 24))) {
            std::string path;
            if (FileDialog::SelectFolder(path)) {
                MapExporter::ExportStratums(path, params, genData);
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
