# Design Brief — Cross-Layer Assembly Grouping, Round 1

*For a dedicated design conversation with the SanGen UI Expert. Read `CLAUDE.md` first. DESIGN
phase only — no code. Output: a design doc (`DESIGN_Assembly_R1.md`) naming every PARAMS/ARCH
decision needed, plus whatever work-orders are already unblocked.*

## Where this comes from

This is the formal continuation of "topic F" from a 2026-08-24 Markers-UI planning session: the
human wants to select markers/props/decals across multiple different layers at once (e.g. a spawn
marker layer + an alloy layer + a decal layer + a prop layer, all visible/unlocked in the preview)
and group them into a named, reusable, nestable **Assembly** — then move/rotate that Assembly as
one rigid unit (optionally symmetry-following), while each member stays on its own original layer.

That session ran a full multi-expert survey (ARCH/Format/IO Architecture/Generator/Compute
Optimization/UI/UI Optimization) before any design work started. The findings below are already
decided or already confirmed — **don't re-derive them, read them as ground truth**:

## Already confirmed this session — read as ground truth

**Persistence is feasible, and the exact mechanism is now proven, not just inferred.**
- Per-instance field injection into `props[].transforms[]`/`decals[].transforms[]`/marker
  transforms is **empirically confirmed safe for real game load** — not just architecturally
  plausible. Live-tested 2026-08-24: an `InstanceId` field was added to all 1,180 prop transforms
  across all 17 prop groups in a real shipped map (`Pandemonium Isthmus.sanmap`), and the map
  loaded successfully in-game. Recorded in `sangen_arch_pack/specs/SANMAP_FORMAT_SPEC.md`
  Correction 14 (2026-08-24 entry). This de-risks adding a new per-instance field to
  `PropTransform`/`DecalTransform`/`MarkerTransform` — it is not an open question anymore.
- A new top-level SanGen-owned section (e.g. `Assemblies`) is safe — unrecognized top-level
  sections are parsed then dropped by the real game loader (already-established precedent, same
  class as `MarkerGroups`/`PropGroups`/`DecalGroups`).

