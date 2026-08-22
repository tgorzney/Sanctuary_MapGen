// MigrationReconciliationDialog_UI.h — the Files tab's "Check for Migrations..." reconciliation
// dialog (STEP26B, IO_MIGRATION_SPEC.md §6's UI-layer selective-apply surface). Layer: UI.
//
// Reachable ONLY after a completed Open reported MapImportResult::bNoVersionMarkerFound == true
// (MapImporter_IO.h) — a post-load review, never a load gate (STEP26B ruling 1: the file is already
// loaded, current-shape-only, by the time the button that opens this is clickable). Shows one
// checklist group per Io::MigrationPreviewStep (Sanmap_MigrationPreview_IO.h), header
// "Version N -> N+1": an entry with BOTH bIndependentlySelectable AND bLosslessIfSkipped gets a real
// Checkbox_UI row, default checked ("apply all" is just the starting state, ruling 3); every other
// entry is a disabled, checkbox-less informational row — it always runs whenever its own step runs at
// all (Sanmap_MigrationManifest_IO.h's dialog-gating law, ratified STEP26A).
//
// THE SPLIT (WidgetHelpers_UI.h): everything here is a pure struct/function — no imgui, and no
// nlohmann::json in sight (Constitution §1's IO-only JSON homing; FilesTab_UI.h's own
// UnknownImportBag comment states the identical constraint for this same tab — several UI/App
// targets do not link nlohmann_json at all). `Io::MigrationPreviewReport` is therefore only
// forward-declared: ResetMigrationDialogFromReport is implemented in the .cpp (the one place that may
// include both imgui.h and Sanmap_MigrationPreview_IO.h) and copies out only the plain fields this
// dialog actually needs into its own MigrationDialogEntry/MigrationDialogStep — never the IO-side
// `diffPatch`, per this ticket's own out-of-scope note (a full JSON-patch visualizer is a later
// enhancement, if ever wanted; `bWouldChangeDocument` is shown as a simple indicator instead).
#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
namespace Io { struct MigrationPreviewReport; }
namespace Ui {

// One entry as the dialog holds it — a UI-owned copy of Io::MigrationPreviewEntry plus the one field
// IO has no business owning: whether a human has ticked it. `bSelected` starts true and is only ever
// READ for a checkbox-eligible entry (IsMigrationDialogEntryCheckboxEligible below); an informational
// entry's own value is never consulted for the same reason it is never drawn as a checkbox.
struct MigrationDialogEntry {
    std::string name;
    std::string description;
    bool        bIndependentlySelectable = false;
    bool        bLosslessIfSkipped       = false;
    bool        bWouldChangeDocument     = false;
    bool        bSelected                = true;   // ruling 3: "apply all" is the default state
};

// True for the one entry shape that may offer a real checkbox (Sanmap_MigrationManifest_IO.h's
// dialog-gating law, ratified STEP26A) — every other entry is a disabled, informational row that
// always runs whenever its own step runs at all, with no human choice to make.
inline bool IsMigrationDialogEntryCheckboxEligible(const MigrationDialogEntry& entry) {
    return entry.bIndependentlySelectable && entry.bLosslessIfSkipped;
}

// One manifest step's own group ("Version N -> N+1", ruling 2). Mirrors Io::MigrationPreviewStep
// minus its own `legacyKeysToDelete` informational mirror — this dialog does not surface that detail.
struct MigrationDialogStep {
    int                               sourceVersion = 0;
    std::vector<MigrationDialogEntry> entries;
};

// Caller-owned, one instance per call site (ARCH §3.2 — ConfirmDialog_UI's own precedent): the widget
// holds no state of its own beyond what the caller gives it back next frame.
struct MigrationReconciliationDialogState {
    bool                              bOpenRequested = false;
    std::vector<MigrationDialogStep>  steps;
};

// Rebuilds `state.steps` from a real preview report, every checkbox-eligible entry starting checked
// (ruling 3's "apply all is just the default state, no separate button needed"). Does NOT touch
// `bOpenRequested` — the caller (the "Check for Migrations..." button's click handler) sets that
// itself once the rebuild is done. The one function in this header that may see an IO type by
// reference; still pure (no imgui), so still headless-testable given a hand-built report.
void ResetMigrationDialogFromReport(MigrationReconciliationDialogState& state,
                                    const Io::MigrationPreviewReport& report);

// Every entry name Io::ApplySelectedSanmapMigrations (Sanmap_MigrationPreview_IO.h) should treat as
// opted in: every CHECKED checkbox-eligible entry, PLUS every entry that is not checkbox-eligible at
// all (ruling 3's "Apply Selected" bullet: "...plus every non-selectable/non-lossless entry
// implicitly"). The second half is load-bearing, not a convenience: that function's own
// bEveryEntrySelected full-step/partial-step decision checks EVERY entry's name against this list,
// independent of bIndependentlySelectable — an entry left out here reads as "not selected" and turns
// a would-be full application into a partial one even though it always runs regardless.
std::vector<std::string> SelectedMigrationNames(const MigrationReconciliationDialogState& state);

// One frame's outcome — ConfirmDialogChange's own shape (ConfirmDialog_UI.h). Two exit actions only
// (ruling 3): the load already happened before this dialog can open, so there is no third "load and
// apply" action.
struct MigrationReconciliationDialogChange {
    bool bApplyClicked = false;
    bool bCloseClicked = false;
};

// `identifier` is the imgui popup id (ConfirmDialog_UI precedent), distinguishing multiple such
// dialogs coexisting in the same frame. Call unconditionally, every frame the caller wants the dialog
// reachable: it opens the popup the frame `state.bOpenRequested` is true and clears the flag, then
// draws nothing further once the popup itself has closed.
MigrationReconciliationDialogChange DrawMigrationReconciliationDialog(
    const char* identifier, MigrationReconciliationDialogState& state);

} // namespace Ui
} // namespace SanmapGen
