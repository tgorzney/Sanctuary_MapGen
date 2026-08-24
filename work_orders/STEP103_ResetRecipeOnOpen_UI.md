# STEP103 — Open resets the live recipe/fields/baked-image cache before parsing the new document

**Layer:** UI (`RunOpenSanmap`, `src/ui/FilesTab_Actions_UI.cpp`), with PARAMS
(`Params::MapRecipe`) and DATA (`Data::MapFields`, `Data::BakedLayerImage`) as the
state being scoped. **Domain:** the Open action's own state ownership, upstream of
STEP99/100/101/102's baked-layer mechanism. **Sequence:** depends on STEP101 (the
decomposition this bug defeats) and STEP102 (the same `assembler.BakedLayerImages()`
cache).

## Root problem
Confirmed by direct read of `src/ui/Application_UI.cpp`: `Application`'s live
`recipe` member is constructed as `MakeDefaultMapRecipe()`
(`src/ui/Application_Recipe_UI.cpp`), which calls `AddDefaultLayerStack` — pushing
one `"Terrain"` `GeoLayer` with two non-baked procedural noise `Layer`s
(`stratumIndex 0` and `defaultDetailStratumIndex`) into `recipe.layerStack.geoLayers`
at app startup, before any file is opened. `assembler(recipe)` binds by reference to
this same object; `DrawFilesTab` (`Application_PanelSystem_UI.cpp`) is handed that
same live `recipe`, `&assembler.Fields()`, and `&assembler.BakedLayerImages()`.

`RunOpenSanmap` (`FilesTab_Actions_UI.cpp`) calls `Io::MapImporter::LoadSanmap`
directly on that live `recipe` — no reset. `LoadSanmap` -> `ParseSanmapJsonText` ->
`ReadHeightmapStackJson` (`MapImporter_HeightmapStack_IO.cpp`) only writes
`outLayerStack` when a `HeightmapStack` key is present in the document; for a real,
externally-authored `.sanmap` with no such section, it no-ops, leaving the two
default noise layers sitting in `recipe.layerStack.geoLayers` untouched.

That leftover non-empty `geoLayers` then defeats STEP101's own gate. Confirmed by
direct read of `DecomposeBakedHeightmapIntoLayers`
(`src/io/MapImporter_HeightmapDecomposition_IO.cpp`):

```cpp
if (!recipe.layerStack.geoLayers.empty()) {
    for (Params::GeoLayer& group : recipe.layerStack.geoLayers) {
        for (Params::Layer& layer : group.layers) {
            if (!layer.bBaked || !layer.bakedImagePath.empty()) continue;   // <-- both default
                                                                             //     layers are !bBaked
            ...
        }
    }
    return;   // never falls through to the fresh-synthesis branch below
}
```
Since `geoLayers` is non-empty (the two default noise layers), the function takes
the RE-HYDRATION branch, not the FRESH-SYNTHESIS branch. Both default layers have
`bBaked == false`, so the `continue` fires for both — the loop body never executes,
`outBakedLayerImages` stays empty, and the two default procedural noise layers are
returned completely unmodified. `NoiseBlendStage` then generates procedural noise
from those two layers on the next `Refresh()`, silently overwriting the imported
terrain. This is the exact, confirmed root cause of "opening a real map still shows
procedural noise."

