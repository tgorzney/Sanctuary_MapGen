# Design Brief — Markers Tab Link Mechanic Correction, Round 1

*For a dedicated design conversation with the SanGen UI Expert (and an ARCH Expert consult on the
`MarkerTransform::linkIdentifier` reversal below). Read `CLAUDE.md` first. DESIGN phase only — no
code. Output: a design doc (`DESIGN_MarkerLinkCorrection_R1.md`) naming every PARAMS/ARCH decision
needed, plus whatever work-orders are already unblocked.*

## Where this comes from

2026-08-31 direct human feedback, post-STEP239-242 (the shipped Link mechanic, ratified by
`ARCH_19_28`/`ARCH_19_29`/`ARCH_19_31`/`ARCH_19_32`, designed in `DESIGN_MarkerLink_R1.md` §3): "Links
need to be revamped and were not done correctly." Four numbered asks, human's own wording preserved:

1. "There should be a separate 'Links' Section after the Global Section — this section is where
   link information is displayed."
2. "There would be a group for each Marker Type, then under each group/type, would be the symmetry
   group/instances of that type, in that link."
3. "We need to change so that new groups/layers are not created in the Marker Sections, they need to
   maintain their layering and grouping — I think this may have been done correct as requested, but
   needs changing — but the instances still need to be 'controlled' by the link instance — so if
   user clicks instance in link OR the instance in the actual marker type section, it shows it
   selected in all parts/sections (links and actual marker type section)."
4. "If ALL markers selected are currently already linked, we need to skip making a new link and
   moving instances to new link, just do nothing instead." — clarified on follow-up: broader than
   "ALL": **if ANY selected instance is already in ANY link, "+Link" does nothing** (no new link
   minted, no instances moved) — the human's own reasoning: proceeding anyway "would then break the
   old link unknowingly."

## Already confirmed this session — read as ground truth, do not re-derive

A full read-only pass (SanGen UI Expert) against the live code and the ratified ARCH/design history
already answered most of the "what's really going on" questions. Summarized findings:

**Point 1 — section placement.** `DrawMarkerLinksSection` is currently the LAST statement in
`DrawMarkersTab` (`MarkersTab_UI.cpp:490`), after all three Type-sections, not right after
`DrawMarkersTabGlobals` (`MarkersTab_UI.cpp:282`). Moving the call site is structurally safe —
`MarkerLinksState_UI::sectionStateByLinkIdentifier` is keyed by `link.identifier` (stable, not by
draw order) and the Type-sections' own collapse state is keyed by string type name. One real wrinkle
to carry into the design: the "+Link" button lives inside each Type-section's body, which runs
AFTER the relocated Links section — minting a Link there won't show in the Links section until the
following frame (one-frame cosmetic lag, same class every other immediate-mode "click, see it next
frame" case already tolerates, but state explicitly rather than leave implicit; confirm whether
"+Link" should move to live at/above the Links section itself instead).

**Also found, independent defect worth folding into this same ticket**: `DrawMarkerLinksSection`
never calls `NotifyPlacementChange`/anything dirty-flagging on a Link's own header-extra edits
(color/hidden/lock/grid/symmetry toggles) — `bAnyCommitted` is computed and dropped on the floor.
Toggling a Link's settings today never trips a regenerate. Fix in the same pass.

**Point 2 — hierarchical body.** `DrawMarkerLinkSummaryBody` (`MarkersTab_LinksHeaderExtras_UI.cpp`)
is a plain read-only `ImGui::BulletText("%s: %d", type, count)` per bound Bundle today — no rows, no
click handling, no symmetry-cluster breakdown. Reusable, verbatim, for the real hierarchical body:
- `DrawSymmetryClusterInstanceList<Item>` (`SymmetryClusterInstanceList_UI.h`) — the exact
  "collapsible Symmetry Group N (k)" clustering `DrawBaseSectionManualInstanceList` already uses
  (`MarkersTab_UI.cpp:220-229`), not tied to the Type-section tree.
- `DrawManualInstanceRow` + `ManualInstanceRowInteractionContext_UI`
  (`MarkersTab_ManualLayerRowBody_UI.h:34-36`, `MarkersTab_ManualInstanceSelection_UI.h:43-62`) — the
  actual per-instance Selectable/click-handling row body.
- `PartitionSelectedManualInstancesByType` (`MarkersTab_ManualInstanceSelection_UI.h`) — already the
  "bucket by canonical type name" helper.

**Point 3 — the mint/move mechanism, and why it can't just be deleted outright.** Confirmed:
`ApplyAddLinkAction` (`MarkersTab_Links_UI.cpp:24-59`) mints a NEW root `MarkerLayerBundle` +
`MarkerInstanceLayer` per represented type and calls `ReassignManualInstanceLayers` to MOVE the
selected instances onto it — exactly the restructuring the human no longer wants.

