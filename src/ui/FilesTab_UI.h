// FilesTab_UI.h — the Files / Save tab: open a `.sanmap`, import a SupCom `_save.lua`, run the
// export actions, and read the import/export log. Layer: UI. Accuracy class: Visual (it moves
// files; it computes nothing). TAB_REBUILD_PLAN "SYSTEM · Files / Save" — PARITY_BACKLOG PB-1/PB-2.
//
// The tab owns no format knowledge: every action is one call into the IO layer
// (`Io::MapImporter` / `Io::MapExporter` / `Io::FileDialog`), exactly as SystemTab_UI drives SYS
// and HeightmapTab_UI drives PIPELINE. It never simulates and it never writes a byte itself.
//
// THERE IS NO `.json` GENERATOR FILE. The `.sanmap` is the single source of truth
// (TAB_REBUILD_PLAN "Files / Save · Removed"), so v1's Open/Save Generator File is gone and
// `MetadataExporter::Load/SaveSettings` is deliberately not ported.
//
// SCOPE NOTES (ARCH §8.4 — a coder never invents a missing module; reported, not invented):
//  1. SUPCOM LUA IMPORT has no `_IO` module in the v2 tree — section D's target list names
//     FileDialog/MapImporter/MapExporter only. The action is therefore routed through an INJECTED
//     seam (`SupComLuaImportFunction`, the same pattern FilePathPicker_UI.h uses for its dialog):
//     with nothing bound the row says so instead of pretending to work. A real `SupComImporter_IO`
//     is its own work-order.
//  2. IMPORTED BAKED FIELDS land in a CALLER-OWNED `Data::MapFields`, never in the pipeline's own
//     fields — every DATA field has exactly one writing stage (ARCH §3.4) and IO is not a stage.
//     With no destination bound the recipe still loads; the textures are simply skipped.
#pragma once
#include "ConfirmDialog_UI.h"
#include "MigrationReconciliationDialog_UI.h"
#include "Section_UI.h"
#include "../io/MapExporter_IO.h"
#include "../io/MapImporter_IO.h"
#include "../io/ScenarioScript_Export_IO.h"
#include <string>
#include <vector>

namespace SanmapGen {
namespace Data { class MapFields; }
namespace Params { struct MapRecipe; }
namespace Pipeline { class PreviewDriver; }
namespace Io { class SanpackReader; class TemplateIngestReport; struct UnknownImportBag; }
namespace Ui {

// See SCOPE NOTE 1. Returns true when the recipe was populated; `outLog` is appended to the panel.
using SupComLuaImportFunction = bool (*)(void* userData, const char* luaFilePath,
                                         Params::MapRecipe& outRecipe, std::string& outLog);

// Every button on the tab, in the order the plan lists them.
enum class FilesTabAction {
    OpenSanmap, ImportSupComLua, ExportSanmapOnly, ExportAll,
    ExportHeightmapRaw, ExportSlopeImage, ExportFlowImage, ExportStratumMasks,
    ExportScenarioScript,       // STEP77 — Io::ExportMapScenario (STEP71), machine-local settings
};
inline constexpr int filesTabActionCount = 9;

// The button caption. Never a literal at the draw site (Constitution §8).
const char* FilesTabActionLabel(FilesTabAction action);

// True for the actions that read `Data::MapFields`: with nothing generated yet they are refused
// with a logged reason rather than writing an empty texture.
inline bool FilesTabActionNeedsBakedFields(FilesTabAction action) {
    return action == FilesTabAction::ExportAll || action == FilesTabAction::ExportHeightmapRaw
        || action == FilesTabAction::ExportSlopeImage || action == FilesTabAction::ExportFlowImage
        || action == FilesTabAction::ExportStratumMasks;
}

// Caller-owned tab state: the three paths, the IO settings (Constitution §8 — every export knob is
// reachable), the injected SupCom seam, and the log the panel shows.
struct FilesTabState {
    SectionState openSection;
    SectionState exportSection;
    SectionState scenarioExportSection;   // STEP77 — "Export Scenario Script" + its own settings
    SectionState logSection;

    std::string sanmapPath;         // the .sanmap file OR the map folder the user picked
    std::string supComLuaPath;      // a Supreme Commander `_save.lua`
    std::string exportFolderPath;   // the destination map folder

    Io::MapImportOptions importOptions;
    Io::MapExportOptions exportOptions;

    // STEP24_ImportNeverRefuses_IO ruling 4/6: the load-edit-save session's own Unknown-Import
    // passthrough — populated by OpenSanmap, re-merged (gap-filling only) by ExportSanmapOnly/
    // ExportAll so data this build does not recognize survives a load/save cycle. NULLABLE, CALLER-
    // OWNED (the `Data::MapFields* fields` precedent, not the `Io::MapImportOptions` by-value
    // precedent): `UnknownImportBag` embeds a real `nlohmann::json`, and this header must stay
    // includable by every UI translation unit that touches the Files tab WITHOUT dragging nlohmann's
    // headers along (several UI/App targets do not link `nlohmann_json` at all — Constitution §1's
    // IO-only JSON homing). The tab never inspects its contents; it only forwards the pointer.
    Io::UnknownImportBag* unknownImportData = nullptr;

    SupComLuaImportFunction ImportSupComLua = nullptr;   // SCOPE NOTE 1
    void*                   supComUserData  = nullptr;

    std::string debugLog;
    int         maximumDebugLogCharacterCount = 65536;   // the panel is bounded, not unbounded
    bool        bLoadBakedFieldsOnImport = true;

