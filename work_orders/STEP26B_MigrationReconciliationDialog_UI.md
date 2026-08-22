# STEP26B — Migration reconciliation dialog (Files tab)

**Layer:** UI. **Domain:** Files tab. **Depends on:** `STEP26A` (the IO-layer preview/apply
functions and `MapImportResult::bNoVersionMarkerFound` this ticket calls). **Consulted:** SanGen
UI Expert (this session).

## Problem
When a `.sanmap` has no version marker at all, SanGen today silently reads it with current-shape
keys only rather than guessing an old version and risking a corrupting blind walk. This dialog
lets a human explicitly preview what a migration walk would find and selectively apply it,
instead of the tool silently skipping recoverable data.

## Ruling (UI Expert consult)
1. **Never automatic.** Auto-popping this on every no-marker Open directly fights STEP43's
   single-click goal. Open runs exactly as it does today, zero added friction. Add one new
   button, **"Check for Migrations…"**, in `DrawOpenSection` (`FilesTab_Draw_UI.cpp`) next to the
   existing Open button — disabled by default, enabled only when the last completed Open reported
   `MapImportResult::bNoVersionMarkerFound == true` (new `FilesTabState::bLastOpenHadNoVersionMarker`,
   copied over inside `RunFilesTabAction`'s `OpenSanmap` branch). Because the file is already
   loaded (current-shape-only) by the time this button is clickable, the dialog is a **post-load
   review**, never a load gate.
2. **Content — a plain checklist grouped by manifest step, not a new list-widget type.** One
   group per `MigrationPreviewStep` (header "Version N → N+1"), then per entry:
   - `bIndependentlySelectable && bLosslessIfSkipped` entries: individual `Checkbox_UI` rows
     (name + description), default checked.
   - Every other entry: shown as a disabled/greyed informational row (name + description,
     no checkbox) — it always runs whenever the step runs at all. Do not offer a checkbox for an
     entry that fails either test (STEP26A's ratified gating law).
   - Compose from existing atoms — a scrolling `ImGui::BeginChild` (precedent: `DrawLogSection`)
     with `Checkbox_UI` rows and plain `ImGui::Separator`/`TextUnformatted` step headers. No new
     widget-library primitive; manifest scale (single-digit steps, low entry counts) doesn't
     warrant `VirtualList<T>`, and `DraggableList<T>` is actively wrong — manifest order is
     load-bearing law, must never look reorderable.
3. **Two exit actions, not four** (the load already happened before this dialog can open):
   - **"Apply Selected"** (primary) — re-derives the document from `state.sanmapPath`, calls
     `Io::ApplySelectedSanmapMigrations` with the checked entry names (pre-checked = "apply all"
     is just the default state, no separate button needed) plus every non-selectable/non-lossless
     entry implicitly, then re-runs `MapImporter::ParseSanmapJsonText` on the mutated document as
     a **fresh, independent call** — its result **replaces** the original direct-read result
     entirely (recipe/fields/`unknownImportData`), never merged. On success, call
     `previewDriver->RequestMapUpdate()`, matching `DrawOpenSection`'s existing Open behavior.
   - **"Close"** (secondary) — dismiss, no re-read, no mutation. Already-loaded recipe/fields
     stand untouched.
   - `ConfirmDialog_UI` is NOT reused — it's locked to a fixed 2-button OK/Cancel contract with
     wording that doesn't fit a selective-apply flow. Build a new small dialog type following the
     same structural pattern (pure options/state/change structs, caller-owned one-per-site state,
     `UI_FRAMEWORK_SPEC.md`'s "THE SPLIT").
4. **New `FilesTabState` fields** (parallel to, not reusing, the existing blueprintPath
   `confirmDialogState`/`pendingConfirmAction` block — different payload shape):
   ```cpp
   MigrationPreviewDialogState migrationDialogState;   // new widget's own open/selection state
   bool bLastOpenHadNoVersionMarker = false;           // gates the "Check for Migrations…" button
   ```
5. **New headless action function**, alongside `RunFilesTabAction` in `FilesTab_Actions_UI.cpp`
   (not folded into the `FilesTabAction` enum — that's a bare enum with no per-call payload today;
   this action carries a selection list, a different shape):
   ```cpp
   bool RunSelectiveMigrationImport(FilesTabState& state, Params::MapRecipe& recipe,
                                    Data::MapFields* fields,
                                    const std::vector<std::string>& selectedMigrationNames);
   ```
   Called from the dialog's "Apply Selected" click handler, the same way
   `DrawPendingBlueprintWarningDialog` calls `RunFilesTabAction(..., /*acknowledged=*/true)`.

## Files touched
- `src/ui/FilesTab_UI.h` — new `FilesTabState` fields, `RunSelectiveMigrationImport` declaration
- `src/ui/FilesTab_Actions_UI.cpp` — `RunSelectiveMigrationImport` implementation
- `src/ui/FilesTab_Draw_UI.cpp` — "Check for Migrations…" button in `DrawOpenSection`, new
  `DrawMigrationReconciliationDialog` called unconditionally every frame (same pattern as
  `DrawPendingBlueprintWarningDialog`)
- New `src/ui/MigrationReconciliationDialog_UI.h`/`.cpp` — the dialog widget itself

## Explicit out-of-scope
- The "old, in-range version" and "newer than current" migration cases — both stay fully
  automatic, unchanged. This dialog is scoped ONLY to the no-version-marker case
  (`IO_MIGRATION_SPEC.md` §6, ratified, not reopened by this ticket).
- Any diff-rendering UI for `MigrationPreviewEntry::diffPatch` beyond showing `bWouldChangeDocument`
  as a simple indicator — a full JSON-patch visualizer is a separate, later enhancement if wanted.

## Verify
Full solo rebuild + `ctest -C Debug`, full suite green. This is UI composition — the underlying
`ApplySelectedSanmapMigrations`/`PreviewSanmapMigrationWalk` correctness is STEP26A's test
responsibility, not re-tested here. Manual on-screen confirmation of the dialog is the human's
own job (no interactive testing by AI).