**The real tension, not to be papered over:** the already-ratified master/slave cascade
(`ARCH_19_31`; 7 governed fields: name, color-override+color, hidden, iconScale, gridSnap pair,
symmetry pair, locked) resolves per-Layer, and is only sound because a Link today owns an
EXCLUSIVE, freshly-minted Layer (guaranteeing "the Layer's field == the Link's field" can never
disagree with any individual instance on it). `MarkerTransform` (`MarkerInstance_PARAMS.h`) has no
per-instance field for any of the 7 governed settings — they live exactly once per
`MarkerInstanceLayer`, applied uniformly to every instance on it, read by `layerIndex` everywhere
(canvas draw, preview compositor, `ResolveEffectiveMarkerSymmetry`, every lock-gate predicate). If
instances stay on their real, pre-existing (non-exclusive, possibly-mixed) Layers, there is no
existing place to resolve a per-instance Link override from — cascading onto the whole shared Layer
would wrongly repaint/hide/lock unrelated markers sharing that same Layer, a real regression.

**RULED by the human, this session, following that tradeoff being laid out explicitly:**
**Add `linkIdentifier` directly to `MarkerTransform`** (today it only lives on
`MarkerLayerBundle`/`MarkerInstanceLayer`, one tier up). Every governed-field resolver
(`MarkersTab_MarkerLinkResolvers_UI.h` and every consumer of the 7 governed settings) checks the
INSTANCE's own `linkIdentifier` first; falls back to its Layer's existing resolution when the
instance itself isn't tagged. This is a real, explicit reversal of one line in
`ARCH_19_29_LinkIdentifierBackReferences.md` — *"Neither field is added to `MarkerRuleLayer` or
`MarkerTransform`... a Link never needs to reach past the Layer down to the raw transform for
anything this ticket requires"* — which was true only under the old Link-owns-an-exclusive-Layer
design; it no longer holds under this correction. **The ARCH Expert must write this reversal
formally** (an amendment to `ARCH_19_29`, cross-referenced from `ARCH_19_31`), not have it inferred
by a coder. Flag every consumer site that currently resolves the 7 governed fields by `layerIndex`
alone — each needs the new "check the transform's own `linkIdentifier` first" step ahead of its
existing per-Layer resolution.

**Consequence for `ApplyAddLinkAction`**: no new `MarkerLayerBundle`/`MarkerInstanceLayer` is minted
by "+Link" at all under this correction. "+Link" becomes: mint the `Params::MarkerLink` entry, then
write `linkIdentifier` directly onto every selected instance's `MarkerTransform` — no
`ReassignManualInstanceLayers` call, no Bundle/Layer creation, existing layering/grouping in the
Marker-Type sections is completely untouched. Delete-Link semantics (`DeleteMarkerLink`) need the
matching update: clear `linkIdentifier` on the affected `MarkerTransform`s (not walk
Bundles/Layers — there won't be any Link-tagged ones left to walk under this correction, since none
are minted going forward; **pre-existing data from the old mechanism, if any survives on disk,
still needs its old Bundle/Layer-tag clearing path kept for backward-compat unless the human
confirms no such data needs to round-trip** — flag for the design pass, don't silently drop).

**Selection-sync mechanism** (point 3's "select in one place, highlights everywhere," confirmed
already free): `state.selectedManualInstanceIdentifier(s)` / `manualInstanceSelectionAnchorIdentifier`
are the single shared state fields every manual-instance row already writes through via
`ManualInstanceRowInteractionContext_UI` + `DrawManualInstanceRow`, round-tripping through
`selectManualMarkerInstanceCallback` <-> `canvas.SyncManualMarkerSelection` <->
`SetSelectionChangedCallback` (`Application_UI.cpp`). Any row anywhere in the tab that goes through
this SAME plumbing against these SAME three state fields gets canvas <-> list two-way sync for free
— no new selection mechanism needed. `DrawMarkerLinksSection`'s current signature
(`MarkersTab_Links_UI.h:167`) does not yet receive any of these three fields or the callback — a
signature-widening gap, not a design problem.

## Rulings from this session — apply directly, do not re-open

1. **Cascade mechanism**: tag the instance itself. Add `linkIdentifier` to `MarkerTransform`;
   resolvers check the instance's own tag first, fall back to Layer-tier resolution as today. (See
   "RULED by the human" above.)
2. **No-op guard for "+Link"**: if ANY selected instance already belongs to ANY existing link
   (`linkIdentifier >= 0` under whatever membership model lands, post point-3 correction), do
   nothing — no new Link, no tagging, full stop. Reasoning: proceeding would silently break the
   instance's existing link membership.
3. **Per-Marker-Type grouping inside a Link's body (point 2)**: a plain static label/divider per
   type (e.g. "Alloy" as a header line), NOT a real collapsible `Section` widget with its own
   settings/collapse state — avoids reopening a second "whose settings win" question one tier
   deeper. Matches how the existing base "Instances" list already reads
   (`DrawBaseSectionManualInstanceList`).
