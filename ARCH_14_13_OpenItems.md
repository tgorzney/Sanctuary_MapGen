[← ARCH index](ARCH.md) · [§14 ARCH_14_PreviewOverlayLayering](ARCH_14_PreviewOverlayLayering.md) · SanGen ARCH §14.13. **Only the ARCH Expert writes this file.**

### 14.13 Open items — status as of this ratification (closed items marked)
R2's own "Consolidated ❓ open items" list, carried forward. Items 4 and 5 were closed in the R2
ratification session by direct expert consult; item 3 was closed on its *design* question in a
later correction session (rulings below). **Its "two work-orders remain unscheduled implementation"
framing is itself now stale — see the correction inside item 3 below; §14.15 records both as shipped.**
Items 1-2 remain genuinely open. A coder or future ARCH pass must not treat items 1-2 as settled
by this ruling:
1. ⚠️ **Real footprint-size source:** placeholder-per-domain now (§14.3); who/when derives real
   mesh bounds is unscheduled.
2. ⚠️ **Cross-layer visible-vertex budget default and Tier B per-resolution costs** (§14.8-14.9):
   need the real benchmark named in §14.9, not the reasoned placeholders in this ruling.
3. ✅ **DESIGN CLOSED — Manual sub-layer stable id + manual props/decals PROC resolution.**
   Closed by three rulings below, in a correction session following the item's original "open,
   sharpened" framing. Two work-orders are now fully specified; implementation status is corrected
   below (this bullet originally called it unscheduled — that is no longer true).
   - **Correction to this item's own prior text.** It previously claimed manual props/decals were
     "NOT yet live-wired into `BuildSanmapJsonText`/`ParseSanmapJsonText`," citing
     `MapRecipe_PARAMS.h:103-104`. **That claim is false and is withdrawn** — confirmed live-wired
     both directions: `MapExporter_DocumentAssembly_IO.cpp:63-64`
     (`document["decals"] = BuildDecalsJson(recipe); document["props"] = BuildPropsJson(recipe);`),
     `MapImporter_ParseDocument_IO.cpp:67-69` (`ReadPropGroupsJson`/`ReadDecalGroupsJson`),
     round-trip-tested (`MapExporter_IO_Test.cpp:86`, `MapImporter_IO_Test.cpp:978,1180-1183`).
     `MapRecipe_PARAMS.h:103-104`'s own comment is itself stale (code stays with the Coder to fix;
     not this ARCH's file to edit). **What is actually still missing — confirmed identical for
     Props and Decals, no asymmetry between the two domains, both unblock together with the same
     work:** zero PROC-side resolution step exists for manual authoring at all. `src/proc/` has
     zero references to `PropInstanceGroup`/`DecalInstanceGroup`/`recipe.props`/`recipe.decals`;
     `Placement_PROC.cpp:62,64`'s `results.props`/`results.decals` are filled exclusively by the
     procedural `ScatterRule` path.
   - **(a) and (b) of the original problem statement still hold, unmodified by the correction
     above:**
     - (a) `PropInstanceLayer`/`DecalInstanceLayer` (`PropInstance_PARAMS.h:30-31`) carry only
       `name`/`color[4]`/`iconScale` — no id. The only backward reference, `layerIndex` on
       `PropTransform`/`DecalTransform` (`PropInstance_PARAMS.h:19-20`), is a plain vector
       position — renumbered on reorder (`RenumberPropLayerIndicesForReorder`) and clamped on
       delete (`ClampPropLayerIndicesForRemovedLayer`, `PropsTab_Manual_UI.cpp:43-67`) — not a
       stable identity.
     - (b) `Data::PlacementInstances` (`PlacementInstances_DATA.h`) has no correlation column back
       to a manual layer at all.
   - **WORK-ORDER A — Manual sub-layer stable id.** Add `int layerId = -1;` to
     `PropInstanceLayer`/`DecalInstanceLayer`, assigned once at creation (Ruling 1 below), and
     **never** touched by `RenumberPropLayerIndicesForReorder`/`ClampPropLayerIndicesForRemovedLayer`
     — those keep renumbering the existing positional `layerIndex` exactly as today, unmodified;
     the two fields coexist, serving different purposes (`layerIndex` = current authoring
     position/JSON linkage from transforms, unchanged in shape and behavior; `layerId` = stable
     identity for correlation). Confirmed genuinely new architecture for this codebase — zero
     `layerId`/`stableId`/monotonic-counter precedent exists anywhere in `src/` prior to this
     ruling. `Army_PARAMS.h`'s reorder logic (`ArmiesTab_UI.cpp`) uses the identical plain-position
     scheme as Props/Decals with no stable id; `GeoLayer_PARAMS.h::stratumIndex` is an unrelated
     fixed-slot reference, not list-position precedent. Neither is reusable prior art.
     **Shipped — STEP56, confirmed by direct read (§14.15).**
   - **WORK-ORDER B — Manual props/decals PROC resolution + correlation column.** One work-order,
     not split — part (b2) has nothing to populate without part (b1):
     - (b1) wire `recipe.props`/`recipe.decals` into a real PROC resolution step, appending into
       `results.props`/`results.decals` (the same `Data::PlacementInstances` SoA the procedural
       scatter path already fills) — a straight 1:1 copy-through per Ruling 3 below, not a
       symmetry-orbit expansion.
     - (b2) add a `manualLayerId` column to `Data::PlacementInstances`, mirroring the existing
       `armyIndex` column's shape (`PlacementInstances_DATA.h:27,38,49,69,86` —
       sentinel-defaulted `std::vector<int>`, threaded through `Append`/`Get`/`Reserve`/`Clear`):
       default `-1` for procedurally-scattered instances; populated with Work-Order A's `layerId`
       (not the renumbered `layerIndex`) for manually-authored instances.
       **Shipped — `Placement_Manual_PROC.cpp`, wired into `Placement_PROC.cpp:50`, confirmed by
       direct read (§14.15).**
   - **Ruling 1 — counter placement: NOT a persisted counter field on `MapRecipe`.**
     `MapRecipe_PARAMS.h`'s own header states it "is exactly what `mapGeneratorData` serializes" —
     every existing field is real recipe content; there is no session-only-scratch precedent to
     extend, and a persisted counter is itself a second piece of state that can desync from the
     ids it is supposed to stay ahead of (e.g. hand-edited JSON). Instead: **derive-on-create, no
     stored counter.** A newly-created layer's `layerId` = `1 + max(layerId across the current
     in-memory propLayers/decalLayers)`, or `0` if empty. This is self-healing across manual JSON
     edits, satisfies "stable across save/load" for free — ids already present in a loaded file are
     never renumbered, only `layerIndex` is — and an O(layer count) scan per creation is free at
     this cardinality (tens of layers, not thousands). Reusing an id number after its owning layer
     is deleted is not a hazard: `ClampPropLayerIndicesForRemovedLayer`'s existing delete path
     leaves nothing referencing the old id, so nothing live can collide with it.
   - **Ruling 2 — wire representation: new key `"Id"` in the `PropGroups`/`DecalGroups` wire
     array.** Lands in `BuildPropGroupsJson`/`ReadPropGroupsJson` (`MapExporter_Props_IO.cpp` /
     its `MapImporter_Props_IO.cpp` inverse) and the Decals siblings, PascalCase to match that
     array's existing sibling keys (`"Name"`/`"Color"`/`"IconScale"`) — a distinct JSON context
     from the transform-level array's camelCase `"layerIndex"`, which stays untouched.
     **Legacy-file backfill:** a `PropGroups`/`DecalGroups` entry with no `"Id"` key on import gets
     `layerId = <its index within that array>` — already unique (array positions are already
     distinct) and safe going forward, since Ruling 1's derive-on-create rule scans whatever ids
     exist post-backfill before minting the next one.
   - **Ruling 3 — no symmetry participation for manual placements; straight copy-through.**
     `PropTransform`/`DecalTransform` (`PropInstance_PARAMS.h:19-20`) carry only
     `InstancedTransform transform; int layerIndex;` — no `bSymmetryUseGlobal`/`symmetryMask`,
     unlike `PropRule`/`DecalRule`/`MarkerRule`/`ScatterRule`, which all carry that pair
     (`ScatterRule_PARAMS.h:29-30,53-54,80-81`, `MarkerRule_PARAMS.h:58-59`). This is not an
     oversight to fix — it follows directly from the already-ratified framing every hand-placed
     type shares (`MapRecipe_PARAMS.h`'s own comment on `armies`/`areas`/`markers`/`chains`,
     extended to props/decals by the §12 ruling): "round-trip fidelity... their entire purpose; no
     PROC stage computes or reinterprets them." Reinforced by direct precedent: `recipe.armies` and
     `recipe.markers` (the hand-placed `MarkerInstanceGroup` kind, distinct from `MarkerRule`)
     never appear anywhere in `src/proc/` — the existing hand-placed-entity family already never
     runs through `BuildSymmetryOrbit`/`ResolveSymmetryMask`. **Ruling: Work-Order B's (b1)
     resolution step is a straight 1:1 copy-through, no symmetry-orbit expansion.** An author who
     wants a mirrored prop places the mirrored copy manually, exactly as they already must for
     armies and markers today. Props and Decals are therefore symmetric in every respect relevant
     to this item — the "manual decals lag behind manual props" framing item 4 below implied no
     longer applies; both domains have always had, and continue to have, an identical gap with an
     identical fix. **Note (ARCH §16): manual markers now diverge from this precedent** —
     `MarkerTransform` DOES participate in symmetry (via `symmetryGroupIdentifier`, §16.5), a
     deliberate, separately-ratified exception the human required for markers specifically; it does
     not reopen this ruling for Props/Decals, which stay straight copy-through.
   - **Nothing left open on the design question.** ~~Work-Orders A and B remain unscheduled
     implementation~~ — **stale, corrected by §14.15**: both have since shipped (confirmed by direct
     read, not by the sequence doc alone — see the "Shipped" notes on Work-Orders A and B above).
     §14.15 also rules the cull-path stable-id migration this shipped state now unblocks
     (`MapCanvas_IconLayer_CullManual_UI.cpp`'s positional match retires in favor of a stable-id
     match).
4. ✅ **CLOSED — Decals data source.** Confirmed (Generator Expert): procedural Decals already
   resolve into `Data::PlacementResults::decals` (`PlacementResults_DATA.h:11-15`), the identical
   `Data::PlacementInstances` SoA type with identical `ruleIndex`/`category` columns
   markers/props/units use (`Placement_PROC.cpp:64` `CollectionFor(3)`,
   `Placement_Rules_PROC.cpp:104-138` `AppendDecalRules`, `Placement_Kernel_PROC.h:52` collection
   index 0=markers/1=props/2=units/3=decals). No compositor currently reads them (confirmed: no
   `Decal` reference anywhere in `PreviewComposite_*`) — the gap is purely a missing draw-pass
   consumer, not a DATA-shape mismatch. `PREVIEW_COMPOSITING_SPEC`'s prior "Decals never
   composited" framing is corrected accordingly. The §14.9 CSR-bucket/`SpatialGrid` scheme applies
   to procedural Decals exactly as written, no special-case needed. Applies **only** to procedural
   decals (`recipe.decalRules`) — manual decals are the separate item 3 gap, now shipped (§14.15),
   and — per item 3's own correction — no longer lagging behind manual props in any respect.
5. ✅ **CLOSED — `OverlayLayer_UI::blendMode` retired; replaced by `opacity: float`.** UI Expert
   verdict: `Ui::PreviewBlendMode` (`src/ui/PreviewComposite_Settings_UI.h:26`) is a two-operand
   GPU raster-compositing enum (`Replace`/`AlphaBlend`/`Add`/`Multiply`/`Maximum`/`Minimum`) wired
   into the GPU composite shader as integer defines (`PreviewComposite_GpuProgram_UI.cpp:43-48`)
   — meaningless for an `ImDrawList::AddImage` icon draw (a textured quad with per-instance
   vertex-color tint under ImGui's one global blend equation). Forcing a per-layer blend-equation
   switch would require a custom render callback per overlay layer, breaking the bulk-batched-
   vertex-write model §14.9 mandates and turning the "zero GPU recompute" screen-space tier into a
   shader-state-change cost. §14.2's struct now carries `opacity: float` (0-1 layer-wide alpha
   multiplier, folded into each instance's tint alpha at draw time) in place of `blendMode`. A
   future additive-glow overlay kind is a narrow per-layer-kind flag, not a shared blend-mode
   enum — not designed in now.

R2's own open item 1 (fieldLayers/overlayLayers unification) is **not** carried forward on this
list — §14.7 above rules it closed, and records the R2 self-inconsistency that made this call
non-trivial.