    // STEP5_PropsDecalsValidation_UI — the long-lived reader Application owns, fed down here.
    // LOAD-BEARING: non-null ONLY when Application's own Open()+ReadCentralDirectoryOnce() both
    // succeeded for the CURRENT sanpack path; nullptr on first launch, after a failed load, or once
    // the path is cleared. A dangling-into-empty reader would report every blueprintPath as
    // unresolved (SanpackReader::HasEntry's "unopened answers false" contract) and pop the confirm
    // dialog on every export for a designer who never loaded a pack at all.
    const Io::SanpackReader* assetPack = nullptr;

    // STEP96_FootprintBakeAndStalenessCheck_IO.md §3.1 call site 1: the live, session-scoped
    // ingestion report ("Resolve Footprint" bake source) — same nullable, caller-owned posture as
    // `assetPack` above. With nothing bound (no install configured, or never ingested this session)
    // OpenSanmap simply skips the post-load staleness check rather than forcing an ingest.
    const Io::TemplateIngestReport* templateIngestReport = nullptr;

    // The blueprintPath confirm-dialog's pending state (Files-tab flow, FilesTab_Draw_UI.cpp only):
    // set on a dirty ExportSanmapOnly/ExportAll click, cleared on OK or Cancel. RunFilesTabAction
    // itself never sees any of this — it stays the same headless, unconditional "just do it" call.
    ConfirmDialogState confirmDialogState;
    FilesTabAction      pendingConfirmAction  = FilesTabAction::ExportSanmapOnly;
    bool                bConfirmActionPending = false;
    std::string         confirmDialogBodyText;

    // STEP26B: the "Check for Migrations..." dialog's own state — a DIFFERENT payload shape from the
    // blueprintPath confirm block above, so a parallel field, never folded into it.
    // `bLastOpenHadNoVersionMarker` mirrors the last Open's MapImportResult::bNoVersionMarkerFound
    // (set in RunOpenSanmap) and gates the button that opens the dialog: post-load review, never a
    // load gate (ruling 1).
    MigrationReconciliationDialogState migrationDialogState;
    bool bLastOpenHadNoVersionMarker = false;

    // STEP77: caller-owned pointers into Application-level machine-local settings (STEP64) — same
    // posture as `assetPack`: non-null once Application wires them; nullptr degrades the row to a
    // clear "not configured" state rather than crashing.
    std::string* gameInstallRoot             = nullptr;
    std::string* scenarioRuntimeOverridePath = nullptr;
    std::string  scenarioRuntimeResourceDirectory;   // resolved once at startup (§5), copied not
                                                     // pointed — it never changes after launch
    Io::ScenarioExportResult lastScenarioExportResult;
};

// Appends one block to the log and holds it inside its budget by dropping WHOLE lines off the
// front — so the panel always shows the most recent output and can never grow without bound.
// Pure, so the trimming contract is testable without an imgui frame.
inline void AppendFilesTabLog(FilesTabState& state, const std::string& text) {
    if (text.empty()) return;
    state.debugLog += text;
    if (state.debugLog.size() && state.debugLog[state.debugLog.size() - 1] != '\n')
        state.debugLog += '\n';
    const int budget = state.maximumDebugLogCharacterCount;
    if (budget <= 0 || state.debugLog.size() <= static_cast<std::size_t>(budget)) return;
    const std::size_t overflow = state.debugLog.size() - static_cast<std::size_t>(budget);
    const std::string::size_type lineStart = state.debugLog.find('\n', overflow);
    state.debugLog = lineStart == std::string::npos ? std::string()
                                                    : state.debugLog.substr(lineStart + 1);
}

// Runs one action end to end and logs the outcome. `recipe` is written by the two import actions;
// `fields` is nullable (SCOPE NOTE 2). `bBlueprintValidationAcknowledged` reaches
// `MapExporter::ExportSanmapOnly`/`ExportAll` unchanged (STEP39_BlueprintValidationGate_IO) — every
// action but the two recipe exports ignores it. Default false is exactly right for
// `DrawGatedExportButton`'s normal click (`FilesTab_ExportGate_UI.cpp`'s own pre-check already
// guarantees every path resolves before this runs); `DrawPendingExportWarningDialog`'s "Export
// Anyway" click is the one call site that passes true. Returns whether the action reported success.
// FilesTab_Actions_UI.cpp — headless: no imgui frame, no window, no GL context.
bool RunFilesTabAction(FilesTabAction action, FilesTabState& state, Params::MapRecipe& recipe,
                       Data::MapFields* fields, bool bBlueprintValidationAcknowledged = false);

// STEP26B: the "Check for Migrations..." button's action (ruling 1) — a fresh, read-only re-derive
// of `state.sanmapPath` into a preview report, copied into `state.migrationDialogState` with
// `bOpenRequested` set. Never mutates the document, never touches `recipe`. Headless
// (FilesTab_MigrationImport_Actions_UI.cpp).
bool RunCheckForMigrations(FilesTabState& state);

// The dialog's "Apply Selected" action: re-derives the document from `state.sanmapPath` (a fresh
// read, independent of Open), applies `selectedMigrationNames` via
// `Io::ApplySelectedSanmapMigrations`, then re-parses with `Io::MapImporter::ParseSanmapJsonText` —
// that fresh result REPLACES `recipe`/`fields`'s baked textures/`state.unknownImportData` entirely,
// never merged with what Open already produced. Headless (FilesTab_MigrationImport_Actions_UI.cpp).
bool RunSelectiveMigrationImport(FilesTabState& state, Params::MapRecipe& recipe,
                                 Data::MapFields* fields,
                                 const std::vector<std::string>& selectedMigrationNames);

// Draws the tab. Every pointer is nullable; the tab still edits its own state with nothing bound.
void DrawFilesTab(Params::MapRecipe& recipe, FilesTabState& state, Data::MapFields* fields,
                  Pipeline::PreviewDriver* previewDriver);

} // namespace Ui
} // namespace SanmapGen
