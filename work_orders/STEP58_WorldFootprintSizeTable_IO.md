# STEP58 — World-footprint-size table: `templateIdentifier -> baseFootprintWidth/Depth`

**Layer:** IO. **Domain:** asset-derived data table (not a `.sanmap` import/export domain — see
naming note below). **Sequence:** Phase 2.3, `work_orders/SEQUENCE_PreviewOverlayLayering.md`
(listed **READY, no longer a blind placeholder**, no work-order drafted until now). Parallel with
Phase 1/2.1/2.2; feeds STEP52/STEP53's icon LOD sizing but does not depend on either landing first.

## Root problem
ARCH_14_03_IconRenderingLod.md §14.3 names a real, currently-unsolved gap: **"No world-footprint-size
data exists anywhere in the codebase today"** — `InstancedTransform` only carries a scale
*multiplier*, never an absolute size — and states the fix needs "a new `templateIdentifier ->
baseFootprintWidth/Depth` table, IO-layer, asset-derived not PARAMS-authored." At the time §14.3
was ratified this was scoped as "buildable now with a placeholder default per domain; real
mesh-derived bounds are separately-scoped later work" (§14.13 item 1, still open).

That framing has partially changed. A Format Expert consult this session confirmed the **real
source data exists and is fully characterized** — this is background, not re-derived here:
- Unit templates: `<gameroot>/engine/LJ/lua/common/units/unitsTemplates/<id>/<id>.santp`, root
  table `UnitTemplate`, field `footprint = {x, y}` (ground-plane extent, real world units) — e.g.
  `uca1001`: `{x=1.2,y=1.2}`, `ucl4005`: `{x=18.4,y=18.4}`. Also `collisionInfo =
  {centerOffset{x,y,z}, collisionSize{x,y,z}}` for the 3D box (`collisionSize.y` = height, out of
  scope here).
- Props, two dialects, same `footprint={x,y}` shape, different collision-box field name and
  harvest-resource key: Dialect A (`Environment.sanpack.unzipped/Environment/<Biome>/Props/**/
  *.santp`, root table `propTemplate` lowercase) uses `collider={center,size}` +
  `footprint={x,y}`; Dialect B (`engine/LJ/lua/common/props/propsTemplates/*/*.santp`, root table
  `PropTemplate` capital) uses `collisionInfo={centerOffset,collisionSize}` + `footprint={x,y}`.
  Cross-checked against `sangen_arch_pack/specs/UNIT_PROP_MARKER_DATA_SPEC.md:43-67`, which
  documents this exact dialect split — citations match.

**This is a parsing/import gap, not a data-existence gap anymore** — but SanGen does not read the
live game folder at runtime today (that live-read effort is the separate, unscoped
sanpack/texture-importer work — `MEMORY.md`'s `project_texture_importer_scope` note). Confirmed by
grep (`santp|lua_State|luaL_|LuaTable` across `src/`): **no Lua-table reader exists anywhere in
`src/io/` or elsewhere in `src/` today.** The only `.santp` hits in the whole tree are unrelated
string literals in existing IO test fixtures (`src/io/MapImporter_PropsDecals_IO_Test.cpp:61`
`propGroup.blueprintPath = "Props/Rock/Rock01.santp"`; `src/io/MapImporter_IO_Test.cpp:999,1021`;
`src/io/MapExporter_BlueprintValidation_IO_Test.cpp:65`) — none of these parse `.santp` contents,
they only round-trip a path string already present in a `.sanmap` document.

**This ticket's scope, precisely:** define the real DATA SHAPE (`templateIdentifier ->
baseFootprintWidth/Depth`, IO-layer, asset-derived not PARAMS-authored, per §14.3's own words) and
ship it with a manual/placeholder-seeded lookup — **not** a live `.santp`/Lua parser. Writing a
Lua-table reader is not invented here; it is explicitly deferred to the not-yet-scoped importer
effort. This mirrors `AssetAtlasCache_PropThumbnail_IO.cpp`'s existing placeholder posture
(`MakePlaceholderImage`/`RenderPropThumbnail`: real render quality out of scope, plumbing shipped
now, "never silently missing, always an explicit stand-in").

## Fix

### 1. New file: `src/io/WorldFootprintSizeTable_IO.h`
Header-only, DATA-free, GPU-free (ARCH_03_ModuleBoundaries.md §3.2), same posture as `IconAtlasPairing_UI.h`
(`src/ui/IconAtlasPairing_UI.h`) — a small tightly-coupled cluster of trivial types, well under
the 100-line soft ceiling (§1.5):

```cpp
// WorldFootprintSizeTable_IO.h — templateIdentifier -> real-world ground-plane footprint size.
// ARCH_14_03_IconRenderingLod.md §14.3: "No world-footprint-size data exists anywhere in the codebase
// today... needs a new templateIdentifier -> baseFootprintWidth/Depth table, IO-layer,
// asset-derived not PARAMS-authored." This is that table's data shape + a manual/placeholder-
// seeded lookup -- NOT a live .santp/Lua reader. No Lua-table reader exists anywhere in src/io/
// (or src/ at all) today -- confirmed by grep for "santp"/"lua_State"/"luaL_"/"LuaTable"; the only
// hits are unrelated blueprintPath string literals in MapImporter/MapExporter test fixtures
// (e.g. MapImporter_IO_Test.cpp:999). Wiring a real .santp Lua-table reader is the separate,
// unscoped asset-importer effort -- see BuildPlaceholderWorldFootprintSizeTable()'s own comment.
#pragma once
#include <string>
#include <unordered_map>

namespace SanmapGen {
namespace Io {

// Ground-plane extent in real world units -- mirrors the game's own UnitTemplate/PropTemplate
// footprint = {x, y} field verbatim (UNIT_PROP_MARKER_DATA_SPEC.md, confirmed identical shape on
// unit templates and both prop-template dialects). x -> baseFootprintWidth, y -> baseFootprintDepth.
// The game's collisionInfo/collider 3D box (collisionSize.y = height) is a separate, out-of-scope
// concern -- this table is ground-plane size only, the exact input §14.3's icon LOD formula needs.
struct WorldFootprintSize_IO {
    float baseFootprintWidth = 0.0f;
    float baseFootprintDepth = 0.0f;
};

// Reasoned-placeholder fallback for any templateIdentifier not (yet) seeded below --
// Constitution §7 basis tag: REASONED-PLACEHOLDER, not measured, not final. Domain guess uses the
// tpId scheme's own char1 (UNIT_PROP_MARKER_DATA_SPEC.md: 'u'=Unit, 'e'=Prop -- a real,
// already-documented game convention, not an invented split) so an unseeded lookup never silently
// returns a zero-size (invisible) icon -- mirrors AssetAtlasCache_PropThumbnail_IO.cpp's
// MakePlaceholderImage "always an explicit stand-in" discipline (Constitution §6).
inline constexpr WorldFootprintSize_IO kDefaultUnitFootprintSize{2.0f, 2.0f};
inline constexpr WorldFootprintSize_IO kDefaultPropFootprintSize{4.0f, 4.0f};
inline constexpr WorldFootprintSize_IO kDefaultUnknownFootprintSize{2.0f, 2.0f};

// Caller-owned, DATA-free, GPU-free -- same posture as Ui::IconAtlasPairingLookup
// (IconAtlasPairing_UI.h). Duplicate SetFootprint calls for one templateIdentifier are
// last-write-wins (documented policy, not an unexamined accident).
class WorldFootprintSizeTable {
public:
    void Clear() { footprintsByTemplateIdentifier.clear(); }

    void SetFootprint(const std::string& templateIdentifier, float baseFootprintWidth,
                       float baseFootprintDepth) {
        footprintsByTemplateIdentifier[templateIdentifier] =
            WorldFootprintSize_IO{baseFootprintWidth, baseFootprintDepth};
    }

    // Unknown templateIdentifier resolves to a domain-guessed default, never a thrown/asserted
    // failure. Domain guess uses the tpId scheme's char1 directly -- NOT Ui::OverlayDomainKind_UI;
    // IO must not depend upward on UI (Constitution §1 layering).
    WorldFootprintSize_IO Resolve(const std::string& templateIdentifier) const {
        const auto found = footprintsByTemplateIdentifier.find(templateIdentifier);
        if (found != footprintsByTemplateIdentifier.end()) return found->second;
        if (!templateIdentifier.empty()) {
            if (templateIdentifier.front() == 'u') return kDefaultUnitFootprintSize;
            if (templateIdentifier.front() == 'e') return kDefaultPropFootprintSize;
        }
        return kDefaultUnknownFootprintSize;
    }

    std::size_t Count() const { return footprintsByTemplateIdentifier.size(); }

private:
    std::unordered_map<std::string, WorldFootprintSize_IO> footprintsByTemplateIdentifier;
};

// MANUAL / PLACEHOLDER SEED ONLY -- not a .santp parse. Hand-entered from the real footprint
// values a Format Expert consult already confirmed this session by reading the shipped Lua
// directly. Exists so STEP52/STEP53 have a real, non-zero table to consume today; ingesting the
// full ~280 unit + ~98+4 prop template set is separate, unscoped asset-importer work (no
// Lua-table reader exists anywhere in src/io/ today -- see this file's own top comment). Do not
// silently grow a hand-rolled Lua parser into this function to "finish" it.
inline WorldFootprintSizeTable BuildPlaceholderWorldFootprintSizeTable() {
    WorldFootprintSizeTable table;
    table.SetFootprint("uca1001", 1.2f, 1.2f);
    table.SetFootprint("ucl4005", 18.4f, 18.4f);
    return table;
}

} // namespace Io
} // namespace SanmapGen
```

### 2. Naming-convention note (pre-empting a misapplied IO rule)
This is **not** a `.sanmap`-domain file — the `IO_MIGRATION_SPEC` convention of one
`MapExporter_<Domain>_IO`/`MapImporter_<Domain>_IO` pair per top-level `.sanmap` section, plus
append-only `<Domain>_Migrate_V<N>_IO` version steps, governs `.sanmap` document sections and does
not apply here. This table is an **asset-derived cache/lookup**, the same category as the existing
`AssetAtlasCache_*_IO.*` file family — no migration versioning, no JSON document shape, nothing to
compose from `JsonPrimitives_IO`.

### 3. Consumption boundary — explicitly not wired in this ticket
STEP53 (`STEP53_OverlayIconDrawPass_UI.md:138`) already states the thumbnail-mode LOD formula's
`baseFootprint` input must come "via whatever accessor STEP51/STEP52 land, **never a placeholder
invented in this file**." Symmetrically here: this ticket ships the table itself and stops; adding
a field to `Application_AssetBridge_UI.h` (mirroring STEP52's `iconPairingLookup` plumbing,
`Application_AssetBridge_UI.h`) and a public accessor on `Application_UI.h` so STEP53's draw pass
can reach it is STEP51's or STEP52's job at their own dispatch time, not invented here as a
shadow wiring path.

## Files touched
- NEW `src/io/WorldFootprintSizeTable_IO.h` — `WorldFootprintSize_IO`, the three
  `kDefault*FootprintSize` constants, `WorldFootprintSizeTable`, `BuildPlaceholderWorldFootprintSizeTable()`.
- NEW `src/io/WorldFootprintSizeTable_IO_Test.cpp` — pure unit tests, no file IO, no live sanpack.
- `CMakeLists.txt` — one new `add_sangen_test(WorldFootprintSizeTable_IO_Test
  src/io/WorldFootprintSizeTable_IO_Test.cpp)` line near the existing `AssetAtlasCache_*_Test`
  block.

## Backend policy
N/A — pure CPU-side `std::unordered_map` lookup, no compute dispatch, no SIMD, no GPU handle. Does
not touch `Dispatch_SYS` (Constitution §1/§4 unaffected).

## ARCH rules invoked
- ARCH_14_03_IconRenderingLod.md §14.3 — the ratified table shape this ticket implements verbatim
  (`templateIdentifier -> baseFootprintWidth/Depth`, IO-layer, asset-derived not PARAMS-authored).
- Constitution §1 layering — IO must not depend upward on UI; domain-guess fallback uses the real
  tpId char1 scheme (`UNIT_PROP_MARKER_DATA_SPEC.md`) instead of `Ui::OverlayDomainKind_UI`.
- Constitution §6 — fallback/validation discipline: an unseeded identifier always resolves to an
  explicit, documented stand-in, never a silent zero-size or a thrown failure — mirrors, does not
  duplicate, `AssetAtlasCache_PropThumbnail_IO.cpp`'s existing pattern.
- Constitution §7 — basis-tag law: the three default constants are explicitly tagged
  REASONED-PLACEHOLDER, not measured, not final; superseded once the real importer lands.
- §1 naming law — fully-spelled `templateIdentifier`/`baseFootprintWidth`/`baseFootprintDepth`
  (`footprint`'s `x`/`y` keys spelled out per §1.8, same treatment `tpId` already gets elsewhere in
  this codebase); `_IO` suffix.
- §1.5 size ceilings — single header well under 100 lines, one tightly-coupled cluster of trivial
  types (mirrors `IconAtlasPairing_UI.h`'s own precedent for this exact shape).
- `IO_MIGRATION_SPEC` — explicitly does **not** apply (§2 above); this ticket does not misuse the
  per-`.sanmap`-domain importer/exporter/migration convention for an asset-derived cache table.

## Solution + performance estimate (basis)
`Resolve()` is O(1) average-case hash lookup over at most a few hundred entries even once the real
importer lands (~280 units + ~100 props total, per `UNIT_PROP_MARKER_DATA_SPEC.md`); today's
placeholder seed is 2 entries. `BuildPlaceholderWorldFootprintSizeTable()` is called at most once
per load, not per frame. No microbenchmark is warranted at this scale — same basis STEP52 already
used for its structurally identical `Resolve()` (Constitution §7: direct algorithmic inspection is
a valid basis tag for O(1)-per-call, small-N, non-per-frame code; a real benchmark is reserved for
STEP53's per-frame draw path, not this ticket's load-time lookup).

## Layer & accuracy class
IO. Accuracy class: **Visual** — this ticket's only declared consumer is STEP53's screen-space
icon LOD sizing (presentation only), the same Visual-class exemption `OPTIMIZATION_PILLARS.md`
pillar 15 already grants GPU-resident preview compositing (§14.11). If a future ticket ever makes
this table feed a gameplay-authoritative (Exact/Accurate) calculation instead of icon sizing, that
ticket must re-examine this accuracy-class assignment — not assumed settled by this one.

## Acceptance test
New `src/io/WorldFootprintSizeTable_IO_Test.cpp` (registered in `CMakeLists.txt`):
- Default-constructed `WorldFootprintSizeTable::Resolve("uca9999")` (unseeded, `u`-prefixed)
  returns `kDefaultUnitFootprintSize` (`{2.0f, 2.0f}`).
- `Resolve("epx9999")` (unseeded, `e`-prefixed) returns `kDefaultPropFootprintSize`
  (`{4.0f, 4.0f}`).
- `Resolve("")` and `Resolve("zzz")` (empty / unrecognized first char) both return
  `kDefaultUnknownFootprintSize`.
- `SetFootprint("uca1001", 1.2f, 1.2f)` then `Resolve("uca1001")` returns exactly `{1.2f, 1.2f}`,
  not a default.
- Duplicate `SetFootprint` calls for the same identifier with different values: the later call
  wins (proves the documented last-write-wins policy rather than leaving it unspecified).
- `BuildPlaceholderWorldFootprintSizeTable().Resolve("uca1001")` returns `{1.2f, 1.2f}`;
  `.Resolve("ucl4005")` returns `{18.4f, 18.4f}`; `.Count() == 2`.
- Full solo rebuild + `ctest -C Debug`: previously-passing suite stays green with zero unrelated
  test files edited; the new `WorldFootprintSizeTable_IO_Test` target passes.

## Explicit out-of-scope
- **The real `.santp` Lua-table parser/ingestion pipeline** — confirmed nonexistent anywhere in
  `src/` today (grep evidence above). This is the separate, unscoped sanpack/texture-importer
  effort (`MEMORY.md` `project_texture_importer_scope`); this ticket must not silently invent a
  Lua parser to "finish" the job.
- **Populating the table beyond the two manually-confirmed seed entries** (`uca1001`, `ucl4005`).
  Ingesting the full ~280 unit + ~98+4 prop template set is that same importer effort's job, not a
  larger hand-typed literal table grown here.
- **Wiring this table into `Application_AssetBridge_UI.h`/`Application_UI.h`, STEP52's icon
  pairing lookup, or STEP53's LOD draw pass** — per STEP53's own text, that accessor lands with
  STEP51 or STEP52, not invented here as a shadow wiring path (§3 above).
- **Height / 3D collision-box data** (`collisionInfo`/`collider`'s `collisionSize`,
  `centerOffset`) — ground-plane footprint only, the exact and only input §14.3's icon-sizing
  formula needs.
- **Reconciling the two prop-template dialects' differing collision-field names** (`collider` vs
  `collisionInfo`) or harvest-resource keys (`plasma` vs `energy`) — irrelevant to this ticket
  since both dialects share the identical `footprint={x,y}` shape per
  `UNIT_PROP_MARKER_DATA_SPEC.md:43-67`; only matters once the real parser (out of scope) exists.
- **§14.13 item 2** (the cross-layer visible-vertex budget benchmark) — unrelated, tracked
  separately under STEP53/Phase 3.3.

## Verify
- New `src/io/WorldFootprintSizeTable_IO_Test.cpp` passes (assertions listed under Acceptance
  test above).
- Full solo rebuild + `ctest -C Debug`: 100% pass, zero pre-existing test files edited or broken —
  same discipline STEP46/STEP52 already verified against.
- Confirm (grep, already done for this work-order, re-verifiable by the coder before dispatch)
  that no `.santp`/Lua parser exists in `src/io/` at implementation time either — if one has since
  landed via the separate importer effort, route back to ARCH before silently duplicating or
  bypassing it rather than shipping this ticket's placeholder table unchanged.
