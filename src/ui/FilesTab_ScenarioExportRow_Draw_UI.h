// FilesTab_ScenarioExportRow_Draw_UI.h — MODULE-INTERNAL draw entry for STEP77's Scenario Script
// export section (Fix §4/§5): the machine-local settings rows (game install root, runtime script
// override path), the export row's own redirect/reason/gated-button states, and the result banner.
// Split out of FilesTab_Draw_UI.cpp for the Constitution §1.5 ceiling. Nothing outside the
// FilesTab module includes this header; it declares no new public type (ARCH §8.4).
#pragma once

namespace SanmapGen {
namespace Data { class MapFields; }
namespace Params { struct MapRecipe; }
namespace Ui {

struct FilesTabState;

// Draws the whole section: two FilePathPicker_UI settings rows, the export row (context-sensitive
// label/reason/button), and the last-result banner. `fields` is forwarded to RunFilesTabAction only
// — the action itself never reads baked fields (FilesTabActionNeedsBakedFields stays false for it).
void DrawScenarioScriptExportSection(FilesTabState& state, Params::MapRecipe& recipe,
                                     Data::MapFields* fields);

} // namespace Ui
} // namespace SanmapGen