**Second, related bug found while tracing this (confirmed real, same fix commits
it):** `assembler.BakedLayerImages()` is a single persistent
`std::vector<Data::BakedLayerImage>` owned by `GenerationAssembler` that survives
across Opens. Once the `geoLayers.empty()` reset below makes the FRESH-SYNTHESIS
branch run, `DecomposeBakedHeightmapIntoLayers` mints new `layerIdentifier`s
starting at 0 (`Params::NextLayerIdentifier` on an empty stack) and `push_back`s new
entries — it never clears the vector first. `Data::FindBakedLayerImage`
(`src/data/BakedLayerImage_DATA.h`) is a **linear scan that returns the FIRST
match**. If a previous file's Open (or STEP102's Import RAW/Bake action) left stale
entries in `assembler.BakedLayerImages()` with the same low `layerIdentifier`s
(routine, since every fresh recipe's counter restarts at 0), the stale entry sorts
first and `NoiseBlendStage` silently paints the PREVIOUS file's frozen pixels onto
the NEW file's layer — a second, independent bleed-through bug, not hypothetical:
it reproduces on "Open file A, then Open file B" with both A and B lacking
`HeightmapStack` sections.

**Broader staleness risk (confirmed, scoped down for this ticket):** every reader
in `MapImporter_ParseDocument_IO.cpp` is individually gated on "only touch this
field if its key is present" — `HeightmapStack` is just one instance. A pre-Open
reset of the ENTIRE live `recipe` closes this gap for the Open case generically:
once `recipe` starts from `Params::MapRecipe()` before any reader runs, "key absent
in the imported document" now correctly means "stays at the fresh-recipe default,"
never "stays at whatever the previous session had." No per-reader gating changes
are needed or in scope.

## Fix
`Params::MapRecipe` (`src/params/MapRecipe_PARAMS.h`) is confirmed to be a flat
aggregate of recipe content ONLY — geometry, layer stack, strata, markers, water,
atmosphere, scenarios, etc. There is no app-level/session-only setting mixed into
it (those live on `Application`/`FilesTabState` separately). A full-struct reset
(`Params::MapRecipe()`) is therefore correct and requires no per-field carve-out.

Rewrite `RunOpenSanmap` (`src/ui/FilesTab_Actions_UI.cpp`) to build fresh scratch
state, run the load against the scratch state, and commit the scratch state onto
the live `recipe`/`fields`/`*outBakedLayerImages` **only when the load actually
succeeds** — never a blind unconditional wipe of the live session on a
refused/failed Open. This matches the file's own stated law in its header comment:
*"Every action is REFUSED with a logged reason rather than half-done"* (Constitution
§6). A failed Open (bad path, unreadable file, unparsable JSON) must leave the
previously-open map exactly as it was — resetting first and only THEN discovering
the path doesn't resolve would destroy a designer's live work for nothing, which is
a strictly worse regression than the bug being fixed.

```cpp
bool RunOpenSanmap(FilesTabState& state, Params::MapRecipe& recipe, Data::MapFields* fields,
                   std::vector<Data::BakedLayerImage>* outBakedLayerImages) {
    if (state.sanmapPath.empty()) {
        AppendFilesTabLog(state, "Open refused: no .sanmap file or map folder is set.");
        return false;
    }
    Io::MapImportOptions options = state.importOptions;
    options.bLoadBakedFields = state.bLoadBakedFieldsOnImport;
    if (state.unknownImportData != nullptr) *state.unknownImportData = Io::UnknownImportBag();

    // A full "open a NEW file" replaces the ENTIRE live recipe/fields/baked-image cache, not a
    // merge onto whatever the previous session/file left behind (the bug this fixes: leftover
    // default-seeded/previous-file GeoLayers survived every field-presence-gated reader). Built
    // on SCRATCH state, never in-place on the live objects, so a refused/failed Open (bad path,
    // unreadable, unparsable) leaves the caller's live session untouched — same "refuse with a
    // reason, never half-done" posture this file's header already documents.
    Params::MapRecipe scratchRecipe;                       // Params::MapRecipe() — NOT
                                                             // MakeDefaultMapRecipe(); a fresh
                                                             // open must not re-seed the default
                                                             // noise layers this fix removes.
    Data::MapFields scratchFields;                          // fresh/unsized (IsSized() == false)
    std::vector<Data::BakedLayerImage> scratchBakedImages;  // fresh/empty — no stale layerIdentifier
                                                             // entries a previous file could have
                                                             // left in assembler.BakedLayerImages()

    const Io::MapImportResult result =
        Io::MapImporter::LoadSanmap(state.sanmapPath, scratchRecipe, fields != nullptr ? &scratchFields : nullptr,
                                    options, state.unknownImportData, state.templateIngestReport,
                                    &scratchBakedImages);
    AppendFilesTabLog(state, result.debugLog);
    state.bLastOpenHadNoVersionMarker = result.bNoVersionMarkerFound;

    if (result.bSucceeded) {
        recipe = std::move(scratchRecipe);
        if (fields != nullptr) *fields = std::move(scratchFields);
        if (outBakedLayerImages != nullptr) *outBakedLayerImages = std::move(scratchBakedImages);
    }
    return result.bSucceeded;
}
```
Verify the exact real current signature of `RunOpenSanmap`/`Io::MapImporter::LoadSanmap`
before implementing — the above is illustrative of the scratch/commit-on-success
shape, not necessarily byte-exact against the current real parameter list.

Notes on the exact commit gate: `result.bSucceeded` is set the moment
`ParseSanmapJsonText` (the recipe parse) succeeds, **before** `LoadBakedFields`
runs — a document that parses but has a missing/corrupt `heightmap.raw` still
counts as a successful Open today (logged warning, not a refusal). The
scratch-then-commit design preserves that exact existing semantic; it only changes
WHAT gets committed (a fresh baseline merged onto, not the stale live objects merged
onto) and WHEN nothing is touched (never, on an actual refusal).

This intentionally diverges from `unknownImportData`'s existing unconditional
eager-reset (`*state.unknownImportData = Io::UnknownImportBag()`, run before the
path is even resolved) — that reset's blast radius is a scratch JSON bag, not a
designer's entire authored recipe; wiping it eagerly on a since-refused Open is
low-cost. Wiping the whole recipe/fields/baked-cache eagerly is not — hence the
scratch/commit-on-success split for those three, while leaving the existing
`unknownImportData` line exactly as-is.

## Files touched
- `src/ui/FilesTab_Actions_UI.cpp` — `RunOpenSanmap` body only, per above. No
  signature change, so `FilesTab_Draw_UI.cpp`, `FilesTab_UI.h`, and
  `Application_PanelSystem_UI.cpp` are untouched — minimal blast radius.
- Verify `#include <utility>` is present for `std::move` if not already
  transitively included.
- Test coverage: extend the existing Files-tab round-trip test file that already
  exercises `FilesTabAction::OpenSanmap`, rather than a new file, unless the
  ARCH §1.5 file-size ceiling forces a split.

## ARCH rules invoked
- Constitution §6, cited verbatim in this file's own header comment: *"Every action
  is REFUSED with a logged reason rather than half-done"* — the reason the fix
  commits on success only, never a blind pre-wipe.
- STEP99/100's single-writer/stable-identity model for `Data::BakedLayerImage`
  (keyed by `Params::Layer::layerIdentifier`, NOT by flat stack position) is exactly
  why a stale entry with a colliding identifier is a real correctness bug, not
  cosmetic: `FindBakedLayerImage` has no way to know an entry belongs to a
  different, previously-open document.
- No PROC/PIPELINE algorithm changes; no dirty-hash/stage-order changes. The
  existing `previewDriver->RequestMapUpdate()` call already made unconditionally
  after a successful Open is what forces the next `Refresh()` to fully regenerate
  every field from the reset recipe — this ticket does not need to touch PIPELINE
  invalidation at all.

## Explicit out-of-scope
- **`RunSelectiveMigrationImport`** (`src/ui/FilesTab_MigrationImport_Actions_UI.cpp`,
  the "Apply Selected" migration-reconciliation path) is a DIFFERENT, surgical merge
  operation — by design it parses onto the SAME already-live `recipe`/`fields` a
  prior Open already populated from the same file, applying only the selected
  migrations on top. It has the identical unguarded-merge and unguarded-append
  shape, but resetting it would break its actual purpose (reconciling onto the
  currently-open document, not replacing it with an unrelated one). Not touched by
  this ticket. If its own staleness turns out to be a real problem, it needs its
  own ticket reasoning about ITS correct merge semantics, not this one's
  reset-to-baseline semantics.
- The TGA channel/array-index off-by-one flagged by STEP101 (still open, still
  routed to IO Architecture/Format Expert — untouched here).
- No change to `MakeDefaultMapRecipe`/`AddDefaultLayerStack`
  (`Application_Recipe_UI.cpp`) — the default noise layers are correct and wanted
  for a brand-new, never-opened session; the bug is that Open never clears them,
  not that they exist.
- No change to `ParseSanmapJsonText`/`ReadHeightmapStackJson` or any other
  per-domain reader in `MapImporter_ParseDocument_IO.cpp` — every one of them stays
  exactly as field-presence-gated as it is today; the reset closes the staleness gap
  from the OUTSIDE (Open resets first), not by changing reader behavior.
- `Data::MapFields` gains no new reset/clear member function; `Data::MapFields()`
  (default-constructed, `IsSized() == false`) is used directly as the fresh
  baseline — no new DATA API surface.

## Acceptance test
1. **The reported bug, end to end:** start from `Application`'s real startup state
   (`recipe == MakeDefaultMapRecipe()`, i.e. `layerStack.geoLayers` has the one
   `"Terrain"` group with 2 non-baked layers). Open a synthetic `.sanmap` with no
   `HeightmapStack` section, a real `heightmap.raw`, and a one-stratum TGA (same
   fixture shape as STEP101's own acceptance test). After Open: `recipe.layerStack
   .geoLayers` has exactly ONE group (`"Imported Bake"`), every layer `bBaked ==
   true`; the original two default procedural noise layers are gone entirely.
   Running `assembler.Run()` once reproduces the imported heightfield, not
   procedural noise (the actual user-visible symptom, now fixed).
2. **BakedLayerImages no longer bleeds across files:** pre-seed
   `assembler.BakedLayerImages()` with a sentinel entry (`layerIdentifier == 0`,
   a distinguishable non-zero pixel pattern) as if left by a prior Open. Open the
   fixture from (1). Assert the resulting `assembler.BakedLayerImages()` contains
   ONLY the new file's entries (size matches exactly what fresh synthesis produces)
   and `layerIdentifier 0`'s image is the NEW file's stratum-0 contribution, not the
   sentinel.
3. **A refused/failed Open never touches live state:** seed `recipe`/`fields`/
   `assembler.BakedLayerImages()` with distinguishable non-default content (e.g. a
   hand-built recipe with a marked field). Call `RunFilesTabAction(OpenSanmap, ...)`
   with `state.sanmapPath` pointing at a nonexistent file. Assert it returns `false`
   AND `recipe`/`*fields`/`*outBakedLayerImages` are byte-identical to what they were
   before the call — no partial reset, no half-done state.
4. **Checkbox-off is unaffected in kind, only in what it now correctly shows:** with
   `state.bLoadBakedFieldsOnImport == false`, Open the same no-`HeightmapStack`
   fixture. `recipe.layerStack.geoLayers` ends up genuinely empty (no default noise
   layers, no synthesized baked layers either — the checkbox explicitly opted out of
   texture loading). This is expected, correct post-fix behavior, not a regression —
   call this out in the PR/verify notes so a tester doesn't mistake a flat preview
   for a new bug.
5. Existing Files-tab round-trip test case (Open -> re-export) stays green with no
   changes beyond the new cases above — a normal SanGen-authored `.sanmap`
   (non-empty `HeightmapStack`, RE-HYDRATION branch) must Open exactly as before.

## Verify
Full solo rebuild + `ctest -C Debug`: previously-passing suite stays green. No file
outside `src/ui/FilesTab_Actions_UI.cpp` (plus the extended test file) is edited.
No manual/interactive verification — automated test binaries only, per the
project's standing testing law.
