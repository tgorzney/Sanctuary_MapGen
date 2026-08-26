# Design Brief — Markers Tab Group/Layer Restructure, Round 1

*For a dedicated design conversation with the SanGen UI Expert. Read `CLAUDE.md` first. DESIGN
phase only — no code. Output: a design doc (`DESIGN_MarkerGroupLayerRestructure_R1.md`) naming
every PARAMS/ARCH decision needed, plus whatever work-orders are already unblocked.*

**STATUS: ON HOLD.** The human has explicitly said not to proceed with this design round until
other in-progress, Assembly-adjacent work (from other active Claude Code sessions this same day)
finishes. This document exists to capture the human's requirements precisely, through several
rounds of clarification, so nothing has to be re-derived when the hold lifts. Do not dispatch a
design consult against this brief until told to proceed.

## Where this comes from — final corrected model (2026-08-25, after several clarification rounds)

The human's original complaint: the Markers tab shows "Procedural Rules," "Manual Marker Layers,"
"Manual Markers," and "Placed Markers" as four separate main sections — "this is entirely
incorrect." Several rounds of back-and-forth clarified and CORRECTED an initially wrong reading of
the target structure (see "Corrections" below) — **this section states only the final, confirmed
model. Do not trust any other document's summary of this brief; if one disagrees with this
section, this section is newer and correct.**

**The confirmed target structure:**
- A top-level collapsible **"Markers"** section.
- Inside it, **"Group"** nodes, hierarchical (a Group may contain child Groups). **A Group is
  scoped to ONE marker type** (e.g. "Alloy") — NOT a mixed-type container. A Group's children are
  **Layers**, each Layer strictly Procedural OR Manual (never both), all Layers under a Group
  share that Group's marker type. Example, verbatim from the human:
  ```
  Markers: (Section)
     ARMY_01: (Group) (Alloy Type)
        Primary:   (Layer: Manual)      → Alloy_01, Alloy_02
        Secondary: (Layer: Manual)      → Alloy_03, Alloy_04
        Procedural:(Layer: Procedural)  → Alloy_05, Alloy_06
  ```
  Multiple Groups of the SAME type are allowed (e.g. two different Alloy Groups for two different
  clusters) — a Group is a named, organizational container, not a type-uniqueness constraint.
- **Assemblies are a separate, already-in-progress feature** (see `work_orders/BRIEF_Assembly_R1.md`/
  `DESIGN_Assembly_R1.md`) — cross-domain, mixed-type containers (Alloy + Spawn + Props + Decals +
  Units etc. all together). Assembly is NOT the same thing as Group, and this brief does not
  redesign or merge into it. The human's confirmed example:
  ```
  Assemblies:
     ARMY_01: (Assembly) → [everything belonging to ARMY_01, across every domain/type]
  ```
- **Assembly membership should be able to reference whole Groups, not just individual leaf
  instances — confirmed, in scope, and must be LIVE, not a snapshot.** If a marker is later added
  to (or removed from) a Group that an Assembly references, the Assembly's resolved membership
  updates automatically — no manual re-tagging, no stale copy. (Note: this is likely the natural
  behavior anyway, given Assembly's own already-ratified design resolves membership by walking
  backward tags at query time rather than storing a snapshot list — confirm this explicitly in the
  design round rather than assuming it "just works.")
- Because Assembly may need to reference a Group belonging to ANY domain (Markers today; Props/
  Decals/other domains later — see "Universal, reusable Group mechanism" below), a Group reference
  inside Assembly needs a domain discriminator (e.g. `{domain: Marker, groupIdentifier: X}` vs.
  `{domain: Prop, groupIdentifier: Y}`) — the human raised this explicitly as an open question,
  not yet answered.
