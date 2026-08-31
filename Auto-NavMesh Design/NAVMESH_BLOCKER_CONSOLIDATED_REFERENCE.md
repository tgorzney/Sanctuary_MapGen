# Navmesh Blocker Feature — Working Technical Reference

**NOT official documentation.** This is a working reference for calculations/findings worth keeping
across sessions — the `.sanmodel` format decode, the transform math, the algorithm shape. It is not
kept in lockstep with every change and should not be treated as binding. The real, current plan lives
in `EXECUTION_PLAN.md` in this same folder — read that first.

Background: this folder condenses four earlier design docs plus a since-frozen ARCH ratification
(`ARCH_22_10`-`ARCH_22_17`, left as-is at the repo root, not extended further). Several corrections
below post-date that ratification and supersede parts of it in practice — see `EXECUTION_PLAN.md`
§0 for the current, corrected understanding.

---

## 0. Workflow (user-facing, as specified)

1. User selects a set of placed props on the canvas (any selection mechanism already in the app).
2. Each nav-type section (Land/Amphibious/Hover/Air/Sea/Submarine) in the Navmesh Tab has a
   **"Create Navmesh Blockers" button, right-aligned on that section's header.**
3. Clicking it runs the pipeline **only against the currently-selected props**, for that section's
   layer only.
4. Pipeline, per selected prop instance: resolve its blueprint → resolve LOD0 mesh path → load the
   mesh → **apply that instance's own position/rotation/scale to place it correctly in 3D space**
   → intersect against the water plane → accumulate into a bitmask → decompose bitmask to
   rectangles → write into that nav-type's layer as `MeshGenerated` rectangles.
5. **CORRECTED (superseded a prior draft of this doc): all six section buttons exist in the UI from
   the start, disabled until that layer's algorithm is proven.** Only Sea is enabled first. As soon
   as Sea works, Air is next (its own, different algorithm — slope-based, not water-plane — see §3.2).
   Submarine/Amphibious/Hover/Land follow later. Nothing is hidden; unproven layers are just disabled
   with an explanation.

**On the "scale first" question:** confirmed mathematically sound to *not* scale the mesh to world
space first. See §2's derivation — transforming the water plane into the prop's local space (instead
of transforming every mesh vertex into world space) produces an **identical** result for any
per-instance scale, including non-uniform scale, because it's the same equation solved from the
other direction, not an approximation. Every placed instance's own individual scale is fully
accounted for. This is cheaper (one plane transform per instance vs. transforming every vertex) with
zero accuracy loss.

