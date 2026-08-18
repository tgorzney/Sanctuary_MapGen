# Work-Order — Step 5: blueprintPath validation + confirm dialog + live props/decals export

*Constitution §7. Executor: SanGen Coder. Spans PARAMS/IO/UI — unusual for this ticket family,
but the pieces are not independently useful (IO validation with nothing to surface it does
nothing; a dialog with no predicate to call has nothing to show). Synthesizes three prior design
rounds: Format Expert (blueprintPath ground truth + `SanpackReader` gap), IO Architecture Expert
(final plumbing — read this work-order's "Finalized IO plumbing" section, it reconciles two
earlier, different proposals from that same expert into one), and UI Expert (`ConfirmDialog_UI`
widget + Files-tab flow). Human-ratified UX: on an unresolved `blueprintPath`, show a warning
dialog explaining the runtime risk; OK exports anyway (designer's call), Cancel aborts the export
entirely, nothing written.*

## Root problem
`work_orders/STEP4_PropsDecals_IO.md` (shipped) built `PropInstanceGroup`/`DecalInstanceGroup`
PARAMS types and pure JSON round-trip functions, but deliberately left them unwired —
`document["props"]`/`document["decals"]` still write empty arrays in the live exported document.
The reason: `ENTITY_AUTHORING_PARAMS_SPEC.md`'s flagged item 1 and the now-corrected
`SANMAP_FORMAT_SPEC.md` (`SPEC-1_PropFormatCorrections_DOCS.md`, applied this session) both
confirm a single unresolvable `blueprintPath` doesn't just fail to load that one prop — it calls
`Engine.Error()` in the live game's Lua loader (`mapUtils.lua:107`), aborting the rest of
`RunMapSetup` and silently killing everything parsed after `props`, including the `markers` block
(spawns/mexes). This ticket closes that gap the way the human ruled: warn, don't silently drop or
silently block.

## Target files

**New:**
- `src/io/MapExporter_BlueprintValidation_IO.cpp` — `ValidatePropAndDecalBlueprintPaths`,
  `BlueprintValidationReport`.
- `src/ui/ConfirmDialog_UI.h`/`.cpp` — the new generic, reusable confirm-dialog widget.

**Modified:**
- `src/io/SanpackReader_IO.h`/`.cpp` — new `SanpackReader::HasEntry(entryName)` const method.
- `src/io/MapExporter_IO.h` — `BlueprintValidationReport` struct declaration; `ExportSanmapOnly`/
  `ExportAll` gain a trailing `const SanpackReader* assetPack = nullptr` parameter; declare
  `ValidatePropAndDecalBlueprintPaths`. **File-size ceiling warning (ARCH Expert flag):** this
  file is already 142 lines against ARCH §1.5's soft-100/hard-150 ceiling before this ticket's
  additions. If the new struct + declarations + an updated SCOPE NOTE 1 push it past 150, trim
  SCOPE NOTE 1's prose rather than silently drifting over — or take the §1.5 documented-exception
  path if a trim genuinely isn't enough. Do not discover this mid-edit; check line count as you go.
- `src/io/MapExporter_IO.cpp` — wire the internal warn-and-log safety net into both export
  functions.
- `src/io/MapExporter_Recipe_IO.cpp` — `BuildSanmapJsonText`: replace the two empty-array literals
  with real `BuildPropsJson`/`BuildDecalsJson` calls, add `document["PropGroups"]`/
  `document["DecalGroups"]`.
- `src/io/MapImporter_IO.cpp` — `ParseSanmapJsonText`: add `ReadPropGroupsJson`/`ReadPropsJson`/
  `ReadDecalGroupsJson`/`ReadDecalsJson` calls, in that order (Groups before instances — the
  `layerIndex` clamp needs `propLayers`/`decalLayers` populated first).
- `src/io/MapExporter_IO_Test.cpp` — correct the now-stale "props/decals deliberately not yet
  called from here" comment.
- `src/io/MapImporter_PropsDecals_IO_Test.cpp` — correct its now-stale "bypasses the live path"
  header comment (it remains valid deep/edge-case coverage; it's just no longer the ONLY coverage).
- `src/io/MapImporter_IO_Test.cpp`/`MapFormat_TestSupport_IO.h` — `BuildPopulatedRecipe` gains a
  `FillFixturePropsAndDecals`, `RunRoundTripTests` gains a `CheckPropsAndDecals`, mirroring
  `CheckArmiesAndAreas`/`CheckMarkersAndChains`'s exact style.
- `src/ui/FilesTab_UI.h` — `FilesTabState` gains `const Io::SanpackReader* assetPack = nullptr;`
  plus the pending-confirmation state (dialog state, stashed pending action, cached warning text).
- `src/ui/FilesTab_Actions_UI.cpp` — `RunRecipeExport` passes `state.assetPack` as the trailing
  argument to `ExportSanmapOnly`/`ExportAll`. **`RunFilesTabAction`'s signature and headless
  contract do NOT change** — `FilesTab_UI_Test.cpp` calls it directly and that must keep working
  unmodified.
- `src/ui/FilesTab_Draw_UI.cpp` — the new pre-check → dialog → deferred-commit flow, gating ONLY
  the `ExportSanmapOnly`/`ExportAll` buttons (never the four texture-only exports — they carry no
  blueprintPath data).
- `src/ui/Application_UI.h`/`Application_Assets_UI.cpp` — a new, long-lived `Io::SanpackReader`
  member, separate from `AssetAtlasCache`'s transient per-call one, opened and
  `ReadCentralDirectoryOnce()`'d whenever `sanpackPath` is (re)loaded, fed down into
  `FilesTabState::assetPack`. **Failure-handling rule (ARCH Expert catch, load-bearing):** feed the
  pointer down ONLY when both `Open()` AND `ReadCentralDirectoryOnce()` succeed for the current
  `sanpackPath`; on first launch (nothing loaded yet), on a failed load, or after `sanpackPath` is
  cleared, `state.assetPack` MUST be `nullptr`, not a pointer to an empty/unopened reader.
  `SanpackReader::HasEntry`'s contract ("unopened/unparsed answers false, never asserts") means a
  dangling-into-empty pointer wouldn't crash — it would instead report EVERY `blueprintPath` as
  unresolved and pop the confirm dialog on every single export, even for a designer who never
  loaded a sanpack at all. That is a direct violation of this ticket's own "assetPack == nullptr →
  zero behavior change" contract. Get this wrong and the feature is unusable for anyone without a
  pack loaded, not just imprecise.
- `sangen_arch_pack/specs/UI_FRAMEWORK_SPEC.md` — **not modified by the Coder.** The UI Expert
  flagged that `ConfirmDialog_UI` belongs in the "Universal widget library" list; that's a spec
  ratification for the ARCH Expert, not something the Coder edits directly. Flag it in the
  implementation report; the human/ARCH Expert applies it as a small follow-up.

## Layer & accuracy class
PARAMS/IO/BRIDGE + UI. Accuracy class: Exact for JSON shape; the validation itself is
best-effort/advisory by design (warn, never block) — see "Design ruling" below.

## Backend policy
CPU only.

## Design ruling — warn, never block, for EVERY caller, not just the dialog path
This is the load-bearing decision the whole ticket hangs on, human-ratified this session:
unresolved `blueprintPath` values are reported, never silently dropped and never used to refuse
an export. This applies uniformly:
- **UI path**: pre-check → if dirty, show `ConfirmDialog_UI` (OK = export anyway, Cancel = abort,
  nothing written) → on OK, call the unchanged `RunFilesTabAction`.
- **Any other caller** (tests, a future non-UI entry point) that passes a non-null `assetPack`:
  `ExportSanmapOnly`/`ExportAll` run the SAME validation internally as a safety net, but only
  **log** the finding — `result.bSucceeded` is never gated by it. This is not two different
  policies; it's one function (`ValidatePropAndDecalBlueprintPaths`) called from two places.
- **`assetPack == nullptr`** (every existing call site before this ticket, and any caller that
  doesn't have one): no validation attempted at all, zero behavior change from before this ticket.

## Finalized IO plumbing (reconciles two earlier, different proposals from the same expert —
this is the ruling, not a menu)
1. `ExportSanmapOnly`/`ExportAll` gain a trailing `const SanpackReader* assetPack = nullptr`
   parameter — **not** a path string on `MapExportOptions`. Reasoning: `BuildSanmapJsonText` must
   stay disk-free (a path field on `MapExportOptions` risks a future reader wiring it in "since
   it's right there"); and the UI pre-check and the internal safety-net gate must validate against
   the IDENTICAL parsed central directory, not two independent opens of the same archive — a
   pointer to an already-`Open()`+`ReadCentralDirectoryOnce()`'d reader, shared by both call sites,
   guarantees that. Default-argument `nullptr` keeps every existing call site (production and
   test) compiling and behaviorally unchanged — confirmed against every real call site in
   `FilesTab_Actions_UI.cpp` and both `*_IO_Test.cpp` files.
2. ```cpp
   struct BlueprintValidationReport {
       std::vector<std::string> unresolvedBlueprintPaths;  // literal strings, props then decals
       bool AllResolved() const { return unresolvedBlueprintPaths.empty(); }
       std::string SummaryText() const;   // ONE wording, shared by the UI dialog body AND the
                                           // IO debugLog line — do not duplicate the phrasing
   };
   // Pure/read-only against assetPack: assetPack MUST already be Open()+ReadCentralDirectoryOnce()
   // by the caller — this function touches no disk itself and is never called from inside
   // BuildSanmapJsonText (that stays a sibling pre-flight step, same tier as recipe.IsValid()).
   BlueprintValidationReport ValidatePropAndDecalBlueprintPaths(const Params::MapRecipe& recipe,
                                                                 const SanpackReader& assetPack);
   ```
   Lives in `MapExporter_BlueprintValidation_IO.cpp`, declared in the PUBLIC `MapExporter_IO.h`
   (not the module-internal `MapExporter_Recipe_IO.h`) — the UI layer must call it directly. This
   is a deliberate exception to the "one domain, one file pair" convention: it's cross-domain
   validation over both `props` and `decals` together, not a JSON builder/reader for either.
3. `SanpackReader::HasEntry(entryName)` — new, in `SanpackReader_IO.h`/`.cpp`:
   ```cpp
   // ONE exact, case-sensitive lookup against the ALREADY-PARSED central directory (same
   // "ReadCentralDirectoryOnce() is the caller's job" contract DirectoryEntries() already has —
   // does NOT parse on demand). An unopened/unparsed reader answers false, never asserts
   // (Constitution §6). blueprintPath strings are literal archive paths (SPEC-1) — never
   // normalized, never fuzzy-matched.
   bool HasEntry(const std::string& entryName) const;
   ```
   Implementation is a plain linear scan over `directoryEntries` — this runs once per unique
   `blueprintPath` (a GROUP-level field, not per-transform) on a human-triggered export click:
   tens of lookups against thousands of entries, not a hot path. No index needed.
4. Internal wiring in `MapExporter_IO.cpp` (same shape for both `ExportSanmapOnly`/`ExportAll`,
   inserted right after the existing `recipe.IsValid()` hard-gate, before `EnsureExportFolderExists`):
   ```cpp
   if (assetPack != nullptr) {
       const BlueprintValidationReport report = ValidatePropAndDecalBlueprintPaths(recipe, *assetPack);
       if (!report.AllResolved()) result.Log(report.SummaryText());   // warn-not-block
   }
   ```

## Live document wiring
Content-shape-only change — no `SanGenVersion` bump, no migration file (`props`/`decals`/
`PropGroups`/`DecalGroups` already validly exist as empty/absent in every prior version; this
changes values, not shape).
```cpp
// MapExporter_Recipe_IO.cpp, BuildSanmapJsonText:
document["decals"]      = BuildDecalsJson(recipe);
document["props"]       = BuildPropsJson(recipe);
document["PropGroups"]  = BuildPropGroupsJson(recipe);
document["DecalGroups"] = BuildDecalGroupsJson(recipe);
```
```cpp
// MapImporter_IO.cpp, ParseSanmapJsonText — order matters, Groups before instances:
ReadPropGroupsJson(document, outRecipe);
ReadPropsJson(document, outRecipe, result);
ReadDecalGroupsJson(document, outRecipe);
ReadDecalsJson(document, outRecipe, result);
```
Still fully disk-free — no `SanpackReader` involved anywhere in the pure builder/reader path.

## Confirm-dialog widget (UI Expert design, ratified)
New, generic, reusable — first consumer is this ticket, not a bespoke one-off (matches this
codebase's existing widget-library convention: `ColorSwatch_UI`/`Combo_UI` are generic too).
```cpp
// ConfirmDialog_UI.h
struct ConfirmDialogOptions {
    std::string title;
    std::string bodyText;                    // caller pre-formats; multi-line ok
    std::string primaryButtonLabel   = "OK";
    std::string secondaryButtonLabel = "Cancel";  // human-ratified: OK + Cancel, not OK-only
    bool        bClosableWithoutChoice = false;   // no ESC/backdrop/X — must click a button
};
struct ConfirmDialogState { bool bOpenRequested = false; };   // caller-owned, one per site
struct ConfirmDialogChange { bool bPrimaryClicked = false; bool bSecondaryClicked = false; };

ConfirmDialogChange DrawConfirmDialog(const char* identifier, ConfirmDialogState& state,
                                      const ConfirmDialogOptions& options);
```
`.cpp` follows `ColorSwatch_UI.cpp`'s established split (`WidgetHelpers_UI.h` "THE SPLIT"): calls
`ImGui::OpenPopup` the frame `bOpenRequested` is true, then unconditionally attempts
`ImGui::BeginPopupModal` and draws title/body/two buttons inside it.

## Files-tab flow (UI Expert design, ratified)
`RunFilesTabAction`/`RunRecipeExport` in `FilesTab_Actions_UI.cpp` stay **completely unchanged** —
still headless, still the single "just do it" call `FilesTab_UI_Test.cpp` drives directly. The
gate lives ONLY in the imgui draw path (`FilesTab_Draw_UI.cpp`, already the one non-headless layer
in this tab):
1. On an `ExportSanmapOnly`/`ExportAll` button click: run
   `ValidatePropAndDecalBlueprintPaths(recipe, *state.assetPack)` (skip entirely if
   `state.assetPack == nullptr` — no pack loaded, nothing to validate against, proceed as today).
2. Clean → call `RunFilesTabAction` exactly as today. **Zero added cost or behavior change for the
   healthy/common case.**
3. Dirty → do NOT export yet. Stash the pending action + `report.SummaryText()` into
   `FilesTabState`'s new confirm-state, set `bOpenRequested = true`, return without exporting.
4. Every frame, unconditionally: call `DrawConfirmDialog(...)`. On `bPrimaryClicked` (OK): call
   `RunFilesTabAction` for the stashed pending action right there, then clear the pending state. On
   `bSecondaryClicked` (Cancel): clear the pending state, nothing exports.
Applies ONLY to `ExportSanmapOnly`/`ExportAll` — never the four texture-only export buttons
(`ExportHeightmapRaw`/`SlopeImage`/`FlowImage`/`StratumMasks`), which carry no blueprintPath data
at all; do not widen the gate to cover them.

## Test coverage
1. `MapImporter_IO_Test.cpp`'s `BuildPopulatedRecipe`/`RunRoundTripTests` gain
   `FillFixturePropsAndDecals`/`CheckPropsAndDecals` — this now genuinely exercises the LIVE
   `BuildSanmapJsonText`/`ParseSanmapJsonText` path for props/decals for the first time (Step 4's
   test only exercised the pure builders directly, bypassing the live path — that path is now
   real). No `SanpackReader` involved; this fixture never calls `ExportSanmapOnly`.
2. New validation-specific coverage exercising `ValidatePropAndDecalBlueprintPaths`/
   `SanpackReader::HasEntry` against a synthetic sanpack — reuse the existing synthetic-archive
   test helper already used by `SanpackReader_IO_Test.cpp`/`AssetAtlasCache_IO_Test.cpp`
   (`AssetPipeline_TestSupport_IO.h`), not a hand-rolled one. Cover: all-resolved (silent pass),
   one unresolved path (report + logged warning, export still succeeds — confirms warn-not-block),
   and `assetPack == nullptr` (skipped entirely, identical to pre-ticket behavior).
3. `MapExporter_IO_Test.cpp`/`MapImporter_PropsDecals_IO_Test.cpp` — correct the now-stale header/
   assertion comments per "Target files" above; no behavioral changes to their existing assertions.
4. UI-side: `ConfirmDialog_UI`'s actual popup sequence is verified by eye (same posture
   `ColorSwatch_UI.cpp`'s picker popup already has — not headless-testable). The pure pre-check
   function (turning a `BlueprintValidationReport` into dialog body text) should be its own small,
   headless-testable function, independently verifiable without an imgui frame.

## Explicit out-of-scope
- **`UI_FRAMEWORK_SPEC.md` ratification** of `ConfirmDialog_UI` as an official widget-library
  entry — flag it in the completion report; the ARCH Expert applies it separately.
- **"Remember my choice" / don't-warn-again** — the UI Expert flagged this as unresolved and
  outside the current interaction model's precedent; not part of this ticket. Every export attempt
  with an unresolved path re-warns.
- **Async/threaded validation** — the UI Expert flagged whether the pre-check needs threading
  (vs. running synchronously on the click frame) as IO/UI-Optimization-Expert territory. Given
  `HasEntry` is a linear scan over tens of lookups (not thousands), implement synchronously; do
  not add threading speculatively.
- **`AssetAtlasCache`'s existing transient `SanpackReader`** — untouched; the new long-lived reader
  on `Application` is separate and does not replace or share lifecycle with it.
- **Props/Decals manual-layer UI** (`PropsTab_Manual_UI.h`'s `ManualPropGroup` retyping onto
  `Params::PropInstanceLayer`) — same UI-wiring exclusion every prior step in this family has had.

## Acceptance test
Full `SanGenV2` build stays clean (the pre-existing, unrelated `PreviewComposite_Wysiwyg_UI_Test.cpp`
build failure flagged separately this session is NOT this ticket's to fix, but should not be made
worse). `MapImporter_IO_Test.exe`/`MapExporter_IO_Test.exe`/`PropsDecals_IO_Test.exe` all pass,
including new assertions. The new validation test binary/coverage passes: all-resolved silent,
one-unresolved logs-and-still-succeeds, `nullptr` pack skips cleanly. `FilesTab_UI_Test.exe`
continues to pass unchanged (confirms `RunFilesTabAction`'s contract truly didn't move). Manual/by-eye
confirmation the dialog appears on an unresolved path and both OK and Cancel behave as designed —
note in the completion report if a fully automated check isn't feasible for the imgui popup itself.
