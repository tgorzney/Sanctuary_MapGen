# Design Doc — Marker Type-Sections, Instance Listing, and Selection Highlight, Round 1

Grounded in `work_orders/BRIEF_MarkerTypeSectionsAndInstanceSelection_R1.md`, `ARCH_19_MarkerLayerBundle.md` + all 12 subsections, and the shipped code (`MarkersTab_Bundles_UI.h/.cpp`, `MarkersTab_BundleNodeBody_UI.cpp`, `MarkersTab_RuleLayers_UI.cpp`, `MarkersTab_ManualLayers_UI.cpp`, `TreeListWidget_UI.h`, `MapCanvas_MarkerDrag_UI.cpp`, `MapCanvas_IconLayer_UI.h`, `SymmetryOrbitQuery_PIPELINE.h`, `MarkerDragGesture_UI.h`, `GlobalMarkerSettings_PARAMS.h`, `MarkerInstance_PARAMS.h`, `MarkerRule_PARAMS.h`, `MarkerLayerBundle_PARAMS.h`, plus `MarkerSymmetryFixCommand_UI.cpp`/`Symmetry_PARAMS.h` for the reusable tolerance).

## Open Q1 — Type-section set: fixed vs. dynamic

**Rule: dynamic.** `MarkerLayerBundle::markerTypeName` is already ratified free-form (§19.3: "same as `MarkerInstanceGroup::name`, NOT `MarkerCategory`"). `GlobalMarkerSettings` itself is *not* the counter-evidence the brief's open question thought it was — it's actually a **fixed 3-field struct** (`colorAlloy/Plasma/Spawn`, `iconNameAlloy/Plasma/Spawn`, `scaleAlloy/Plasma/Spawn`) with a hardcoded 3-name resolver (`ResolveMarkerGroupTypeTintColor`/`ResolveMarkerGroupTypeScale`, `GlobalMarkerSettings_PARAMS.h:32-52`) that falls back to a neutral default (white / 1.0) for anything else. So two *different* precedents already coexist in this codebase: the **grouping key** (`MarkerInstanceGroup::name`/`MarkerLayerBundle::markerTypeName`) is open-set; the **settings storage** keyed off it is closed-set-with-fallback. A hardcoded 3-section UI would make any Bundle/Layer with `markerTypeName == "Generic"`/`"Expansion"` (already legal today) unrenderable — a real correctness gap, not a simplification. Type-sections must be dynamic.

**Ordering rule (new, not in brief):** Alloy, Plasma, Spawn first, in that fixed order (matches `GlobalMarkerSettings`' own field order and the human's worked example), then any other distinct `markerTypeName` value present in the data, alphabetical, then a final `"(Unassigned)"` bucket for `markerTypeName == ""` (pre-this-round Bundles/Layers, and any hand-edited file). Distinct names are collected from `recipe.markerLayerBundles[*].markerTypeName` **and** `recipe.markerRuleLayers[*].markerTypeName` **and** `recipe.markerLayers[*].markerTypeName` (union, deduped) — not from `MarkerInstanceGroup::name`, which is a different axis (manual roster grouping, not Layer/Bundle type-scoping).

## Item 2 — `markerTypeName` on `MarkerRuleLayer`/`MarkerInstanceLayer`

