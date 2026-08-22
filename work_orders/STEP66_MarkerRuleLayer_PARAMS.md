# STEP66 — Procedural marker layer (`MarkerRuleLayer`) + `MarkersStack` IO rewrite

**Layer:** PARAMS, IO. **Domain:** `MarkerRule`, the new `MarkerRuleLayer`/`SymmetrySetting` types,
`MarkersStack` wire shape. **Sequence:** the procedural-side counterpart to
`STEP60_MarkerInstanceLayer_PARAMS.md` (manual side). Implements `ARCH_16_01_NewParamsShapes.md` §16.1 +
`SANMAP_FORMAT_SPEC.md` Correction 15 — both already ratified this session; this ticket is the
first real code against them. No dependency on other undone work-orders.

**⚠️ Enabled semantics, ruled by the human (binding for this ticket and for
`Placement_Rules_PROC.cpp`'s consumer ticket, not yet drafted):** disabling a *preview* layer is
visual-only. Disabling an actual generation layer (this ticket's `MarkerRuleLayer`, and the same
family as height/terrain layers) means it is **not calculated at all** — a real generation gate,
not a display toggle — but stays stored, ready to re-enable. `MarkerRuleLayer::bEnabled` therefore
carries the identical generation-gating semantic `MarkerRule::bEnabled` already has today, just
promoted one tier. `bHidden` is unchanged: still generates (clearance/fairness), just doesn't
render.

## Root problem
`Params::MarkerRule` currently carries its own `bSymmetryUseGlobal`/`symmetryMask`/
`radialSymmetryRepeatCount` (confirmed live: `src/params/MarkerRule_PARAMS.h`,
`MapExporter_MarkersStack_IO.cpp:46-48`, `MapImporter_MarkersStack_IO.cpp:49-52`), and
`Params::MapRecipe::markerRules` is a flat `std::vector<MarkerRule>` with no layer tier at all —
unlike `HeightmapStack`'s `GeoLayer`/`Layer` two-level model. ARCH_16_01_NewParamsShapes.md §16.1 moves the symmetry triplet
up one tier onto a new wrapping type; this ticket implements that move end to end (PARAMS + wire).

## Target files
- `src/params/Symmetry_PARAMS.h` — new `SymmetrySetting` struct.
- `src/params/MarkerRule_PARAMS.h` — remove the 3 symmetry fields from `MarkerRule`; add
  `MarkerRuleLayer`.
- `src/params/MapRecipe_PARAMS.h` — `markerRules` → `markerRuleLayers`
  (`std::vector<MarkerRuleLayer>`).
- `src/io/MapExporter_MarkersStack_IO.cpp` — `BuildMarkersStackJson` restructured to the two-level
  shape; `BuildMarkerRuleJson` loses its 3 symmetry-key writes; new `BuildMarkerRuleLayerJson`.
- `src/io/MapImporter_MarkersStack_IO.cpp` — `ReadMarkersStackJson` restructured to match;
  `ReadMarkerRuleJson` loses its 3 symmetry-key reads; new `ReadMarkerRuleLayerJson`.

## Layer & accuracy class
PARAMS + IO/BRIDGE. Accuracy class: Exact.

## Backend policy
CPU only.

## ARCH rules invoked
- `ARCH_16_01_NewParamsShapes.md` §16.1 — the ratified PARAMS shape, binding, verbatim below.
- `SANMAP_FORMAT_SPEC.md` Correction 15 — the ratified wire shape, binding, verbatim below.
- Constitution §2 — no abbreviations (`SymmetrySetting`, not `SymSettings`).

## Solution — shape

```cpp
// Symmetry_PARAMS.h — new, alongside the existing SymmetryAxis/etc.
struct SymmetrySetting {
    bool bSymmetryUseGlobal = true;
    int  symmetryMask       = 0;
    int  radialSymmetryRepeatCount = 3;
};
```
```cpp
// MarkerRule_PARAMS.h
struct MarkerRule {
    // ... every existing field UNCHANGED except the 3 removed below ...
    // REMOVED: bool bSymmetryUseGlobal; int symmetryMask; int radialSymmetryRepeatCount;
};

struct MarkerRuleLayer {
    std::string name;
    bool bEnabled = true;    // generation gate — see the ruling above, not a UI-only flag
    bool bHidden  = false;   // still generates (clearance/fairness), doesn't render — unchanged semantic
    SymmetrySetting symmetry;
    std::vector<MarkerRule> rules;
};
```
`MapRecipe::markerRules` (`std::vector<MarkerRule>`) → `MapRecipe::markerRuleLayers`
(`std::vector<MarkerRuleLayer>`).

**Wire shape** (Correction 15, `Rules` key ruled by the Format Expert — bare plural of the
contained type's role, same pattern `HeightmapStack`'s `GeoLayers.Layers[]` already establishes):
```
MarkersStack → [ N × {
    Name                       (string)
    Enabled                    (bool)   // "b" dropped, this array's established casing
    Hidden                     (bool)
    SymmetryUseGlobal          (bool)   // SymmetrySetting, flattened as sibling keys
    SymmetryMask               (int)
    RadialSymmetryRepeatCount  (int)
    Rules                      ([...])  // each element = today's per-rule shape, MINUS the 3
                                         // removed symmetry keys — every other field unchanged
} ]
```
`GlobalMarkerSettings` (a separate top-level key, ARCH_11_GlobalMarkerSettings.md §11) is untouched by this ticket — its
exporter/importer functions in the same two files are not modified.

**Exporter** (`BuildMarkersStackJson`): iterate `recipe.markerRuleLayers`; per layer, build
`{Name, Enabled, Hidden, SymmetryUseGlobal, SymmetryMask, RadialSymmetryRepeatCount}` from
`layer.name`/`layer.bEnabled`/`layer.bHidden`/`layer.symmetry.*`, then `"Rules"` = an array of
`BuildMarkerRuleJson(rule)` for `rule : layer.rules` (that function unchanged except deleting its
3 symmetry-key lines).

**Importer** (`ReadMarkersStackJson`): replace the current `ReadRuleArray(document, "MarkersStack",
outRecipe.markerRules, ReadMarkerRuleJson)` one-liner with a two-level walk: for each element of
`document["MarkersStack"]`, construct a `MarkerRuleLayer`, read `Name`/`Enabled`/`Hidden`/
`SymmetryUseGlobal`/`SymmetryMask`/`RadialSymmetryRepeatCount` (the last via the existing
`ReadJsonIntegerClamped` + `Params::radialSymmetryRepeatCountMinimum/Maximum` pattern, relocated
from the old per-rule read), then walk `"Rules"` with the existing `ReadRuleArray` helper
(unchanged) into `layer.rules`, using `ReadMarkerRuleJson` with its 3 symmetry-key reads deleted.
Push the layer onto `outRecipe.markerRuleLayers`.

## Explicit out-of-scope
- **`Placement_Rules_PROC.cpp`/`Placement_Hash_PROC.cpp`** — the PROC-layer consumer update
  (walking `markerRuleLayers` instead of `markerRules`, reading `layer.symmetry.*`) is a separate,
  already-scoped ticket (Generator Expert consult, not yet drafted as a STEP file) — this ticket
  only changes the PARAMS shape and its IO round-trip; it does not touch `src/proc/`. Those files
  will fail to compile against this ticket's new shape until that follow-up lands — **land them
  together or in immediate sequence**, not as an isolated merge.
- **`MarkersStack_Migrate_V3` (old-file backward compatibility)** — separate ticket (IO
  Architecture Expert design, ready to draft), must land **after** this ticket, not bundled with it.
- **`symmetryGroupIdentifier`, drag-follow, or any manual-marker (`recipe.markers`) change** —
  unrelated domain (manual instances, not procedural rules); see `STEP60`/the not-yet-drafted
  linkage ticket instead.
- **Any UI tab change.** `MarkersTab_Rules_UI.h`/`.cpp` currently draw per-rule symmetry controls
  (`DrawPlacementSymmetryAxes` called per rule) — those call sites will need to move to the layer
  level to keep compiling meaningfully, but that's UI-tab work, not this ticket's file set. Flagged
  so it isn't discovered as a surprise build break with no ticket covering the fix.

## Acceptance test
A `Params::MapRecipe` with 2 `MarkerRuleLayer`s (different symmetry settings, 2+ rules each,
including non-default `HydroMultiplier`/etc. per-rule fields) round-trips exactly through
`MarkersStack`. Confirm no `MarkerRule` JSON object in the exported document contains
`SymmetryUseGlobal`/`SymmetryMask`/`RadialSymmetryRepeatCount` (grep the JSON text, not just call
sites). Confirm the layer's own three symmetry keys appear once, at the layer level. Full
`SanGenV2` build does **not** need to stay clean by itself — per the explicit out-of-scope note,
`src/proc/` will not compile until the companion PROC ticket lands; state this plainly in the PR/
commit rather than treating a red build as a regression in this ticket alone.
