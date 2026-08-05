#include "UITabs.h"
#include "imgui.h"
#include "FileDialog.h"
#include "MapExporter.h"

namespace SanmapGen {
namespace UI {

    void RenderPerformanceTab(GenerationParams& params, bool& bNeedsMapUpdate) {
        ImGui::Text("Hardware Acceleration Options");
        ImGui::Separator();
        if (ImGui::Checkbox("Use GPU Terrain Generation", &params.UseGPUTerrain)) bNeedsMapUpdate = true;
        if (ImGui::Checkbox("Use GPU Hydraulic Erosion", &params.UseGPUHydraulic)) bNeedsMapUpdate = true;
        if (ImGui::Checkbox("Use GPU Soil Deposition", &params.UseGPUDeposition)) bNeedsMapUpdate = true;
    }

    void RenderSaveExportTab(GenerationParams& params, bool& bNeedsMapUpdate) {
        ImGui::Text("Export Map Data");
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
                MapExporter::LoadSettings(path, params); // For now uses LoadSettings as placeholder for loading
                bNeedsMapUpdate = true;
            }
        }

        if (ImGui::Button("Save Generator File", ImVec2(-1, 30))) {
            std::string path;
            if (FileDialog::SaveFile("Generator Settings\0*.json\0All Files\0*.*\0", "json", path)) {
                if(path.find(".json") == std::string::npos) path += ".json";
                MapExporter::SaveSettings(path, params);
            }
        }
        
        if (ImGui::Button("Export Sanmap", ImVec2(-1, 30))) {
            std::string path;
            if (FileDialog::SelectFolder(path)) {
                MapExporter::ExportSanmap(path, params);
            }
        }
        
        ImGui::Spacing();
        ImGui::Text("Legacy");
        if (ImGui::Button("Import Supcom FAF Map", ImVec2(-1, 30))) {
            // Placeholder
        }
    }

} // namespace UI
} // namespace SanmapGen
