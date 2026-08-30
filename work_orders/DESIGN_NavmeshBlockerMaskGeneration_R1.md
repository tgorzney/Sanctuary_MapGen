# DESIGN — Navmesh Blocker Mask Generation: mesh × water-plane intersection → rectangle decomposition (R1)

*Authored by the SanGen Generator Expert, 2026-08-30. **Design only — no code, no ARCH edit, no
work-order dispatch.** Read-only against `src/**`.*

*Grounded against: `sangen_arch_pack/CONSTITUTION.md` §1/§4/§7/§8, `sangen_arch_pack/specs/
{NAVMAP_MODIFIER_BLOCKER_SPEC,MASKING_SPEC,PLACEMENT_SCATTER_SPEC,DETERMINISM_SPEC}.md`,
`ARCH_18_02_IngestedDataDeterminism.md`, `ARCH_18_03_CatalogDataOwnership.md`,
`ARCH_22_07_MaskToRectangleWorkflow.md`, `ARCH_22_09_OwnershipScopeRuling.md`,
`forum_posts/TUTORIAL_NavMeshBlockers.md` (informal ground truth for the six-layer height-band
table — see ARCH-flag 2), and a direct read this session of `src/pipeline/
GenerationAssembler_Stages_PIPELINE.cpp`, `src/proc/Placement_PROC.h`, `src/data/
{PlacementInstance_DATA,PlacementInstances_DATA,PlacementResults_DATA}.h`, `src/params/
{Water_PARAMS,Geometry_PARAMS}.h`. Format template: `work_orders/DESIGN_SantpFootprintIngestion_R1.md`.*

---

## 0. Why this document exists

`NAVMAP_MODIFIER_BLOCKER_SPEC.md` §7.1 names the gap this design closes: the current
navmesh-blocker mask-authoring/decomposition workflow is entirely hand-run, outside SanGen, and
flags itself as "a strong candidate for a future SanGen-native masking/placement feature — not
designed here." `MASKING_SPEC.md` and `PLACEMENT_SCATTER_SPEC.md` each carry a matching
forward-pointer and equally decline to design it. `ARCH_22_09_OwnershipScopeRuling.md` §22.9 is
explicit that no coder should build toward this "without a real, separately-scoped design consult
first" — this document is that consult, for the geometry/PROC slice specifically (mesh-vs-water
intersection → rasterize → decompose). It does **not** design mesh ingestion (a parallel Format
Expert design) or the Lua/IO export leg (a downstream ticket), and it does **not** re-design
`NAVMAP_MODIFIER_BLOCKER_SPEC.md` §7's decomposition algorithm — that stays exactly as ratified.

---

## 1. Verified ground truth (this session)

### 1.1 The reused, unmodified §7 algorithm
`NAVMAP_MODIFIER_BLOCKER_SPEC.md` §7 (ratified `ARCH_22_07_MaskToRectangleWorkflow.md`): author
a mask at `heightmapResolution`, threshold ~50%, 8-connected label, drop <20px components, then
two passes — (a) exact zero-overshoot largest-rectangle decomposition (100% coverage by
construction) and (b) greedy agglomerative merge via a 2D integral image, stopping at a
human-tunable overshoot threshold — verified by rasterizing the kept-rectangle union and diffing
against the true mask (missed-white-pixel count must be exactly zero). This design's only
obligation to that algorithm is to **feed it a correctly-computed boolean mask**; nothing about
steps (a)/(b) changes here.

### 1.2 The six nav layers and their height bands
`NAVMAP_MODIFIER_BLOCKER_SPEC.md` §2 ratifies only two of the six bands explicitly (Submarine
`-∞..(waterLevel-1.5)`, Sea `-∞..waterLevel`, both gated on `Engine.HasWater()`). The full
six-row table exists only in `forum_posts/TUTORIAL_NavMeshBlockers.md` (informal, not the
ratified pack — see ARCH-flag 2):

| Layer | Always created? | Height band | Max slope |
|---|---|---|---|
| `Land` | yes | sea level to +∞ | 30° |
| `Amphibious` | yes | -∞ to +∞ | 30° |
| `Hover` | yes | -∞ to +∞ | 30° above water, ∞ below |
| `Air` | yes | -∞ to +∞ | ∞ |
| `Submarine` | only if `Engine.HasWater()` | -∞ to (waterLevel - 1.5) | ∞ |
| `Sea` | only if `Engine.HasWater()` | -∞ to waterLevel | ∞ |

This table is directly load-bearing for §3.3's scoping decision below.

### 1.3 Pixel↔world convention (§8), the ownership ruling (§9), and no-rotation (§1)
`world.x = (heightmapResolution-1) - col; world.z = row` (pixel→world direction, empirically
validated twice). Same convention `SANMAP_FORMAT_SPEC.md` uses for `heightmap.raw` sampling —
**not** the separate entity-position convention. `NavmapModifierTemplate.size` is a plain
axis-aligned `float2`, no rotation — a diagonal feature staircase-approximates. Per §9/`ARCH_22_09`:
this whole technique (both the Lua-authoring half and the not-yet-built mask-generation half) is
**not currently SanGen-owned architecture** — recorded, not scheduled.