The trigger mechanism above (selection-scoped, per-section button) is more specific than what the UI
design doc originally left open (Q5 there asked "own trigger vs. extend the template-ingest button" —
this answers it: neither, it's its own button, scoped to selection, per nav-type section).

---

## 1. Mesh ingestion (Format)

### 1.1 `.sanmodel` binary format — fully decoded, ground truth from the real open-source Blender importer
```
<name-string, NUL-terminated>
9 fixed segments, each: [count][count * N floats], little-endian:
  0 vertices(3)  1 normals(3)  2 tangents(4)  3 uv1(2)  4 uv2(2, =boneweights if skinned)
  5 uv3(2)  6 colors(4)  7 indices(3, per triangle)  8 bindposes(16, only if skinned)
```
- **Critical gotcha:** the `count` field and every index value in segment 7 are raw `int32` bit
  patterns stored in a 4-byte float slot — read with `std::bit_cast<int32_t>`, **never**
  `static_cast<int>`/`round`. Wrong cast silently corrupts every segment boundary and every index.
- Coordinate system: Unity-style Y-up, **local/model space** (not world space).
- No separate collision mesh exists in this format or in the game's data at all — the render mesh
  (whichever LOD) is the only real geometry available, full stop.

### 1.2 LOD0 resolution
- `visuals.lods[]` array in the prop's `.santp`, each entry `{distance, model, material}`.
- **Selection rule: minimum-`distance` entry**, not array index 0 (self-correcting against
  malformed/reordered files).
- **Never synthesize a model path from tpId/folder name** — confirmed real defect:
  `Environment/01_Highlands/Props/edmm0101/` contains files literally named `edms0103_lod0.sanmodel`
  through `_lod4` (folder name ≠ model filename stem). Always read the literal `model` string.
- Dialect-B props (4 engine-lua test templates: `exe0000`-`exe0002`, `defaultWreckage`) have **no**
  `lods[]`/model path at all — graceful skip, never a hard failure, expected to affect ~0 real
  instances.
- Static, non-skinned props only. Skinned meshes (segment 8 present) get rest-pose geometry only;
  every real Environment prop has an empty skeleton field anyway (checked, zero exceptions found).
- Units already have their own native blocker (`skirtSize`) — this feature is **props only**.

### 1.3 Two prop populations, two different resolution paths (real finding, not invented)
- **Procedural** (scatter-generated): walk `Data::PlacementInstances` where `manualLayerId == -1`.
  `templateIdentifier` (tpId) is populated correctly. Transform lives in the same DATA columns.
- **Manual** (hand-placed): walk `Params::MapRecipe::props` (`PropInstanceGroup::blueprintPath` +
  `PropTransform`) **directly** — `Data::PlacementInstances::templateIdentifier` is **empty** for
  every manual instance (confirmed: `Placement_Manual_PROC.cpp` never sets it; deliberate design,
  not a bug). Do not try to recover manual identity from DATA.
- **CORRECTED: `bCollidable` is NOT used anywhere in this feature.** An earlier draft proposed
  gating mesh-intersection eligibility on a prop's `bCollidable` flag (and even proposed adding that
  field where it was missing on manual props). That's wrong for two reasons, both stated directly:
  not all props have accurate collision data, and collision is not accurate to the real mesh anyway
  — mesh accuracy is the entire point of this feature. **The real filter is the user's own selection**
  (§0): whichever props are selected on canvas when the section's button is clicked are the ones
  processed. No PARAMS flag needed, no fix needed to `Placement_Manual_PROC.cpp` for this feature.
- **`.sanprop` is deprecated — `.santp` only.** A prop whose only source is a `.sanprop` file is
  simply unsupported (same graceful-skip treatment as the engine-lua test templates in §1.2).

### 1.4 New IO/SYS pieces (all ratified)
| Piece | Layer | Ruling |
|---|---|---|
| `Sys::SanmodelMesh` / `ReadSanmodelMesh()` | SYS | New binary reader. Positions + indices only (no normals/UV/color/bindposes carried forward). Bounds-checked, capped byte size, never trusts a `count` field — Constitution §6 (untrusted third-party asset data). Not Lua execution — no sandbox machinery needed, just buffer-in/struct-out. |
| `visuals.lods[]` extraction | IO | **RULED:** new additive sibling file `src/io/TemplateVisualLod_IO.h/.cpp` — does **not** extend the shipped `Io::TemplateRecord` (that option was rejected — reopening a shipped type's scope for one feature is exactly what's being avoided). |
| `Io::DetectTemplateRootTable(...)` | IO | **RULED binding:** the existing private `DetectRootTable` branch in `TemplateDialect_IO.cpp` is extracted to a shared function, reused by both the existing file and the new sibling file. |
| `Io::ResolvePackRelativeAssetBytes(...)` | IO | New — resolves a pack-relative path to bytes (unzipped tree first, `SanpackReader` fallback). Reuses `SanpackReader::ExtractFiltered` (already returns raw bytes) — no new zip code. |
| Per-template mesh cache | IO | **RULED: in-memory, run-scoped only** — no disk cache. Corpus per bake is "however many distinct templates are placed on one map" (tens), not the whole install. |

### 1.5 What already ships and is reused unmodified
`LuaTableEvaluate_SYS` (sandboxed Lua eval), `SanpackReader_IO`, `TemplateIngestReport::
FindByTemplateIdentifier` (resolves a tpId to its `.santp` source path for free), `AppSettings::
gameInstallRoot`/`ValidateGameInstallRoot`. None of this needed to be redesigned.

---

## 2. Math (Compute Optimization)

