// FilesTab_ExportGate_UI.h — MODULE-INTERNAL confirm-dialog pre-check/deferred-commit gate shared
// by every gated Files-tab export button (ExportSanmapOnly/ExportAll's blueprintPath warning,
// STEP5_PropsDecalsValidation_UI; ExportScenarioScript's mandatory-spawns warning, STEP77 Fix §3).
// Split out of FilesTab_Draw_UI.cpp for the Constitution §1.5 ceiling. Nothing outside the FilesTab
// module includes this header; it declares no new public type (ARCH §8.4) — `FilesTabState`/
// `FilesTabAction` stay in FilesTab_UI.h.
#pragma once

namespace SanmapGen {
namespace Data { class MapFields; }
namespace Params { struct MapRecipe; }
namespace Ui {

struct FilesTabState;
enum class FilesTabAction;

// Clean (or the relevant source of truth absent) -> true, the caller runs the action exactly as
// today, zero added cost. Dirty -> stashes the pending action + a summary into the state's
// confirm-state, requests the dialog open, and returns false — the caller must NOT export this
// frame. Reused verbatim for BOTH gates it serves (see this header's own top comment); never both
// pending at once, since this runs from exactly one button click per frame.
bool PreCheckGatedExport(FilesTabAction action, FilesTabState& state, const Params::MapRecipe& recipe);

// ExportSanmapOnly/ExportAll/ExportScenarioScript ONLY — never the four texture-only exports,
// which carry no gate-relevant data at all (Files-tab flow, final line).
void DrawGatedExportButton(FilesTabAction action, FilesTabState& state, Params::MapRecipe& recipe,
                           Data::MapFields* fields);

// Drawn every frame, unconditionally, regardless of whether a warning is currently pending — an
// imgui modal popup must be given the chance to run its own frame every frame it might be open.
// OK exports the stashed action anyway (the designer's call); Cancel aborts with nothing written.
void DrawPendingExportWarningDialog(FilesTabState& state, Params::MapRecipe& recipe,
                                    Data::MapFields* fields);

} // namespace Ui
} // namespace SanmapGen