### 1.4 The real pipeline, confirmed by direct code read
`GenerationAssembler_Stages_PIPELINE.cpp`: `NoiseBlend → Erosion → Thermal → FlowAccumulation →
Mask → Placement → Bake`, each registered via `AddStage(name, tier, computeParameterHash, run)`.
Every stage through Placement is `RegenerationTier::FullRegeneration`; **Bake alone is
`PreviewOnly`** ("the bake is decorative and determinism-exempt, ARCH §4.2/§4.5"). Placement's own
header (`Placement_PROC.h`) states plainly: "Cpu is authoritative (Exact class); `generationContext
= Sys::GenerationContext::Output`." `BuildSpatialGridSet()`/`BuildRuleBucketIndex()` run inline,
immediately after `placementStage.Run()`, inside the SAME registered stage — i.e. there is already
a precedent for "derived work that must see Placement's final output, done right after it."

### 1.5 The exact per-instance input already available — no new DATA needed on this side
`Data::PlacementInstances` (`PlacementInstances_DATA.h`, confirmed by direct read) already carries,
per resolved prop instance: `positionX/Y/Z` (world/game units, absolute), `rotationX/Y/Z/W`
(quaternion), `scaleX/Y/Z`, `templateIdentifier` (the 7-character tpId,
`Data::TemplateIdentifier`), and **`bCollidable`** — "collidable props are gameplay
(`AI_HOSTCLIENT_SPEC`)." This is the complete world-transform contract point 1 of the brief asks
for, on the instance side, already shipped. Nothing new needs to be added to
`Data::PlacementInstances` for this design.

### 1.6 Water level source, confirmed
`src/params/Water_PARAMS.h`:
```cpp
struct Water {
    bool  bEnabled              = false;
    float waterLevelMaximum     = 0.0f;   // surface height (game units)
    float deepWaterDepthMinimum = 0.0f;
    float deepWaterDepthMaximum = 0.0f;
};
```
`waterLevelMaximum` is the single world-space water height the brief asks for.
**`Params::Water::bEnabled` is exactly SanGen's own mirror of `Engine.HasWater()`** — the gate the
task instructs this design to honor for Sea/Submarine generation.

### 1.7 Geometry constants
`src/params/Geometry_PARAMS.h`: `mapSize` (cells/side), `VertexSize() = mapSize+1` (this is
`heightmapResolution`), `worldUnitsPerCell` (default `1.0f`, explicitly a designer-tweakable
field, not guaranteed to stay 1:1).

### 1.8 The load-bearing precedent — ARCH §18.2/§18.3 (footprint ingestion)
`ARCH_18_02_IngestedDataDeterminism.md`: ingested, install-sourced data (not `PARAMS`, not
computed by any `PROC` stage) **may influence generation only via a human-triggered, one-shot bake
into an ordinary `PARAMS` field — never a live read from `PROC`/generation, never written into
`DATA`.** Reasoning: `DATA` is Constitution-defined pure computed output regenerated from
`PARAMS`+seed every run; an externally-sourced value can't be `DATA`'s writer without violating
that. Once baked into `PARAMS`, the value is ordinary recipe data, serialized in the `.sanmap`,
hand-overridable, and closes the Exact/Deterministic chain (`ARCH §4.6`) because PROC never touches
the live external source again. `ARCH_18_03_CatalogDataOwnership.md` additionally rules that the
**raw, un-baked** ingested form stays `IO`-owned (mirrors `Io::WorldFootprintSizeTable`), never a
new `DATA`-layer type. This precedent is the single most important piece of ground truth for §4
below.

---

## 2. Scope boundary

**IN SCOPE (this document):**
- The per-instance mesh-vs-water-plane slice algorithm and its exact case logic.
- The per-nav-layer "which side blocks" rule.
- World→pixel rasterization and cross-instance union, feeding §7's *unmodified* decomposition.
- Output contract (rectangle list per nav layer).
- Accuracy-class assignment and the Exact/Deterministic-chain reasoning the task specifically asked
  for, reasoned explicitly and flagged to ARCH.
- Where this sits in PIPELINE (a new on-demand bake, not a `RegisterStages()` dirty-hash stage) and
  what PARAMS/DATA/IO shapes are newly needed (categories only — exact field layout is the Format
  Expert's/ARCH's call).
- A ticket-breakdown enumeration.

**OUT OF SCOPE:**
- Mesh-ingestion IO (the parallel Format Expert design) — this document states its *expected*
  input shape as an interface only (§3.1).
- The Lua/export IO ticket that turns the rectangle-list output into the per-map orchestration
  files `NAVMAP_MODIFIER_BLOCKER_SPEC.md` §3/§4 already prove work.
- Any UI for triggering the bake.
- §7's decomposition algorithm internals (component labeling, exact largest-rectangle, the
  integral-image merge) — reused exactly as ratified, not redesigned.
- The ARCH ownership ruling itself (§22.9) — this document is input to that ruling, not the ruling.
- Land/Amphibious/Hover/Air generation via this specific technique — see §3.3's scoping argument.

---

## 3. The algorithm

### 3.1 Input contract (interface to mesh-ingestion, not designed here)

Per placed **collidable** prop instance (already fully available, §1.5 — no new DATA needed):
`positionX/Y/Z`, `rotationX/Y/Z/W`, `scaleX/Y/Z`, `templateIdentifier`, `bCollidable`, all read
straight from `Data::PlacementResults.props` after Placement has run.

**Expected new IO surface (mesh-ingestion's deliverable, stated as an interface):**
```
MeshFootprintRecord, keyed by Data::TemplateIdentifier (same key space Io::WorldFootprintSizeTable
already uses):
    localVertices[]            // local-space float3, LOD0/highest fidelity, static non-skinned
    localTriangleIndices[]      // uint32 triples
    localBoundsMinimumY, localBoundsMaximumY   // RECOMMENDED addition — a cheap per-template
                                                 // Y-extent, computed once at ingestion time,
                                                 // reused by every instance of that template
                                                 // (see 3.2's early-out)