4. **Row interaction inside the Links body**: full selection behavior — Ctrl/Shift multi-select,
   drag, everything the Marker-Type section's own instance rows already support — via the SAME
   shared `DrawManualInstanceRow`/`ManualInstanceRowInteractionContext_UI` plumbing, not a
   simplified single-click-only variant.

## What this brief needs designed

1. Concrete `MarkerTransform::linkIdentifier` field placement/naming/casing (mirrors the existing
   `linkIdentifier` convention on `MarkerLayerBundle`/`MarkerInstanceLayer` — same spelling, same
   `-1` sentinel), and the wire-shape addition (mirrors `ARCH_19_29`'s existing
   `LinkIdentifier`-on-`MarkerGroups[i].Transforms[i]`-shaped addition — Format Expert should
   confirm the exact JSON path, since `MarkerTransform` entries live nested under `MarkerGroups[i]`
   on the wire, not top-level).
2. The precise resolver contract change: every one of the 7 governed-field resolvers in
   `MarkersTab_MarkerLinkResolvers_UI.h` (and every non-UI consumer reading those 7 fields by
   `layerIndex` — canvas draw, preview compositor, `ResolveEffectiveMarkerSymmetry`, lock-gate
   predicates for hit-test/drag/marquee) needs an explicit "resolve from the transform's own
   `linkIdentifier` first, else fall back to the Layer's existing resolution" ordering, named
   per-site, not left to a coder's inference. Flag which of these are UI-tier only vs. touch
   PIPELINE/PROC consumers (likely needs Compute/Generator Expert sign-off on any non-UI site).
3. The corrected `ApplyAddLinkAction`/`DeleteMarkerLink`/no-op-guard shapes described above,
   including the backward-compat question for any already-shipped `.sanmap` data carrying the old
   Bundle/Layer-tag Link shape.
4. `DrawMarkerLinksSection`'s widened signature (selection state + callback) and its new
   hierarchical-body composition (ruling 3/4 above): per-type label, then
   `DrawSymmetryClusterInstanceList` + `DrawManualInstanceRow` over that type's Link-tagged
   instances.
5. The relocated call-site (point 1) and the one-frame-lag / "+Link" button placement question
   flagged above.
6. The independent `NotifyPlacementChange` wiring gap found in `DrawMarkerLinksSection` — fold into
   this ticket, don't split into a separate one.

## Specs and files to read first

- This brief's "already confirmed"/"rulings" sections above (don't re-derive).
- `ARCH_19_28_MarkerLinkParamsType.md`, `ARCH_19_29_LinkIdentifierBackReferences.md`,
  `ARCH_19_31_PropagatedPropertyMechanisms.md`, `ARCH_19_32_MarkerSelectedScaleFields.md`.
- `work_orders/DESIGN_MarkerLink_R1.md` §0, §3 in full (the mechanism being corrected).
- `src/ui/MarkersTab_Links_UI.h`/`.cpp`, `MarkersTab_LinksHeaderExtras_UI.cpp`,
  `MarkersTab_MarkerLinkResolvers_UI.h`.
- `src/params/MarkerLink_PARAMS.h`, `MarkerLayerBundle_PARAMS.h`, `MarkerInstance_PARAMS.h`.
- `src/ui/MarkersTab_ManualInstanceSelection_UI.h`/`.cpp`, `MarkerSelectionHighlight_UI.h`/`.cpp`,
  `SymmetryClusterInstanceList_UI.h`.
- `src/io/MapExporter_MarkerLink_IO.cpp`/`.h`, `MapImporter_MarkerLink_IO.cpp`/`.h`,
  `MapExporter_Markers_IO.cpp`, `MapImporter_MarkerGroups_IO.cpp`,
  `MapImporter_MarkerLayerBundle_IO.cpp` (the existing `LinkIdentifier` wire wiring to extend).

## Who to consult

ARCH Expert first and formally — this brief's core change (`MarkerTransform::linkIdentifier`)
directly reverses a named, ratified sentence in `ARCH_19_29`, and the whole "+Link no longer mints a
Bundle/Layer" mechanism reopens `ARCH_19_31`'s master/slave text. Needs a real ARCH amendment, not an
advisory aside inside the design doc this time — this correction should not proceed to STEP tickets
until the ARCH Expert has written the amendment. SanGen UI Expert for the resolver-contract/body
composition/signature-widening design. Loop the Format Expert for the wire-shape confirmation (item
1 above) and the IO Architecture Expert for which existing IO file(s) the new field lands in
(mirrors the existing `LinkIdentifier` file-home split in `DESIGN_MarkerLink_R1.md` §3.8). Loop the
Compute Optimization Expert or Generator Expert only if item 2's consumer-site audit finds a
non-UI/PIPELINE/PROC read site — flag, don't assume.

## Response style (carry forward)

Terse, ❓ for questions, ⚠️ for problems, no narration.
