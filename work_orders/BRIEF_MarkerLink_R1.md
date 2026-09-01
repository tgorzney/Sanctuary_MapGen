# Design Brief — Markers Tab Selection Actions & Link Grouping, Round 1

*For a dedicated design conversation with the SanGen UI Expert. Read `CLAUDE.md` first. DESIGN
phase only — no code. Output: a design doc (`DESIGN_MarkerLink_R1.md`) naming every PARAMS/ARCH
decision needed, plus whatever work-orders are already unblocked.*

## Where this comes from

2026-08-31 Markers Tab / Markers Preview planning conversation with the human. Three related asks
surfaced, in increasing order of scope:

1. Delete key should delete the currently-selected marker(s).
2. The "+ Group" / "+ Layer" section-header buttons should move the current selection into the
   newly-created container, not just create an empty one.
3. Mid-conversation, the human floated "isn't this an Assembly?" for the case where the selection
   spans multiple Marker Types, then concluded no — Assembly is cross-domain (props/decals/markers
   together) and deliberately leaves every member on its own original layer. What the human actually
   wants is Markers-specific: a new **Link** mechanic — a shared `linkId` tag that (a) creates a
   same-named/same-id Group in every Marker-Type section a selected instance belongs to, (b) moves
   each selected instance into its own type's copy, and (c) propagates certain Group-level settings
   (color override confirmed explicitly; the human's wording suggests broader propagation — see §3
   below) across every one of a Link's per-type Groups so they stay in sync. Alongside this, the
   human asked for an icon-size/scale UI rework in the same section header.

## Already confirmed this session — read as ground truth

**Selection model (ratified, don't re-derive):**
- Canvas and list selection are unified through `OverlayInstanceKey_UI`
  (`ARCH_19_25_SelectionRepresentationUnification.md` §19.25, `bManual` tag added to resolve an
  index-space collision).
- Multi-select is real and ratified cross-domain (Markers/Props/Decals) on `MapCanvas`:
  `OverlayInstanceKeySet_UI` (ordered vector, "primary" = last/MRU), `Replace`/`Toggle`/`Union`
  mutators for click/ctrl-click/shift-click and marquee (`ARCH_21_01_MultiSelectRepresentation.md`
  §21.1, part of `ARCH_21_CanvasInteractionUnification.md`).
- List-side multi-select for manual marker Instance rows already ships independently:
  `src/ui/MarkersTab_ManualInstanceSelection_UI.h` (STEP141) — Ctrl-toggle/Shift-range, backed by
  `MarkersTabState::selectedManualInstanceIdentifiers` (plural) + a single primary + a range anchor.
- Procedural (non-manual) marker instances use a separate, session-only, array-position-keyed
  selection mechanism (`ARCH_19_27_ProceduralInstanceSelectionMechanism.md` §19.27) that converges on
  the same `OverlayInstanceKey_UI` representation. **They have no stable per-instance identity across
  bakes** — this is why Assembly membership was ruled manual-instances-only (see below), and is
  almost certainly the same constraint here.

**Delete — wholly unspecified, not partially built:**
- No spec anywhere rules keyboard-delete behavior. No keyboard shortcut of any kind exists in the
  shipped UI today (only `io.KeyCtrl`/`io.KeyShift` modifier reads for click-selection, and
  `ImGuiKey_Enter` for text-input commit).
- The only prior single-instance delete affordance,
  `DrawMarkerInstanceListButtons`'s "Remove Selected" (`src/ui/MarkersTab_ManualInstance_UI.cpp:42-74`,
  single-selection only), is **dead code** — its caller chain
  (`DrawMarkerInstanceSection` → `DrawManualMarkers`, `MarkersTab_Manual_UI.cpp:105-121`) has zero
  live call sites in the real tab draw path. `MarkersTab_ManualLayerRowBody_UI.cpp:110-111` documents
  this gap directly: *"No delete/reorder affordance: deletion/repositioning stays owned by the roster
  editor"* — that roster editor is the dead code just named. **Live, today, there is no way to
  delete a manual marker instance from the Markers Tab UI at all.**
- The existing Group/Layer *container* delete IS live and ratified: a cascade-delete flow
  (`DeleteMarkerLayerBundleCascade`/`DeleteMarkerInstanceLayerCascade`/`DeleteMarkerRuleLayer`,
  `MarkersTab_UI.cpp:371-406`) — no leaf-instance equivalent exists yet.

