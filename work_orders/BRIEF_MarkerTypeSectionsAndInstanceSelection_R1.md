# Design Brief — Marker Type-Sections, Instance Listing, and Selection Highlight, Round 1

*For a dedicated design conversation with the SanGen UI Expert. Read `CLAUDE.md` first. DESIGN
phase only — no code. Output: a design doc naming every PARAMS/ARCH decision needed, plus whatever
work-orders are already unblocked.*

## Where this comes from

STEP119/120 (landed this session, ratified as `ARCH_19_MarkerLayerBundle.md`) shipped a Markers
tab restructure with a single flat "Groups" tree under one "Markers" section. The human has
corrected this: **that was not the hierarchy they asked for.** This brief captures the corrected
requirements precisely, so this round doesn't repeat that miss.

## The corrected hierarchy — read as ground truth, do not re-derive

```
Markers (Section)
  Alloy (Type Section)
    ARMY_01 (Group)
      Primary   (Layer: Manual)     → instance list, click to select
      Secondary (Layer: Manual)     → instance list, click to select
      Procedural (Layer: Procedural)
    ARMY_02 (Group)
      ...
    <ungrouped Alloy Layers, listed individually here — NOT in a separate global section>
  Plasma (Type Section)
    ...
  Spawn (Type Section)
    ...
```

- **One collapsible section PER MARKER TYPE** (Alloy, Plasma, Spawn, and any other type present in
  the data — confirmed dynamic, not a hardcoded fixed 3; see the open question below).
- Under each Type section: **Groups** (each Group scoped to exactly that one type — a Group cannot
  mix types; this was already settled in the prior Bundle round and does not change).
