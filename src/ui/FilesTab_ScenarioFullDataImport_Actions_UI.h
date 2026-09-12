// FilesTab_ScenarioFullDataImport_Actions_UI.h — MODULE-INTERNAL seam for STEP266's
// FilesTabAction::ImportScenarioFullData action, split out of FilesTab_Actions_UI.cpp for the
// Constitution §1.5 file-size ceiling (mirrors FilesTab_ScenarioExport_Actions_UI.h's own posture).
// Nothing outside the FilesTab module includes this header; it declares no new public type
// (ARCH §8.4) — FilesTabState/FilesTabAction stay in FilesTab_UI.h.
#pragma once

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Ui {

struct FilesTabState;

// ARCH §15.11/§15.14 — human-triggered, one-shot: this action exists ONLY as an explicit click,
// same posture as RunImportScenarioAreas (FilesTab_Actions_UI.cpp). Calls
// Io::ImportFullScenarioDataFromScenarioScriptFile, logs a per-shape banner (counts + near-miss
// reasons, ARCH_15_14 Part B / STEP266 §3) in addition to the IO layer's own `debugLog`, and
// stashes the three non-auto-attached candidate lists into `state.scenarioImportReview`
// (FilesTab_ScenarioImportReview_UI.h) for the review/assign panel to present. Returns true when the
// file was actually read (matches RunImportScenarioAreas's own refusal-only-on-guard-failure
// contract — an accepted file with zero candidates in every shape still counts as success, since
// "nothing found" is itself a valid, reportable outcome for a foreign file, unlike the area-only
// importer's own "at least one area written" bar).
bool RunImportScenarioFullData(FilesTabState& state, Params::MapRecipe& recipe);

} // namespace Ui
} // namespace SanmapGen
