# STEP52 — Icon atlas pairing lookup: `templateIdentifier -> {thumbnailIconId, strategicIconId}`

**Layer:** UI. **Domain:** `ApplicationAssetBridge`, icon-atlas bridge. **Accuracy class:** Visual.
**Sequence:** Phase 2.2, `work_orders/SEQUENCE_PreviewOverlayLayering.md`. No dependency on other
undone work-orders in that sequence; feeds STEP53 (the screen-space icon draw pass, Phase 3.1/3.4),
which is not part of this ticket.

## Root problem
ARCH_14_03_IconRenderingLod.md §14.3 (`ARCH.md:1131-1135`) ratifies a future overlay renderer that must resolve, per
`templateIdentifier`, **two** icon ids — a thumbnail id (true-size draw mode) and a strategic id
(constant-size draw mode, §14.3 items 1-2) — but the only lookup that exists today,
`Ui::IconAtlasManifest` (`src/ui/IconGridWidget_UI.h:34-46`), is one `iconId` -> one UV rect, and
its only other consumer, the icon-picker grid (`DrawIconGrid`, same file), legitimately wants
exactly one slot per entry. ARCH is explicit that widening `IconAtlasEntry` to carry a second UV
rect would carry dead weight through that unrelated consumer and is not permitted
(`ARCH.md:1131-1132`: "stays one `iconId` -> one UV rect; do not widen it"). The file's own header
comment already names this exact seam as anticipated:

> "SCOPE NOTE (ARCH_08_04_CoderScopeLaw.md §8.4): the atlas image and its real manifest are built by the asset pipeline
> (M5-4)... When M5-4 lands it either publishes this shape or an adapter fills it; no new type was
> created in a folder this work-order does not own." (`IconGridWidget_UI.h:9-14`)

M5-4 has since landed (`Application_Assets_UI.cpp`'s `BuildIconAtlasManifest`,
`Application::LoadAssetAtlas`) and publishes exactly the single-slot shape that comment predicted.
This ticket is the "adapter" that comment anticipated: a **separate pairing lookup**
(`ARCH.md:1132-1135`), consumed by STEP53, that resolves each `templateIdentifier` to both ids —
each id still resolving through the existing single-slot manifest unchanged.

This ticket does **not** solve where `strategicIconId` values actually come from. ARCH_14_03_IconRenderingLod.md §14.3
(`ARCH.md:1127-1130`) rules that a strategic icon per entity type is new authored visual content,
bespoke per blueprint, "real authoring/asset-pipeline work, out of this ruling's scope" — a
separate, unscheduled ticket. This ticket only builds the plumbing/lookup structure; every
`strategicIconId` resolves to an invalid sentinel until that future work lands.

## Fix

### 1. New pure type: `IconAtlasPairingLookup` (`src/ui/IconAtlasPairing_UI.h`, new file)
A small, header-only, DATA-free, imgui-free type — same posture as `IconGridWidget_UI.h`'s own
grouped value types (`IconAtlasEntry`/`IconAtlasManifest`/`IconGridLayout`/... all live together in
one header because they are tightly coupled and each trivial). Everything here is well under the
100-line soft ceiling (§1.5).