**Data model — already decided by the human, not open for re-litigation:**
- **No multi-membership.** Each leaf instance (`PropTransform`/`DecalTransform`/`MarkerTransform`)
  carries exactly one `AssemblyId` scalar tag (`-1`/absent = none) — same shape/precedent as
  `layerIndex` (a backward tag pointing at what it belongs to), not a list. Rejected explicitly:
  an object belonging to two sibling Assemblies simultaneously creates ambiguous transform
  composition (which Assembly's move "wins" / does moving one re-apply on top of the other) with
  no clean semantics — every reference system checked (Unity, Unreal, image-editor groups) avoids
  this the same way.
- **Nesting solves the "belong to more than one thing" need instead of multi-membership.** A
  top-level `Assemblies` table: `{id, name, parentAssemblyId}` (`-1`/absent = root). Strict tree
  (forest), not a DAG — one parent per Assembly. An Assembly's full membership = every leaf
  instance directly tagged with its id, plus every leaf tagged with any descendant Assembly
  (recursive). Moving/rotating an Assembly = walk the tree down from it, collect that full set,
  apply the rigid transform to all of them.
- **Cycle prevention is required**, both on reparent (walk the new parent's chain; reject if it
  contains the Assembly being reparented) and on import (a hand-edited/foreign file could contain
  a cycle — same loud-non-fatal recovery convention already used elsewhere in this format: log and
  treat as root, don't hard-fail the whole file).
- **Deleting an Assembly = ungroup, not cascade-delete.** Its direct members and child Assemblies
  get promoted up to the deleted Assembly's own parent (or to root). Never destructively deletes
  the members/children themselves. Matches this codebase's established non-destructive-default
  posture (no auto-delete on Spawn-group shrink, Fix Symmetry defaults to skip-not-overwrite,
  etc.) and how "ungroup" behaves in every comparable editor.
- The `Assemblies` metadata table does **not** carry a members list (rejected a
  forward-reference/`members: []` shape as a real alternative that was considered and dropped —
  it duplicates membership truth in two places that can go out of sync; the backward per-instance
  tag is the single source of truth, consistent with how `layerIndex`/layer membership already
  works everywhere else in this format).

**Selection interaction — already decided:**
- Click on a grouped instance in the viewport selects the **outermost (root-most) ancestor
  Assembly** by default, not the immediate one. Double-click drills in one level at a time.

**Open scoping question this brief must answer — not yet decided:**
- **Can a procedurally-placed (rule-generated) instance be an Assembly member, or only
  manually-authored instances?** `AssemblyId` as scoped lives on `PropTransform`/
  `DecalTransform`/`MarkerTransform` — all manually-authored PARAMS types, not
  `Data::PlacementInstances` (live-regenerated procedural scatter output, which has no stable
  per-instance identity to tag in the first place). The working assumption is **Assembly
  membership is manual-instances-only** — name this explicitly as the ruling, or explain why it
  should be otherwise and what stable identity a procedural instance would need first.

**Prior art / directly reusable machinery — read before designing, don't reinvent:**
- `work_orders/STEP81_MarkersTabManualLayers_UI.md:481` — explicit prior exclusion of multi-select
  ("one picker on the selected instance only... Not asked for, not designed") — confirms no
  cross-layer selection concept exists anywhere upstream to build on; this is greenfield.
- `src/ui/MarkerDragGesture_UI.h`/`MarkerOrbitCorrespondence_UI.h` (STEP94, already shipped) — the
  existing single-marker drag-and-follow-symmetry gesture. If "optional symmetry-following" on an
  Assembly group-move is in scope for this round, this is the machinery to generalize (per-marker
  independent orbit resolution was the Generator Expert's recommendation last round — each
  selected member resolves its own orbit/correspondence, not one combined group orbit), not
  reimplement.
- `src/params/PropInstance_PARAMS.h`'s `Params::ResolvePropInstanceLayerId`/
  `ResolveDecalInstanceLayerId` (landed 2026-08-24, this session) — the just-shipped precedent for
  "a shared, PARAMS-level pure resolver function used by both a PROC pass and a live UI read path,
  instead of two independent formulas that can drift." If Assembly membership needs an analogous
  resolved-lookup helper anywhere, follow this shape.
- **No tree/hierarchy widget exists anywhere in the UI library today** (confirmed by direct read
  of `src/ui/DraggableListWidget_UI.h`, `Section_UI.h`, `VirtualListWidget_UI.h`,
  `LayerEditor_Group_UI.cpp`, `ArmiesTab_Units_UI.cpp`, 2026-08-24). `DraggableList` is strictly
  flat (one `std::vector<T>`, index-based reorder signal). The only nesting precedent
  (`LayerEditor_Group_UI.cpp`'s GeoLayer→Layer, `ArmiesTab_Units_UI.cpp`'s Army→Units) is two
  hardcoded levels via caller composition — does not generalize to arbitrary Assembly-in-Assembly
  depth. **The human has chosen the full option: build a real tree widget** (drag-to-reparent,
  distinct above/below/as-child drop zones, per-node expand/collapse), not a cosmetic
  flat-list-with-indentation fallback. This is a new widget-library primitive, scope accordingly.
  `Section_UI.h`'s caller-owned-state convention (`SectionState.bOpen`, never a function static —
  "the v1 bug this library exists to kill") is the precedent to follow for per-node expand/collapse
  state (a stable per-node-id → bool map, not a single bool).

## What this brief needs designed

1. **The tree widget itself** — data shape, render/signal contract (mirroring `DraggableList`'s
   "Render detects, caller applies, MUTATES NOTHING" split), reparent-via-drag mechanics (source
   node + destination parent + position among siblings; three drop-zone types per row).
2. **The cross-layer multi-select mechanism** that lets a human pick markers/props/decals across
   several different tabs' layers at once, as the precursor to "create an Assembly from selection."
   This needs: a new selection-set concept (not `MapCanvas`'s existing single
   `selectedEntityIdentifier`, and not any one tab's existing single-index state) — where does it
   live, given `Ui::ApplicationTabState`'s own header comment states its "one instance each" rule
   specifically to prevent tabs *unintentionally* sharing state (this may be the first legitimate
   case for deliberately wanting shared state — confirm with the ARCH Expert whether it extends
   that rule or needs a different home); gating so only visible+unlocked instances are selectable;
   a new canvas gesture (ctrl+click toggle-add and/or marquee/rubber-band) disambiguated against
   the existing pan/click/marker-drag branching in `MapCanvas_Draw_UI.cpp`.
3. **The Assemblies tab** — a dedicated tab (spans marker/prop/decal domains, doesn't belong nested
   in any one of them, same reasoning as why Scenarios has its own tab), showing the tree.
4. **Group move/rotate mechanics** — gathering full recursive membership, applying a rigid
   transform, the pivot-point question (centroid of the full member set? a designated anchor?),
   and how optional symmetry-following composes with a multi-member group move (flag, per last
   round's Generator Expert input, the correctness hazard when a selected member and its own
   mirror twin are both in the moved set simultaneously — needs an explicit precedence rule, don't
   let it silently oscillate).
5. **The `.sanmap` shape**: `Assemblies: [{id, name, parentAssemblyId}]` plus the per-instance
   `AssemblyId` field on the three transform types — confirm field names/casing (ARCH Expert
   naming-law call), confirm additive/no-version-bump status (should be, per the now-proven
   per-instance-field-injection precedent), and rule on the manual-instances-only scoping question
   above.
6. **Flag, don't invent, any further ARCH module-boundary ruling.** Given the size of this (new
   widget primitive + new selection-set concept + new PARAMS shape), expect this needs an ARCH
   Expert pass before any of it is coder-dispatchable — name exactly what needs ratifying rather
   than assuming.

## Specs and files to read first

- This brief's "already confirmed" section above (don't re-derive).
- `sangen_arch_pack/specs/SANMAP_FORMAT_SPEC.md` Correction 14 (per-instance field injection proof).
- `work_orders/STEP81_MarkersTabManualLayers_UI.md` (prior multi-select exclusion).
- `work_orders/STEP94_MarkerDragAndFollowSymmetry_UI.md`, `src/ui/MarkerDragGesture_UI.h`,
  `src/ui/MarkerOrbitCorrespondence_UI.h` (symmetry-follow machinery to generalize, if in scope).
- `src/ui/DraggableListWidget_UI.h`, `src/ui/Section_UI.h` (widget conventions the new tree widget
  must follow — caller-owned state, Render/Apply split).
- `src/ui/Application_TabState_UI.h` (the "one instance each" selection-state rule to confirm
  against with ARCH).
- `src/params/PropInstance_PARAMS.h` (the `ResolvePropInstanceLayerId` shared-resolver precedent).

## Who to consult

SanGen UI Expert first (tree widget + selection mechanism + tab design — the bulk of this brief).
Loop the ARCH Expert once the selection-set-state-home question and the `.sanmap` shape need
ratifying — likely immediately, given the scope. Loop the Generator Expert for the symmetry-follow
composition question in item 4, if group-move symmetry-following is confirmed in scope for this
round (it may be deferred to a later round — state that explicitly if so).

## Response style (carry forward)

Terse, ❓ for questions, ⚠️ for problems, no narration.
