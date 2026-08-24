// FilesTab_MigrationDialog_Draw_UI.h — MODULE-INTERNAL draw-side glue for the "Check for
// Migrations..." reconciliation dialog (STEP26B). Layer: UI. Split out under the ARCH §1.5 file-size
// ceilings, FilesTab_Browse_UI.h's own precedent: FilesTab_Draw_UI.cpp composes the tab's sections,
// this pair owns the one thing specific to this dialog — drawing it and running its "Apply Selected"
// click through the headless actions (FilesTab_MigrationImport_Actions_UI.cpp). Nothing outside the
// FilesTab module includes this header; it declares no new public type (ARCH §8.4).
#pragma once
#include <vector>

namespace SanmapGen {
namespace Data { class MapFields; struct BakedLayerImage; struct StratumArt; }
namespace Params { struct MapRecipe; }
namespace Pipeline { class PreviewDriver; }
namespace Ui {

struct FilesTabState;

// Drawn every frame, unconditionally, regardless of whether the dialog is currently open —
// `DrawPendingExportWarningDialog`'s own precedent (FilesTab_ExportGate_UI.cpp): an imgui modal
// popup must be given the chance to run its own frame every frame it might be open. "Apply Selected" runs
// `RunSelectiveMigrationImport` and (on success) requests a full map update, matching
// `DrawOpenSection`'s own Open behavior; "Close" dismisses with nothing re-read, nothing mutated.
// `outBakedLayerImages`/`outStratumArt` — see `RunFilesTabAction` (FilesTab_UI.h, STEP101/STEP105),
// same nullable posture, forwarded straight to `RunSelectiveMigrationImport`.
void DrawFilesTabMigrationReconciliationDialog(FilesTabState& state, Params::MapRecipe& recipe,
                                               Data::MapFields* fields,
                                               Pipeline::PreviewDriver* previewDriver,
                                               std::vector<Data::BakedLayerImage>* outBakedLayerImages = nullptr,
                                               std::vector<Data::StratumArt>* outStratumArt = nullptr);

} // namespace Ui
} // namespace SanmapGen