**"+Group" / "+Layer" — create-empty-only today, but the reassignment primitive already exists:**
- Current live behavior (`MarkersTab_UI.cpp:288-354`): both buttons create a brand-new, empty
  `Params::MarkerLayerBundle`/`MarkerInstanceLayer`/`MarkerRuleLayer`, stamp its `markerTypeName`
  from the current Type-section, nest under the selected same-type Group if any, select it. Neither
  ever touches the current marker selection.
- The reusable primitive for "move a live multi-select into a container" already ships, for
  drag-and-drop: `ReassignManualInstanceLayers(markers, movedIdentifiers, newLayerIndex)` (pure,
  in-place bulk reassignment) and `DetectManualInstanceDropTarget(selectedIdentifiers)` (for
  dropping onto a target with no concrete Layer yet — returns the identifiers so the caller can
  create the Layer first, then reassign), both in
  `src/ui/MarkersTab_ManualInstanceSelection_UI.h`. This is the exact shape "+Layer moves the
  selection into the brand-new layer" needs — no ARCH ruling extends it to the buttons yet.

**Marker-Type scoping of Layers/Groups — ratified, but soft-enforced only:**
- Every `MarkerLayerBundle`/`MarkerInstanceLayer`/`MarkerRuleLayer` carries a scalar `markerTypeName`
  (`ARCH_19_13_MarkerRuleLayerTypeName.md` §19.13, `ARCH_19_MarkerLayerBundle.md` §19.3/§19.4). The
  Markers Tab's Type-section tier is UI-derived (not a stored PARAMS container), enumerating the
  union of `markerTypeName` values present, with a fixed bind order — Alloy, Plasma, Spawn, then
  other values alphabetically, then a final `"(Unassigned)"` bucket
  (`ARCH_19_14_TypeSectionUiDerived.md` §19.14). The shipped tree is now **fully type-section-scoped
  by construction** — `ARCH_19_15_TypeSectionTreeComposition.md` and
  `MarkersTab_Bundles_UI.h:291`'s own comment: "every call site is now type-scoped, there is no more
  'root/global' tree render."
- **But nothing structurally enforces it.** `ARCH_19_12_SoftTypeConsistency.md` §19.12 rules this
  explicitly soft: *"Nothing structurally validates that a `MarkerInstanceLayer`'s actual transforms
  all belong to the Bundle's declared `markerTypeName`."* Confirmed against shipped code — the
  drop-target reassignment path in `MarkersTab_Bundles_UI.cpp`/`.h` performs no type check. It's an
  authoring-metadata mismatch that degrades silently (same class as a stale `layerIndex`, per
  Constitution §6's soft-degrade posture), not a hard constraint — the UI simply never today offers a
  cross-section creation surface, so the case never arises in practice.

**Assembly is a real, separate, unratified concept — do not conflate it with Link:**
- `work_orders/DESIGN_Assembly_R1.md` / `BRIEF_Assembly_R1.md` describe a cross-domain
  (props+decals+markers) rigid-group-move mechanic: each member gets a scalar `assemblyIdentifier`
  tag purely so the group can be moved/rotated together. Per the brief itself: *"each member stays
  on its own original layer."* **Assembly never reassigns Layer/Bundle membership and has no
  type-homogeneity requirement at all.** `ARCH_19_MarkerLayerBundle.md`'s own preamble: "Assembly...
  remains a separate, still-unbuilt feature — this ratification does not merge them." Several of
  Assembly's own sub-questions got ruled as a side effect of ratifying §19 (scalar-not-list
  membership tag, `ARCH_19_05_AssemblyReferencesBundle.md`; recursion cutoff on nested-Bundle
  retagging, `ARCH_19_06_NestedBundleAssemblyCutoff.md`; shared rigid-transform math with Bundle,
  `ARCH_19_08_SharedMathConfirmed.md`) but the feature as a whole has **no coder-dispatchable
  ticket**. Assembly's own still-open items (selection-state architectural home, the Assemblies tab,
  `Params::Assembly` IO sign-off, group move/rotate math placement, symmetry-follow composition,
  "Locked" gating) are unrelated to this brief's Link mechanic and should not be conflated with it —
  they may, however, share infrastructure (see the cross-layer/cross-type multi-select question,
  both briefs need something in this shape) — flag reuse opportunities, don't silently duplicate.

## What this brief needs designed

