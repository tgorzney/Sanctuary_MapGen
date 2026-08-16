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
#include "Section_UI.h"
#include "../io/MapExporter_IO.h"
#include "../io/MapImporter_IO.h"
#include <string>

namespace SanmapGen {
namespace Data { class MapFields; }
namespace Params { struct MapRecipe; }
namespace Pipeline { class PreviewDriver; }
namespace Ui {

// See SCOPE NOTE 1. Returns true when the recipe was populated; `outLog` is appended to the panel.
using SupComLuaImportFunction = bool (*)(void* userData, const char* luaFilePath,
                                         Params::MapRecipe& outRecipe, std::string& outLog);

// Every button on the tab, in the order the plan lists them.
enum class FilesTabAction {
    OpenSanmap, ImportSupComLua, ExportSanmapOnly, ExportAll,
    ExportHeightmapRaw, ExportSlopeImage, ExportFlowImage, ExportStratumMasks
};
inline constexpr int filesTabActionCount = 8;

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
    SectionState logSection;

    std::string sanmapPath;         // the .sanmap file OR the map folder the user picked
    std::string supComLuaPath;      // a Supreme Commander `_save.lua`
    std::string exportFolderPath;   // the destination map folder

    Io::MapImportOptions importOptions;
    Io::MapExportOptions exportOptions;

    SupComLuaImportFunction ImportSupComLua = nullptr;   // SCOPE NOTE 1
    void*                   supComUserData  = nullptr;

    std::string debugLog;
    int         maximumDebugLogCharacterCount = 65536;   // the panel is bounded, not unbounded
    bool        bLoadBakedFieldsOnImport = true;
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
// `fields` is nullable (SCOPE NOTE 2). Returns whether the action reported success.
// FilesTab_Actions_UI.cpp — headless: no imgui frame, no window, no GL context.
bool RunFilesTabAction(FilesTabAction action, FilesTabState& state, Params::MapRecipe& recipe,
                       Data::MapFields* fields);

// Draws the tab. Every pointer is nullable; the tab still edits its own state with nothing bound.
void DrawFilesTab(Params::MapRecipe& recipe, FilesTabState& state, Data::MapFields* fields,
                  Pipeline::PreviewDriver* previewDriver);

} // namespace Ui
} // namespace SanmapGen