```cpp
// IconAtlasPairing_UI.h — templateIdentifier -> {thumbnailIconId, strategicIconId}, the "adapter"
// IconGridWidget_UI.h's own header comment anticipated for when the real M5-4 asset manifest
// landed (IconGridWidget_UI.h:9-14). Deliberately separate from Ui::IconAtlasManifest
// (IconGridWidget_UI.h) rather than widening IconAtlasEntry — ARCH_14_03_IconRenderingLod.md §14.3 rules that manifest stays
// one iconId -> one UV rect because its other consumer, the icon-picker grid, wants exactly one
// slot per entry. Every id here still resolves through that unchanged single-slot manifest.
#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace SanmapGen {
namespace Ui {

// Reuses the sentinel convention IconAtlasEntry::iconId and IconGridState::selectedIconId already
// establish ("-1 = no entry / nothing selected") rather than inventing a second invalid-value
// convention (IconGridWidget_UI.h:26, :80).
inline constexpr int kInvalidIconId = -1;

struct IconIdentifierPairing {
    int thumbnailIconId = kInvalidIconId;
    int strategicIconId = kInvalidIconId;
};

// Caller-owned, DATA-free, GPU-free (ARCH_03_ModuleBoundaries.md §3.2) — a plain string-keyed lookup, no atlas pixels, no
// GL handle. Duplicate templateIdentifier inputs to SetThumbnailIconId are last-write-wins (the
// natural std::unordered_map assignment semantics below) — documented here so it is a known policy,
// not an unexamined accident, should two atlas entries ever share a file stem.
class IconAtlasPairingLookup {
public:
    void Clear() { pairingsByTemplateIdentifier.clear(); }

    void SetThumbnailIconId(const std::string& templateIdentifier, int iconId) {
        pairingsByTemplateIdentifier[templateIdentifier].thumbnailIconId = iconId;
    }

    // Unknown templateIdentifier resolves to a default-constructed pairing (both ids
    // kInvalidIconId) — mirrors Application::TemplateIdentifierOfIcon's own
    // empty-result-on-miss contract (Application_Assets_UI.cpp:75-79), never a thrown/asserted
    // failure.
    IconIdentifierPairing Resolve(const std::string& templateIdentifier) const {
        const auto found = pairingsByTemplateIdentifier.find(templateIdentifier);
        return found != pairingsByTemplateIdentifier.end() ? found->second : IconIdentifierPairing();
    }

    std::size_t Count() const { return pairingsByTemplateIdentifier.size(); }

private:
    std::unordered_map<std::string, IconIdentifierPairing> pairingsByTemplateIdentifier;
};

// Pure builder: one pairing per distinct templateIdentifier, thumbnailIconId = that identifier's
// index into the vector (the same "iconId is its own index" contract BuildIconAtlasManifest
// already establishes, Application_Assets_UI.cpp:57). strategicIconId is intentionally left at
// kInvalidIconId — no authored strategic-icon source exists yet (ARCH_14_03_IconRenderingLod.md §14.3, separate ticket).
// This posture — an entry still gets an explicit, obviously-a-placeholder id rather than being
// silently absent or crashing a future lookup — mirrors the Constitution §6 fallback discipline
// AssetAtlasCache_PropThumbnail_IO.cpp's MakePlaceholderImage already applies to a missing/corrupt
// thumbnail image (AssetAtlasCache_PropThumbnail_IO.cpp:1-3); no new fallback pattern invented.
IconAtlasPairingLookup BuildIconAtlasPairingLookup(const std::vector<std::string>& iconTemplateIdentifiers) {
    IconAtlasPairingLookup lookup;
    for (std::size_t iconId = 0; iconId < iconTemplateIdentifiers.size(); ++iconId)
        lookup.SetThumbnailIconId(iconTemplateIdentifiers[iconId], static_cast<int>(iconId));
    return lookup;
}

} // namespace Ui
} // namespace SanmapGen
```

Note: `BuildIconAtlasPairingLookup` takes the **already-built** `iconTemplateIdentifiers` side
table (`iconId -> tpId`, `Application_Assets_UI.cpp:64`, stored at
`Application_AssetBridge_UI.h:34`) rather than re-walking `Io::AssetAtlas` — it is a pure
re-indexing of data the shell already computed, zero new IO/atlas coupling, and it leaves
`BuildIconAtlasManifest`'s existing signature and tested behavior completely untouched (STEP47's
own discipline: no existing call site changes behavior).

### 2. Wire it into the asset bridge (`src/ui/Application_AssetBridge_UI.h`)
Add the field next to the existing manifest/side-table it derives from:
```cpp
#include "IconAtlasPairing_UI.h"   // new include, next to "IconGridWidget_UI.h"
...
IconAtlasManifest             iconManifest;
std::vector<std::string>      iconTemplateIdentifiers;   // iconId -> `tpId` side table
IconAtlasPairingLookup        iconPairingLookup;          // templateIdentifier -> {thumbnail, strategic}
```