### 1. Delete key
Wire keyboard Delete to remove every instance in the current live selection
(`selectedManualInstanceIdentifiers` list-side / `OverlayInstanceKeySet_UI` canvas-side — confirm
which is authoritative for this action, they should already be kept in sync per the ratified
selection-unification work). Needs: where the keypress is polled (currently nothing polls any key
anywhere in the UI — this is new surface, not wiring an existing hook); a real "delete selected
instances" helper with the same non-destructive-cascade posture as the existing Group/Layer delete
(`MarkersTab_UI.cpp:371-406`) generalized down to leaf instances (which currently has no analogue at
all); confirm scope — manual instances only, or does procedural selection also need a delete
concept (❓, likely manual-only given procedural instances have no persisted identity to delete).

### 2. "+ Group" / "+ Layer" — move current selection into the new container
When one or more markers are selected at click-time, the newly-created Group/Layer should receive
them (reassign `parentBundleIdentifier`/`layerIndex`) instead of always creating empty. Reuse
`ReassignManualInstanceLayers`/`DetectManualInstanceDropTarget`
(`MarkersTab_ManualInstanceSelection_UI.h`) rather than inventing a second mechanism — this is
already the exact shape the drag-and-drop path uses. **Scoped to a same-Marker-Type selection only**
— if the selection spans types, that's item 3 below (Link), not this button.

### 3. The Link mechanic (new) — cross-type marker grouping with propagated per-type Groups
The human's own description, preserved close to verbatim so nothing is lost in restatement:

> I want to create a new "Link" mechanic for markers, where markers can be linked to a link id...
> Clicking +Link would set the link id on each marker instance to the same id. In the tab UI, we
> would need a "Links" section where the Link Name is an editable name (this is essentially no
> different than a Section and should have exact same options — except when disabling etc., Links
> would just propagate those changes so all instances would match). Each Marker Section would have
> a new Group created with the Link's Name and ID, and instances would be moved to that Link Group.
> If a link is deleted, the Groups would be deleted, but the contents would remain — only the link
> relationship would be deleted. Links need to be a different hue than other Sections like Alloy and
> Plasma and Spawn. Links would be where the color override is set, and a Group with a link (since
> Groups are created in Sections to match a link) in a different section, would automatically have
> its override-color set the same as the Link, and the override toggle would auto-toggle when the
> Link's color-override toggle is toggled.

Concretely, this brief needs the following designed (not guessed):