Additive, mirrors §19.3 exactly:
```cpp
// MarkerRuleLayer gains:      std::string markerTypeName;   // free-form, same space as
// MarkerInstanceLayer gains:  std::string markerTypeName;   // MarkerLayerBundle::markerTypeName
```
Wire key **`"MarkerTypeName"`** (same string as the Bundle's own key — same concept, different owning struct, no reason to diverge). Default `""` → renders in `"(Unassigned)"`. No version bump — same class of change as `parentBundleIdentifier` (STEP119), same IO files: `MapExporter_MarkersStack_IO.cpp`/`MapImporter_Markers_IO.cpp` (RuleLayer) and `MapExporter_Markers_IO.cpp`/`MapImporter_Markers_IO.cpp` (InstanceLayer). No import-time cross-check against `MarkerLayerBundle::markerTypeName` or the containing Group's name — soft, UI-authored only, same posture as §19.12.

**Explicitly resolved, not re-opened (per brief's own "resolved" section):** `MarkerRule::category` (closed enum: Generic/Spawn/Alloys/Expansion) and this new free-form `markerTypeName` stay two independent concepts, forever. `category` is per-rule AI-analysis metadata; `markerTypeName` is Layer-level UI-section scoping. No future ticket should collapse them.

## Item 3 — Ungrouped-layers-within-Type-section shape

**Rule: repurposed, not retired.** STEP120's `"Ungrouped Procedural Rules"`/`"Ungrouped Manual Marker Layers"` `DraggableList` instantiations survive as *mechanism* — same widget (`DraggableList<Params::MarkerRuleLayer>`/`DraggableList<Params::MarkerInstanceLayer>`), same backing vectors (`recipe.markerRuleLayers`/`recipe.markerLayers`), same `bRowSuppressed`-for-filtering trick already precedented (`DraggableListWidget_Types_UI.h:32-38`, "Lets a caller present a FILTERED view... without DraggableList itself gaining filtering logic"). What changes: they become **one instantiation per Type-section** instead of one global instantiation, and the suppression predicate gains a second AND'd condition:
```cpp
row.bRowSuppressed = (layer.parentBundleIdentifier != -1) || (layer.markerTypeName != thisSection.typeName);
```
drawn *after* that section's `DrawMarkerLayerBundleTree` call, inside the same Type-section collapsible, per the brief's ground-truth diagram. Section labels stay `"Ungrouped Procedural Rules"`/`"Ungrouped Manual Marker Layers"` (now nested one level deeper) rather than inventing new copy.

⚠️ Known, accepted quirk (inherited, not new): each Type-section's `DraggableList` render still walks the SAME full un-filtered vector under a different suppression predicate. A reorder-drag issued from one Type-section's filtered view still operates on real vector indices and can silently "land" past a row belonging to a different, invisible type — the exact same accepted tradeoff the comment at `DraggableListWidget_Types_UI.h:37` already documents for the single-axis case, now composed across two independent filter axes at once. Flag to ARCH for an explicit sign-off that composing two predicates through one `bRowSuppressed` bool is still within that field's documented contract — not a defect, but a widening of its blast radius that should be a recorded decision, not a silent one.

## Open Q6 / Item 1 & 4 — Type-section-vs-tree-widget composition

**Rule: filter-per-instantiation (brief's own recommended option), confirmed structurally safe by reading `TreeListWidget_UI<T>::Render`'s contract, not just asserted.** `Render` takes `const std::vector<T>& nodes` **by value-semantics for tree layout only** — every mutation path (`DrawMarkerLayerBundleNodeBody`, the `Reparent`/`Select` signal application in `MarkersTab_Bundles_UI.cpp:105-132`) resolves the real `Params::MarkerLayerBundle&` by `identifier` lookup into the CALLER's real `bundles` vector, never by position in whatever was passed to `Render`. So a per-type **filtered copy** (`std::vector<Params::MarkerLayerBundle>`, built fresh each frame, containing only bundles whose `markerTypeName == thisSection.typeName`) is safe to pass — reads use the copy, writes go through identifier lookups into the real vector regardless. No widget change needed.

`DrawMarkerLayerBundleTree` gains one new parameter, `const std::string& markerTypeNameFilter`, builds the filtered copy internally, and its "Add Group" button (currently minting `bundle.markerTypeName` empty) sets `bundle.markerTypeName = markerTypeNameFilter` at creation — mirroring `parentBundleIdentifierForNewLayer`'s existing threading pattern exactly.

**Cross-type nesting cutoff — confirmed for free, needs an explicit ARCH sign-off anyway.** If a nested child Bundle's own `markerTypeName` differs from its parent's, it won't appear in the parent's Type-section's filtered copy — `Render`'s own dangling-parent-resolves-to-root logic (`TreeListWidget_UI.h:50-55`, "a node whose own parent id does not resolve to another node in `nodes`... is a ROOT") automatically renders that child as a **root within its own, different Type-section's tree**, with zero new widget code. This is the same shape §19.6 already ratified one tier up ("nested child Bundle with its own different `assemblyIdentifier` stops the recursive walk there") — treated as the same precedent applied by analogy, but it should be a written ARCH ruling, not an implicit consequence nobody signed off on.

## Item 4 / Open Q2, Q3, Q7, Q4, Q5 — Instance listing, selection, highlight

### Open Q2 — stable manual-instance identity
```cpp
// MarkerTransform gains:
int instanceIdentifier = -1;   // stable, GLOBALLY unique across the whole `markers` roster
                                 // (all groups, not per-group) — never reused, -1 = unassigned
```
Named `instanceIdentifier`, not bare `identifier` — `MarkerTransform` already carries `layerIndex`/`symmetryGroupIdentifier`, so the fully-descriptive name avoids ambiguity among its several int fields; consistent with §1.9's full-word law. Wire key **`"InstanceIdentifier"`**. Minted via a new `NextMarkerInstanceIdentifier(const std::vector<MarkerInstanceGroup>& markers)` scanning `max(instanceIdentifier)` across **every** group's transforms (not per-group), same shape as `NextMarkerLayerId`. **Legacy-backfill on import mirrors `layerId`'s own precedent exactly** (`MapImporter_Markers_IO.cpp:121`, `layer.layerId = static_cast<int>(outRecipe.markerLayers.size())`): when `"InstanceIdentifier"` is absent, assign a running sequential counter across the whole nested group/transform import walk (not reset per group) — every legacy transform gets a fresh, globally-unique id, additive, no version bump. `MakeNamesUnique`'s name-based repair is untouched — this is a second, independent numeric identity alongside the existing name key, not a replacement for it.

### Open Q7 — instance-list-per-Layer UI shape
**Rule: NOT `DraggableList<Params::MarkerTransform>`** — this deviates from the brief's own tentative suggestion, with reasoning grounded in the data model. A `MarkerInstanceLayer` does not own a contiguous `std::vector<MarkerTransform>`; its instances are `transform.layerIndex`-tagged entries **scattered across potentially multiple `MarkerInstanceGroup::transforms` arrays** (nothing structurally prevents cross-group `layerIndex` reuse; §19.12's own text — "the 'Add Marker' action inside a Bundle-scoped Layer creates instances in the matching `MarkerInstanceGroup`" — confirms today's authoring flow is convention-scoped to one group, not structurally guaranteed). `DraggableList<T>` requires one real homogeneous backing vector for its reorder/delete signal application; a cross-group filtered view doesn't map onto that contract, and reorder has no semantic meaning for `MarkerTransform` anyway (unlike `MarkerRuleLayer` stack order, which is Z/priority order).

Design: a per-frame filtered index built once (mirroring `BuildMarkerLayerBundleLeafIndex`'s own pattern exactly — `std::unordered_map<int, std::vector<{groupIndex,transformIndex}>>` keyed by `layerIndex`), rendered inside `DrawLayerRowBody`'s existing expanded body as **plain `ImGui::Selectable` rows** (not a shared-widget instantiation), one per `(groupIndex, transformIndex)` hit, labelled `"{group.name} — {transform.name or index}"`. A click writes `rootState.selectedManualInstanceIdentifier = transform.instanceIdentifier` (new top-level `MarkersTabState` field — must be visible regardless of which Layer body happens to be expanded, so it does not live in per-Layer widget-local state). No delete/reorder affordance from this list — deletion/repositioning stays owned by the existing roster editor (`MarkersTab_Manual_UI.h`), consistent with the narrowest-slice-first framing Open Q4 itself invokes. `markers` (all groups) is already threaded into `DrawLayerRowBody` today — no new plumbing needed to reach the data, only to build the index and draw the rows.

❓ Open: should selecting a row here also drive `ManualMarkersState::selectedGroupIndex/selectedInstanceIndex` (the existing roster editor's own, confirmed-dead selection)? Not asked by the brief; ruled explicitly **out of scope** for this round rather than silently coupling two independent selection concepts — cheap follow-up if wanted.

### Open Q3 — selection-highlight color composition and priority
**Rule: full replacement of fill color** (the human's own "change color" reads as replacement, and there's no existing outline-draw precedent in this file to reuse for a compose-style ring without colliding with the drag-ghost's own unfilled-ring visual language, `MapCanvas_MarkerDrag_UI.cpp:144`, which already means "unclaimed drag slot" — reusing that vocabulary for "selected" would be a real semantic collision, not just an aesthetic one).

**Explicit priority order, highest to lowest** (extends `DrawManualMarkerRoster`'s existing branch chain, `MapCanvas_MarkerDrag_UI.cpp:119-127`):
1. Refused-drag red (`bThisGroupDragging && dragState.bSpawnCardinalityRefused`) — unchanged, wins. Active-error communication must never be masked by a passive authoring aid.
2. **Selected highlight (new)** — replaces fill entirely, including army color and layer/type color.
3. Army color (Spawn groups, `ManualSpawnArmyTint`).
4. Per-layer override color / type-default color (`ManualMarkerTint`, unchanged).

`bLocked` never had a tint effect (only gates drag/reposition, per its own doc comment) — no conflict, no ruling needed; a locked+selected instance still gets the select tint normally.

### Open Q4 — camera pan on select
**Rule: no.** Color-only, no camera movement, consistent with the session's established narrowest-slice-first pattern the brief itself invokes.

### Open Q5 — procedural instance selection
**Rule: manual-only**, explicitly ruled (not silently assumed), extending §19.9's already-ratified manual-only-membership reasoning one more time: `Data::PlacementInstances` is PROC-regenerated every bake with no cross-bake stable identity to hang a selection on — same DATA-is-pure-computed-output constraint, same layer boundary, same conclusion Bundle's own Move/Rotate scope already accepted. `OverlayInstanceKey_UI`'s existing procedural-only selection-key pipeline is untouched by this round; the new `instanceIdentifier`/select-color/highlight mechanism is a fully separate, Manual-marker-specific surface — no shared plumbing, no risk of the two selection concepts drifting into each other.

### Per-type select-color PARAMS field
**Rule: fixed 3, mirroring `GlobalMarkerSettings`' own existing shape exactly, plus one deliberate addition:**
```cpp
// GlobalMarkerSettings gains:
float selectColorAlloy[4]   = {1.0f, 1.0f, 0.0f, 1.0f};   // matches colorAlloy/Plasma/Spawn's
float selectColorPlasma[4]  = {1.0f, 1.0f, 0.0f, 1.0f};   // shape/placement precisely
float selectColorSpawn[4]   = {1.0f, 1.0f, 0.0f, 1.0f};
float selectColorDefault[4] = {1.0f, 1.0f, 0.0f, 1.0f};   // NEW — deliberate deviation, see below
```
New resolver `ResolveMarkerGroupSelectTintColor(groupName, settings, outR, outG, outB)`, mirroring `ResolveMarkerGroupTypeTintColor`'s exact vocabulary (Spawn/Spawns, Alloy/Alloys, Plasma/Plasmas), **except** the unmatched-name case: `ResolveMarkerGroupTypeTintColor` falls back to white for "no special color" (a genuine no-op convention). "Selected" needs to be visually distinctive for *any* group name, including free-form ones — falling back to white would make selection indistinguishable from "unset" for a Generic/Expansion group. `selectColorDefault` is the one deliberate, explained departure from strict 3-field mirroring; flag to ARCH for sign-off rather than silently adding a 4th field to a precedented 3-field shape.

### Symmetric-sibling computation
**Rule: fresh, one-shot, discard-every-frame — `Pipeline::BuildWorldSymmetryOrbit` + a small inline nearest-match, NOT `MarkerOrbitCorrespondence_UI.h`.** Per the brief's own explicit framing ("enumerating them once for a static selection," not the drag machinery's cross-frame stability problem):
1. Locate the selected transform (linear scan over `markers` by `instanceIdentifier` — cheap at authoring scale).
2. `ResolveEffectiveMarkerSymmetry(markerLayers, transform.layerIndex, globalSymmetryMask, globalRadialRepeatCount, outMask, outCount)` (`MarkerDragGesture_UI.h:67-78`, already documented for exactly this no-live-gesture use case).
3. `Pipeline::BuildWorldSymmetryOrbit(geometry, outMask, outCount, transform.transform.positionX, transform.transform.positionZ, points, maxPoints)`.
4. If `orbitCount <= 1`: no siblings, highlight only the selected instance (naturally subsumes the `symmetryGroupIdentifier == 0` "ungrouped: free drag, zero orbit calls" convention with zero special-casing).
5. Else, for each orbit point beyond slot 0, nearest-match against the OTHER transforms in the **same `MarkerInstanceGroup`** using **`recipe.markerSymmetryFixSettings.distanceTolerance`** (`Symmetry_PARAMS.h:77`, default 0.5 world units) — reused directly, no new tolerance field; same semantic meaning ("how close counts as the same symmetric position"), already a Constitution §8 tweakable exposed in the tab.
6. Collect matched transforms' `instanceIdentifier`s into a small per-frame `std::vector<int>`, consulted inline in `DrawManualMarkerRoster`'s existing per-transform loop.

Deliberately **not** `symmetryGroupIdentifier`-equality (the cheaper alternative considered and rejected): `"Add Marker"` never populates `symmetryGroupIdentifier` (confirmed — it's written only by drag-materialize, `MarkerDragGesture_Frame_UI.cpp`, and by the `MarkerSymmetryFixCommand_UI.cpp` repair tool). A freshly-authored marker under a symmetric layer, never dragged, would have `symmetryGroupIdentifier == 0` and thus show zero siblings under that approach even when geometric siblings visibly exist — position-driven orbit matching is strictly more correct/complete and is what the brief is actually pointing at.

### Render-side wiring
One new `MapCanvas` pointer, mirroring `SetManualMarkerDragSource`'s exact injection pattern (`MapCanvas_UI.h:104-111`, `Application_UI.cpp:101`):
```cpp
void SetManualMarkerSelectionSource(const int* selectedInstanceIdentifier);
// ...
const int* manualMarkerSelectedInstanceIdentifier = nullptr;   // null-safe-refuses, same posture
                                                                 // as activePanelSource
```
Everything else the highlight computation needs (`geometry`, `globalSymmetryMask`, `radialSymmetryRepeatCount`, `markerSymmetryFixSettings.distanceTolerance`, the new `selectColor*` fields) is already reachable through the existing `manualMarkerDragGeometry`/`manualMarkerDragRecipe` pointers `DrawManualMarkerDragPass` threads today — no other new plumbing.

## ARCH module-boundary rulings needed (exhaustive)

1. `markerTypeName` on `MarkerRuleLayer`/`MarkerInstanceLayer` — field spelling, wire key `"MarkerTypeName"`, additive/no-bump (extends §19.3).
2. Confirm the Type-section tier is UI-derived (dynamic enumeration over existing `markerTypeName` values), **not** a new stored `Params` container — no `Params::MarkerTypeSection` struct.
3. The per-type `TreeListWidget_UI<MarkerLayerBundle, MarkerGroupLeafKey_UI>` filtered-copy-per-instantiation pattern as a legal composition (extends §19.7) — including the confirmed-safe "reads from a filtered copy, writes via identifier-keyed lookup into the real vector" contract.
4. The cross-Type-section nested-Bundle cutoff (child's `markerTypeName` differs from parent → renders as root in its own, different Type-section) — same shape as §19.6 one tier up; needs its own explicit citation, not an implicit consequence.
5. `bRowSuppressed` composing two independent filter predicates (bundle-membership AND type-mismatch) at once — sign-off that this stays within the field's documented single-purpose contract.
6. `MarkerTransform::instanceIdentifier` — spelling, wire key `"InstanceIdentifier"`, additive/no-bump, global (not per-group) uniqueness, legacy-backfill-by-sequential-order-on-import mirroring `layerId`'s precedent for a nested two-level walk.
7. `GlobalMarkerSettings` new fields `selectColorAlloy/Plasma/Spawn` (strict mirror) **and** `selectColorDefault` (the deliberate 4th-field deviation) — explicit sign-off on the deviation.
8. The tint-resolution priority order (refused-drag > selected > army-color > layer/type color) as a canonical, recorded cross-cutting rule, not just an implementation detail buried in one function.
9. "Selected replaces fill" as the visual language, explicitly distinct from the existing drag-ghost unfilled-ring vocabulary (no shared meaning between the two).
10. Reuse of `Pipeline::BuildWorldSymmetryOrbit` + a fresh one-shot inline nearest-match (not `MarkerOrbitCorrespondence_UI.h`) for the static highlight — confirm this stays UI-resident logic calling an existing PIPELINE query, no new PIPELINE surface.
11. Reuse of `markerSymmetryFixSettings.distanceTolerance` for the highlight's nearest-match epsilon (no new tolerance field) — confirm as the correct existing tweakable, not a coincidental reuse that should have its own field.
12. Manual-only selection scope as formal law, cross-referencing §19.9 (a short new subsection, so a future reader doesn't re-derive it).
13. `MarkerRule::category` vs. `markerTypeName` staying two permanently independent concepts — one explicit sentence closing the door, per the brief's own "resolved, do not re-litigate" framing.
14. (Low-risk, mention only) `MapCanvas::SetManualMarkerSelectionSource` as the correct null-safe-injection shape, same layer, same pattern as `SetManualMarkerDragSource` — not a new boundary, just confirm.

## Delivery split recommendation

**Yes — split into (at least) three work-orders, mirroring STEP119/120's PARAMS-then-UI split:**

- **Ticket A (PARAMS/IO):** `markerTypeName` on `MarkerRuleLayer`/`MarkerInstanceLayer`; `MarkerTransform::instanceIdentifier` + minting + legacy-backfill; `GlobalMarkerSettings` select-color fields + resolver. All additive, all IO-only, independently testable, no UI dependency — this is exactly STEP119's own shape one round later.
- **Ticket B (UI — Type-section restructure):** the Type-section outer loop, filtered `TreeListWidget_UI` instantiation-per-type, per-type "ungrouped" `DraggableList` repurposing, per-type "Add Group"/"Add Layer" wiring. Depends on Ticket A's `markerTypeName` fields only.
- **Ticket C (UI — instance list + selection highlight):** the per-Layer instance-list rows, the selection state, the `MapCanvas` wiring, the tint-priority rewrite, the sibling-orbit computation. Depends on Ticket A's `instanceIdentifier`/select-color fields, and is logically independent of Ticket B (could ship before, after, or interleaved with B — the instance list composes into `DrawLayerRowBody` regardless of which Type-section currently contains that Layer).

B and C touch almost entirely disjoint files (B: `MarkersTab_Bundles_UI.*`, `MarkersTab_RuleLayers_UI.cpp`, `MarkersTab_ManualLayers_UI.cpp`, a new `MarkersTab_TypeSections_UI.h/.cpp`; C: `MarkersTab_ManualLayers_UI.cpp` for the instance-list rows only, plus `MapCanvas_MarkerDrag_UI.cpp`, `MapCanvas_UI.h`, `GlobalMarkerSettings_PARAMS.h`) — splitting them keeps each ticket's diff reviewable and lets C proceed even if B's tree-widget composition needs a second pass.