```
This should be cached **per template**, not re-fetched per instance — many instances typically
share one `templateIdentifier`. Same category as `Io::WorldFootprintSizeTable`
(`ARCH_18_03_CatalogDataOwnership.md`): IO-owned, Visual-class, asset-derived.

**Filter:** only `bCollidable == true` instances are candidates. This directly matches
`DETERMINISM_SPEC.md`'s own scope line — *"Collidable props and reclaim (gameplay-relevant);
purely decorative props are exempt"* — the one existing field that already tells us which props are
gameplay-relevant is reused verbatim, not reinvented.

### 3.2 Per-instance plane intersection

**Cheap early-out (before any per-triangle work):** transform the template's
`localBoundsMinimumY/MaximumY` conservatively through the instance's scale to get a world-space
Y-range; skip the instance entirely if that range doesn't straddle any target layer's slice height
(§3.3). Most placed props never touch water at all — this is the dominant cost saver, and it is
what makes "transform only the small submerged sub-mesh" (below) actually cheap in aggregate.

**Deriving the local-space plane, done once per surviving instance — the correctness-critical
step.** World transform is `p_world = T + R·(S∘p_local)` (translate ∘ rotate ∘ scale). The world
plane is horizontal: normal `N_world = (0,1,0)`, height `H` (§3.3 picks `H` per layer). Substituting
and using the standard plane-normal transform duality (`N_local = M^T · N_world` for `M = R·S`,
since going *world→local* uses the transpose, not the inverse-transpose used for the *forward*
local→world direction):

```
N_local = S ∘ (Rᵀ · N_world)     // apply the inverse rotation to N_world, THEN scale
                                  // componentwise by (Sx,Sy,Sz) — multiply, never divide
p_local0 = S⁻¹ · Rᵀ · (p_world0 − T)   // any world point with p_world0.y = H (regular POINT
                                        // inverse transform: translate, inverse-rotate, inverse-scale)