### 2.1 The core derivation — why "transform the plane, not the mesh" is exact, not approximate
Instance transform: `p_world = T + R·(S·p_local)` (translate ∘ rotate ∘ scale). World plane:
normal `N_world`, height `H`. Substituting and solving for the local-space plane:
```
N_local = S · (Rᵀ · N_world)     — inverse-rotate the normal, THEN scale componentwise (multiply, never divide)
d_local = H − N_world·T          — (for a horizontal plane; general case subtracts N_world·T)
```
This is algebraically the same equation as transforming the mesh forward and intersecting in world
space — **not a cheaper approximation, the identical result**, valid under uniform or non-uniform
per-axis scale. ARCH ruled this general (inverse-transpose) form is used **unconditionally**,
regardless of whether real props ever actually carry non-uniform scale — costs nothing extra, removes
a silent correctness trap either way.

### 2.2 New/extended MATH primitives (all in `src/math/`)
- Extend `RigidTransformPivot_MATH.h` (currently only has 2D pivot-rotate + quaternion multiply +
  yaw-construction) with: `RotateVectorByQuaternion`, `TransformPointByRigidTransform`,
  `InverseTransformPointByRigidTransform`, `InverseTransformPlaneByRigidTransform` (the actual
  primitive this feature needs).
- New file `TrianglePlaneIntersection_MATH.h`: `ClassifyTriangleAgainstPlane` (signed distances +
  sign bitmask) and `TrianglePlaneIntersectionSegment` (lerp the crossing edges). Pure algebra, no
  transcendentals — **Exact class**, trivially bit-identical if ever ported to GPU.
- Nothing in `src/math/` already does triangle-plane intersection or point-in-polygon/rasterization
  — confirmed by direct grep, these are genuinely new.

### 2.3 Dispatch — RULED: CPU-only, no GPU
Runs at author-time (one-shot batch, human-triggered), not per-frame — GPU upload/dispatch/readback
overhead would dominate for this workload shape, unlike the resident-grid stages (Noise/Erosion) that
amortize it across many passes. Per-instance work is embarrassingly parallel — thread-pool
partitioning by instance is the right lever, not GPU. Per-vertex classification vectorizes trivially
across `FloatVector_MATH`'s SIMD lanes as a future follow-up once real triangle/instance counts are
known — not a launch requirement.

**Performance principle, stated explicitly:** transforming one plane into local space is O(1) per
instance regardless of mesh size; transforming every vertex to world space is O(vertex count) per
instance. For a massively-scaled prop where only a sliver of the mesh is near the water line, this is
the difference between O(instances) and O(instances × vertices) — confirms §0's "scale first?"
question is resolved correctly by NOT scaling the mesh first.

### 2.4 Determinism — confirmed, no bit-identical cross-machine requirement
Runs once, author-time, produces a static baked result — never independently recomputed by more than
one machine. No portable-transcendental / ordered-reduction discipline needed; standard `std::sqrt`/
hardware FMA are fine on pure performance grounds.

---

## 3. Algorithm (Generator)

### 3.1 Six nav layers — height bands (now folded into `NAVMAP_MODIFIER_BLOCKER_SPEC.md` §1.1 directly)
| Layer | Always created? | Height band | Max slope |
|---|---|---|---|
| Land | yes | sea level to +∞ | 30° |
| Amphibious | yes | -∞ to +∞ | 30° |
| Hover | yes | -∞ to +∞ | 30° above water, ∞ below |
| Air | yes | -∞ to +∞ | ∞ |
| Submarine | only if `Engine.HasWater()` | -∞ to (waterLevel − 1.5) | ∞ |
| Sea | only if `Engine.HasWater()` | -∞ to waterLevel | ∞ |

### 3.2 v1 scope — build order, not permanent exclusion
Sea ships first (this is the water-plane-slice algorithm below). Land/Amphibious/Hover are a
genuinely different technique (full mesh/AABB silhouette projected to XZ, no plane math — their bands
are `-∞..+∞` or otherwise water-independent) — not designed yet. **Air is next after Sea, and is a
third, different algorithm again — not silhouette, not water-plane:** a local terrain-slope test
(same per-cell gradient method the existing Mask stage already uses for its own 30° slope gate, just
at a 60° threshold) combined with flight height — terrain that's both above flight height AND steeper
than the slope threshold blocks; gently-sloped rising terrain does not (aircraft climb with it). Any
region fully enclosed by blocked terrain must also be marked blocked even if its own floor doesn't
exceed the slope threshold (a flood-fill/enclosed-region pass on the mask before decomposition) — real
requirement, not yet designed in detail. Whether all air units share one flight height is unconfirmed,
to check when Air work starts.

