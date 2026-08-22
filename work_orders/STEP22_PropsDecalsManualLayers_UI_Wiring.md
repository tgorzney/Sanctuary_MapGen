# Work-Order — Step 22: wire Props/Decals manual-layer UI to `PropInstanceLayer`/`DecalInstanceLayer`

*Constitution §7. Executor: SanGen Coder. Retires `PropsTab_Manual_UI.h` SCOPE NOTE 1 by editing
the real `Params::PropInstanceLayer`/`Params::DecalInstanceLayer` (shipped Step 4/5) instead of
the UI-only `ManualPropGroup`, and builds the Decals equivalent that never existed. Design from a
dedicated UI Expert consult (binding) that corrected a real misunderstanding surfaced mid-session
— read "Ruled by this ticket" in full before touching anything.*

## Root problem
`ManualPropGroup` (`PropsTab_Manual_UI.h`) is caller-owned UI-only state, explicitly flagged
temporary (SCOPE NOTE 1). `Params::PropInstanceLayer`/`DecalInstanceLayer` now exist and
round-trip through `PropGroups`/`DecalGroups` (Step 4/5), but the tab still edits the disconnected
presentation type. No Decals equivalent exists at all — decals are drawn as a sub-block inside
`DrawPropsTab` (`DrawDecalRuleStack`), never their own tab.

## Ruled by this ticket (UI Expert consult — binding; corrects a real misreading of this domain)
1. **`ManualPropGroup` IS the right retype target for `Params::PropInstanceLayer`** — confirmed
   not a coincidental shape match: `ENTITY_AUTHORING_PARAMS_SPEC.md` states outright the two are
   "the same kind of authoring-convenience metadata," and `ARCH_12_ManualPropDecalLayers.md §12` confirms `PropInstanceLayer`'s
   shape was deliberately matched to the already-live `ManualPropGroup` identifier.
