// FilesTab_Draw_UI.cpp — the imgui composition of the Files / Save tab. Layer: UI.
// Three sections: Open (sanmap + SupCom lua), Export (the six actions), and the debug log.
// Every button is one call into RunFilesTabAction — the draw path decides nothing, WITH ONE
// EXCEPTION: ExportSanmapOnly/ExportAll gain a blueprintPath pre-check -> confirm-dialog ->
// deferred-commit flow (STEP5_PropsDecalsValidation_UI). This is the ONE non-headless layer in the
// tab, which is why all of that new logic lives here and nowhere else — RunFilesTabAction itself
// never changes.
#include "FilesTab_UI.h"
#include "FilesTab_Browse_UI.h"
#include "Checkbox_UI.h"
#include "ConfirmDialog_UI.h"
#include "TextInput_UI.h"
#include "../data/MapFields_DATA.h"
#include "../io/MapExporter_BlueprintValidation_IO.h"
#include "../io/SanpackReader_IO.h"
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

// The STEP5 pre-check: clean (or no pack loaded) -> true, the caller runs the action exactly as
// today, zero added cost. Dirty -> stashes the pending action + report.SummaryText() into the
// state's confirm-state, requests the dialog open, and returns false — the caller must NOT export
// this frame. `state.assetPack == nullptr` skips validation entirely (Files-tab flow item 1).
bool PreCheckGatedExport(FilesTabAction action, FilesTabState& state, const Params::MapRecipe& recipe) {
    if (state.assetPack == nullptr) return true;
    const Io::BlueprintValidationReport report =
        Io::ValidatePropAndDecalBlueprintPaths(recipe, *state.assetPack);
    if (report.AllResolved()) return true;
    state.pendingConfirmAction              = action;
    state.bConfirmActionPending             = true;
    state.confirmDialogBodyText             = report.SummaryText();
    state.confirmDialogState.bOpenRequested = true;
    return false;
}

// ExportSanmapOnly/ExportAll ONLY — never the four texture-only exports, which carry no
// blueprintPath data at all (Files-tab flow, final line).
void DrawGatedExportButton(FilesTabAction action, FilesTabState& state, Params::MapRecipe& recipe,
                           Data::MapFields* fields) {
    if (!ImGui::Button(FilesTabActionLabel(action))) return;
    if (PreCheckGatedExport(action, state, recipe)) RunFilesTabAction(action, state, recipe, fields);
}

// Drawn every frame, unconditionally, regardless of whether a warning is currently pending — an
// imgui modal popup must be given the chance to run its own frame every frame it might be open.
// OK exports the stashed action anyway (the designer's call); Cancel aborts with nothing written.
void DrawPendingBlueprintWarningDialog(FilesTabState& state, Params::MapRecipe& recipe,
                                       Data::MapFields* fields) {
    ConfirmDialogOptions options;
    options.title               = "Unresolved blueprintPath";
    options.bodyText            = state.confirmDialogBodyText;
    options.primaryButtonLabel  = "Export Anyway";
    options.secondaryButtonLabel = "Cancel";
    const ConfirmDialogChange change =
        DrawConfirmDialog("filesTabBlueprintWarning", state.confirmDialogState, options);
    if (change.bPrimaryClicked) {
        RunFilesTabAction(state.pendingConfirmAction, state, recipe, fields);
        state.bConfirmActionPending = false;
    } else if (change.bSecondaryClicked) {
        state.bConfirmActionPending = false;
    }
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
                  Pipeline::PreviewDriver* previewDriver) {
    ImGui::PushID("filesTab");
    DrawOpenSection(state, recipe, fields, previewDriver);
    DrawExportSection(state, recipe, fields, previewDriver);
    DrawLogSection(state);
    // Unconditional (Files-tab flow item 4): a warning triggered from inside the (possibly now
    // collapsed) Export section must still get its popup frame every frame it might be open.
    DrawPendingBlueprintWarningDialog(state, recipe, fields);
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
