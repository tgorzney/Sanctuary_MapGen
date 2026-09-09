# STEP257 — Strategic-mode marker icons ignore `instanceScale` (confirmed rendering bug fix)

**Layer:** UI. **Domain:** `src/ui/MapCanvas_IconLayer_CullEmit_UI.cpp` (the fix — one branch of
`ResolveLodModeAndIcon`), `src/ui/IconAtlasPairing_UI.h` (new test-enabling setter),
`src/ui/MapCanvas_IconLayer_TestFixture_UI.h` (new test-fixture helper),
`src/ui/MapCanvas_IconLayer_Cull_UI_Test.cpp` (new acceptance test). **Executor:** SanGen Coder.
Pure rendering-bug fix — no PARAMS/resolver/UI-dial/composition change. Re-verified against the
current tree at authoring time (all citations below re-read, not carried over from the investigation
that found this bug).

**Perf sign-off (SanGen Compute Optimization Expert, this session):** confirmed GO — one additional
`float * float` multiply against an already-resident value in a branch that already does a table
lookup, a divide, and two multiplies is negligible at 100k+-entity scale, no benchmark needed.

## The bug (confirmed)

`ResolveLodModeAndIcon`, `src/ui/MapCanvas_IconLayer_CullEmit_UI.cpp:28-46`, computes a two-mode LOD
result from `instanceScale` (the caller's already-fully-composed
`transform.transform.scaleX * groupTypeScale * layerIconScale * selectedTypeScale`, see
`src/ui/MapCanvas_IconLayer_CullManual_UI.cpp:221-232`, which is correct and out of scope here).
The THUMBNAIL branch folds `instanceScale` into its size formula correctly (line 36). The STRATEGIC
branch — line 43 — does not:

```cpp
    ResolvedLod_UI resolved;
    if (thumbnailScreenSize >= layer.thumbnailLodThresholdPixels) {
        resolved.iconId = pairing.thumbnailIconId; resolved.screenSize = thumbnailScreenSize;
    } else {
        resolved.iconId = pairing.strategicIconId; resolved.screenSize = layer.strategicIconScreenSizePixels;   // BUG
    }
```

`layer.strategicIconScreenSizePixels` is a fixed per-layer constant (default `16.0f`,
`src/ui/OverlayLayer_Settings_UI.h:42`) with zero reference to `instanceScale`. Strategic mode is
the DEFAULT branch for most markers at typical zoom (`thumbnailLodThresholdPixels` defaults to
`5.0f`, `OverlayLayer_Settings_UI.h:38`), so this discards ALL of `instanceScale`'s contributors —
`scaleSelectedAlloy`/`scalePlasma`/`scaleSpawn` (the human-reported symptom), the base "Icon Scale
(Global)" dial, and any per-layer icon-scale override — whenever a marker is resolved in strategic
mode. `ResolvedLod_UI::screenSize` is written verbatim into `OverlayVisibleInstance::screenSize`
(`MapCanvas_IconLayer_CullEmit_UI.cpp:60`) and consumed directly as the drawn quad's half-size in
`FlushIconLayerBucket` (`src/ui/MapCanvas_IconLayer_Draw_UI.cpp:95`) — nothing downstream re-applies
scale.

**The fix — one line, `MapCanvas_IconLayer_CullEmit_UI.cpp:43`:**

```cpp
        resolved.iconId = pairing.strategicIconId; resolved.screenSize = layer.strategicIconScreenSizePixels * instanceScale;
```

Dimensionally/behaviorally correct: `instanceScale` is already the same dimensionless multiplier the
thumbnail branch applies one line above (line 36, `baseFootprint * instanceScale`), so multiplying
it into the strategic branch's fixed pixel constant is the identical "multiplicative no-op at 1.0"
convention this composition already uses everywhere else (`Params::ResolveMarkerGroupTypeScale`'s own
`1.0f` no-op default, `MarkerInstance_PARAMS.h`). A scale of `1.0` reproduces today's exact strategic
pixel size exactly — `x * 1.0f == x` is exact under IEEE754, so no existing unscaled strategic-mode
visuals shift.

**Do NOT touch:**
- `GlobalMarkerSettings::scaleSelectedAlloy/Plasma/Spawn` (`src/params/GlobalMarkerSettings_PARAMS.h:29-31`)
- `ResolveMarkerGroupSelectedTypeScale` (`:88-92`)
- The "Icon Scale (Selected)" dial (`src/ui/MarkersTab_Globals_UI.cpp:107-109`, `:114-143`)
- The `bSelected`-gated composition into `instanceScale` in `ResolveMarkersManual`
  (`src/ui/MapCanvas_IconLayer_CullManual_UI.cpp:221-232`)
