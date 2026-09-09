# STEP255 — Execute `ARCH_21_07`'s deferred `MapCanvas_UI.h` file-size-ceiling split

**Layer:** UI. **Domain:** `src/ui/MapCanvas_UI.h` + 1 new header + 2 sibling `.cpp` edits.
**Sequence:** depends on nothing undone. File-disjoint from STEP254 — no shared files, safe to run
in parallel.

**Origin:** `ARCH_21_07_FileSizeCeilingFlag.md` §21.7, ratified against `MapCanvas_UI.h` at 261 lines,
directing whoever next touches the file to re-measure and split "a companion header" once
§21.1–§21.6 landed — confirmed by direct read that this never happened: the file is now 520 lines
(2x its flagged size, ~3.5x `ARCH_01_05_FileSizeCeilings.md` §1.5's 150-line hard ceiling). This
ticket is that overdue execution, authorized by §21.7 directly — no new ARCH ruling needed for the
move this ticket actually makes (see §2 below for the one thing this ticket does NOT attempt, and
why that DOES need a future ARCH ruling if the human wants it done).

## 0. What §21.7's own cited precedent actually is — read before assuming a header split is free

§21.7 cites `MapCanvas_MarkerHitTest_UI.cpp`/`MapCanvas_MarkerRosterDraw_UI.cpp` (STEP126) as "direct
precedent" for "split a companion header." Direct read of both files plus their shared header
(`MapCanvas_MarkerDrag_UI.h`) shows the ACTUAL precedent is narrower than that phrasing suggests:
STEP126 split **free functions** (`HitTestManualMarkers`, `DrawManualMarkerRoster` — neither is a
`MapCanvas` member) across two `.cpp` files fronted by ONE unchanged header. No companion HEADER was
ever created for that split — only companion `.cpp` files. `MapCanvas_UI.h`'s actual problem is
different in kind: its size is dominated by its OWN CLASS's method declarations (~35 public +
private methods) and gesture-state fields, not by one header's `.cpp` being oversized. A class's own
method list cannot be split across two headers without either (a) converting private methods to
free functions (changes every call site's calling convention across `MapCanvas_Draw_UI.cpp` and
whichever other `.cpp` calls them — a real logic-shape change, not a header split) or (b) composing
a delegate/mixin object (a structural redesign needing its own ARCH ruling on the right shape). Both
are out of scope for this ticket (see §2). What IS a genuine, zero-risk "companion header" move —
already twice-precedented **inside this exact class**, not just cited by analogy — is grouping a
cohesive cluster of pure-data, injected-pointer fields into one small struct, exactly what
`MapCanvas_ManualDragSources_UI.h` already did for Props/Decals/Areas (`ManualPropDragSources_UI`,
`ManualDecalDragSources_UI`, `ManualAreaDragSources_UI`). This ticket applies that SAME technique to
the one remaining ungrouped cluster: the overlay-icon-draw-pass sources.

## 1. The move — `MapCanvas_OverlaySources_UI.h`, consolidating 8 fields + 2 caches into 1

**AMENDMENT (post-dispatch correction, real build failure found this):** the original grep pass
below missed one real read site. `MapCanvas_MarkerDrag_UI.cpp`'s `DrawManualMarkerDragPass` also
reads the bare `overlayLayerSettings` member directly (line 68, threading it into
`DrawManualMarkerRoster`; see the `BUGFIX_OverlayVisibilityAndPropIconFallback_R1` comment at line
61-63 for why). This is a genuine THIRD `.cpp` consumer, not covered by §4's original "checked and
clean" list either. **Fix, now added to scope:** in `src/ui/MapCanvas_MarkerDrag_UI.cpp`, change
line 68's `overlayLayerSettings` argument to `overlaySources.layerSettings`, and update the line
61-63 comment's `` `overlayLayerSettings` `` mention to `` `overlaySources.layerSettings` `` for
accuracy. Same mechanical pattern as every other site in this ticket — no new design content, only
a missed file. `MapCanvas_MarkerDrag_UI.cpp` is now part of this ticket's Files Touched (§4).