- A Group can hold multiple Layers — Manual, Procedural, or both — as many of either as wanted.
- **Ungrouped Layers of that type are listed individually AFTER the Groups, within the SAME Type
  section** — not in a separate top-level "Ungrouped Procedural/Manual" section spanning every
  type (STEP120's current shape). This is a real structural change from what shipped.
- **A Layer lists its individual instances.** Clicking an instance selects it. The selected
  instance, and all its symmetric siblings, change color on the map preview to a distinct
  "selected" color — configurable per marker type (a select-color setting per type, analogous to
  the existing per-type default color).

## Resolved: the type-bucketing gap (do not re-litigate)

Earlier drafting of this brief wrongly framed "how does a Layer know its type" as an inference
problem. Corrected by the human: **it isn't inference — a Layer needs to know which Type-section
it belongs to, exactly the same way it already needs to know which Group it belongs to
(`parentBundleIdentifier`).** This is a plain assignment made when a Layer is created or moved, not
derived from marker instance data. Fix: add an explicit `markerTypeName` field directly to both
`Params::MarkerRuleLayer` and `Params::MarkerInstanceLayer` (mirroring the field
`Params::MarkerLayerBundle` already has, `MarkerLayerBundle_PARAMS.h`) — every Layer always knows
its own Type-section directly, grouped or not. A Group's own `markerTypeName` (already shipped)
constrains which Layers may be added to it (soft/UI-enforced, per the already-ratified
`ARCH_19_12_SoftTypeConsistency.md` posture) — a Layer's own new field is the actual source of
truth for which Type-section renders it, not an inference from its parent Group.

## Already confirmed this session — read as ground truth, don't re-derive

**Instance-selection/highlight ground truth (investigated in full this session):**
- **No real "selected → distinct color" mechanism exists anywhere today.** `OverlayInstanceKey_UI`/
  `input.selectedInstanceKey`/`instance.bSelected` (`src/ui/MapCanvas_IconLayer_UI.h:24-32`,
  `MapCanvas_IconLayer_Cull_UI.cpp`) is a real, working selection-KEY pipeline, but confirmed its
  ONLY observable effects today are decimation-priority exemption
  (`MapCanvas_IconLayer_Budget_UI.cpp:19`) and render-cache-bypass
  (`MapCanvas_IconLayer_Draw_UI.cpp`) — `bSelected` never touches tint/color anywhere. A
  color-changing consumer of "selected" would be genuinely new code, not existing-but-unwired.
- **That selection-key pipeline is also scoped to procedural Alloy/SpawnsArmies markers only**
  (`MapCanvas_IconLayer_Cull_UI.cpp:113-128`) — manual markers have zero path into it.
  `HitTestManualMarkers` (`MapCanvas_MarkerDrag_UI.cpp`) is called only to seed a DRAG gesture, never
  writes a selection key.
- **Manual markers have no stable per-instance identity today.** `Params::MarkerTransform`
  (`MarkerInstance_PARAMS.h:53-69`) carries no `instanceId`/stable-index field — only the transient,
  reorder-fragile pair `(groupIndex, transformIndex)`. Contrast `MarkerInstanceLayer::layerId`
  (line 27), which IS stable but identifies the LAYER, not one marker instance. This is a real gap
  that must be closed for click-to-select to work reliably (a reorder/insert/delete elsewhere must
  not silently reassign what's "selected").
- **The Manual Markers tab's OWN existing `selectedInstanceIndex`
  (`ManualMarkersState`, `MarkersTab_Manual_UI.h:50`) is entirely UI-panel-local** — confirmed zero
  connection to `MapCanvas`/the render pipeline (`SetManualMarkerDragSource`,
  `Application_UI.cpp:101`, passes only raw `Params::MapRecipe` data pointers, no tab-state pointer
  at all). Selecting a row in that list today has zero visual effect on the preview.
- **The symmetric-sibling lookup DOES already exist, reusable, non-drag-specific**:
  `Pipeline::BuildWorldSymmetryOrbit` (`src/pipeline/SymmetryOrbitQuery_PIPELINE.h:34-37`) — a
  stateless query taking geometry/mask/repeat-count/world position, explicitly documented in its
  own header as designed for exactly this "future UI caller, not gated on an active drag" use case.
  `MarkerDragGesture_UI.h`'s `ResolveEffectiveMarkerSymmetry` (lines 67-78) already shows how to
  resolve the effective mask/count for any `layerIndex` without a live gesture. Use this directly —
  do not build a second sibling-finder, and do not require `MarkerOrbitCorrespondence_UI.h`'s
  drag-specific correspondence-tracking machinery (that solves a different problem: matching
  siblings ACROSS FRAMES during a live drag, not enumerating them once for a static selection).
- **`Params::GlobalMarkerSettings` has no select-color concept at all today** — confirmed via full
  read (`GlobalMarkerSettings_PARAMS.h:14-24`) and repo-wide grep. This is new fields, not existing
  data needing a consumer.
- **Manual markers render via a completely separate function than procedural markers.**
  `DrawManualMarkerRoster`/`ManualMarkerTint` (`MapCanvas_MarkerDrag_UI.cpp`) draws plain
  `AddCircleFilled` dots, entirely independent of the `OverlayVisibleInstance`/icon-atlas pipeline
  procedural markers use. A "selected" tint branch needs to be added to THIS function's tint
  resolution (already extended once for type-default/army color, STEP112/116) — it cannot reuse
  whatever the procedural side eventually gets.

## Open questions this brief must answer

1. **Is the Type-section set fixed (Alloy/Plasma/Spawn, hardcoded) or dynamic (one section per
   distinct `markerTypeName` value actually present in the data, including free-form/imported
   types like "Generic"/"Expansion")?** The human's own example names exactly three, but nothing
   else in this session's ground truth suggests a hardcoded closed set — `MarkerInstanceGroup::name`
   and `GlobalMarkerSettings`'s type-default resolution are both already free-form/open-set. Lean
   dynamic unless the human corrects this; state the recommendation clearly either way.
2. **Manual marker stable instance identity — what shape?** A new `int instanceIdentifier` field on
   `MarkerTransform`, minted the same way `MarkerInstanceLayer::layerId`/`MarkerLayerBundle::identifier`
   already are (sequential, never reused, `-1` sentinel)? Confirm this is additive/no-version-bump
   per the already-proven precedent, and confirm nothing about `MakeNamesUnique`'s existing
   name-based uniqueness handling conflicts with adding a second, numeric identity alongside it.
3. **Selection-highlight color composition** — does the "selected" color fully REPLACE the marker's
   normal type/override color while selected, or compose with it (e.g. an outline/ring instead of a
   fill replacement)? The human said "change color," which reads as replacement — confirm and state
   the resolution priority explicitly (does a locked layer, a refused-drag red tint, or an army
   color still take precedence over "selected," or does selected override everything?).
4. **Does clicking an instance in the LIST also pan/center the canvas on it**, or only change its
   color? Not stated by the human — flag as open, recommend the smaller scope (color only, no
   camera movement) unless told otherwise, consistent with this session's general "ship the
   narrowest correct slice first" pattern.
5. **Does this selection concept also need to work for PROCEDURAL marker instances** (which DO
   already have the `OverlayInstanceKey_UI` pipeline, just no color consumer), or is instance-level
   listing+selection explicitly Manual-only for this round (Procedural instances have no stable
   per-instance identity across bakes either — same restriction Bundle's own move/rotate membership
   already accepted for Procedural layers, `ARCH_19_09_ManualOnlyMembership.md`)? Recommend
   Manual-only, consistent with that already-ratified precedent, but confirm explicitly rather than
   assuming silently.
6. **Relationship to the just-shipped Bundle tree/`TreeListWidget_UI<T>`.** The Type-section tier is
   NEW (above what Bundle's tree currently renders). Does the Type-section become a new outer loop
   that instantiates one `TreeListWidget_UI<Params::MarkerLayerBundle, MarkerGroupLeafKey_UI>` per
   type (filtering `recipe.markerLayerBundles` by `markerTypeName` before each instantiation), or
   does the generic tree widget itself need a third tier? Recommend the former (filter-per-instantiation,
   no widget change) as the smaller, most consistent-with-existing-design option — state clearly if
   a different answer is actually needed.
7. **Instance listing UI shape** — a `DraggableList<Params::MarkerTransform>` nested inside a
   Layer's own expanded body (STEP120's leaf-body callback), or something else? Confirm this
   composes cleanly with the existing per-Layer settings body (`DrawLayerRowBody`) rather than
   replacing it.

## What this brief needs designed

1. The Type-section tier and its relationship to the existing Bundle tree (item 6).
2. `markerTypeName` on `MarkerRuleLayer`/`MarkerInstanceLayer` — exact field placement, wire key,
   additive/no-version-bump confirmation (should be trivial, same precedent class as every prior
   field addition this session).
3. The "ungrouped Layers listed after Groups, within the same Type section" UI shape — replaces
   STEP120's current cross-type-spanning "Ungrouped Procedural Rules"/"Ungrouped Manual Marker
   Layers" sections. Confirm what happens to those two section labels/DraggableList instantiations
   — retired entirely, or repurposed as the per-type "ungrouped" list, filtered by `markerTypeName`?
4. The instance-list-per-Layer UI (item 7), a stable manual-instance-identity field (item 2), the
   selection-key/highlight-color mechanism (items 3-4 of "Open questions"), a new per-type
   select-color PARAMS field, and the render-side consumer in `ManualMarkerTint`/
   `DrawManualMarkerRoster` composing the symmetric-sibling lookup
   (`Pipeline::BuildWorldSymmetryOrbit`, already reusable) with the new highlight color.
5. Flag, don't invent, any further ARCH module-boundary ruling this needs — expect a real ARCH pass
   given the PARAMS additions and the tree-widget composition question (item 6).

## Specs and files to read first

- `ARCH_19_MarkerLayerBundle.md` and its 12 subsections (the just-ratified Bundle design this
  round builds on top of, not replaces).
- `work_orders/DESIGN_MarkerGroupLayerRestructure_R1.md`, `work_orders/STEP119_MarkerLayerBundleParamsIO_PARAMS.md`,
  `work_orders/STEP120_MarkersTabBundleUI_UI.md` (what actually shipped, to correct from).
- `src/ui/MapCanvas_IconLayer_UI.h`, `MapCanvas_IconLayer_Cull_UI.cpp` (the existing
  procedural-only selection-key pipeline — the shape to mirror, not reuse verbatim, for manual).
- `src/pipeline/SymmetryOrbitQuery_PIPELINE.h`, `src/ui/MarkerDragGesture_UI.h`'s
  `ResolveEffectiveMarkerSymmetry` (the reusable sibling-lookup machinery).
- `src/ui/MapCanvas_MarkerDrag_UI.cpp` (`DrawManualMarkerRoster`/`ManualMarkerTint` — where the new
  highlight-color consumer must be added).
- `src/params/GlobalMarkerSettings_PARAMS.h`, `src/params/MarkerInstance_PARAMS.h`,
  `src/params/MarkerRule_PARAMS.h`, `src/params/MarkerLayerBundle_PARAMS.h`.
- `src/ui/MarkersTab_Bundles_UI.h/.cpp`, `MarkersTab_BundleNodeBody_UI.cpp`, `TreeListWidget_UI.h`
  (what actually ships today, to build the Type-section tier on top of).

## Who to consult

SanGen UI Expert first (the Type-section tier, instance-list UI, and tree-widget composition
question — the bulk of this brief). Loop the ARCH Expert immediately given the scope (new PARAMS
fields, a real architectural call on the selection/highlight mechanism, and the tree-widget
composition question). Loop the Generator/Compute Optimization Experts only if the symmetric-sibling
lookup's per-frame cost at authoring scale needs a real perf ruling (unlikely — same query already
proven cheap enough for live drag).

## Response style (carry forward)

Terse, ❓ for questions, ⚠️ for problems, no narration.