- The THUMBNAIL branch of `ResolveLodModeAndIcon` (line 36) — already correct
- The pairing-miss placeholder branch (`MapCanvas_IconLayer_CullEmit_UI.cpp:102-107`) — deliberately
  fixed-size by design (its own comment: "a generic 'unresolved' marker is not a real footprint the
  LOD math has any authored size for"), not the bug this ticket fixes
- `Ui::EmitCandidateIfVisible`/`Ui::AppendCandidate` — unchanged, correct
- `Picking_UI.cpp` — grep-confirmed zero references to `screenSize`/`ResolveLodModeAndIcon`; picking
  hit-radius is computed independently and is unaffected by this fix
- `BuildIconAtlasPairingLookup` (`IconAtlasPairing_UI.h:72-78`) — stays unchanged; every real
  production pairing's `strategicIconId` still resolves `kInvalidIconId` (no authored strategic-icon
  source exists yet, `ARCH_14_03_IconRenderingLod.md` §14.3, separate unscoped ticket) — this fix
  changes how an already-resolved strategic icon is SIZED, never which markers get one

## Test-coverage gap and the infrastructure needed to close it

`MapCanvas_IconLayer_Cull_UI_Test.cpp`'s `CheckManualMarkerSelectedScaleComposesOnlyWhenSelected`
(lines 925-954) is the STEP240-mandated selected-scale test, but the shared `ManualMarkerTestFixture`
(lines 577-624) pins `alloyLayer.thumbnailLodThresholdPixels = 1.0f` (line 609) — every fixture built
from it stays in the THUMBNAIL branch, the one branch that already worked. The file's only existing
strategic-mode tests, `CheckStrategicModeBelowThreshold` (lines 118-134) and
`CheckUnresolvedPropFallsBackToPlaceholder` (lines 50-73), both exercise a **pairing miss** (an
unresolved `strategicIconId`, `kInvalidIconId`) — `CheckStrategicModeBelowThreshold`'s own header
comment (lines 112-117) explains why: `IconAtlasPairingLookup` has no way to seed a real
`strategicIconId`. Confirmed by reading `IconAtlasPairing_UI.h:41-62` — `SetThumbnailIconId` is the
ONLY public mutator; `pairingsByTemplateIdentifier` is private and `strategicIconId` is otherwise only
ever set to its `kInvalidIconId` default.

This means the acceptance test this ticket needs — a REAL, resolved strategic-mode candidate whose
`screenSize` we can assert against — is **not writable against the current public API**. A small,
additive, test-enabling widening of `IconAtlasPairingLookup` is required first:

**1. New method, `src/ui/IconAtlasPairing_UI.h`, immediately after `SetThumbnailIconId` (`:45-47`):**

```cpp
    // STEP257 — the strategic-mode counterpart of SetThumbnailIconId above, added strictly so a
    // headless test can seed a RESOLVED (non-miss) strategicIconId and exercise
    // ResolveLodModeAndIcon's strategic branch end to end (MapCanvas_IconLayer_CullEmit_UI.cpp) —
    // before this, only the pairing-MISS path was reachable in strategic mode from any test
    // (CheckStrategicModeBelowThreshold's own header comment already documents this gap). No
    // production caller uses this yet: BuildIconAtlasPairingLookup (below) is UNCHANGED, still
    // leaving every real pairing's strategicIconId at kInvalidIconId — no authored strategic-icon
    // source exists yet (ARCH_14_03_IconRenderingLod.md §14.3, a separate, unscoped ticket). This is
    // test-enabling infrastructure only, not a behavior change for any real map.
    void SetStrategicIconId(const std::string& templateIdentifier, int iconId) {
        pairingsByTemplateIdentifier[templateIdentifier].strategicIconId = iconId;
    }
```

File grows from 81 non-blank lines to ~90 — still comfortably under `ARCH_01_05_FileSizeCeilings.md`
§1.5's soft 100-line ceiling; no split needed.

**2. New fixture helper, `src/ui/MapCanvas_IconLayer_TestFixture_UI.h`, immediately after
`SeedAtlasEntry` (`:104-115`)** — mirrors `SeedAtlasEntry`'s own shape exactly (grows the manifest
the same way), the strategic-icon-id counterpart:

```cpp
inline void SeedStrategicAtlasEntry(IconAtlasPairingLookup& pairingLookup, IconAtlasManifest& manifest,
                                    const std::string& templateIdentifier, int iconId, int atlasPage = 0) {
    pairingLookup.SetStrategicIconId(templateIdentifier, iconId);
    while (static_cast<int>(manifest.entries.size()) <= iconId) {
        IconAtlasEntry entry; entry.iconId = static_cast<int>(manifest.entries.size());
        manifest.entries.push_back(entry);
    }
    manifest.entries[static_cast<std::size_t>(iconId)].atlasPage = atlasPage;
    while (static_cast<int>(manifest.pageTextureIdentifiers.size()) <= atlasPage)
        manifest.pageTextureIdentifiers.push_back(
            static_cast<std::uint64_t>(manifest.pageTextureIdentifiers.size()) + 1u);
}
```