2. **The retype reaches ONLY the group-metadata list, never the transform list.** Read
   `PropsTab_Manual_UI.cpp`'s actual logic (not just the header): today's "groups" list and
   "transforms" list are two INDEPENDENTLY-drawn widgets sharing one collapsible section — there
   is NO existing filter/parent-child relationship between them, and `Data::PlacementInstances`
   (the procedural buffer the transform list reads) has no `layerIndex`-equivalent field at all.
   **Do not build a filter connecting them — none exists today, none is being added.** The
   transform list stays exactly as-is: unfiltered, read-only, previewing PROCEDURAL results,
   completely unrelated to manual-layer membership. Reword its SCOPE NOTE 2 to say this explicitly
   (it's accurate today but reads as confusing once the group list is backed by real PARAMS).
3. **`layerIndex` lives on `PropTransform`/`DecalTransform` inside `recipe.props`/`recipe.decals`
   (the hand-placed/imported pass-through tree)** — a genuinely different data source from
   `Data::PlacementInstances`. This ticket's repair logic (ruling #5 below) operates on THAT tree.
4. **Do this ticket for BOTH Props and Decals together, not sequentially/separately.** Every prior
   ticket in this family (Step 4/5, `PropInstance_PARAMS.h` itself) deliberately paired them —
   splitting now reopens the same divergence-window risk Step 20's shared-helper ruling closed for
   Armies/Areas.
5. **New repair logic is required, not optional — a real gap, not just "wire the metadata."**
   Deleting or reordering a layer must repair every `layerIndex` that referenced it, mirroring
   `DropUnitRulesForRemovedArmy`/`RenumberUnitRuleArmyIndicesForReorder` (Step 20) — but with a
   DELIBERATE DIVERGENCE on delete: an orphaned `UnitRule` is dropped (a unit with no army is
   meaningless); an orphaned `PropTransform` must NEVER be dropped (a prop losing its layer tag is
   still a real, still-rendered prop, just ungrouped). **Clamp to layer `0` on delete, exactly the
   semantic ARCH_12_ManualPropDecalLayers.md §12 already ratified for out-of-range import** — never remove the prop/decal
   instance itself.
6. **Name uniqueness is cosmetic here, not a data-loss fix.** `PropGroups`/`DecalGroups` export as
   plain arrays (Step 4 finding 5), not name-keyed dicts — duplicate layer names don't collide on
   export, unlike Armies. Reuse `UniqueNameList_UI.h`'s `NextUniqueLabel`/`MakeNamesUnique` for
   "Add Prop Layer"/"Add Decal Layer" anyway, for UX consistency — but this ticket must say
   explicitly it's cosmetic, not framed as fixing a bug like Step 20's army-name fix was.
7. **The Decals block is a new sibling file, NOT a new tab.** There is no "Decals tab" — confirmed
   by reading `PropsTab_UI.cpp`: decals are a sub-block drawn inside `DrawPropsTab`. The manual-
   decal-layers block belongs there too: `PropsTab_ManualDecals_UI.h`/`.cpp`. Inventing a new
   top-level tab would violate ARCH_08_04_CoderScopeLaw.md §8.4 (a coder never invents a missing structural element).
8. **`DrawPropsTab` needs a new `placedDecals` parameter** (nullable, mirrors the existing
   `placedProps`) — it doesn't receive one today. Find and update its one call site.
9. **Dirty-flag posture: unchanged, stays silent** — same reasoning as Step 20's Army ruling:
   nothing downstream hashes a layer's name/color/scale. SCOPE NOTE 1 loses its "no home" framing
   but keeps its "does not notify `PreviewDriver`" substance.

## Target files
- `src/ui/PropsTab_Manual_UI.h`/`.cpp` — delete `ManualPropGroup` (its `RealtimeToggle` members
  cannot live on `Params::PropInstanceLayer`, a pure round-tripping type); state gains a single
  shared toggle set for the selected row (`selectedLayerColorToggle`/`selectedLayerIconScaleToggle`
  — same pattern as `ArmiesTabState`); `SelectedManualPropLayer`/`ManualPropLayerRowLabel`/
  `EffectiveManualPropLayerColor` retype onto `Params::PropInstanceLayer`;
  `DrawManualPropLayers(state, recipe.propLayers, placedProps)` edits the real vector; reword
  SCOPE NOTE 2 per ruling #2.
- New `src/ui/PropsTab_ManualDecals_UI.h`/`.cpp` — exact mirror for `Params::DecalInstanceLayer`/
  `recipe.decalLayers`/`placedDecals`.
- `src/ui/PropsTab_UI.h`/`.cpp` — `DrawPropsTab` gains `const Data::PlacementInstances*
  placedDecals` parameter, calls the new decal manual-layers block; update the one call site.
- New repair functions (exact shape given below), placed alongside the retyped state (or a small
  shared location if the Coder judges the Props/Decals versions genuinely identical enough to
  template — Coder's call, not re-litigated here since both are small and domain-typed):
  `ClampPropLayerIndicesForRemovedLayer`, `RenumberPropLayerIndicesForReorder`, and their
  decal-typed mirrors over `recipe.decals`.
- `src/ui/PropsTab_UI_Test.cpp` — retype existing manual-prop-layer fixtures onto
  `Params::PropInstanceLayer`; add the clamp/renumber repair test cases.
- New `src/ui/PropsTab_ManualDecals_UI_Test.cpp` — mirror coverage for decals.

## Layer & accuracy class
UI. Accuracy class: Visual/Exact (layer metadata is now real recipe content; the repair logic is
correctness-critical for hand-placed/imported prop and decal data).

## Backend policy
N/A — pure UI/imgui composition plus plain vector-repair logic (no rendering dependency).

## ARCH rules invoked
- `ENTITY_AUTHORING_PARAMS_SPEC.md`, ARCH_12_ManualPropDecalLayers.md §12 — the ratified `PropInstanceLayer`/
  `DecalInstanceLayer` shape and the `layerIndex` clamp-to-0-on-out-of-range semantic this
  ticket's delete-repair reuses (already the ratified import-time behavior, now also the
  UI-delete-time behavior — same rule, two trigger points).
- ARCH_08_04_CoderScopeLaw.md §8.4 — no new tab invented; the Decals block joins the existing Props tab as a sibling file.
- Constitution §6 — deleting a layer never destroys the prop/decal instance that referenced it.

## Solution — repair functions (exact shape, UI-Expert-provided)
```cpp
// Mirrors DropUnitRulesForRemovedArmy, but layerIndex is authoring metadata (ARCH_12_ManualPropDecalLayers.md §12) — clamp
// to layer 0, never drop the instance. Deleting a LAYER must never delete an imported PROP.
inline bool ClampPropLayerIndicesForRemovedLayer(std::vector<Params::PropInstanceGroup>& props,
                                                  int removedLayerIndex) {
    bool bRecipeMoved = false;
    for (auto& group : props)
        for (auto& t : group.transforms) {
            if (t.layerIndex == removedLayerIndex)      { t.layerIndex = 0; bRecipeMoved = true; }
            else if (t.layerIndex > removedLayerIndex)  { --t.layerIndex; bRecipeMoved = true; }
        }
    return bRecipeMoved;
}
// RenumberPropLayerIndicesForReorder(props, sourceLayerIndex, targetLayerIndex, layerCount) —
// identical shape/math to RenumberUnitRuleArmyIndicesForReorder (Step 20), applied to layerIndex.
// + decal-typed mirrors of both, over recipe.decals / Params::DecalInstanceGroup.
```

## Explicit out-of-scope
- **Authoring/editing individual `PropTransform`/`DecalTransform` rows** (assigning a specific
  hand-placed instance to a layer, or placing a new one) — no UI anywhere for this, same class of
  gap Step 20 ruling #8 left open for `Army.groups`. Hand-edit/import only, pending a dedicated
  canvas/manual-entry ticket (the bigger, separately-flagged design question).
- **Filtering the procedural `Data::PlacementInstances` transform list by layer** — no field to
  filter by exists, and none is being added. That list stays exactly as-is.
- **Any change to `Params::PropInstanceLayer`/`DecalInstanceLayer`'s PARAMS shape.**
- **`PropsTab_Decals_UI.cpp`'s `DrawDecalRuleStack`** (the procedural `DecalRule` stack, fixed for
  symmetry in Step 7) — unrelated, untouched.

## Acceptance test
`PropsTab_UI_Test.exe`/`PropsTab_ManualDecals_UI_Test.exe` pass with: `Params::PropInstanceLayer`/
`DecalInstanceLayer`-typed fixtures; deleting a layer clamps every referencing `layerIndex` to `0`
without removing any prop/decal instance; reordering layers correctly renumbers `layerIndex`
(below-target, above-target, no-op cases, mirroring Step 20's reorder tests); two "Add Prop
Layer"/"Add Decal Layer" clicks produce distinct names. The procedural transform list's behavior
is unchanged (still unfiltered, still read-only, still previews `Data::PlacementInstances`
directly). Full `SanGenV2` build stays clean.