- **A new `+ Link` button** on the Marker-Type section header (alongside `+ Group`/`+ Layer`),
  enabled when the current selection is non-empty (unlike +Group/+Layer, explicitly meant to work
  **across** Marker Types). Clicking it: (a) mints a new Link identity (id + editable name), (b) for
  every Marker Type represented in the current selection, creates a `MarkerLayerBundle` (Group) in
  that type's section carrying the Link's name and a back-reference to the Link's id (mirroring the
  already-ratified `ARCH_19_05_AssemblyReferencesBundle.md` shape — a scalar `linkIdentifier` on the
  Group, not a forward members-list on the Link), and (c) moves every selected instance of that type
  into that new Group (reuse item 2's reassignment primitive, per-type).
- **A new "Links" top-level tier in the Markers Tab**, sibling to the Marker-Type sections, listing
  every Link by its editable name. ❓ Exact composition relative to `ARCH_19_14_TypeSectionUiDerived.md`
  §19.14's existing type-section enumeration — is this a new UI-derived tier alongside Alloy/Plasma/
  Spawn/(Unassigned), or a wholly separate list outside that tree? The human's framing ("exact same
  options as a Section") suggests it should reuse the Section header widget, not a bespoke layout.
- **Distinct hue for Links vs. Type-sections** — a real visual-differentiation ask, needs a color/
  theme decision (not just "pick something," should follow whatever section-header theming
  convention already exists, if any).
- **Propagation semantics — the part most in need of an explicit ruling, not an inference.** The
  human's wording says a Link "has exact same options as a Section" but that settings changes
  "propagate... so all instances would match" — i.e. editing certain properties at the Link level
  (definitely color-override + its toggle, per the explicit example; possibly others, e.g.
  enable/disable, given the "except when disabling etc." phrasing) writes through to every one of
  that Link's per-type Groups so they render identically. Needs a ruling on: (a) the *complete* list
  of properties that propagate vs. stay independent per-type-Group, (b) direction — is the Link the
  single source of truth with per-type Groups as read-only mirrors for those fields, or can editing a
  synced field on one Link-bound Group also write back up to the Link (and thus fan out to its
  siblings)? (c) the actual propagation mechanism — live re-derivation on read (Link-bound Groups
  don't store their own copy, they resolve it from the Link every frame) vs. write-through-and-copy
  (each Group stores its own value, kept in sync by an explicit propagation step whenever the Link's
  value changes). Read-and-resolve avoids drift entirely and is the shape this codebase already
  favors elsewhere (`Params::ResolvePropInstanceLayerId`-style shared resolvers) — flag as the
  likely-preferred direction, don't self-ratify.
- **Delete-Link semantics — already decided by the human, restate as ground truth for the design
  pass**: deleting a Link deletes its per-type Groups (ungroup — members promoted up, per the same
  non-destructive posture already ruled for Assembly and for the existing Group/Layer cascade
  delete), but never deletes the member instances themselves; only the link relationship
  (`linkIdentifier` tag) is removed.
- **Scoping question, mirroring Assembly's own §0 ruling — not yet answered here**: manual instances
  only, or can a Link also span procedural marker instances? Given procedural instances have no
  cross-bake stable identity (`ARCH_19_27`), the working assumption should be manual-only unless
  there's a reason to invent a new procedural stable-id system first (out of scope for this brief) —
  rule this explicitly rather than leaving it implicit.

### 4. Icon-size / scale UI rework (Marker-Type section header)
- Compact the existing icon-scale slider to make room for the new controls above.
- Replace the horizontal icon-size slider with a **circular slider/rotator** control, same height
  as the current horizontal slider (fits the compacted header row). ❓ No radial/knob widget exists
  anywhere in the current widget library (confirm during design pass) — likely a new widget-library
  primitive, same category as Assembly's new tree widget; scope and name accordingly.
- Icon size range: **clamp to [0.25, 2.0]**.
- **Two separate icon-size inputs**: base icon size, and a second "selected size" (the icon's
  rendered size specifically while its marker is selected — presumably for visual emphasis on
  canvas). Both use the same circular-slider widget and the same [0.25, 2.0] clamp. ❓ Confirm this
  reading against the human directly if ambiguous — "add one more for selected size" could also mean
  a per-marker-type distinct "selected" scale rather than a global default+selected pair; the design
  pass should state which interpretation it's building and why.

## Specs and files to read first

- This brief's "already confirmed" section above (don't re-derive).
- `ARCH_19_MarkerLayerBundle.md` and §19.3/§19.4/§19.5/§19.6/§19.12/§19.13/§19.14/§19.15.
- `ARCH_21_01_MultiSelectRepresentation.md`, `ARCH_19_25_SelectionRepresentationUnification.md`,
  `ARCH_19_27_ProceduralInstanceSelectionMechanism.md`.
- `work_orders/BRIEF_Assembly_R1.md` / `DESIGN_Assembly_R1.md` — read in full for the
  cross-layer-selection and scalar-backreference precedent; this brief's Link mechanic is a sibling
  concept and should reuse machinery where it genuinely overlaps (esp. any cross-type/cross-layer
  selection-set concept Assembly's design pass proposes) rather than inventing a second one.
- `src/ui/MarkersTab_UI.cpp` (current +Group/+Layer button logic, lines 288-354; cascade-delete,
  371-406).
- `src/ui/MarkersTab_ManualInstanceSelection_UI.h` (the reassignment/drop-target primitives to
  reuse for both item 2 and item 3).
- `src/ui/MarkersTab_ManualInstance_UI.cpp`, `MarkersTab_Manual_UI.cpp` (dead single-select delete
  code — read to understand what NOT to resurrect as-is).
- `src/ui/MarkersTab_Bundles_UI.h`/`.cpp` (Group/Bundle creation and type-scoping, drop-target
  reassignment with no type check).
- `sangen_arch_pack/specs/UI_FRAMEWORK_SPEC.md` (section-header widget conventions).

## Who to consult

SanGen UI Expert first — this is the bulk of the brief (button behavior, new Links tier, new radial
widget, header layout). Loop the ARCH Expert once the Link data-model shape (new `linkIdentifier`
field naming/casing, whether "Links" is a new UI-derived tier or a new stored PARAMS concept, and
the propagation-ownership question in item 3) needs ratifying — likely required before any of this
is coder-dispatchable, given the new widget + new cross-type selection semantics + new propagation
mechanism. Loop the Format Expert for the `.sanmap` wire shape of the new `linkIdentifier` field and
any new Link-level color-override field. Confirm the manual-only scoping question (item 3) and the
icon-size dual-input interpretation (item 4) with the human directly if the design pass can't resolve
them from context alone.

## Response style (carry forward)

Terse, ❓ for questions, ⚠️ for problems, no narration.