## New acceptance test

**`src/ui/MapCanvas_IconLayer_Cull_UI_Test.cpp`** — new function, placed immediately after
`CheckManualMarkerSelectedScaleComposesOnlyWhenSelected` (after line 954), same file, same anonymous
namespace, mirroring that function's own baseline-vs-scaled/selected-vs-unselected shape but forced
into the STRATEGIC branch (`thumbnailLodThresholdPixels` raised to `100.0f`, well above the fixture's
own ~2-6px `thumbnailScreenSize` at scale 1.0-3.0, confirmed against
`CheckThumbnailModeAboveThreshold`'s own math, lines 93-110: `baseFootprint(2) * scale(1)` ≈ the
fixture's per-pixel unit factor) instead of below it, and seeding a real `strategicIconId` via the new
helper so a genuine resolved candidate (not a pairing miss) is produced:

```cpp
// ARCH §19.32 / STEP257 — the confirmed bug fix: ResolveLodModeAndIcon's STRATEGIC branch
// (MapCanvas_IconLayer_CullEmit_UI.cpp) previously ignored `instanceScale` entirely, so
// scaleSelectedAlloy (and every other instanceScale contributor) had zero visual effect on a marker
// resolved in strategic mode -- the common case, since strategicIconScreenSizePixels' default
// threshold (5.0f) puts most real markers here at typical zoom. Strategic-mode mirror of
// CheckManualMarkerSelectedScaleComposesOnlyWhenSelected above, forcing the STRATEGIC branch (raise
// thumbnailLodThresholdPixels ABOVE the fixture's own thumbnailScreenSize instead of below it) and
// seeding a real, resolvable strategicIconId via SetStrategicIconId (IconAtlasPairing_UI.h) --
// SeedAtlasEntry alone only ever seeds thumbnailIconId, so the pre-existing
// CheckStrategicModeBelowThreshold above could only ever exercise the pairing-MISS path, never a
// real resolved strategic candidate's screenSize.
void CheckManualMarkerSelectedScaleComposesInStrategicMode() {
    ManualMarkerTestFixture fixture;
    fixture.fixture.recipe.globalMarkerSettings.scaleAlloy = 1.0f;           // isolate the selected term
    fixture.fixture.recipe.globalMarkerSettings.scaleSelectedAlloy = 3.0f;
    fixture.fixture.recipe.markers[0].transforms[0].instanceIdentifier = 77;   // globally-unique id
    fixture.alloyLayer.thumbnailLodThresholdPixels = 100.0f;   // forces strategic mode
    SeedStrategicAtlasEntry(fixture.fixture.pairingLookup, fixture.fixture.atlasManifest, "Alloy", 1);
    fixture.fixture.overlaySettings.overlayLayers = {fixture.alloyLayer};

    DrawOverlayIconLayersInput input = fixture.fixture.Input();
    OverlayInstanceKeySet_UI emptySelection;
    input.selectedInstanceKeys = &emptySelection;
    std::vector<OverlayVisibleInstance> unselectedCandidates;
    ResolveVisibleCandidates(input, fixture.fixture.aabbCache, nullptr, unselectedCandidates);
    check(unselectedCandidates.size() == 1,
          "the unselected Alloys manual instance resolves exactly one candidate in strategic mode");

    OverlayInstanceKeySet_UI selectionSet;
    selectionSet.keys = {OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers, 77, true, /*bManual=*/true}};
    input.selectedInstanceKeys = &selectionSet;
    std::vector<OverlayVisibleInstance> selectedCandidates;
    ResolveVisibleCandidates(input, fixture.fixture.aabbCache, nullptr, selectedCandidates);
    check(selectedCandidates.size() == 1,
          "the selected Alloys manual instance resolves exactly one candidate in strategic mode");

    if (!unselectedCandidates.empty() && !selectedCandidates.empty()) {
        check(!unselectedCandidates[0].bSelected, "the first resolve's own candidate correctly reports unselected");
        check(selectedCandidates[0].bSelected, "the second resolve's own candidate correctly reports selected");
        // Regression guard: instanceScale == 1.0 in the unselected leg must reproduce the EXACT
        // pre-fix strategic-mode constant (x * 1.0f is exact under IEEE754) -- proves unscaled
        // strategic-mode output is byte-identical to pre-fix behavior, the "silent regression in the
        // common case" this bug itself was.
        check(unselectedCandidates[0].screenSize == fixture.alloyLayer.strategicIconScreenSizePixels,
              "an unscaled (instanceScale == 1.0) strategic-mode candidate's screenSize is unchanged "
              "from the pre-fix constant strategicIconScreenSizePixels");
        const float expected = unselectedCandidates[0].screenSize * 3.0f;
        check(selectedCandidates[0].screenSize > expected * 0.99f && selectedCandidates[0].screenSize < expected * 1.01f,
              "scaleSelectedAlloy(3.0) composes into a SELECTED manual marker's rendered screenSize in "
              "STRATEGIC mode too -- the confirmed STEP257 bug fix -- and is absent from the same "
              "instance's UNSELECTED screenSize");
    }
}
```

**Registration** — add the call immediately after
`CheckManualMarkerSelectedScaleComposesOnlyWhenSelected();` (currently line 1278, in the file's
top-level test-runner sequence):

```cpp
    CheckManualMarkerSelectedScaleComposesOnlyWhenSelected();
    CheckManualMarkerSelectedScaleComposesInStrategicMode();
```

No CMakeLists.txt change needed — `MapCanvas_IconLayer_Cull_UI_Test.cpp` is already registered as
part of the `MapCanvas_IconLayer_UI_Test` binary (`CMakeLists.txt:674-676`).

## Explicit out-of-scope

- `GlobalMarkerSettings::scaleSelectedAlloy/Plasma/Spawn`, `ResolveMarkerGroupSelectedTypeScale`, the
  "Icon Scale (Selected)" dial, and `ResolveMarkersManual`'s `instanceScale` composition — all
  confirmed correct, untouched (see "Do NOT touch" above).
- The THUMBNAIL branch of `ResolveLodModeAndIcon` (line 36) — already correct, untouched.
- The pairing-miss placeholder branch (`MapCanvas_IconLayer_CullEmit_UI.cpp:102-107`) — deliberately
  unscaled by design, untouched.
- `BuildIconAtlasPairingLookup` (`IconAtlasPairing_UI.h:72-78`) — stays unchanged; real strategic-icon
  authoring is a separate, unscoped ticket (ARCH §14.3).
- `Picking_UI.cpp` — confirmed no `screenSize` dependency, no change needed.
- `CheckStrategicModeBelowThreshold` and `CheckUnresolvedPropFallsBackToPlaceholder` — both stay
  valid and unmodified; they cover the pairing-miss and placeholder-chip paths respectively, distinct
  from this ticket's resolved-candidate strategic-mode coverage.
- No file-size-ceiling remediation split for `MapCanvas_IconLayer_Cull_UI_Test.cpp` — it is already
  well past the soft/hard per-file ceiling pre-existing to this ticket (a long-standing,
  already-accepted "one test binary's TUs accumulate acceptance tests" convention in this codebase,
  e.g. STEP240's own addition to this same file); adding ~30 lines here is not this ticket's
  remediation to invent, matching STEP256's own identical stance on the same question.
- No `ResolveLodModeAndIcon` parameter/signature change — `instanceScale` is already an existing
  parameter (line 30); the fix is a pure arithmetic addition to an existing expression.

## Files touched

**New:** none.

**Modified:**
- `src/ui/MapCanvas_IconLayer_CullEmit_UI.cpp` — the fix: `resolved.screenSize =
  layer.strategicIconScreenSizePixels * instanceScale;` (line 43).
- `src/ui/IconAtlasPairing_UI.h` — new `SetStrategicIconId` method (after `:47`).
- `src/ui/MapCanvas_IconLayer_TestFixture_UI.h` — new `SeedStrategicAtlasEntry` helper (after
  `SeedAtlasEntry`, `:104-115`).
- `src/ui/MapCanvas_IconLayer_Cull_UI_Test.cpp` — new `CheckManualMarkerSelectedScaleComposesInStrategicMode`
  function (after `:954`) plus its registration call (after `:1278`).

## Acceptance

1. **The fix itself** — `CheckManualMarkerSelectedScaleComposesInStrategicMode` (above) passes:
   unselected strategic-mode `screenSize` equals `layer.strategicIconScreenSizePixels` exactly
   (byte-identical pre-fix-behavior regression guard), selected `screenSize` is
   `scaleSelectedAlloy(3.0)` times that.
2. **No regression** — every previously-passing test in `MapCanvas_IconLayer_UI_Test` (the whole
   binary, per `CMakeLists.txt:674-684`) stays green, in particular `CheckThumbnailModeAboveThreshold`,
   `CheckStrategicModeBelowThreshold`, `CheckUnresolvedPropFallsBackToPlaceholder`, and
   `CheckManualMarkerSelectedScaleComposesOnlyWhenSelected` (thumbnail-mode leg, unaffected by this
   fix).
3. Full solo rebuild + `ctest -C Debug` clean before commit, per this repo's standing commit protocol.