### 3. Populate it in `Application::LoadAssetAtlas()` (`src/ui/Application_Assets_UI.cpp:89-123`)
Clear it alongside the other bridge fields already cleared at the top of the function (line 90-93),
and populate it right after the existing `BuildIconAtlasManifest` call (line 115-117) — same place
`iconTemplateIdentifiers` itself becomes valid:
```cpp
assetBridge.iconPairingLookup.Clear();     // alongside the existing clears, Application_Assets_UI.cpp:90-93
...
BuildIconAtlasManifest(assetBridge.assetAtlasCache.Atlas(), assetBridge.atlasResidency,
                       gpuResourceManager.get(), assetBridge.iconManifest,
                       assetBridge.iconTemplateIdentifiers);
assetBridge.iconPairingLookup = BuildIconAtlasPairingLookup(assetBridge.iconTemplateIdentifiers);
```

### 4. Public accessor (`src/ui/Application_UI.h`, next to `IconManifest()`/`TemplateIdentifierOfIcon()`, line 69-71)
```cpp
// The atlas-pairing lookup STEP53's overlay renderer consumes: templateIdentifier ->
// {thumbnailIconId, strategicIconId}. strategicIconId presently always resolves to
// kInvalidIconId — no authored strategic-icon content exists yet (ARCH_14_03_IconRenderingLod.md §14.3, separate,
// unscheduled ticket); this accessor's contract does not change when that content lands, only
// the resolved value does.
const IconAtlasPairingLookup& IconPairingLookup() const { return assetBridge.iconPairingLookup; }
```

## Files touched
- `src/ui/IconAtlasPairing_UI.h` — new file: `kInvalidIconId`, `IconIdentifierPairing`,
  `IconAtlasPairingLookup`, `BuildIconAtlasPairingLookup()`.
- `src/ui/Application_AssetBridge_UI.h` — new include, new `iconPairingLookup` field.
- `src/ui/Application_Assets_UI.cpp` — `LoadAssetAtlas()`: one new clear line, one new populate
  line. `BuildIconAtlasManifest()` itself is NOT edited — zero risk to its existing tested shape.
- `src/ui/Application_UI.h` — new `IconPairingLookup()` accessor.
- `src/ui/IconAtlasPairing_UI_Test.cpp` — new file, pure unit tests (no live atlas needed).
- `src/ui/ApplicationShell_IconBridge_UI_Test.cpp` — extended with an end-to-end wiring check
  (`LoadAssetAtlas()` -> populated lookup), alongside the existing `RunManifestShapeChecks`.
- `CMakeLists.txt` — one new `add_sangen_test(IconAtlasPairing_UI_Test
  src/ui/IconAtlasPairing_UI_Test.cpp)` line near the existing `IconGridWidget_UI_Test` block
  (~line 336). `ApplicationShell_IconBridge_UI_Test.cpp` is already registered (line 393) — no
  CMake change needed for that file.

## Backend policy
N/A — pure CPU-side lookup structure (a `std::unordered_map`), no compute dispatch, no SIMD, no
GPU handle. Does not touch `Dispatch_SYS` or any rival-toggle surface (ARCH_03_ModuleBoundaries.md §3.2/ARCH_04_DispatchContract.md §4 unaffected).

## ARCH rules invoked
- §14.3 (`ARCH.md:1107-1135`) — the ratified pairing-lookup requirement this ticket implements,
  including the explicit "do not widen `IconAtlasEntry`" constraint and the "strategic icon is
  separately-scoped authored content" ruling.
- §3.1/§3.2 — the shell (`Application`) is the one legal IO/UI bridge unit; no new GL handle
  created outside SYS (none is — this ticket adds no GPU object).
