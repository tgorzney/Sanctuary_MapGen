[← ARCH index](ARCH.md) · [§14 ARCH_14_PreviewOverlayLayering](ARCH_14_PreviewOverlayLayering.md) · SanGen ARCH §14.14. **Only the ARCH Expert writes this file.**

### 14.14 Alloy/SpawnsArmies Manual sub-layer routing — no discriminator field on `MarkerInstanceLayer`; route per-transform by the reserved `"Spawn"` group name (responds to `work_orders/STEP97_AlloySpawnsArmiesManualSubLayers_UI.md`)

**Ruled: "something else," not a new field.** STEP97 asked whether `Params::MarkerInstanceLayer`
needs a new enum/tag field to tell an Alloy manual sub-layer from a SpawnsArmies one. It does not,
and adding one would be architecturally wrong: STEP97's own analysis is correct —
`MarkerInstanceLayer` is a cross-cutting **display bucket** (§16.1), and nothing prevents one
hand-authored layer from mixing a Spawn-type instance and an Alloys-type instance under the same
`layerIndex`. A field on the LAYER cannot resolve a distinction that is a property of each
individual transform's owning group, not of the layer.

**The real discriminator already exists, one level down, as a format-reserved literal.**
`MarkerInstanceGroup::name` is the marker-TYPE key (`"Spawn"`/`"Alloys"`/etc.) — confirmed a
format-reserved literal, not a free guess: `Ui::kSpawnMarkerGroupName`'s own header comment reads
"The fixed group name SANMAP_FORMAT_SPEC reserves for the commander-spawn roster... confirmed live
in-game," and shipped code already treats it as load-bearing
(`src/io/MapImporter_ArmyIdentityNormalize_IO.cpp:59`, `src/ui/MarkersTab_Manual_UI.h`/`.cpp`).
This is the manual side's exact counterpart of the procedural side's typed `MarkerRule::category`,
and §14.6 already ratifies the identical 2-way split for the procedural side ("Alloy/SpawnsArmies
re-slice the existing `markers` buffer by its existing `category` column... splits markers 2
ways"). The manual roster gets the same 2-way split from the same signal family, spelled as a name
comparison instead of an enum comparison: a `MarkerTransform` whose owning
`MarkerInstanceGroup::name == "Spawn"` belongs to SpawnsArmies; every other manual transform
(including `"Alloys"` and any free-form/unrecognized type name) belongs to Alloy — Alloy is the
same "everything else" bucket §14.6 already established, not a `MarkerCategory::Alloys`-only
filter.

**Not in tension with the already-ratified cardinality ruling.** `ENTITY_AUTHORING_PARAMS_SPEC.md`'s
"Cardinality ruling" forbids retyping `MarkerInstanceGroup::name` itself to `MarkerCategory` (a
closed C++ enum would silently corrupt an open wire-format string key on import). This ruling does
not retype `name` and does not add any new PARAMS field to `MarkerInstanceGroup`/
`MarkerInstanceLayer` — it only authorizes comparing the existing `std::string name` against one
reserved literal, exactly as `MapImporter_ArmyIdentityNormalize_IO.cpp` already does today. Zero
data-fidelity risk, zero closed-set risk: an unrecognized type name still round-trips verbatim, and
still resolves to a domain (Alloy, the "rest" bucket) — it never falls through to nowhere.

**Single named source of truth — promote the constant, do not duplicate the literal a third time.**
`kSpawnMarkerGroupName` currently lives UI-only (`src/ui/MarkersTab_Manual_UI.h:37`); `IO` already
duplicates the raw literal independently (`MapImporter_ArmyIdentityNormalize_IO.cpp:59`, correctly,
since `IO` cannot depend on `UI`). A third independent occurrence for overlay routing is not
acceptable under the naming law's determinism intent. **Ruled: the constant moves to `PARAMS`** —
`Params::kSpawnMarkerGroupName`, `inline constexpr const char*`, value `"Spawn"`, declared in
`src/params/MarkerInstance_PARAMS.h` beside `MarkerInstanceGroup` (both `IO` and `UI` already
legally depend on `PARAMS`). `MapImporter_ArmyIdentityNormalize_IO.cpp`'s raw literal and
`MarkersTab_Manual_UI.h`'s UI-local constant both become references to this one symbol — a small,
independent cleanup a coder work-order may fold in alongside STEP97's routing fix, or ship
separately; it does not block the routing ruling below.

**`SeedMarkerDomains` routes per-transform, not per-`markerLayers` entry.** For each
`recipe.markerLayers[i]`, `SeedMarkerDomains` (already walking `recipe.markers[*].transforms[*]` to
seed refs) determines, independently:
- does `layerIndex == i` have at least one contributing transform whose owning
  `MarkerInstanceGroup::name == Params::kSpawnMarkerGroupName`? If so, push a `Manual`
  `OverlaySubLayerRef_UI{ index = i }` into **SpawnsArmies**' `subLayers`.
- does `layerIndex == i` have at least one contributing transform whose owning group's `name !=
  Params::kSpawnMarkerGroupName`? If so, push the same-shaped ref into **Alloy**'s `subLayers`.

A single `recipe.markerLayers[i]` entry legally appears in **both** lists when it mixes types —
this is correct, not a bug: the direct consequence of layers being cross-cutting display buckets.

**Corollary — amends §14.2's "index" semantics for these two domains only.** For Props/Decals/
Units, a Manual `OverlaySubLayerRef_UI::index` alone fully resolves an instance set. For
Alloy/SpawnsArmies specifically, `index` selects the `markerLayers` entry but the render-time
consumer that gathers that ref's drawable instances must ALSO filter by owning-group name (Spawn
vs. not, matching whichever domain the ref was pushed into) — `index` no longer implies "every
transform at this layerIndex" for these two domains, the same asymmetry §14.6 already names in
different words ("a coder must not assume `domain == DATA-bucket identity`"). §14.2's sub-layer
mapping table is corrected (in place) to carry this row's final shape instead of its prior
placeholder/forward-reference text.

**Files a coder work-order will touch (STEP97's own successor, not built here):**
`src/params/MarkerInstance_PARAMS.h` (the promoted constant), `src/io/
MapImporter_ArmyIdentityNormalize_IO.cpp` + `src/ui/MarkersTab_Manual_UI.h` (reference it instead
of duplicating), `src/ui/Application_OverlaySetup_UI.cpp`'s `SeedMarkerDomains` (once STEP51 lands
— still blocked on STEP51 exactly as STEP97 reported; this ruling only removes the ARCH-level gap
STEP97 could not resolve itself), and its render-time Manual-sub-layer instance-gathering consumer
(not yet named/built).
