#include "UITabs.h"
#include "imgui.h"
#include "FileDialog.h"
#include "MapExporter.h"
#include "MapImporter.h"
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
    }

    void RenderSaveExportTab(GenerationParams& params, const FloatMask& heightmap, const GenerationResult& genData, bool& bNeedsMapUpdate) {
        ImGui::Text("Export Map Data");
        ImGui::Separator();
        
        static std::string importDebugLog = "";
        
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
                if (MapImporter::LoadSanmap(path, params, importDebugLog)) {
                    bNeedsMapUpdate = true;
                } else {
                    importDebugLog += "\nFailed to load Sanmap.\n";
                }
            }
        }

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
