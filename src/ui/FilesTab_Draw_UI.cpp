// FilesTab_Draw_UI.cpp — the imgui composition of the Files / Save tab. Layer: UI.
// Open (sanmap + SupCom lua), Export (the six recipe/texture actions), the debug log, plus STEP77's
// Scenario Script export row (FilesTab_ScenarioExportRow_Draw_UI.cpp — split out for the §1.5
// ceiling). Every button is one call into RunFilesTabAction — the draw path decides nothing, WITH
// ONE EXCEPTION: the confirm-dialog pre-check/deferred-commit gate
// (FilesTab_ExportGate_UI.cpp, STEP5_PropsDecalsValidation_UI + STEP77 Fix §3). This is the ONE
// non-headless layer in the tab; RunFilesTabAction itself never changes.
#include "FilesTab_UI.h"
#include "FilesTab_Browse_UI.h"
#include "FilesTab_ExportGate_UI.h"
#include "FilesTab_MigrationDialog_Draw_UI.h"
#include "FilesTab_ScenarioExportRow_Draw_UI.h"
#include "Checkbox_UI.h"
#include "TextInput_UI.h"
#include "../data/BakedLayerImage_DATA.h"
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
                     Pipeline::PreviewDriver* previewDriver,
                     std::vector<Data::BakedLayerImage>* bakedLayerImages) {
    if (!DrawSectionBegin("Open", state.openSection)) return;
    DrawCheckbox("Load Baked Textures On Import", state.bLoadBakedFieldsOnImport);
    if (fields == nullptr)
        ImGui::TextUnformatted("No field destination bound; the recipe loads, the textures are skipped.");
    if (DrawFilesTabOpenButton(FilesTabActionLabel(FilesTabAction::OpenSanmap), state.sanmapPath)) {
        const bool bSucceeded = RunFilesTabAction(FilesTabAction::OpenSanmap, state, recipe, fields,
                                                  false, bakedLayerImages);
        if (bSucceeded && previewDriver != nullptr) previewDriver->RequestMapUpdate();
    }
    // STEP26B ruling 1: only a completed Open that found no version marker at all may offer this —
    // the dialog is a post-load review, never a load gate.
    ImGui::SameLine();
    ImGui::BeginDisabled(!state.bLastOpenHadNoVersionMarker);
    if (ImGui::Button("Check for Migrations...")) RunCheckForMigrations(state);
    ImGui::EndDisabled();
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
    DrawTextInput("Map Name", recipe.mapName, nameRules);
    DrawTextInput("Credits", recipe.mapCredits);

    DrawGatedExportButton(FilesTabAction::ExportSanmapOnly, state, recipe, fields);
    ImGui::SameLine();
    DrawGatedExportButton(FilesTabAction::ExportAll, state, recipe, fields);
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
                  Pipeline::PreviewDriver* previewDriver,
                  std::vector<Data::BakedLayerImage>* bakedLayerImages) {
    ImGui::PushID("filesTab");
    DrawOpenSection(state, recipe, fields, previewDriver, bakedLayerImages);
    DrawExportSection(state, recipe, fields, previewDriver);
    // STEP77: machine-local settings + the Export Scenario Script row/banner — a SEPARATE section,
    // never buried in Scenarios (Fix §5's own reasoning).
    DrawScenarioScriptExportSection(state, recipe, fields);
    DrawLogSection(state);
    // Unconditional (Files-tab flow item 4): a warning triggered from inside the (possibly now
    // collapsed) Export section must still get its popup frame every frame it might be open.
    DrawPendingExportWarningDialog(state, recipe, fields);
    // Same unconditional posture (STEP26B) — the reconciliation dialog's own popup.
    DrawFilesTabMigrationReconciliationDialog(state, recipe, fields, previewDriver, bakedLayerImages);
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