Originally confirmed by direct grep across `src/ui/` (INCOMPLETE, corrected above): two `.cpp` files
read these fields as `MapCanvas` members — `MapCanvas_Draw_UI.cpp` (`DrawOverlayIconLayerPass`, 9
read-sites) and `MapCanvas_SelectionGesture_UI.cpp` (`ApplyMarqueeGesture`, 4 read-sites, reusing
`overlayPlacements` for the marquee's procedural region query — the file's own header comment
already documents this dual use) — **plus the third site above.** No other `.cpp`, and no test
file, reads these as bare `MapCanvas` members (matches on `overlayLayerSettings` etc. elsewhere are
a *different*, coincidentally-same-named field on `DrawOverlayIconLayersInput`,
`MapCanvas_IconLayer_Ops_UI.h` — confirmed by direct read, not assumed). **Re-verify this "no other
site" claim yourself once more before implementing** — this ticket's own first grep pass already
missed one real site once.

**NEW `src/ui/MapCanvas_OverlaySources_UI.h`** (header-only, no `.cpp` — mirrors
`MapCanvas_ManualDragSources_UI.h`'s own "pure data, no logic of its own" shape exactly):

```cpp
// MapCanvas_OverlaySources_UI.h — the screen-space overlay-icon-draw-pass's injected sources +
// its own owned per-canvas caches, consolidated out of MapCanvas_UI.h (ARCH §21.7 remediation,
// STEP255) so this cluster is one field instead of ten scattered ones — mirrors
// MapCanvas_ManualDragSources_UI.h's ManualPropDragSources_UI/ManualDecalDragSources_UI shape.
// Layer: UI. Pure data, no logic of its own.
#pragma once
#include "MapCanvas_IconLayer_UI.h"       // OverlayRenderingSettings, IconLayerAabbCache_UI, IconLayerFrameCache
#include "OverlayLayer_Settings_UI.h"     // OverlayLayerSettings
#include "../io/WorldFootprintSizeTable_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Data { struct PlacementResults; struct RuleBucketIndexSet; }
namespace Ui {
struct IconAtlasManifest;
class IconAtlasPairingLookup;   // AMENDMENT — real declaration is `class`, not `struct`; `struct`
                                 // compiles but trips an avoidable C4099 mismatch warning

struct OverlayIconPassSources_UI {
    const OverlayLayerSettings*         layerSettings      = nullptr;
    const OverlayRenderingSettings*     renderingSettings  = nullptr;
    const Data::PlacementResults*       placements         = nullptr;
    const Data::RuleBucketIndexSet*     ruleBucketIndex    = nullptr;
    const Params::MapRecipe*            recipe             = nullptr;
    const IconAtlasPairingLookup*       pairingLookup      = nullptr;
    const IconAtlasManifest*            atlasManifest      = nullptr;
    const Io::WorldFootprintSizeTable*  footprintSizeTable = nullptr;
    IconLayerAabbCache_UI               layerAabbCache;
    IconLayerFrameCache                 iconLayerFrameCache;
};

} // namespace Ui
} // namespace SanmapGen
```

**EDIT `src/ui/MapCanvas_UI.h`** — add `#include "MapCanvas_OverlaySources_UI.h"` (alphabetical
position, after `MapCanvas_ManualDragSources_UI.h`); replace the 8 fields + 2 caches at the current
lines 444–454 with one field:

```cpp
    // STEP53/STEP255 — overlay icon draw pass sources + owned caches, consolidated (ARCH §21.7).
    OverlayIconPassSources_UI overlaySources;
```

Update the 6 public setters (current lines 118–132) to write into `overlaySources.*` instead of the
retired bare members — same public names/signatures, only the bodies change (byte-identical external
behavior):

```cpp
    void SetOverlayLayerSettings(const OverlayLayerSettings* settings) { overlaySources.layerSettings = settings; }
    void SetOverlayRenderingSettings(const OverlayRenderingSettings* settings) {
        overlaySources.renderingSettings = settings;
    }
    void SetOverlayPlacementSource(const Data::PlacementResults* placements,
                                   const Data::RuleBucketIndexSet* ruleBucketIndex) {
        overlaySources.placements = placements;
        overlaySources.ruleBucketIndex = ruleBucketIndex;
    }
    void SetOverlayRecipe(const Params::MapRecipe* recipe) { overlaySources.recipe = recipe; }
    void SetIconAtlasSource(const IconAtlasPairingLookup* pairingLookup, const IconAtlasManifest* atlasManifest) {
        overlaySources.pairingLookup = pairingLookup;
        overlaySources.atlasManifest = atlasManifest;
    }
    void SetWorldFootprintSizeTable(const Io::WorldFootprintSizeTable* table) {
        overlaySources.footprintSizeTable = table;
    }
```

**EDIT `src/ui/MapCanvas_Draw_UI.cpp`** — `DrawOverlayIconLayerPass`'s 8 assignment lines (current
lines 67–74) and the `DrawOverlayIconLayers` call (current line 93) change from bare field reads to
`overlaySources.*` reads:

```cpp
    iconLayerInput.overlayLayerSettings = overlaySources.layerSettings;
    iconLayerInput.renderingSettings    = overlaySources.renderingSettings;
    iconLayerInput.placements           = overlaySources.placements;
    iconLayerInput.ruleBucketIndex      = overlaySources.ruleBucketIndex;
    iconLayerInput.recipe               = overlaySources.recipe;
    iconLayerInput.pairingLookup        = overlaySources.pairingLookup;
    iconLayerInput.atlasManifest        = overlaySources.atlasManifest;
    iconLayerInput.footprintSizeTable   = overlaySources.footprintSizeTable;
    ...
    DrawOverlayIconLayers(iconLayerInput, overlaySources.layerAabbCache, overlaySources.iconLayerFrameCache, ...);
```

(Note: `iconLayerInput.overlayLayerSettings` is `DrawOverlayIconLayersInput`'s OWN field — a
different struct, coincidentally same-named — unaffected by this rename; only the right-hand side of
each assignment changes.)

**EDIT `src/ui/MapCanvas_SelectionGesture_UI.cpp`** — `ApplyMarqueeGesture`'s 4 uses of
`overlayPlacements` (current lines 123, 125, 129, 133) become `overlaySources.placements`.

## 2. What this ticket does NOT do, and why — flag for a future ARCH consult, don't guess

This move alone brings `MapCanvas_UI.h` down by only ~10–15 lines (520 → ~505–510) — it does **not**
clear the hard ceiling, and no available "pure structural" move does: the file's remaining bulk is
~35 method declarations (many with multi-line ARCH-citation rationale comments) that belong to ONE
class and cannot be split across headers without changing calling convention. Two candidates
considered and REJECTED for this ticket:

- **(a) "Extract the Area-gesture state+method-declarations into `MapCanvas_AreaGesture_UI.h`,
  mirroring the Marker precedent exactly."** Direct read of `MapCanvas_AreaDragDispatch_UI.cpp`
  shows why this is NOT a pure header split once you actually look at what the Marker precedent
  did (§0 above): `MapCanvas::TryBeginAreaDrag`/`ContinueAreaDrag`/`EndAreaDrag`/`CreateAreaFromDrag`/
  `AreaGestureEligible`/`IsAreaLocked` each touch 5–7 different `MapCanvas` members beyond
  `manualAreaDrag` (`activePanelSource`, `composite`, `view`, `bAreaDragActive`,
  `areaRecompositeThrottle`, `areaCompositeRefreshCallback`). Converting them to free functions
  (the ACTUAL shape of the Marker precedent) means threading all of those through explicit
  parameters and rewriting every call site in `MapCanvas_Draw_UI.cpp` — a real calling-convention
  change, not a "pure structural, zero behavior change" split. `ManualAreaDragSources_UI` (the DATA
  half of Area's own state) is already extracted into `MapCanvas_ManualDragSources_UI.h` from a
  prior ticket — there is no more Area-gesture DATA left ungrouped that isn't already symmetric with
  how Prop/Decal's own `bManualPropDragActive`/`bManualDecalDragActive` flags are deliberately kept
  as direct `MapCanvas` members (not folded into their sibling structs) — `bAreaDragActive` staying
  put matches that established, deliberate precedent rather than deviating from it.
- **(b, partial) grouping the picking sources** (`pickMarkerInstances`, `pickMarkerSpatialGrid`,
  `pickSpatialGridSet`, `pickRadiusScreenPixels`) similarly to overlay sources: rejected for THIS
  ticket because, unlike the overlay cluster, these are read directly across multiple `.cpp` files
  (`MapCanvas_SelectionGesture_UI.cpp`, `MapCanvas_ManualDragDispatch_UI.cpp`) alongside `composite`
  (which is NOT a candidate for grouping — it's a foundational, universally-read dependency, not a
  domain-specific injected source) in ways that would make a partial rename read as more confusing
  than the four scattered fields it replaces. Worth a look in its own follow-up if `composite`'s own
  centrality is separately addressed first.

**If the human wants `MapCanvas_UI.h` under the hard ceiling for real**, that requires deciding the
actual target shape (free-function extraction with explicit params vs. a composed delegate/mixin
class MapCanvas holds one of per gesture family) — a real architecture call, not a coder's to make
unilaterally inside a "pure structural" ticket. Route that decision to the ARCH Expert as a
follow-up if wanted; this ticket does the one safe, real, zero-risk reduction available today and
is honest that it isn't sufficient on its own.

## 3. Out of scope

- Any conversion of `MapCanvas`'s private Area/Marker/Selection-gesture methods to free functions or
  a delegate object (§2).
- Grouping the picking-source fields (§2, partial-b).
- Any behavior change — every setter keeps its exact name/signature; every read site's VALUE is
  unchanged, only its access path (`overlaySources.x` vs. bare `x`) changes.
- Updating `ARCH_21_07_FileSizeCeilingFlag.md`'s own stale "261 lines" figure — that file is
  ARCH-Expert-owned; flag the new post-ticket line count to the ARCH Expert as a documentation-sync
  item, don't edit it here.

## 4. Files touched

- EDIT `src/ui/MapCanvas_UI.h`
- NEW `src/ui/MapCanvas_OverlaySources_UI.h`
- EDIT `src/ui/MapCanvas_Draw_UI.cpp`
- EDIT `src/ui/MapCanvas_SelectionGesture_UI.cpp`
- EDIT `src/ui/MapCanvas_MarkerDrag_UI.cpp` (AMENDMENT — added after a real build failure found this
  missed read site; see §1)

`MapCanvas_AreaDragDispatch_UI.cpp`, `MapCanvas_AreaDraw_UI.cpp`, `MapCanvas_ManualDragDispatch_UI.cpp`,
`MapCanvas_UI.cpp` were each checked by direct grep and do not reference any of the 8 renamed fields
— but re-verify this yourself before implementing (§1's amendment note applies here too: the
original grep pass that produced this list already missed one real site once). No test file
references these fields as bare `MapCanvas` members (confirmed by grep — the only test-file matches
are on `DrawOverlayIconLayersInput`'s own coincidentally-same-named field, a different struct,
unaffected).

## 5. Verify

- Full solo rebuild, clean.
- Full `ctest` pass, 100%, with zero test file edits — `MapCanvas_UI_Test.cpp`,
  `MapCanvas_IconLayer_Draw_UI_Test.cpp`, `MapCanvas_IconLayer_Cull_UI_Test.cpp`,
  `MapCanvas_Picking_UI_Test.cpp`, `MapCanvas_GestureOwnership_UI_Test.cpp`, and every other
  `MapCanvas_*_Test.cpp` pass unchanged. Note: `EXECUTION_CONFLICT_MAP.md` already records
  `MapCanvas_UI_Test.cpp` as one of two PRE-EXISTING, unrelated baseline failures (a manual-marker
  drag-release regression, no work order) — confirm this ticket doesn't change that failure's
  count/nature, don't treat its continued failure as caused by this ticket, and don't fix it here.
- Re-measure `MapCanvas_UI.h` after implementing and record the real resulting line count in the
  commit — expected ~505–510, still over the hard ceiling, which is the documented, expected outcome
  per §2.
