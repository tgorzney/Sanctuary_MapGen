// FilesTab_Draw_UI.cpp — the imgui composition of the Files / Save tab. Layer: UI.
// Three sections: Open (sanmap + SupCom lua), Export (the six actions), and the debug log.
// Every button is one call into RunFilesTabAction — the draw path decides nothing.
#include "FilesTab_UI.h"
#include "FilesTab_Browse_UI.h"
#include "Checkbox_UI.h"
#include "TextInput_UI.h"
#include "../data/MapFields_DATA.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../pipeline/PreviewDriver_PIPELINE.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// A recipe reload moves everything at once, which is precisely the change no parameter hash can
// see (PreviewDriver_PIPELINE.h) — so an import asks for a full map update, not a recolor.
void DrawActionButton(FilesTabAction action, FilesTabState& state, Params::MapRecipe& recipe,
                      Data::MapFields* fields, Pipeline::PreviewDriver* previewDriver) {
    if (!ImGui::Button(FilesTabActionLabel(action))) return;
    const bool bSucceeded = RunFilesTabAction(action, state, recipe, fields);
    const bool bImported = action == FilesTabAction::OpenSanmap
                        || action == FilesTabAction::ImportSupComLua;
    if (bSucceeded && bImported && previewDriver != nullptr) previewDriver->RequestMapUpdate();
}

void DrawOpenSection(FilesTabState& state, Params::MapRecipe& recipe, Data::MapFields* fields,
                     Pipeline::PreviewDriver* previewDriver) {
    if (!DrawSectionBegin("Open", state.openSection)) return;
    DrawFilesTabPathRow("Sanmap File Or Folder", FilesTabBrowseKind::SanmapDocument, state.sanmapPath);
    DrawCheckbox("Load Baked Textures On Import", state.bLoadBakedFieldsOnImport);
    if (fields == nullptr)
        ImGui::TextUnformatted("No field destination bound; the recipe loads, the textures are skipped.");
    DrawActionButton(FilesTabAction::OpenSanmap, state, recipe, fields, previewDriver);
    ImGui::Separator();
    DrawFilesTabPathRow("SupCom Save Lua", FilesTabBrowseKind::SupComLuaDocument, state.supComLuaPath);
    DrawActionButton(FilesTabAction::ImportSupComLua, state, recipe, fields, previewDriver);
    if (state.ImportSupComLua == nullptr)
        ImGui::TextUnformatted("No SupCom Lua importer is bound to this build.");
    DrawSectionEnd();
}

// The two naming settings the format carries, then the whole-map actions and the four single-file
// ones. The `.json` generator file is deliberately absent: the sanmap is the source of truth.
void DrawExportSection(FilesTabState& state, Params::MapRecipe& recipe, Data::MapFields* fields,
                       Pipeline::PreviewDriver* previewDriver) {
    if (!DrawSectionBegin("Export", state.exportSection)) return;
    DrawFilesTabPathRow("Destination Map Folder", FilesTabBrowseKind::ExportFolder,
                        state.exportFolderPath);
    TextInputRules nameRules;
    nameRules.bAllowEmpty  = false;
    nameRules.fallbackText = "mapdef";
    DrawTextInput("Map Name", state.exportOptions.mapName, nameRules);
    DrawTextInput("Credits", state.exportOptions.mapCredits);

    DrawActionButton(FilesTabAction::ExportSanmapOnly, state, recipe, fields, previewDriver);
    ImGui::SameLine();
    DrawActionButton(FilesTabAction::ExportAll, state, recipe, fields, previewDriver);
    ImGui::Separator();
    DrawActionButton(FilesTabAction::ExportHeightmapRaw, state, recipe, fields, previewDriver);
    ImGui::SameLine();
    DrawActionButton(FilesTabAction::ExportStratumMasks, state, recipe, fields, previewDriver);
    DrawActionButton(FilesTabAction::ExportSlopeImage, state, recipe, fields, previewDriver);
    ImGui::SameLine();
    DrawActionButton(FilesTabAction::ExportFlowImage, state, recipe, fields, previewDriver);
    if (fields == nullptr || !fields->IsSized())
        ImGui::TextUnformatted("Nothing generated yet: the texture exports are refused for now.");
    DrawSectionEnd();
}

void DrawLogSection(FilesTabState& state) {
    if (!DrawSectionBegin("Import / Export Log", state.logSection)) return;
    if (ImGui::Button("Clear Log")) state.debugLog.clear();
    ImGui::BeginChild("filesTabLog", ImVec2(0.0f, 180.0f), true,
                      ImGuiWindowFlags_HorizontalScrollbar);
    if (state.debugLog.empty()) ImGui::TextUnformatted("(nothing logged yet)");
    else ImGui::TextUnformatted(state.debugLog.c_str(),
                                state.debugLog.c_str() + state.debugLog.size());
    ImGui::EndChild();
    DrawSectionEnd();
}

} // namespace

void DrawFilesTab(Params::MapRecipe& recipe, FilesTabState& state, Data::MapFields* fields,
                  Pipeline::PreviewDriver* previewDriver) {
    ImGui::PushID("filesTab");
    DrawOpenSection(state, recipe, fields, previewDriver);
    DrawExportSection(state, recipe, fields, previewDriver);
    DrawLogSection(state);
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