- If a Group is selected and moved/rotated, the transform applies to each member independently,
  and each member stays on its own original Layer (it doesn't get reparented).

## Universal, reusable Group mechanism — human's explicit requirement, read carefully

**The human does NOT want a Markers-only `MarkerLayerGroups` table/widget.** Verbatim: "props
could have groups, decals could have groups, and those groups would all have the same rules, so we
should really design universal code that can be reused for any type including Markers and Props
and Decals, and NavMesh etc, basically a universal widget for display, but the input changes and
the output is the hierarchical widget display, but the end entities will have their own settings
for that particular type (Alloy has its settings, Spawns have theirs, Units have theirs etc.)."

This means the design round must treat "Group" as a **generic mechanism** from the start — one
reusable hierarchy/tree concept (data shape + UI widget) parameterized by domain, not a Markers-
specific one-off that would need to be rebuilt per-domain later. Concretely:
- The **UI widget** (tree render/expand-collapse/drag-to-reparent) should be domain-agnostic — the
  same widget code renders a Group tree whether the leaves are Alloy markers, Prop instances, or
  (eventually) NavMesh blockers. Each leaf/Layer's OWN settings panel stays fully type-specific
  (Alloy's settings ≠ Spawn's ≠ Units') — genericize the CONTAINER/tree, not the leaf content.
- The **data model** should follow one shared shape/pattern across domains (mirroring how
  `layerIndex`, `layerId`, and the `Resolve*InstanceLayerId`/`Resolve*InstanceLayerColor` function
  families already establish "one pattern, one per-domain implementation" as this codebase's
  existing convention for exactly this kind of cross-domain reuse) — NOT necessarily one single
  shared table with a domain column (weigh both: a generic pattern instantiated per-domain, vs. one
  literal shared table discriminated by domain — the design round must make this call explicitly,
  it is now the central open architecture question, more central than the earlier Assembly-merge
  question, which the human's clarifications above have already resolved as "separate, not merged").
- ❓ **Open**: does genericizing now (for Markers + a future Props/Decals/NavMesh) risk premature
  abstraction if only Markers is being built first? Weigh against Constitution's general posture
  and the human's own explicit "design universal code... reused for any type" instruction — this
  is not premature since the requirement is stated up front, not speculative; the design round
  should build the generic foundation but the FIRST delivered implementation is Markers-only (see
  Delivery sequencing below). Flag to ARCH for a ruling on the right shape of genericity (a shared
  C++ template/CRTP pattern? a shared base PARAMS struct? shared free functions parameterized by
  the two existing per-domain types, mirroring the `Resolve*` function-family precedent? — don't
  guess, this is exactly the kind of module-boundary call ARCH owns).

## Delivery sequencing — human's explicit priority order

1. **Get Markers working and usable first.** This is the actionable, near-term priority — the
   Group/Layer restructure as it applies to Markers specifically (the human's own worked example
   above) is what should actually ship first.
2. Props/Decals (and whatever else needs the same Group mechanism) build on the SAME generic
   foundation later — not redesigned from scratch, but also not necessarily built in this same
   round. The design round should design the generic foundation now (per the section above) but
   scope the first work-order(s) to Markers only.
3. **NavMesh blockers are explicitly future, different-session work.** NavMesh does not exist as a
   domain in this codebase today (confirmed: no NavMesh-related PARAMS/UI/IO found anywhere this
   session — a fresh grep should be re-run to confirm this is still true when the design round
   starts, don't trust this brief's staleness). Do not design NavMesh's actual data model in this
   round — only ensure the generic Group mechanism doesn't structurally foreclose NavMesh adopting
   it later. A full NavMesh investigation/BRIEF is separate, future, out of this brief's scope.
   The human has previewed that NavMesh will need several movement-type layers — Land, Sea,
   Submarine, Amphibious, Hover, Air, "and I might be missing some" (their own words, expect this
   list to grow/change) — worth keeping in mind only as a rough sense of NavMesh's likely shape
   (a per-movement-type layer set, plausibly a good fit for whatever generic Group/Layer mechanism
   this brief designs), not as a spec to design against now. When the future NavMesh session
   starts, it should check for any other active session that may have picked up NavMesh work in
   the meantime, per this project's standing peer-coordination practice.

## Already confirmed this session — read as ground truth, don't re-derive

**Current backend/UI shape (investigated in full this session, accurate as of the latest commits —
STEP106-118 landed, none of them touched this structural question):**
- `Params::MapRecipe` carries FOUR independent, unrelated arrays for markers:
  `markerRuleLayers` (`std::vector<MarkerRuleLayer>`, procedural — rules only, no manual field),
  `markers` (`std::vector<MarkerInstanceGroup>`, manual instances keyed by TYPE string),
  `chains` (unrelated — ordered marker-name sequences), `markerLayers`
  (`std::vector<MarkerInstanceLayer>`, manual-layer metadata: name/color/lock/grid-snap/symmetry
  — carries no rules and no transforms itself). Confirmed: `src/params/MapRecipe_PARAMS.h`.
- The Markers tab UI (`src/ui/MarkersTab_UI.cpp`) draws five flat sibling sections — Global,
  Procedural Rules, Manual Marker Layers, Manual Markers, Placed Markers — each its own
  collapsible header, no shared parent, no nesting. This is a faithful, correct rendering of the
  backend's flatness, not an independent UI-layer bug layered on top of a better-organized model.
- **`MarkerRuleLayer`/`MarkerInstanceLayer` stay exactly as they are** — two separate types, two
  separate arrays, per `work_orders/DESIGN_MarkerLayerSymmetry_R1.md` §1's prior ruling, which
  this brief does NOT reopen. A given Layer is strictly Procedural or Manual, never both. The only
  new concept is the Group container ABOVE Layer (and, per the sections above, generalized beyond
  Markers).
- **No "Group" container (a node above Layer) exists anywhere** — not in PARAMS, not in the UI,
  not even in the deferred-but-named "MarkersStack Group/Layer/LayerType" design mentioned in
  `SANMAP_FORMAT_SPEC.md` Correction 7/15/16 (those corrections built exactly ONE tier —
  `Group(MarkerRuleLayer)→Rule` for procedural, `MarkerGroups(MarkerInstanceLayer)→Transform` for
  manual — not a container ABOVE Layer).
- `SANMAP_FORMAT_SPEC.md`'s own Correction 7/15/16 already reuses the word "Group" for two
  DIFFERENT existing concepts (the procedural rule-layer wrapper, and the manual layer-metadata
  wrapper) — this brief's new "Group" concept is a THIRD, different meaning of the same English
  word. Flag this naming collision explicitly to the ARCH Expert; do not let a design or ticket
  silently conflate the three. (A real, distinct name for this brief's Group — e.g. something
  other than bare "Group" — may be the cleanest fix; flag as an option, don't force it.)

**Current wire format, precisely confirmed (2026-08-25, post-STEP106-118):**
- Procedural already has ONE tier: `"MarkersStack": [{Name, Enabled, SymmetryUseGlobal,
  SymmetryMask, RadialSymmetryRepeatCount, "Rules": [...]}]` — each `MarkerRuleLayer` wraps its
  own `Rules` array, real JSON nesting (`MapExporter_MarkersStack_IO.cpp:54-76`).
- Manual is TWO DISJOINT top-level sections, linked only by a bare integer index, never JSON
  nesting: `"markers": {"<Type>": {resource, "transforms": {"<name>": {position, rotation, scale,
  alias, symmetryGroupIdentifier, iconNameOverride}}}}` (instances) and `"MarkerGroups": [{Name,
  Color, IconScale, Id, SymmetryUseGlobal, SymmetryMask, RadialSymmetryRepeatCount, Locked,
  GridSnapEnabled, GridSnapSizeWorldUnits, ColorOverrideEnabled}]` (layer metadata) — a
  transform's `layerIndex` is a plain array-position reference into `MarkerGroups`, resolved
  entirely at runtime, no structural parent/child relationship in the JSON itself
  (`MapExporter_Markers_IO.cpp:17-84`, `MapImporter_Markers_IO.cpp:9-11,87-97`).
- **Nothing in the current wire format goes deeper than these two tiers, for ANY domain** —
  Props/Decals (`PropGroups`/`DecalGroups`) are flat one-tier arrays with no Group-above-Layer
  concept either. The only place N-deep nesting is even conceived is Assembly's own unshipped
  `{id, name, parentAssemblyId}` flat-array-plus-parent-back-pointer design (adjacency-list style,
  not true JSON nesting) — this remains the one directly reusable shape precedent (see storage
  recommendation below), even though Assembly and Group are confirmed separate concepts.
- ⚠️ **Real bug, found while researching this brief, separate from this restructure**: the manual
  marker exporter never writes `layerIndex` back out on export (`BuildMarkerTransformJson`,
  `MapExporter_Markers_IO.cpp:17-39`, has no `json["layerIndex"] = ...` line at all) — unlike Props/
  Decals, which both do (`MapExporter_Props_IO.cpp:35`, `MapExporter_Decals_IO.cpp:32`). Every
  exported marker's layer membership currently round-trips as "always absent → clamps to 0" on
  reimport. Worth its own quick investigation/ticket; not blocking this design round, but flag to
  whoever picks it up — this restructure's new Group storage must not silently inherit the same bug.
- ⚠️ **`SANMAP_FORMAT_SPEC.md` is stale relative to the live exporter/importer**: Correction 16's
  documented `MarkerGroups` field list is missing `Locked`/`GridSnapEnabled`/
  `GridSnapSizeWorldUnits`/`ColorOverrideEnabled`, and its `markers[type].transforms[name]` field
  list is missing `iconNameOverride` — confirmed genuinely absent from the spec doc via grep, not
  merely uncommitted-and-pending. Only the ARCH Expert writes the ARCH pack; flag this as a needed
  cleanup pass, likely bundled with whatever ARCH ratification this restructure needs anyway.

**Format Expert storage-shape research, complete (2026-08-25) — now simplified since Group and
Assembly are confirmed SEPARATE (not a merge question), but re-scope for the genericity
requirement above before treating as final:**
- Original recommendation (Markers-only framing): a flat parent-pointer forest table
  (`{id, name, parentGroupId}`) plus an explicit scalar back-reference field added directly to
  both `MarkerRuleLayer` and `MarkerInstanceLayer`. Rejected name-match resolution (option ii) for
  the same reason Assembly's own design rejected a `members: []` forward-reference — explicit id
  references only, names aren't guaranteed unique.
- **This must be re-evaluated against the "universal/generic across domains" requirement above**:
  does the Group table become one shared, domain-discriminated table (`Groups: [{id, name,
  parentGroupId, domain: "Marker"|"Prop"|...}]`) reused by every domain, or a repeated
  per-domain table following one shared shape/code pattern (`MarkerLayerGroups`, `PropLayerGroups`,
  ... each independently instantiated)? This is now the design round's central storage question —
  the earlier "gated on Assembly relationship" framing is resolved (separate, confirmed), but this
  new genericity question replaces it as the open item.
- Back-reference field placement: additive to each domain's own Layer struct directly (confirmed
  for Markers: `MarkerRuleLayer` currently has NO stable id field at all, `MarkerInstanceLayer`
  does — this asymmetry doesn't block the design). Exact field spelling rides the same pending ARCH
  naming question already raised for `AssemblyId`/`layerId` in `DESIGN_Assembly_R1.md` §7.
- Additive/no-version-bump confirmed achievable, same precedent class as every prior field/section
  addition this session.
- Group-in-Group nesting: cleanly supported via `parentGroupId` on the Group table entries
  themselves — the human's worked example doesn't show nesting but the "hierarchical" framing in
  earlier rounds implies it should be supported; confirm explicitly in the design round. Cycle
  handling on import: the already-ratified Assembly convention ("log and treat as root," loud
  non-fatal) should apply verbatim, not a new ruling.
- Scan cost to resolve Group membership: an O(n) scan over the relevant Layer array(s) filtering on
  the back-reference field is acceptable at this codebase's established authoring scale (tens of
  layers, not 100k) — same posture already adopted for Assembly's own
  `CollectAssemblyRecursiveMembership`.

**Prior art / directly reusable machinery — read before designing, don't reinvent:**
- `src/ui/DraggableListWidget_UI.h` — the existing flat-list widget (index-based reorder, no
  nesting). `DESIGN_Assembly_R1.md` already established that NO tree/hierarchy widget exists
  anywhere in this UI library today, and that the human already chose to build a real one (drag-
  to-reparent, distinct drop zones, per-node expand/collapse) as part of Assembly's own design.
  Given the genericity requirement above, this brief's Group tree widget and Assembly's tree
  widget should very likely be the SAME widget (or share a common base) — flag this explicitly as
  a design question, don't build two tree widgets.
- `Params::ResolveMarkerGroupTypeTintColor`/`ResolveMarkerIconTemplateIdentifier`
  (`GlobalMarkerSettings_PARAMS.h`/`MapCanvas_IconLayer_CullManual_UI.cpp`, both landed this
  session, STEP114/116) — the established "group name → type" resolution pattern; also a good
  precedent for how this codebase already does "one function-family shape, one implementation per
  domain" (see `Resolve*InstanceLayerId`/`Resolve*InstanceLayerColor` in `PropInstance_PARAMS.h`
  too) — the likely right shape for genericizing the new Group mechanism without a literal shared
  base class, if that's the direction ARCH picks.
- `Params::MarkerInstanceLayer::bLocked`/`bGridSnapEnabled`/`bColorOverrideEnabled` (STEP106/116)
  — per-Layer settings that must continue to mean exactly what they mean today; a Group wrapping
  multiple Layers must not blur or override individual Layers' own settings.

## What this brief needs designed

1. **The generic Group mechanism's shape** — data model AND UI widget, parameterized by domain,
   per the "Universal, reusable Group mechanism" section above. This is now the central design
   question, more central than the old Assembly-relationship question (already resolved: separate).
2. **The Group container's own data shape**, per-domain instantiation: `{id, name, parentGroupId}`
   forest, single-type-scoped (confirmed: a Group belongs to exactly one marker/prop/decal TYPE,
   never mixed), back-reference field added to each domain's own Layer-equivalent type.
3. **Assembly-references-Group, live-updating** — the domain-discriminated reference shape
   (`{domain, groupIdentifier}`), and confirm/design the "live, not snapshot" resolution mechanism
   (very likely already natural given Assembly's query-time resolution, per the note above —
   verify, don't assume).
4. **The Markers tab UI**: the actual tree rendering (Markers → Groups → Layers → Procedural/
   Manual sub-sections), reusing Assembly's tree widget (very likely, per "Prior art" above) or
   justifying a divergence.
5. **Group-select + move/rotate mechanics**: gathering a Group's full recursive membership,
   applying a rigid transform to every contained instance while each keeps its own Layer, the
   pivot-point question, and how this interacts with the existing per-marker drag-and-follow-
   symmetry gesture (STEP94) if a Group member also has a symmetric sibling outside the Group.
6. **Delivery scoping**: confirm the first work-order(s) out of this design round are Markers-only
   (per "Delivery sequencing" above), even though the underlying mechanism is designed generically.
7. **Flag, don't invent, any further ARCH module-boundary ruling** — this is large (a new generic
   cross-domain container concept, a shared tree widget with Assembly, a PARAMS shape change
   touching every marker-layer-scoped ticket landed this session, a real architectural call on HOW
   to genericize — shared table vs. per-domain tables, shared base type vs. function-family
   pattern). Expect this needs a full ARCH Expert pass before any of it is coder-dispatchable —
   name exactly what needs ratifying rather than assuming.

## Specs and files to read first

- `work_orders/BRIEF_Assembly_R1.md`, `work_orders/DESIGN_Assembly_R1.md` (full — the Assembly-
  references-Group mechanism cannot be designed without reading Assembly's actual current design).
- `work_orders/DESIGN_MarkerLayerSymmetry_R1.md` (the prior "two arrays, not one" ruling — stays
  in force, NOT reopened by this brief).
- `sangen_arch_pack/specs/SANMAP_FORMAT_SPEC.md` Corrections 7, 14, 15, 16 (the three different
  existing "Group" meanings; the proven field-injection precedent).
- `src/params/MarkerRule_PARAMS.h`, `src/params/MarkerInstance_PARAMS.h`,
  `src/params/GlobalMarkerSettings_PARAMS.h`, `src/params/PropInstance_PARAMS.h` (current, real,
  post-STEP106-118 shapes, including the `Resolve*` function-family precedent worth generalizing).
- `src/ui/MarkersTab_UI.cpp`, `MarkersTab_RuleLayers_UI.cpp`, `MarkersTab_ManualLayers_UI.cpp`,
  `MarkersTab_Manual_UI.cpp`, `MarkersTab_Placed_UI.cpp` (the current five-flat-sections UI).
- `src/ui/DraggableListWidget_UI.h` (the flat-list widget; the tree-widget gap Assembly already
  identified, and the likely shared-widget candidate for this brief).

## Who to consult

SanGen UI Expert first (the generic-mechanism shape and Markers tab UI — the bulk of this brief).
Loop the ARCH Expert immediately given the scope — the genericity architecture call (shared table
vs. per-domain, shared widget vs. two widgets) is squarely ARCH's to make. Loop the Generator
Expert if the group-move + drag-and-follow-symmetry interaction (item 5) turns out to need PROC-
side input beyond what STEP94's existing machinery already handles. Loop the Format Expert again
once the genericized storage shape needs re-confirming against the new domain-discriminated
requirement (their prior research, above, assumed Markers-only).

## Response style (carry forward)

Terse, ❓ for questions, ⚠️ for problems, no narration.