- §1 naming law — fully-spelled `templateIdentifier`/`thumbnailIconId`/`strategicIconId` (no
  `tpId`-style abbreviation invented at this layer; `tpId` itself only appears in this ticket as a
  quoted format-dictated term from existing comments, §1.8's own carve-out).
- §1.5 size ceilings — new file well under 100 lines; one primary cluster of tightly-coupled
  trivial types, mirroring `IconGridWidget_UI.h`'s own precedent for grouping such types.
- Constitution §6 fallback discipline — the placeholder-sentinel posture for `strategicIconId`
  mirrors, not duplicates, `AssetAtlasCache_PropThumbnail_IO.cpp`'s existing
  "never silently missing, always an explicit stand-in" pattern.

## Solution + performance estimate (basis)
`BuildIconAtlasPairingLookup` is one linear pass, O(entryCount) hash-map insertions, over the same
`iconTemplateIdentifiers` vector `BuildIconAtlasManifest` already finishes populating one pass
earlier in the identical function — it adds no new algorithmic order to `LoadAssetAtlas()`, only a
second O(n) pass at the same n. This runs once per sanpack load (a user-initiated, infrequent, not
per-frame action) — ARCH_14_09_RenderingPerformance.md §14.9's frame-budget/microbenchmark bar governs STEP53's per-frame draw
path, not this ticket's load-time cost, so no new microbenchmark is warranted here; the basis is
direct algorithmic inspection against code already shipped and measured-acceptable at the same
scale (`BuildIconAtlasManifest`, same file, same loop bound). `Resolve()` is O(1) average-case
hash lookup, called by STEP53 per distinct `templateIdentifier` (not per instance) if that renderer
chooses to cache results per draw call rather than per instance — a STEP53 concern, not this
ticket's.

## Acceptance test
- New `src/ui/IconAtlasPairing_UI_Test.cpp` (registered in `CMakeLists.txt`), pure/no-atlas:
  - Default-constructed `IconAtlasPairingLookup::Resolve("anything")` returns
    `{kInvalidIconId, kInvalidIconId}`.
  - `BuildIconAtlasPairingLookup({"unitA", "unitB"})` resolves `"unitA"` to
    `{thumbnailIconId=0, strategicIconId=kInvalidIconId}` and `"unitB"` to
    `{thumbnailIconId=1, strategicIconId=kInvalidIconId}`; an unlisted identifier still resolves to
    the all-invalid default.
  - Duplicate input identifiers (`{"unitA", "unitA"}`) resolve `"unitA"`'s `thumbnailIconId` to the
    LAST index (`1`), proving the documented last-write-wins policy rather than leaving it
    unspecified.
- `src/ui/ApplicationShell_IconBridge_UI_Test.cpp`: extend the existing real-sanpack,
  real-`LoadAssetAtlas()` flow with a check that `application.IconPairingLookup().Resolve(
  "ucl3001").thumbnailIconId` equals the same id `IconIdOfTemplate(application, "ucl3001")`
  already resolves via `TemplateIdentifierOfIcon` (the existing helper at line 32-36), and that its
  `strategicIconId` is `kInvalidIconId` — proving the lookup is actually wired into the real
  `LoadAssetAtlas()` path, not only unit-testable in isolation.
- Full solo rebuild + `ctest -C Debug`: previously-green suite stays green with zero unrelated test
  files edited (same discipline STEP46 already verified against); new/extended targets
  (`IconAtlasPairing_UI_Test`, `ApplicationShell_IconBridge_UI_Test`) pass.

## Explicit out-of-scope
- Where `strategicIconId` values actually come from (authored content, asset-pipeline work) — ARCH
  §14.3, a separate, unscheduled future ticket. This ticket only ships the plumbing and the
  `kInvalidIconId` placeholder posture.
- Widening `Ui::IconAtlasEntry`/`Ui::IconAtlasManifest` itself — explicitly forbidden by ARCH
  §14.3; not touched by this ticket.
- STEP53 (the screen-space icon draw pass) and any consumption of this lookup by a renderer,
  including the two-mode LOD switch (§14.3 items 1-2, Phase 3.4) and the `baseFootprintWidth/Depth`
  table (Phase 2.3, a separate READY sequence item).
- Real mesh-derived thumbnail rendering quality (`AssetAtlasCache_PropThumbnail_IO.cpp`'s own
  documented out-of-scope, unaffected and unchanged here).
- Any View-toolbar/overlay-layer UI surfacing of this lookup (Phase 4) — this ticket is pure
  data-plumbing, no new widget, no new tab surface.