### 3.3 Per-instance algorithm
1. **Filter: the user's canvas selection at button-click time** (§0/§1.3) — not a PARAMS flag.
2. **Cheap early-out:** transform the template's local Y-bounds through the instance's scale; skip
   the instance entirely if it can't possibly straddle the target layer's slice height. Most placed
   props never touch water — this is the dominant cost saver.
3. **Derive the local-space plane** for the surviving instance (§2.1's formula), once — not per
   vertex.
4. **Per-triangle classification/clip** (signed distance `s = N_local·v − d_local`; `s≤0`=below,
   `s>0`=above):

   | Above-count | Action |
   |---|---|
   | 0 (incl. coplanar) | emit whole triangle unmodified (conservative — never under-block) |
   | 3 | emit nothing |
   | 1 above / 2 below | clip to a quad (2 triangles) |
   | 2 above / 1 below | clip to 1 triangle |
5. Only the resulting (typically much smaller) "below" sub-mesh is transformed forward to world
   space — never the whole mesh.
6. **Slice height per target layer:** `Sea` uses `H = waterLevelMaximum`; `Submarine` uses
   `H = waterLevelMaximum − submarineDepthOffset` (new PARAMS-tunable, default `1.5`, **never** a
   live read of an engine constant — keeps this out of the determinism hazard zone). Exact slice AT
   height H is used, not "everything below H unioned" (the exact-slice form is strictly more correct
   at no extra cost — handles a hollow mesh with an opening at height H correctly, where "union
   below" would not).
7. **Rasterize** the world-space clipped triangles into a per-layer boolean grid at
   `heightmapResolution²`, OR-accumulated across every qualifying instance of every prop type.
   `col = round((heightmapResolution−1) − world.x)`, `row = round(world.z)` — **confirmed no
   `worldUnitsPerCell` scaling needed** (see §7.1 below — this was investigated and the original
   concern was disproven).
8. **Decompose** the finished mask using the **existing, already-proven, unmodified** algorithm from
   `NAVMAP_MODIFIER_BLOCKER_SPEC.md` §7 (this is real prior art, already used live in-game on
   Pandemonium Isthmus — not redesigned here): threshold ~50%, 8-connected component label, drop
   <20px components, exact zero-overshoot largest-rectangle decomposition (100% coverage by
   construction), then greedy agglomerative merge via a 2D integral image up to a human-tunable
   overshoot threshold, verified by rasterizing the kept-rectangle union and diffing against the true
   mask (missed-pixel count must be exactly zero).

### 3.4 Accuracy classes
Plane-classify/clip/rasterize/exact-decompose = **Exact class** (a missed pixel is a real pathing
hole). The agglomerative merge pass = **Accurate class** (has a stated human tolerance).

### 3.5 PIPELINE placement — RULED: a new, third kind of PIPELINE responsibility
Not an automatic `RegisterStages()` dirty-hash stage (unlike Noise/Erosion/Thermal/etc. — this must
never auto-re-run on every regeneration, since its raw mesh input is install-sourced, non-`PARAMS`
data). Not an extension of the existing stateless-query passthrough either (that mechanism must be
read-only/side-effect-free; this **writes** a new PARAMS value). **RULED:** its own new PIPELINE
responsibility class — "the one-shot, human-triggered bake entry point" (`NavmeshBlockerBake_PIPELINE.
h/.cpp`) — runs strictly after Placement, never wired into `RegisterStages()`, invoked only by the
button in §0. A later Placement re-run does **not** auto-invalidate an existing bake.

---

## 4. Determinism / where the baked data lives — RULED

- **Storage: an ordinary `Params::` field** (not a companion export-only `.lua` file). Rides inside
  the `.sanmap`'s settings+seed payload exactly like the already-shipped baked footprint scalar does
  — trivially bit-identical across any shared-generation peers because it's literally the same
  transported bytes, not because it's independently recomputed.
- **Outside the Deterministic bit-exact chain, structurally** — not merely "arguably outside."
  Enforced by construction: the bake never runs inside the live PROC regeneration DAG (§3.5), so no
  peer machine ever "regenerates" this value from seed — every peer just receives the already-baked
  result. `DETERMINISM_SPEC.md` has been amended with a new section recording this general pattern
  (applies to both this feature and the earlier footprint-bake precedent): gameplay-authoritative
  `Params::` fields that achieve cross-machine parity by **transport**, not recomputation, sit outside
  the bit-exact Scope list — while still remaining gameplay-authoritative (not visual/exempt) and
  still barred from ever being wired into a live-regenerated PROC stage.
- **Mechanism mirrors the existing shipped footprint-bake pattern exactly:** discrete human-triggered
  bake action (never implicit inside Generate/export), a staleness fingerprint over the actual
  external input (the resolved `.sanmodel` files consumed), and the baked result stays an ordinary,
  re-editable value after baking — never a read-only mirror.

---

## 5. Navmesh Tab UI (structure)

### 5.1 Already-existing generic widgets reused, zero new widget-library code
ARCH's own prior ruling (`ARCH_19_02`) **pre-named Navmesh** as the third proof of "Group/Bundle tree
mechanics are pure and generic" (Markers = 1st proof, Props/Decals = 2nd). `TreeListWidget_UI<T,
LeafKeyT>` and `DraggableListWidget_UI<T>` are reused as-is; Navmesh gets its own hand-written
`Params::NavmeshBlockerLayerBundle`/`NavmeshBlockerLayer` structs (never a shared/discriminated
cross-domain struct — that's against this codebase's own established law).

### 5.2 Layout
- **One Section per nav layer** (fixed 6-entry list, not dynamically enumerated — the set is closed
  engine ground truth). Sea/Submarine sections are **entirely hidden** (not grayed) when
  `!Params::Water::bEnabled`, re-evaluated live every frame.
- **Group→Layer tree** within each section, `TreeListWidget_UI<NavmeshBlockerLayerBundle, int>` —
  leaf key collapses to a plain `int` (Navmesh has only one leaf kind, unlike Markers which needs a
  discriminated union for procedural-vs-manual leaves — Navmesh is manual-only, so this is simpler by
  construction, not by omission).
- **Layer row:** name, color/tint override, lock, hide, grid snap. **No symmetry section** (real
  geometric reason: a quarter-turn requires swapping `sizeX`/`sizeZ`, which this codebase's orbit math
  was never built to do; a Radial orbit produces a non-axis-aligned rectangle the engine format can't
  represent at all — same limitation `Params::MapArea` already has and never got a symmetry field
  for). **No icon-scale field** (rectangles have no icon; size is edited directly).
- **Rectangle list per layer:** Ctrl/Shift multi-select at the list/tree tier (real numbers: 88–815
  rectangles from a single mask-decomposition pass, per the spec's own live data — bulk operations are
  a real need, not theoretical).
- **The "Create Navmesh Blockers" button** (§0): right-aligned on each section header.

### 5.3 Canvas rectangle editing — reuses Areas' interaction, now via a shared template
`AreaDragGesture_UI` is **promoted** (RULED) into a shared accessor-parameterized
`RectangleDragGesture_UI<Accessor>` template — both `AreaDragGesture_UI` and the new
`NavmeshBlockerDragGesture_UI` become thin instantiations of it (Areas' own files get refactored as
part of this same ticket, not left duplicated). Same 8-handle set (no rotate — matches the engine's
own no-rotation limit), same Ctrl-doubles-from-center / Shift-locks-aspect-ratio resize math, same
live-write-every-frame posture (no materialize-on-release step). **Canvas selection is a single
rectangle at a time** (unlike the list's multi-select) — matches Areas' own existing posture; bulk
ops belong to the list, live geometric editing belongs to the canvas.

### 5.4 Rendering
Composited like `MapAreas` (a real GPU `PreviewFieldLayer`, filled/bordered), **not** like the
Markers/Props/Decals point-icon overlay stack — the render shape (filled rectangles, potentially
hundreds, no icon, no LOD) matches Areas, not Markers, despite the domain-name similarity to Markers.

### 5.5 Z-order — RULED
Navmesh gets its **own independent** size-sorted insertion convention (same shape as the existing
`InsertMapAreaSortedBySize`, but a wholly separate function/array — never touches `recipe.areas`).
Batch-insert cost at the spec's own largest observed batch (815 rectangles) is ~6.6×10⁵ float
comparisons worst case — sub-millisecond, human-triggered, rare — no bulk-sort special path needed.

### 5.6 Provenance
`NavmeshBlockerRectangle::provenance` = `HandPlaced` or `MeshGenerated`, a read-only label on the row.
No other UI logic special-cases it — a `MeshGenerated` rectangle drags/resizes/deletes/reparents/locks
identically to a hand-placed one.

### 5.7 PARAMS shape (ratified categories; exact field layout still Format/PARAMS Expert's to finalize at ticket time)
```cpp
enum class NavLayerKind { Land, Amphibious, Hover, Air, Sea, Submarine };  // closed enum — confirmed correct,
                                                                              // NOT an open string like markerTypeName
enum class NavmeshBlockerProvenance { HandPlaced, MeshGenerated };

struct NavmeshBlockerRectangle {
    float originX, originZ;     // min corner — mirrors MapArea's origin+extent shape, NOT the engine's center convention
    float sizeX, sizeZ;
    int   layerIndex;
    int   instanceIdentifier;
    NavmeshBlockerProvenance provenance;
};
struct NavmeshBlockerLayer {
    std::string name; float color[4]; bool bColorOverrideEnabled, bLocked, bHidden, bGridSnapEnabled;
    float gridSnapSizeWorldUnits; int layerIdentifier, parentBundleIdentifier; NavLayerKind navLayerKind;
    // no symmetry field, no iconScale field — deliberate, see §5.2
};
struct NavmeshBlockerLayerBundle {
    int identifier; std::string name; int parentBundleIdentifier; NavLayerKind navLayerKind; int assemblyIdentifier;
};
```
Storage — **RULED**: one shared tagged vector per type (`navLayerKind` tag), filtered per-section at
draw time — mirrors the existing Markers/Props/Decals convention. Not six parallel per-layer vectors.
Bundle transform — **RULED**: Move only; Rotate dropped entirely (a rotated axis-aligned rectangle
isn't representable). Origin↔engine-center conversion happens once, at the Lua-export boundary only.

---

## 6. Ownership / ARCH gate — RULED, fully lifted

`ARCH_22_09` previously said this whole area was **not** SanGen-owned architecture and required "a
real, separately-scoped design consult" before any coder could build toward it. **That gate is now
lifted** — the four design docs collectively satisfied it (`ARCH_22_10`). New capability entered the
layer stack as: SYS (mesh reader) + IO (pack-resolve/visual-LOD/mesh-cache) → `ARCH_22_11`; PROC/scope
→ `ARCH_22_12`; PARAMS-storage/determinism → `ARCH_22_13`; MATH/dispatch → `ARCH_22_14`; UI PARAMS
shape → `ARCH_22_15`; drag-gesture promotion → `ARCH_22_16`; Z-order → `ARCH_22_17`. The
hand-authored Lua-authoring half of the existing spec (§22.1–§22.8, i.e. the two Lua techniques
already proven live in-game) is untouched — this is purely additive: SanGen now produces the resolved
rectangle list that those existing Lua techniques still consume.

---

## 7. Two things found and fixed along the way (not part of this feature, recorded here so they aren't lost)

### 7.1 `worldUnitsPerCell` — investigated, resolved, one real bug surfaced
Not an official `.sanmap`/engine field — a SanGen-internal round-trip setting. **Confirmed by direct
code read:** the exported `.sanmap`'s real, engine-facing `width`/`length` fields are always
literally `geometry.mapSize`, with **no** `worldUnitsPerCell` factor in that computation anywhere —
so the engine's own pixel↔world ratio is always exactly 1:1 regardless of this setting. This means
§3.3 step 7's rasterization formula needs **no** scaling factor (confirmed, not assumed).

**Separate bug surfaced, NOT part of navmesh blockers, flagged for its own investigation:**
`Placement_Emit_PROC.cpp` DOES scale every exported entity position by `worldUnitsPerCell`, while the
map's own declared `width`/`length` never reflects that same factor. Any map where a designer sets
that dial away from `1.0` likely exports every prop/marker/unit position scaled relative to a map
size that itself never changed. Real, code-confirmed, general map-export correctness concern —
separate from this feature, not yet fixed, not yet turned into a work-order.

### 7.2 `MATH_SIMD_SPEC.md` — corrected
Was describing the old, retired `core/math/Sanmath_*.h` stub files instead of the current, much more
capable `src/math/*_MATH.h` set. Corrected in place; `core/math/Sanmath_*.h` still exists and still
compiles against old `core/` pipeline code but is legacy, not law.

---

## 8. Tickets — consolidated, dependency order

| # | Ticket | Layer | Scope |
|---|---|---|---|
| 227 | `NavmeshBlocker_PlaneSlice_PROC` | PROC | Local-space plane derivation + per-triangle clip. Depends on mesh-ingestion output + `Data::PlacementResults.props`. |
| 228 | `NavmeshBlocker_LayerSliceHeight_PROC` | PROC | Per-layer slice height + `Water.bEnabled` gate. |
| 229 | `NavmeshBlocker_Rasterize_PROC` | PROC | World-space triangle → per-layer boolean grid, OR-accumulated. |
| 230 | `NavmeshBlocker_Decompose_PROC` | PROC | First real SanGen implementation of the existing §7 decomposition algorithm. |
| 231 | `NavmapModifierBlocker_PARAMS` | PARAMS | The §5.7 types + per-layer merge/drop-size settings. |
| 232 | `NavmeshBlocker_Bake_PIPELINE` | PIPELINE | The one-shot bake entry point (§3.5), composes 227→230. |
| — (Format) | `.sanmodel` reader + LOD/pack resolution | SYS/IO | §1.4's table. Parallel to 227-232, gates them. |
| — (Format) | `NavmeshBlockerLayerBundle_IO` | IO | `.sanmap` round-trip for the new PARAMS type. |
| 230 (UI) | `RectangleDragGesture_UI<Accessor>` template | UI | Promotes `AreaDragGesture_UI`, adds `NavmeshBlockerDragGesture_UI`. (Ticket-number collision with PROC's own "230" — both docs numbered independently; renumber at dispatch time.) |
| 231 (UI) | Canvas dispatch/draw pair | UI | New `ApplicationPanel::Navmesh` entry. |
| 232 (UI) | `NavmeshTab_TypeSections_UI` | UI | Fixed 6-section water-gated outer loop. |
| 233 (UI) | Group tree | UI | `TreeListWidget_UI` instantiation. |
| 234 (UI) | Layer list + row body | UI | `DraggableListWidget_UI` instantiation + the header button (§0). |
| 235 (UI) | Perf pass | UI-Opt | VirtualList/spatial-index, deferred until measured. |
| — | Lua/export IO ticket | IO | Turns the baked rectangle list into the `<MapName>_data.lua` orchestration the existing Lua techniques already consume. Not yet designed. |

Note: the Format, Generator, and UI docs each numbered their own tickets starting near 227-232
independently — real dispatch will need one consolidated numbering pass against the live
`work_orders/` directory.

---

## 9. Still genuinely open (human/UX calls, not blocking)

- Which `ApplicationPanelGroup` the Navmesh Tab lives in (Environment, alongside Water/Areas, is the
  suggested default).
- Whether re-running the bake on a layer a designer has since hand-edited should overwrite, merge, or
  refuse (safe default: refuse/warn, decide at UI implementation time).
- Whether `Sys::SanmodelMesh` should also carry normals (currently: positions+indices only).
- The general `worldUnitsPerCell` export bug (§7.1) — real, separate, not yet a work-order.