d_local  = N_local · p_local0
```

**RESOLVED indirectly, per `ARCH_22_14_GeometryMathAndDispatch.md` point 2 (ratifying
`ARCH_22_12`'s point 1 cross-reference):** ARCH ruled to implement the general (inverse-transpose)
form **unconditionally**, not gated behind first confirming whether placed props ever carry
non-uniform scale (this doc's own Q7). Because this design's §3.2 general formula already handles
non-uniform scale correctly and unconditionally at no extra cost, Q7's answer no longer blocks
anything — ARCH's framing is that the general form costs nothing extra, and both Constitution §6's
"never trust structure blindly" and this pack's standing aversion to silent-correctness traps behind
an unconfirmed assumption argue for the unconditional general path over a conditional fast path
gated on an unverified premise.

**Per-triangle case logic** — classify each vertex by signed local distance
`s_i = N_local · v_i − d_local` (convention: `s ≤ 0` → below-or-on, `s > 0` → above; a small epsilon
only damps float noise near zero, it is not a third tier):

| Above-count | Action |
|---|---|
| 0 (all below/on, including the fully-coplanar case) | Emit the **whole triangle unmodified** into the local-space "below" sub-mesh — coplanar triangles resolve to "below" (conservative, matches §7's zero-missed-pixel doctrine: never under-block). |
| 3 | Emit nothing. |
| 1 above / 2 below | Clip the two edges from the above-vertex to each below-vertex at `s=0` (`t = s_below/(s_below − s_above)`); emit the resulting **quad** (2 below-vertices + 2 new intersection points, as 2 triangles). |
| 2 above / 1 below | Clip the two edges from the below-vertex to each above-vertex at `s=0`; emit the single resulting **triangle** (the below-vertex + 2 intersection points). |

Only this — typically far smaller — local-space "below" sub-mesh is ever transformed to world space
(the regular *forward* point transform this time, not the normal-transform used to derive the
plane), matching the brief's cost argument exactly. The result is projected to world XZ (drop Y) as
a triangle soup, not merged into a single boundary polygon first — rasterizing overlapping/adjacent
triangles directly into a boolean grid (§3.4) is naturally idempotent under OR-accumulation, so
polygon union is an unnecessary extra step.

### 3.3 Which side blocks — the layer-specific slice height (the crux)

Given §1.2's table, the correct cross-section rule is **not one universal slice** — it is a
**layer-specific slice height**, and for the two layers this technique actually serves, the correct
operation is a **slice exactly AT that height**, not "everything below, unioned." (Counter-example
for why "everything below H, unioned" is subtly wrong in general: a hollow mesh with an opening
exactly at height H — e.g. a porthole — has solid material below H elsewhere along the same
column, so "union below H" would wrongly mark that column blocked even though a unit navigating
*exactly at* H could pass straight through. The true cross-section at H does not have this defect.
Both formulations coincide for the vast majority of real terrain-prop meshes, which are not hollow
with mid-height gaps, but the exact-slice formulation is strictly more correct at no extra cost —
it is literally what §3.2's clip algorithm already computes when evaluated at the chosen `H`.)

- **`Sea`** (only if `Water.bEnabled`): `H = waterLevelMaximum`. Sea craft operate at/near the
  surface; the correct test is "does solid material occupy this column at the surface" — the exact
  slice at `H`.
- **`Submarine`** (only if `Water.bEnabled`): `H = waterLevelMaximum − 1.5`, using the exact offset
  the engine's own table uses (§1.2). **Do not hardcode `1.5` as a bare PROC literal** — Constitution
  §8's total-tweakability law applies; expose it as a designer-tunable `PARAMS` field defaulting to
  `1.5` (see Q5 — the alternative, reading it live from the engine's own Lua constant, reopens
  exactly the ARCH §18.2 hazard §4 below is about).
- **`Land`, `Amphibious`, `Hover`, `Air`: explicitly OUT OF SCOPE for this technique.**
  - `Land`'s own native band already starts at sea level — the terrain itself already keeps land
    units above water; the only thing this technique could add is an *above*-water slice of a
    straddling prop, a real but optional refinement, not the core deliverable.
  - `Amphibious`/`Hover` bands are `-∞..+∞` — these layers don't care about the water surface at
    all. The correct obstruction footprint for them is the mesh's **full silhouette across its
    entire height range**, not a plane slice — and it applies equally to props that never touch
    water, which this technique inherently cannot see (a dry prop produces an empty clip and
    contributes nothing).
  - `Air`'s band is likewise `-∞..+∞`, unrestricted by height/slope — structurally the same
    "full silhouette, unrelated to water" case as Amphibious/Hover.

  **RESOLVED by `ARCH_22_12_MaskGenerationAlgorithmAndScope.md` point 3: v1 layer scope is Sea +
  Submarine only**, this doc's Q1 option (a) confirmed, not merely accepted as a recommendation. Land,
  Amphibious, Hover, and Air are ruled explicitly OUT OF SCOPE for this water-plane-slice technique —
  per the six-layer height-band table, now folded directly into `NAVMAP_MODIFIER_BLOCKER_SPEC.md`
  §1.1 (see corrected flag 2 below) — because those four layers' bands are either `-∞..+∞` or
  otherwise water-independent: the water-plane technique is structurally the wrong tool for them, not
  merely an unfinished extension of the right one. A future, structurally different "full mesh
  silhouette" technique is recorded, not designed, as the correct eventual tool for
  Amphibious/Hover/Air (and an optional dry-land Land refinement) — project the instance's full
  mesh/AABB to XZ directly, no plane math, reusing this design's §3.4/§3.5/§7 machinery unchanged. A
  future ticket toward it needs its own separately-scoped design consult before any coder builds
  toward it (ARCH_22_12).

**The `Water.bEnabled` gate:** when false, `Sea`/`Submarine` simply have zero target layers and
produce no output — not an error, exactly mirroring the shipped Lua technique's own documented
"waterless map → silent no-op" behavior (§1.2/`NAVMAP_MODIFIER_BLOCKER_SPEC.md` §2 note).

### 3.4 Rasterization — world→pixel, the inverse of §8

```
col = round( (heightmapResolution − 1) − world.x )   // clamp [0, heightmapResolution-1]
row = round( world.z )                                // clamp [0, heightmapResolution-1]
```

**Correction, superseding this doc's original ARCH-flag 1 (retracted, not merely revised):** an
earlier draft of this section proposed dividing by `worldUnitsPerCell` here, reasoning that it was a
real map-scale factor the engine's own coordinate system might honor. **Verified false by direct
code read, not assumed:** `MapExporter_DocumentAssembly_IO.cpp` writes the `.sanmap`'s real,
engine-facing `width`/`length` fields (confirmed official — the code comment cites the game's own
C# `SanMap.cs` type directly) as `document["width"] = geometry.mapSize` and
`document["length"] = geometry.mapSize` — **literally, with no `worldUnitsPerCell` multiplication
anywhere in that computation.** Since `heightmapResolution` is always `mapSize + 1`
(`Geometry_PARAMS.h::VertexSize()`), the exported map's official size is always exactly
`mapSize × mapSize`, independent of whatever `worldUnitsPerCell` is set to — meaning the **engine's**
own pixel↔world ratio is always exactly 1:1, full stop. `worldUnitsPerCell` is a SanGen-internal
`GeneralMapSettings` round-trip field (not one of the format's documented official top-level keys)
that scales *other* SanGen-internal quantities (entity export positions in `Placement_Emit_PROC.cpp`,
preview rendering, symmetry math) — it never reaches the exported map's actual declared size, so it
is not a relevant input to this formula at all. §8's original raw formula was correct as written;
this design's §3.4 is now restated to match it exactly, with no scaling factor.

**A separate, more serious finding surfaced while checking this (out of scope for this design,
flagged for a different investigation):** because `Placement_Emit_PROC.cpp` DOES scale exported
entity positions by `worldUnitsPerCell` while the exported map's own declared `width`/`length` never
reflects that same factor, any map where `worldUnitsPerCell != 1.0` likely exports every prop/marker/
unit position scaled relative to a map size that itself never changed — a probable general
map-export correctness bug, unrelated to navmesh blockers, worth its own separate investigation.

Rasterize each world-space clipped triangle (§3.2) into the target layer's boolean grid at
`heightmapResolution²` using a standard scanline/edge-function fill (pixel-center-inside test,
consistent with §7's own "threshold at ~50%" style), OR-accumulated — filling is monotonic and
order-independent, so this is trivially parallelizable per triangle (a Compute Optimization Expert
concern, not designed here).

### 3.5 Union across instances, feeding the existing §7 decomposition unchanged

One boolean/coverage grid **per target nav layer** (2 grids given §3.3's v1 scope: Sea, Submarine).
Every qualifying instance (collidable, water-crossing, non-empty clip) of **every** prop
type/template ORs its rasterized triangles into that same layer's grid — union across instances and
across prop types happens here, before §7's unmodified four-step process runs once per layer
(threshold — already boolean, so this step is a no-op; 8-connected component label; drop <20px;
exact zero-overshoot decompose; greedy merge with the human-tunable threshold; mandatory
zero-missed-pixel verify-by-rasterize-and-diff).

**The human-tunable merge threshold, exposed as PARAMS:** recommend one instance **per target nav
layer** (mirroring `MASKING_SPEC.md` §1.7's per-stratum, not-global, precedent — Sea and Submarine
cross-sections will differ in area/shape, exactly the reason different strata get different slope
windows), carrying the merge-overshoot threshold and the component drop-size threshold. Exact field
names/wire keys are the Format Expert's call (Q4); the *category* (a new small `Params::` type, not
folded onto `Stratum`) is this document's recommendation.

### 3.6 Output contract

Per target nav layer: an ordered `{x, z, sizeX, sizeZ}` world-space rectangle list, using §8's
existing **forward** pixel→world formula (no new conversion needed at this end) — the exact shape
`NAVMAP_MODIFIER_BLOCKER_SPEC.md` §3/§4/§9's worked example already shows working. Ready for a
downstream IO/export ticket (not designed here) to fold into the per-map Lua orchestration.

---

## 4. Accuracy class and the Exact/Deterministic-chain question (the task's central ask)

### 4.1 The geometry math itself
Per-triangle plane-classify/clip, rasterize, and §7's exact largest-rectangle pass are all
**Exact class** (Constitution §4): a missed pixel is a real hole in pathing blocking, the same
character of error as a wrong slope→passability classification, and §7's own process already
enforces zero-missed-pixel verification as a non-negotiable acceptance gate — it is decision-exact
by its own existing definition, just not yet labeled with SanGen's vocabulary. The agglomerative
merge pass is **Accurate class** — it has a stated, human-tunable tolerance (the overshoot
threshold), exactly Constitution §4's Accurate definition, and is neither Exact nor Visual.

### 4.2 The determinism question, reasoned explicitly
`ARCH_18_02_IngestedDataDeterminism.md` (§1.8 above) rules that install-sourced, non-`PARAMS` data
(mesh geometry is exactly this class, same as footprint) must never be read live by `PROC` — it
must bake once, human-triggered, into `PARAMS`. The reason that rule exists: Constitution §4's
Deterministic sub-mode requires two machines to reproduce **bit-identical gameplay-authoritative
output from `PARAMS`+seed alone, with no file transfer** (`DETERMINISM_SPEC.md`: "the host sends
only settings+seed... so the large heightmap/texture files never have to be transferred") — a mode
where each player's own machine *independently regenerates* the map, including Placement (which
`MASKING_SPEC.md` §1.6 confirms sits in the Output Exact chain for exactly this reason).

**Why this case may be meaningfully different, as the task's own framing suggested:** the navmesh-
blocker *result* — the rectangle list — is, by the current shipped mechanism
(`NAVMAP_MODIFIER_BLOCKER_SPEC.md` §3/§4, `ARCH_22_09`), always delivered to players as a **static,
per-map, exported Lua artifact that ships with the map package** — the same distribution model as
`heightmap.raw`/stratum TGAs, i.e. the "file transfer" model `DETERMINISM_SPEC.md` exists
specifically to make *optional* for terrain, **not** the "settings+seed, no file transfer" live
per-client regeneration model. Every player with the map file sees the identical shipped
rectangles regardless of what game install (or mesh data) any of them individually has — this is
actually a *stronger*, trivial form of cross-machine agreement (byte-identical because transferred,
not because independently recomputed) than `DETERMINISM_SPEC.md`'s own bar requires.

**Conditional conclusion:** provided this feature only ever participates in the ordinary
"generate once on the designer's machine, export/ship the result" model — the *only* model its
downstream consumer (the exported Lua orchestration file, which `ARCH §22.9` confirms lives in the
never-SanGen-written `<MapName>_data.lua` orchestrator and its SanGen-owned, export-only companions)
currently supports — the mesh-derived input can safely stay exactly where footprint's *raw* ingested
form stays today: `IO`-owned, Visual-class, read once at a human-triggered bake/export action, with
no obligation to further bake the raw mesh into a `PARAMS` scalar first, because nothing downstream
of this stage is ever independently regenerated by a second machine at all.

**The risk this rests on, stated plainly:** this conclusion is conditional on the navmesh-blocker
Lua export permanently staying in the "shipped file" model. Nothing in the current spec pack rules
out a future extension of the "settings+seed, no file transfer" Deterministic sub-mode to cover
these exported files too. **Recommend hedging regardless of which model turns out to govern**: bake
the *resolved rectangle list* (small, table-shaped — already the natural output of §3.6) once,
human/export-triggered, into an ordinary `PARAMS`-serialized field, exactly mirroring §18.2's own
mechanism — this costs nothing extra (the design already produces that shape) and forecloses the
risk permanently if ARCH ever extends the live-regeneration mode to this feature. What must NOT
happen, under either reading, is wiring the *raw mesh* into an implicit, dirty-hash-driven
`RegisterStages()` PROC stage that re-reads the mesh-ingestion table on every regeneration (§5).

**This is presented as reasoning toward a recommendation, not an assertion of settled law — ARCH
should rule on it explicitly (see ARCH-flag 4 / Q2).**

---

## 5. PIPELINE integration — an on-demand bake, not a `RegisterStages()` dirty-hash stage

This is genuinely new `PROC` work (the slice/rasterize kernels are CPU+GPU pairs per Constitution
§1's PROC definition), but it should **not** be registered via `GenerationAssembler::AddStage(...)`
the automatic way NoiseBlend…Bake are (`GenerationAssembler_Stages_PIPELINE.cpp`). Two independent
reasons converge on the same answer:

1. It needs `Data::PlacementResults.props` **fully resolved** — i.e. it must run strictly after
   Placement, the same dependency `BuildSpatialGridSet()`/`BuildRuleBucketIndex()` already have
   (today satisfied by running inline, in the same registered Placement stage-run).
2. Its raw mesh input is exactly the asset-derived, install-sourced, non-`PARAMS` category
   `ARCH_18_02` already rules must never be read implicitly inside the automatic regeneration loop
   (§4.2).

**Recommendation:** an explicit, human/export-triggered entry point — "Resolve navmesh blockers,"
mirroring §18.2 point 1's "explicit resolve footprints action" — invoked on demand (from a future
tab, or as part of export), never wired into `RegisterStages()`. It snapshots the *current* baked
`Data::PlacementResults.props` + the *current* `Params::Water` + the *current* mesh-ingestion table,
runs §3.2→§3.5, and writes the result into the new `PARAMS`-owned rectangle-list type (§6). A later
Placement re-run (a designer tweaks a scatter rule) does **not** auto-invalidate the baked blocker
set — mirroring §18.2 point 4's "un-baked/stale is not an error state" — the designer re-triggers
the bake explicitly. A staleness hint (compare last-baked-against Placement hash vs. current) is a
reasonable UI affordance but never an auto-recompute; exact mechanism deferred to the UI Expert.

**This is a genuinely new PIPELINE capability**, distinct from both existing patterns: it is not an
automatic dirty-hash stage (unlike NoiseBlend…Bake), and it is not `ARCH §16.3`'s stateless query
passthrough either (that mechanism is read-only/side-effect-free; this bake *writes* a new `PARAMS`
value). RESOLVED by `ARCH_22_13_BakedArtifactStorageAndDeterminism.md` point 3: neither — it is its own new
PIPELINE responsibility class, "the one-shot, human-triggered bake entry point," distinct from both
§3.3's dirty-hash DAG and §16.3's stateless query passthrough (see corrected Q6).

---

## 6. PARAMS / DATA / IO shape needs — flagged, not designed

| Layer | New surface | Category (this doc's recommendation) | Exact shape |
|---|---|---|---|
| `IO` | Per-template mesh geometry table | Visual-class, mirrors `Io::WorldFootprintSizeTable` (`ARCH_18_03`) | Format Expert (parallel design) |
| `PROC` | 2–4 new kernel files (plane-slice, layer-slice-height, rasterize, decompose-wrapper) | Exact class (slice/rasterize/coverage-decompose), Accurate class (merge pass) | This doc, §3 + ticket table §7 |
| `PARAMS` | Per-nav-layer merge/drop-size settings | New small type, mirrors `MASKING_SPEC` §1.7's per-target pattern | Format Expert |
| `PARAMS` | Resolved rectangle-list output | `ENTITY_AUTHORING_PARAMS_SPEC`'s "resolved/baked per-map entity data" family — **not** `Data::` (must survive independent of a fresh PROC run, must `.sanmap`-round-trip) and **not** `Io::`-only (unlike the raw ingested source, §18.3, this IS the human-baked, hand-editable result) | Format Expert |
| `PIPELINE` | The on-demand bake entry point (§5) | New verb, not a `RegisterStages()` stage | ARCH (Q6) |
| unchanged | `Data::PlacementResults.props`, `Params::Water`, `Params::Geometry` | Reused verbatim — confirmed by direct read, no changes needed | — |

---

## 7. Proposed work-order breakdown (enumeration only)

Highest `STEP` number sampled this session: `STEP226` (per `INDEX.md`'s own text); proposed numbers
below start at **227** and should be re-confirmed against the live `work_orders/` directory at
dispatch time, same caveat the reference design doc records for its own numbering.

| # | Ticket | Layer | One-line scope |
|---|---|---|---|
| **227** | `NavmeshBlocker_PlaneSlice_PROC` | **PROC** | Per-instance local-space plane derivation (§3.2's `Sᵀ`/scale-after-rotate formula) + per-triangle clip (0/1/2/3-above case table), CPU+GPU pair, Exact class. Depends on mesh-ingestion's per-template table (parallel ticket) and `Data::PlacementResults.props`. |
| **228** | `NavmeshBlocker_LayerSliceHeight_PROC` | **PROC** | Small: resolves the per-target-layer slice height (`Sea = waterLevelMaximum`, `Submarine = waterLevelMaximum − submarineDepthOffset` [new tunable, default 1.5]) and the `Water.bEnabled` gate (mirrors `Engine.HasWater()`). |
| **229** | `NavmeshBlocker_Rasterize_PROC` | **PROC** | World-space clipped-triangle → per-layer boolean grid at `heightmapResolution²`, world→pixel (§3.4, generalized for `worldUnitsPerCell`), OR-accumulated across instances/prop types. CPU+GPU pair, Exact class. |
| **230** | `NavmeshBlocker_Decompose_PROC` | **PROC** | First real SanGen implementation of §7's unmodified four-step process (component-label+drop, exact zero-overshoot decompose, integral-image-driven greedy merge, mandatory verify-by-rasterize-diff). Exact class for coverage, Accurate class for the merge/overshoot step. |
| **231** | `NavmapModifierBlocker_PARAMS` | **PARAMS** | New per-nav-layer merge/drop-size settings type + the resolved rectangle-list output type (§6). Exact field layout/wire keys: Format Expert. |
| **232** | `NavmeshBlocker_Bake_PIPELINE` | **PIPELINE** | The on-demand, human/export-triggered bake entry point (§5), composing 227→228→229→230, snapshotting current `Data::PlacementResults.props`/`Params::Water`/mesh-ingestion table, writing into 231's type. Explicitly NOT registered in `RegisterStages()`. |
| 233+ | (parallel/downstream, not designed here) | — | Mesh-ingestion IO (Format Expert's parallel design); Lua/export IO ticket; UI trigger + staleness hint (UI Expert). |

Dependency order: mesh-ingestion (parallel) → 227 → 228 → 229 → 230, strict chain. 231 can be
drafted in parallel with 227–230 (shape only depends on §3's algorithm, not its implementation).
232 depends on all of 227–231. UI/export tickets depend on 232.

---

## ⚠️ Flagged for the ARCH Expert — I do not edit `ARCH.md`/any `ARCH_NN_*.md`

1. **RETRACTED (was: "§8's pixel↔world formula is silent on `worldUnitsPerCell` scaling").** This
   flag was wrong and is withdrawn, not merely softened. Verified by direct read of
   `MapExporter_DocumentAssembly_IO.cpp`: the exported `.sanmap`'s real, engine-facing `width`/
   `length` fields are always `geometry.mapSize`, with no `worldUnitsPerCell` factor anywhere in
   that computation — the engine's own pixel↔world ratio is therefore always exactly 1:1, regardless
   of that setting. `worldUnitsPerCell` is a SanGen-internal round-trip field that never reaches the
   exported map's declared size. §8's original formula needs no correction. See §3.4's own updated
   text for the full reasoning and the separate, more serious export-consistency issue this surfaced
   (entity positions ARE scaled by `worldUnitsPerCell` on export while the declared map size is
   not) — flagged there as its own, unrelated investigation candidate, not part of this feature.
2. **RESOLVED.** ARCH folded the full six-layer height-band table directly into
   `NAVMAP_MODIFIER_BLOCKER_SPEC.md` §1.1 ("The six navigation layers and their height bands,"
   ratified 2026-08-30, citing `ARCH_22_12_MaskGenerationAlgorithmAndScope.md`) — no longer confined
   to the informal `forum_posts/TUTORIAL_NavMeshBlockers.md` source. §1.2 above should be read as
   superseded by that ratified §1.1 text.
3. **RESOLVED — the gate is lifted.** `ARCH_22_10_MeshIngestionOwnershipRuling.md` rules that this
   document, together with the parallel Format Expert, Compute Optimization Expert, and UI Expert
   consults (all dated 2026-08-30), collectively ARE the "real, separately-scoped design consult"
   §22.9 required — its gate is satisfied for the file/ticket set `ARCH_22_10` through `ARCH_22_17`
   name. §22.9's own text stands, historical rather than retracted (it correctly described the
   architecture's state before these four consults existed; the hand-authored Lua half, §22.1-§22.8,
   is unaffected). ARCH_22_10's layer-membership table records exactly which layer each new
   capability enters (SYS/IO §22.11, MATH §22.14, PROC §22.12, PARAMS §22.11/§22.15, PIPELINE §22.13,
   UI §22.15-§22.17). This ratification is scoped, not a blank check — it authorizes exactly the
   tickets this doc's own §7 breakdown enumerates (227-232), corrected/superseded wherever
   §22.11-§22.17 diverge from this doc's own open questions (see corrections 1, 2, 4, 5, 6, 7 above
   and below).
4. **RESOLVED by `ARCH_22_13_BakedArtifactStorageAndDeterminism.md`.** Storage RULED: an ordinary
   `Params::` field, never a companion export-only `.lua` artifact (ARCH_22_13 point 1) — this doc's
   Q2 option (a), bake unconditionally, confirmed as the load-bearing structural choice, not merely a
   hedge. Stated reasoning: it mirrors `ARCH_18_02_IngestedDataDeterminism.md`'s already-shipped
   footprint-bake mechanism exactly, no third mechanism is invented, and because `Params::` is
   definitionally the "settings" `DETERMINISM_SPEC.md`'s "the host sends only settings+seed" already
   transports, a `Params::`-resident rectangle list travels bit-identically to every shared-generation
   peer automatically as ordinary recipe payload — not because it is independently recomputed and
   happens to agree, but because it is literally the same transported bytes. The Exact/Deterministic-
   chain question itself is separately RULED (ARCH_22_13 point 2): this feature sits entirely outside
   `DETERMINISM_SPEC.md`'s bit-exact regeneration bar, settled rather than merely "arguably outside"
   — see corrected flag 5 below for the mechanism ARCH actually added to the spec.
5. **CORRECTED, not merely resolved — ARCH did not adopt this doc's recommendation as written.**
   This doc originally recommended adding navmesh blocker rectangles to `DETERMINISM_SPEC.md`'s
   bit-exact Scope list. `ARCH_22_13_BakedArtifactStorageAndDeterminism.md` point 4 did NOT do that.
   Instead it added a new, distinct section directly below `DETERMINISM_SPEC.md`'s `## Scope`
   section — "A third category — gameplay-authoritative data that achieves parity by transport, not
   recomputation" — explaining that some gameplay-authoritative `Params::` fields (the baked
   prop-footprint scalar per `ARCH_18_02`, and the baked navmesh-blocker rectangle list per this
   ruling) sit **outside** the Scope list entirely: they are baked once, human-triggered, from an
   external source into an ordinary `Params::` field, and achieve cross-machine parity by
   **transport** (the field rides with settings+seed like any other recipe data, so every peer
   receives the literal same bytes) rather than by independent per-machine bit-exact recomputation.
   They are explicitly NOT exempt/visual either — a navmesh blocker remains gameplay-authoritative,
   arguably more directly pathing-critical than "collidable props" itself — but the Scope list
   specifically governs values more than one machine independently regenerates from seed, and these
   values are computed by exactly one machine (the author's), once. The spec further states these
   fields must never be wired into a live-regenerated PROC stage, matching this doc's own §5 warning.

---

## ❓ Open questions — decisions with options

**Q1 — RESOLVED.** Option (a), Sea+Submarine only for v1, confirmed by
`ARCH_22_12_MaskGenerationAlgorithmAndScope.md` point 3. See corrected §3.3 above.

**Q2 — Bake the rectangle list into `PARAMS` regardless of which distribution model governs, or
only if/when ARCH extends the live-regeneration mode to cover this feature?**
*(a)* Bake now, unconditionally (this doc's recommendation, §4.2) — free insurance, zero rework
later. *(b)* Skip the bake step; read the mesh-ingestion result live at export time only, revisit if
the live-regeneration mode is ever extended. **Recommend (a).**

**Q3 — Where does the resolved rectangle-list `PARAMS` type live?**
Recommend the `ENTITY_AUTHORING_PARAMS_SPEC` "resolved/baked" family (§6) — Format Expert's call on
exact shape/file.

**Q4 — Per-nav-layer merge/drop-size settings, or one global pair?**
*(a) Per-layer* (this doc's recommendation, mirrors `MASKING_SPEC` §1.7's per-stratum precedent).
*(b) Global.* (a) costs one more settings surface; (b) is simpler but conflates two layers with
different cross-section characteristics. Human's call.

**Q5 — The Submarine `-1.5` offset: `PARAMS`-tunable default, or a live read of the engine's own
Lua constant at ingestion time?**
*(a) PARAMS-tunable default seeded from the confirmed value* (this doc's recommendation) — consistent
with the rest of this design's determinism posture (§4.2), never a live external read.
*(b) Live engine read* — more accurate if the engine constant ever changes, but reopens exactly the
`ARCH §18.2` hazard for a single scalar. **Recommend (a).**

**Q6 — RESOLVED.** `ARCH_22_13_BakedArtifactStorageAndDeterminism.md` point 3 names a new PIPELINE
responsibility class: **"the one-shot, human-triggered bake entry point,"** explicitly NOT an
extension of §16.3. It is ruled distinct from both existing PIPELINE responsibilities: not §3.3's
dirty-hash DAG orchestration (never `AddStage`/`RegisterStages()`, never auto-re-run, no dirty-hash
edges), and not §16.3's stateless query passthrough (§16.3 point 1 requires the wrapped function be
already, independently pure with no `DATA` dependency and no side effects, re-callable every frame
with no carried state — this bake reads `Data::PlacementResults.props` plus IO-sourced mesh data and
**writes** a new `Params::` value, failing both tests by design). Binding shape: its own narrow
`PIPELINE` file (e.g. `NavmeshBlockerBake_PIPELINE.h/.cpp`), sized/scoped like
`PreviewDriver_PIPELINE.h`; runs strictly after Placement; never wired into `RegisterStages()`,
invoked only by an explicit human/UI/export action; snapshots current `Data::PlacementResults.props`
+ `Params::Water` + the mesh-ingestion IO table and writes the resolved rectangle-list `Params::`
field (`ARCH_22_15_NavmeshTabParamsShape.md`); a later Placement re-run does not auto-invalidate the
bake (mirrors `ARCH §18.2` point 4's "un-baked/stale is not an error state").

**Q7 — RESOLVED indirectly, per `ARCH_22_14_GeometryMathAndDispatch.md` point 2.** ARCH ruled to
implement §3.2's general (inverse-transpose) form unconditionally regardless of whether placed props
ever actually carry non-uniform scale — the answer to the underlying question no longer changes any
implementation decision, so it no longer blocks anything. See corrected §3.2 "Flag" paragraph above.
