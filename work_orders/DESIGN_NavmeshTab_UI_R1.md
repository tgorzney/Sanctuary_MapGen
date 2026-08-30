# DESIGN — Navmesh Tab UI (R1)

*Authored by the SanGen UI Expert, 2026-08-30. **Design only — no code, no work-order.** Read-only
against `src/**`. Does not write `ARCH.md` or any `ARCH_NN_*.md` — see the ⚠️ ARCH-flag section
instead.*

*Grounded against: `sangen_arch_pack/CONSTITUTION.md`, `sangen_arch_pack/specs/NAVMAP_MODIFIER_BLOCKER_SPEC.md`
§1/§2/§7/§8, `sangen_arch_pack/specs/UI_FRAMEWORK_SPEC.md`, `sangen_arch_pack/INDEX.md`, and a direct
read this session of: `ARCH_22_NavmapModifierBlockers.md` + `ARCH_22_09_OwnershipScopeRuling.md`;
`ARCH_19_MarkerLayerBundle.md` + `ARCH_19_02_GenericitySplit.md` + `ARCH_19_07_TreeListWidgetOwnership.md`;
`ARCH_20_PropsDecalsAuthoringParity.md` + `ARCH_20_01_ParamsGenericitySplit.md`;
`ARCH_21_03_DragGestureGenericization.md` + `ARCH_21_08_AreaCanvasGesture.md`;
`ARCH_14_19_AreaZOrderInversionAndImportSizeSort.md`;
`src/ui/{AreasTab_List_UI.h,AreaDragGesture_UI.h,AreaDragGesture_UI.cpp}` (read fresh this session,
post-STEP227, confirmed to already contain `Params::InsertMapAreaSortedBySize`/`IsAreaLocked` — not a
stale copy); `src/ui/{MarkersTab_UI.h,MarkersTab_TypeSections_UI.h,MarkersTab_Bundles_UI.h,
MarkersTab_ManualLayers_UI.h,MarkersTab_ManualLayerRowBody_UI.h,MarkersTab_ManualLayerHelpers_UI.h,
TreeListWidget_UI.h,DraggableListWidget_UI.h,PlacementRuleSections_UI.h}`;
`src/params/{MarkerInstance_PARAMS.h,MarkerLayerBundle_PARAMS.h,Symmetry_PARAMS.h,Water_PARAMS.h,
MapArea_PARAMS.h(via AreasTab_List_UI.h)}`; `src/ui/Application_Panels_UI.h` (`ApplicationPanel` enum).*

*Format template: `work_orders/DESIGN_SantpFootprintIngestion_R1.md`.*

**Note on delivery.** This sub-agent invocation has no file-write tool available (Read/Grep/Glob
only). The content below is the complete document; whoever dispatched this task must persist it at
`work_orders/DESIGN_NavmeshTab_UI_R1.md` themselves.

---

## 0. Why this document exists

`ARCH_22_09_OwnershipScopeRuling.md` §22.9 is explicit: the navmap-modifier-blocker technique
`NAVMAP_MODIFIER_BLOCKER_SPEC.md` documents is, today, **"not backed by any `Params::`/`IO`/`UI`
type, and this ratification does not create one."** It also says a coder "should [not] build toward
it without a real, separately-scoped design consult first." This document is that consult's UI half:
it designs the tab's structure and interaction model, proposes (but does not ratify) the PARAMS shape
the UI needs to exist, and hands the PARAMS/IO/Generator side to their respective Experts.

The human's brief asks three linked questions this document answers directly, with evidence:
1. Should the Navmesh Tab mirror Markers' Section→Group→Layer structure, simplified to manual-only?
   **Yes — see §5.** Most of the mechanism is already generic and reusable as-is.
2. Should it reuse the Areas rectangle-editing interface verbatim? **Yes for the canvas gesture's
   shape and behavior; the underlying code is a ported sibling, not a shared call, for reasons
   specific to this codebase's own genericity law — see §6.**
3. Does SanGen already have a universal sectioned group/layer widget, or does one need building?
   **It already exists, and ARCH already pre-authorized Navmesh as its next consumer by name — see
   §1.**

---

## 1. Verified ground truth — the "already universal?" question, answered

**This is not speculative — it is written down, ratified, and current.**
`ARCH_19_02_GenericitySplit.md` §19.2, "Domain-touching-vs-pure-mechanics genericity split," title
line: *"ratified as the general rule for all future Group/Bundle work (**Props, Decals, NavMesh**)."*
Verbatim: *"Pure container/graph/UI mechanics with zero domain-field access — tree render,
expand/collapse, drag-to-reparent, cycle-detection over bare id/parent-id pairs — gets one shared
C++ template or accessor-callback-parameterized function... this is the dividing line future
Props/Decals/**NavMesh** Bundle work-orders apply without re-asking."*

Concretely, two generic widgets already exist and already carry zero domain (`Params::`) coupling in
their own template signature, confirmed by direct read:

- **`TreeListWidget_UI<T, LeafKeyT>`** (`src/ui/TreeListWidget_UI.h`) — the Group/Bundle tree.
  Accessor-lambda-parameterized (`idOf`/`parentIdOf`/`nameOf`/`drawNodeBody`/`describeLeaves`/
  `leafLabel`/`drawExpandedLeafBody`, plus the STEP129 header-extra pair) — "not virtualized on
  purpose — authoring scale (tens of nodes/leaves)," which is the right scale class for a Bundle
  tree (it is explicitly the WRONG scale class for the rectangle-instance list itself — see §9's
  perf flag). Already instantiated once for `Params::MarkerLayerBundle`; `ARCH_20_01_ParamsGenericitySplit.md`
  confirms it needs **"zero new widget-library code"** to add `TreeListWidget_UI<PropLayerBundle,
  PropGroupLeafKey_UI>` — the exact same sentence applies verbatim to a third instantiation,
  `TreeListWidget_UI<Params::NavmeshBlockerLayerBundle, NavmeshBlockerLeafKey_UI>` (§4 below).
- **`DraggableListWidget_UI<T>`** (`src/ui/DraggableListWidget_UI.h`) — the reorderable layer stack
  inside one Group/Section, already the shared mechanism for both Markers' procedural rule stack and
  its manual layer stack (`MarkersTab_ManualLayers_UI.h`'s `DrawLayerList`). Same posture: a new
  `Params::NavmeshBlockerLayer` instantiation is free.

**What is deliberately NOT generic, by explicit ARCH ruling, and must not be generalized:** the
domain struct itself (`MarkerLayerBundle`/`PropLayerBundle`/`DecalLayerBundle`, each "independently
written, not templated, not sharing a base class, not discriminated by a `domain` enum inside one
shared table" — §19.2) and the domain-touching pure-function families that read a real `Params::`
field (`ResolvePropInstanceLayerId`/`ResolveDecalInstanceLayerId`, the per-domain
`WouldReparent<Domain>LayerBundleCreateCycle` predicates, §20.1). §19.2's own reasoning: *"A single
mixed-domain `Groups: [{id, name, parentGroupId, domain}]` table... breaks the established 'array
order is the layer's identity' convention... genericity lives in the mechanism..., never in the
data shape."*

**Verdict for this design:** no new widget-library work is needed or recommended for the Group/Layer
tree tier. Navmesh gets its own hand-written `Params::NavmeshBlockerLayerBundle` /
`Params::NavmeshBlockerLayer` structs (per §19.2's law, exactly as Props/Decals got their own), and
reuses `TreeListWidget_UI`/`DraggableListWidget_UI` as pure mechanism, exactly as designed. This
satisfies the human's "proven twice" framing already — Markers is one proof, Props/Decals (§20) is
the second, and ARCH's own §19.2 text named Navmesh as the anticipated third **before this document
was written**. There is nothing left to promote on this axis. (§6 below identifies one *different*
axis — the rectangle drag-gesture algorithm — where the "proven twice" bar is newly met by this
design and *is* worth flagging to ARCH.)

---

## 2. Scope boundary

**IN SCOPE:** the Navmesh Tab's layout (Type-section → Group/Bundle → Layer → rectangle-instance
list), the water-presence gate, the manual-only Group/Layer authoring model, the canvas
select/move/resize interaction for a rectangle, and the minimal provenance representation.

**OUT OF SCOPE, explicitly:**
- **No procedural generation settings anywhere in this tab.** No rule stack, no gate/transform
  sections, no symmetry-orbit *generation* — see §5.3 for why symmetry specifically does not belong
  on a rectangle layer at all, a stronger claim than "not built yet."
- **No rotation support**, anywhere in the data model or the canvas gesture — `NavmapModifierTemplate.size`
  is a plain axis-aligned `float2` (`NAVMAP_MODIFIER_BLOCKER_SPEC.md` §1); rectangle editing is
  move + resize on 2 axes only, mirroring `AreaDragGesture_UI`'s own 8-handle set (no rotate handle
  exists there either).
- **This tab does not run the mesh-intersection pipeline.** That is a separate, not-yet-designed
  Generator-Expert-owned feature (`NAVMAP_MODIFIER_BLOCKER_SPEC.md` §7.1, `PLACEMENT_SCATTER_SPEC.md`/
  `MASKING_SPEC.md`'s own forward-pointers to it). This tab only displays/edits the resulting
  rectangles plus whatever the user hand-adds directly — the same "consumer of a resolved instance
  list, not a generator" posture `MarkersTab_UI.h` already holds for `Data::PlacementInstances`.
- **No `.sanmap`/Lua export mechanism is designed here.** How a `NavmeshBlockerLayer`/rectangle list
  becomes the per-map `<MapName>_data.lua` Technique-B helper (`NAVMAP_MODIFIER_BLOCKER_SPEC.md` §4)
  is an IO/Format-Expert-owned surface this document does not touch, beyond noting in §4 that the
  PARAMS shape proposed there is meant to be exportable.
- **No PARAMS/IO/PROC type is ratified here.** §4 proposes a shape *for the UI's own use*, explicitly
  flagged non-binding, per ARCH §22.9's "no coder should build toward it without a real,
  separately-scoped design consult" — this document is one input to that consult, not the consult's
  conclusion.

---

## 3. The gating fact: `Params::Water::bEnabled` is SanGen's own `Engine.HasWater()` mirror

`NAVMAP_MODIFIER_BLOCKER_SPEC.md` §2: *"Land, Amphibious, Hover, Air always; Submarine and Sea only
if `Engine.HasWater()` is true."* Confirmed by direct read, `src/params/Water_PARAMS.h` already
carries exactly the field this gate needs: `Params::Water::bEnabled` (default `false`). No new
PARAMS field is needed for the gate itself — the Navmesh Tab reads `recipe.water.bEnabled` the same
way any other water-conditioned control would, and hides (does not merely gray) the Sea/Submarine
sections when it is false, per the task's own instruction. "Hide, don't gray" also avoids the tab
implying a designer *could* configure a nav layer the engine will never create on this map — a real
correctness distinction, not styling.

---

## 4. The PARAMS shape this design assumes — proposed, not ratified

No `Params::` type for a navmesh blocker exists in `src/` today (confirmed, ARCH §22.9). The shape
below is what the UI design in §5-§7 requires; it is offered to the PARAMS/Generator/Format Experts
as a starting proposal, not asserted as decided. Two structural findings below are genuinely
load-bearing and should survive whatever the eventual ratified shape turns out to be, because they
come from the spec's own geometry, not from UI taste:

**4.1 — the six layers are a closed engine-defined enum, not a free-form string like `markerTypeName`.**
`markerTypeName` (Markers/Props §19.13/§20.6) is deliberately open text because a designer can invent
any Bundle "type" they like. A nav layer is the opposite: exactly six named values exist
(`NAVMAP_MODIFIER_BLOCKER_SPEC.md` §2), the engine creates them by fixed name, and two of the six
are conditionally absent. Proposed: `enum class Params::NavLayerKind { Land, Amphibious, Hover, Air,
Sea, Submarine }`, not a string field. This is a deliberate divergence from `ARCH_19_14_TypeSectionUiDerived.md`'s
"no `Params::MarkerTypeSection` struct — dynamic enumeration over an open string space" ruling — that
ruling's own reasoning (open, designer-extensible string space) does not hold for Navmesh, whose type
set is closed engine ground truth. §5.1 below states the corresponding Type-section-tier consequence.

**4.2 — coordinate convention: mirror `MapArea`'s origin+extent shape internally; convert to the
engine's center+size shape only at the export boundary.** `NAVMAP_MODIFIER_BLOCKER_SPEC.md` §1
states the engine's own runtime shape is **center**-anchored (`float2 size`, "centered on wherever
its owning prefab instance is placed"). `Params::MapArea` (`AreasTab_List_UI.h`,
`AreaDragGesture_UI.h`) is **origin**-anchored (`originX`/`originZ` = min corner, `width`/`length` =
extent growing from there). These are two different parameterizations of the same shape, silently
convertible (`origin = center - size/2`) but **not interchangeable inside a ported algorithm** — a
naive port of `AreaDragGesture_UI`'s math onto a center-anchored struct would silently double every
resize delta on axes it doesn't expect. Recommend `Params::NavmeshBlockerRectangle` use the SAME
origin+extent field shape as `MapArea` (`{originX, originZ, sizeX, sizeZ}` or literally reuse
`width`/`length` naming) specifically so §6's algorithm port is a mechanical field-rename, not a
math rewrite, and so `AreasTab_List_UI.h`-style helpers (`IsWorldPointInsideArea`-equivalent,
"Set to Map Size"-equivalent) port the same way. The origin→center conversion becomes the
Lua-export step's problem, one line, at the one place it's needed — the same "convert at the
boundary, never mid-pipeline" posture `NAVMAP_MODIFIER_BLOCKER_SPEC.md` §8 itself models for its own
two distinct pixel/world coordinate conventions.

**Proposed sketch** (PARAMS/Generator/Format Experts' call on every field name and every wire key —
this is a UI-consumer's shape request, not a ratified struct):

```cpp
enum class NavLayerKind { Land, Amphibious, Hover, Air, Sea, Submarine };

enum class NavmeshBlockerProvenance { HandPlaced, MeshGenerated };  // §8 — read-only label only

struct NavmeshBlockerRectangle {
    float originX = 0.0f, originZ = 0.0f;   // min corner — mirrors MapArea, NOT the engine's center convention (§4.2)
    float sizeX   = 1.0f, sizeZ   = 1.0f;
    int   layerIndex   = -1;                // indexes navmeshBlockerLayers, mirrors MarkerTransform::layerIndex
    int   instanceIdentifier = -1;          // stable UI-selection address, mirrors MarkerTransform::instanceIdentifier (ARCH §19.16)
    NavmeshBlockerProvenance provenance = NavmeshBlockerProvenance::HandPlaced;  // read-only in UI, §8
};

struct NavmeshBlockerLayer {
    std::string name;
    float color[4] = {1,1,1,1};
    bool  bColorOverrideEnabled = false;    // mirrors MarkerInstanceLayer
    bool  bLocked = false;
    bool  bHidden = false;
    bool  bGridSnapEnabled = false;
    float gridSnapSizeWorldUnits = 1.0f;
    int   layerIdentifier = -1;             // spelled in full per ARCH §1.9 — NOT "layerId"
    int   parentBundleIdentifier = -1;
    NavLayerKind navLayerKind = NavLayerKind::Land;   // replaces markerTypeName — closed enum, §4.1
    // Deliberately NO `Params::SymmetrySetting symmetry` field and NO `bSymmetryEnabled` — §5.3.
    // Deliberately NO `iconScale` — rectangles have no icon.
};

struct NavmeshBlockerLayerBundle {   // mirrors MarkerLayerBundle field-for-field
    int identifier = -1;
    std::string name;
    int parentBundleIdentifier = -1;
    NavLayerKind navLayerKind = NavLayerKind::Land;   // replaces markerTypeName
    int assemblyIdentifier = -1;
};
```

Whether these live as one shared `recipe.navmeshBlockers` vector tagged by `navLayerKind` (mirroring
how `MarkerLayerBundle` carries `markerTypeName` and the tab builds a per-section *filtered copy*,
`ARCH_19_15_TypeSectionTreeComposition.md`) or as six parallel per-layer-kind vectors is a PARAMS
question, not a UI one — this design's own tab structure (§5) works identically either way, since it
only ever needs a filtered/partitioned view per section regardless of the underlying storage. The
filtered-copy convention is recommended as the default, purely for consistency with the proven
Markers/Props/Decals precedent (`ARCH_19_15`) rather than inventing a fourth storage shape.

---

## 5. Tab structure: Section → Group → Layer → Rectangle instances

**5.1 — Top level: one Section per `NavLayerKind`, water-gated, fixed order, not dynamically
enumerated.** Unlike `EnumerateMarkerTypeSectionNames` (`MarkersTab_TypeSections_UI.h`), which walks
live data to find which type strings are *present*, the Navmesh Tab's outer loop is a fixed,
compile-time list of exactly six entries in spec order (Land, Amphibious, Hover, Air, Sea,
Submarine), because the set itself is engine ground truth, not something a designer can add to or
that "isn't present yet." Sea and Submarine's sections are entirely absent (not drawn, not a grayed
placeholder) whenever `!recipe.water.bEnabled` (§3) — re-evaluated live every frame, so toggling
Water on the Water tab immediately reveals/hides them, mirroring how any other cross-tab-derived
gate in this codebase degrades (Constitution §6, "no dead controls").

**5.2 — Within each Type-section: a `TreeListWidget_UI<NavmeshBlockerLayerBundle, int>` Group tree**
(mirroring `DrawMarkerLayerBundleTree`), whose leaves are `NavmeshBlockerLayer` entries. Because
Navmesh has **only one leaf kind** (there is no procedural-rule-layer sibling the way Markers'
`MarkerGroupLeafKey_UI::Kind{Procedural, Manual}` needs to discriminate), the leaf key collapses to a
**plain `int layerIndex`** — no discriminated-union leaf key struct is needed at all. This is a
genuine, spec-driven simplification over Markers, not merely "fewer fields": Markers' `Kind` enum
exists solely to let one tree host two structurally different leaf types side by side, and Navmesh
never has two leaf types to host. `DraggableListWidget_UI<NavmeshBlockerLayer>` supplies the same
Group's own flat "ungrouped layers at this Section's root" list, mirroring
`DrawManualMarkerLayerListBody`.

**5.3 — Layer row content: name, color/tint override, lock, hide, grid snap — NO symmetry section,
by deliberate design, not oversight.** `MarkersTab_ManualLayerRowBody_UI.h`'s `DrawLayerRowBody`
draws name/tint/**icon scale**/grid snap/**symmetry**/instance-list. Two of those five do not carry
over to a rectangle:
- **Icon scale drops** — a rectangle has no icon; its own `sizeX`/`sizeZ` fields (edited via the
  canvas, §6, or a compact numeric field in the row) are its visual footprint, already.
- **Symmetry drops — for a real geometric reason, not simplification-for-its-own-sake.** `MapArea`
  (also an axis-aligned rectangle, also no rotation) already carries **zero** symmetry field
  (`symmetryGroupIdentifier` and friends are absent, confirmed `ARCH_21_08` correction 1) — this is
  existing, proven precedent for exactly this shape, not a gap Areas has yet to fill. The reason
  generalizes: `Params::SymmetryAxis::QuarterTurns` (90°) applied to an axis-aligned rectangle
  produces a congruent rectangle **only if `sizeX`/`sizeZ` are swapped** for the rotated clone — a
  real transform the existing `MarkerRuleLayer`/`MarkerInstanceLayer` `SymmetrySetting` machinery
  was never built to do (it orbits *positions*, not oriented extents). `Params::SymmetryAxis::Radial`
  is strictly worse: an arbitrary N-fold rotation (e.g. 120° for N=3) produces a rectangle that is
  **not axis-aligned at all**, which `NavmapModifierTemplate` (§1 of the spec) cannot represent —
  the orbit clone would be geometrically impossible to author, not merely unimplemented.
  `MirrorAcrossX`/`MirrorAcrossZ`/`RotateHalfTurn` (180°) all stay geometrically valid (they preserve
  axis-alignment and don't swap extents), but shipping "symmetry, except two of five axes are
  silently illegal" is a worse designer experience than shipping none, matching what Areas already
  chose. **Recommendation: no `SymmetrySetting` field on `NavmeshBlockerLayer` at all**, reusing
  Areas' own precedent rather than Markers'. If a future ticket wants mirror-only (X/Z/180°)
  symmetry for blocker layers, that is a real, separately-scoped follow-up requiring new orbit math
  (extent-swap-aware for quarter-turns, Radial refused outright) — not something this design should
  gesture at by including a field the row can't correctly act on.

**5.4 — Selection at the tree/list tier allows Ctrl/Shift multi-select on rectangle-instance rows**
(mirroring `MarkersTab_ManualInstanceSelection_UI.h`'s STEP141 mechanism, reused verbatim), because
`NAVMAP_MODIFIER_BLOCKER_SPEC.md` §7's own mask-to-rectangle workflow numbers (**88 to 815
rectangles from one mask component**) make bulk select/delete/reparent/re-layer a realistic, not
theoretical, authoring need for an *ingested* batch — a scale Markers' own hand-authored rosters
rarely approach. This is a tree-tier decision only; §6 makes the opposite call for the canvas.

---

## 6. Canvas rectangle interaction — reusing Areas, precisely stated

**What "reuse the exact same interface as Areas" means, mechanically.** `ARCH_21_08_AreaCanvasGesture.md`
correction 2 (confirmed by fresh read of `AreaDragGesture_UI.h`/`.cpp` this session, both current
post-STEP227) is explicit that `AreaDragGesture_UI` is **not** a `Traits`-templated instantiation of
`InstanceDragGesture_UI<Traits>` (§21.3's generic mechanism for Markers/Props/Decals point
instances) — it is a **standalone, hand-written, non-template algorithm**, because `MapArea` has no
Group/Transform two-level shape for that template to wrap. `Params::NavmeshBlockerRectangle` (§4)
has the same flat-vector shape (no group/transform indirection at the rectangle level — only a
`layerIndex` tag). Therefore the reuse mechanism this design specifies is:

**A new, structurally-parallel sibling substrate — `NavmeshBlockerDragGesture_UI.h`/`.cpp` — ported
from `AreaDragGesture_UI.h`/`.cpp` field-for-field**, not a shared template and not a runtime call
into Areas' own code:
- The same `enum class ...Handle_UI { None, N, NE, E, SE, S, SW, W, NW, Center }` (8 handles + body,
  no rotate).
- The same `ComputeXHandleWorldPoints`/`HitTestXHandles` fixed N/NE/E/SE/S/SW/W/NW/Center
  priority-order screen-space hit-test, at the same `kAreaHandleScreenRadiusPixels`-equivalent
  named constant (Constitution §8 — its own named constant, not a literal copy of Areas' value,
  even if it starts equal).
- The same Ctrl-doubles-from-center / Shift-locks-aspect-ratio / per-axis-floor resize math
  (`UpdateAreaDragGesture`'s body, lines 74-140 of the current file), which — given §4.2's origin+
  extent field-shape recommendation — is a mechanical field-rename of `originX/originZ/width/length`
  to whatever `NavmeshBlockerRectangle` ultimately calls them, not a re-derivation.
- The same live-write-every-frame / no-materialize-on-release posture (`AreaDragGesture_UI.h`'s own
  header comment: "Areas have no materialize/cascade-delete step... every field write already landed
  live during Update").
- A new sibling `MapCanvas_NavmeshBlockerDragDispatch_UI.cpp` / `MapCanvas_NavmeshBlockerDraw_UI.cpp`
  pair, mirroring `MapCanvas_AreaDragDispatch_UI.cpp`/`MapCanvas_AreaDraw_UI.cpp` file-for-file: its
  own `NavmeshBlockerGestureEligible()` gate (keyed to a new `ApplicationPanel::Navmesh` panel entry
  — `Application_Panels_UI.h`'s `ApplicationPanel` enum has no such entry today; adding one is an
  additive, mechanical change), its own `ManualNavmeshBlockerDragSources_UI` injected-pointer bundle,
  its own create-by-drag (mirroring `CreateAreaFromDrag`), and — per §21.8 ruling 5's own precedent —
  Navmesh gesture-eligibility **pre-empts** the ordinary click/marquee-select fallback exactly the
  way Areas' does, never falls through to `ApplyMarqueeGesture`, and stays outside
  `OverlayInstanceKeySet_UI`/§21.1's multi-select machinery entirely: **canvas selection is a single
  scalar index, exactly Areas' own posture**, independent of §5.4's tree-tier multi-select (the same
  split already exists nowhere else in this codebase only because no other domain has needed it —
  it is not inconsistent, it is a genuinely new combination: bulk operations belong to the list
  (batch scale, hundreds of rows), live geometric editing belongs to the canvas (one shape at a
  time, the same reason Areas never grew marquee-select either).

**Recommendation to ARCH — a real "proven twice" candidate, flagged, not decreed.** `AreaDragGesture_UI`'s
own hit-test/resize/aspect-lock algorithm touches exactly four float fields
(`originX`/`originZ`/`width`/`length`) and nothing else `MapArea`-specific — it is, by its own
authoring history, "pure mechanics with zero domain-field access" in every sense except that its
signature is hardcoded to `Params::MapArea` rather than accessor-lambda-parameterized (the same
distinction `§21.3`'s `HitTestManualInstances<GroupT>` already resolved for position-only duck-typing
across three domains). §21.8 correction 2 was correct **at the time it was written**, when Areas was
the only consumer and genericizing a single-consumer algorithm would have been premature abstraction.
This design is the second real consumer of the *identical* 4-float rectangle shape. **RESOLVED, see
§6 flag 1 above — `ARCH_22_16_RectangleDragGesturePromotion.md` rules the promotion BINDING**: the
resize/aspect-lock/handle-hit-test core is generalized into an accessor-parameterized
`RectangleDragGesture_UI<Accessor>` template that both `AreaDragGesture_UI` and
`NavmeshBlockerDragGesture_UI` become thin instantiations of; Navmesh does NOT hand-port a
byte-identical ~140-line copy of §21.8's math. The paragraph below is left as historical context for
*why* this was flagged, not as a still-open recommendation. It was flagged because declining it here,
with the evidence already in hand, would just relocate the same "should this have
been generic by now?" question the human already raised once for the Bundle tree.

---

## 7. Z-order — addressing the coordinator's follow-up directly

**Recommendation: adopt the SAME `InsertMapAreaSortedBySize`-style convention, but as `Navmesh`'s own
independent per-section sort — never touching `recipe.areas` — with the underlying rationale weighed
explicitly, not silently inherited.** `ARCH_14_19_AreaZOrderInversionAndImportSizeSort.md`'s core
argument (item 1) is that "a small carve-out renders on top of the large area it sits inside" is the
*desirable default* because Areas are named, individually-colored, semantically distinct regions —
misordering actively hides one region's own identity under another's. **This exact argument is
weaker, but not absent, for navmesh blockers**, for a reason worth stating plainly: within one
`NavLayerKind` section, every rectangle represents the same semantic fact ("pathing is blocked here
on this layer") — overlapping rectangles union, they don't compete for which one is "the real"
answer the way two differently-colored, differently-named Areas do. If every rectangle within a
section renders with one flat per-section tint (not a per-rectangle distinguishable color), Z-order
has **zero visual consequence** and matters only for click-hit-test disambiguation (which specific
rectangle a click selects when several overlap) — a real but strictly smaller stake than Areas'.

Given that, this is presented as an **open question (Q4 below)**, not a clear-cut adoption, with a
recommended default:
- **Recommended default: yes, adopt it** — same `InsertMapAreaSortedBySize`-shaped function
  (mirroring §14.19 item 3's "one insertion function, used everywhere the array grows" law, its own
  new PARAMS-resident function per `Params::` type per §3.5), scoped to whatever vector(s) hold
  `NavmeshBlockerRectangle` (§4's storage-shape question), because it is the "first unlocked hit is
  the topmost, ascending index" click-disambiguation rule that carries real weight even without a
  rendering consequence: a designer editing an ingested batch of hundreds of ~1-unit rectangles
  (§7 of the spec's own numbers) benefits from small rectangles staying clickable-on-top of whatever
  larger rectangle a mask-decomposition pass happened to also cover that spot with, exactly the
  chokepoint-inside-a-big-zone case §14.19 item 1 describes for Areas.
- **Why it is genuinely open, not settled by that argument alone:** unlike Areas (dozens of
  hand-curated, individually-named entries where insertion order is rare and deliberate), a
  Navmesh section populated by an ingestion pipeline could insert hundreds of rectangles in one
  batch — re-deriving sort rank on every single insert (`InsertMapAreaSortedBySize`'s `O(N)` linear
  scan per insert) is `O(N^2)` for a batch import of N rectangles, a cost class Areas never
  approaches (its own numbers are "dozens," never "hundreds"). This may be fine (hundreds squared is
  still small in absolute terms) or may want a single sort-after-batch-insert path instead of
  N individual sorted-inserts — a decision for whoever designs the ingestion pipeline's own IO/PARAMS
  side, flagged here so it isn't silently inherited as "the Areas way, unchanged" without noticing
  the batch-size difference. **This document does not resolve that batch-cost question** — see Q4.

---

## 8. Provenance representation — minimal, read-only, non-gating

Per the task's own instruction to keep this minimal: `NavmeshBlockerRectangle::provenance`
(`enum class NavmeshBlockerProvenance { HandPlaced, MeshGenerated }`, §4) is the entire
representation. In the UI:
- A small read-only label/glyph on the instance row (mirroring how a Manual leaf's row already shows
  static metadata with no interactive control attached — `ManualMarkerLayerRowLabel`'s own posture).
- **No interaction anywhere else special-cases it.** A `MeshGenerated` rectangle drags, resizes,
  deletes, reparents to a different Layer, and locks exactly like a `HandPlaced` one — the human's
  own instruction that "the tab's editing model should not need to special-case origin beyond
  perhaps a read-only provenance label" is honored literally: this field is read by exactly one
  draw call and by nothing else.
- Whether re-running the (out-of-scope) mesh-intersection pipeline should overwrite/merge/leave-alone
  a `MeshGenerated` rectangle a designer has since hand-edited is an ingestion-pipeline design
  question, not a tab-structure one — flagged, not answered, here (see Q3).

---

## 9. Perf flags — routed, not resolved here

Per this Expert's own charter, throughput/batching/picking-at-scale is the UI Optimization Expert's
call, not designed here. Flagged so it is not silently inherited from Markers' current (small-scale)
implementation without re-examination:
- **The per-Layer instance-row list.** `DrawLayerRowBody`'s own instance list (Markers) is a plain
  loop of `Selectable` rows, not `VirtualListWidget_UI`-backed — fine at Markers' authoring scale
  (tens of hand-placed markers per layer). `NAVMAP_MODIFIER_BLOCKER_SPEC.md` §7's own numbers (88 to
  815 rectangles from a *single* mask-decomposition pass, before a designer even starts hand-adding
  more) put a `MeshGenerated`-heavy Navmesh Layer well outside that scale class. Whether this needs
  `VirtualListWidget_UI` from day one, or can wait for a real measured stall, is the UI Optimization
  Expert's call.
- **The canvas hit-test.** `AreaDragGesture_UI`'s own body-hit-test is an `O(N)` forward scan with
  early exit (§14.19 item 2) — correct and cheap at Areas' scale (dozens). A Navmesh section with
  hundreds of ingested rectangles active on-canvas simultaneously is a materially different `N`.
  Whether this needs a spatial index (mirroring `MarkerSpatialGrid`'s own precedent,
  `UI_FRAMEWORK_SPEC.md` item 5) before shipping, or can reuse the plain scan until measured
  otherwise, is likewise routed, not resolved.
- **Draw-pass compositing.** Whether Navmesh blockers should be a composited `PreviewFieldLayer`
  (mirroring `PreviewLayerKind::MapAreas`, `ARCH_14_17_MapAreaFieldLayer.md`) rather than an
  overlay-icon-stack domain (ARCH §14's six-domain overlay list, which this Expert's own charter
  says markers/props/decals never leave) is a real, load-bearing choice this document takes a
  position on (§10 below) but whose GPU-recomposite cost/benefit at scale is the UI Optimization
  Expert's to weigh, not this document's.

---

## 10. One more structural finding: Navmesh blockers belong with `MapAreas`, not with the overlay-icon stack

Stated because it is easy to get wrong by pattern-matching on "Markers/Props/Decals" alone. This
Expert's own charter draws a hard line: markers/armies/props/decals/reclaim "are never baked into
the shared composite texture... they draw screen-space, every frame" (ARCH §14) — but that rule is
about **point-entity icons**, not about filled rectangles. `Params::MapArea` is the existing
precedent for a filled, bordered, handle-editable **rectangle** in this preview, and it lives as a
real composited `PreviewFieldLayer` (`PreviewLayerKind::MapAreas`, `ARCH_14_17_MapAreaFieldLayer.md`),
with only the border/handle *chrome* drawn immediate-mode during an active edit
(`ARCH_21_08`'s 2026-08-29 amendment). A navmesh blocker rectangle is the same rendering shape (a
filled, colored, axis-aligned rectangle, potentially hundreds of them, no per-instance icon, no LOD
concept) — **recommend it follow `MapAreas`' compositing model, not the overlay-icon stack's**: a new
`PreviewLayerKind::NavmeshBlockers` (or one per `NavLayerKind`, TBD by whoever designs the composite
side), steady-state fill from the GPU composite, immediate-mode border+handles only for the actively-
selected/dragged rectangle (mirroring §14.17 item 11's suppressed-index mechanism exactly, including
its "two recomposites per gesture, never one per frame" cost discipline). This is a genuinely
different call than the Markers/Props/Decals precedent, made deliberately and grounded in the
rendering-shape match to Areas rather than the domain-name match to Markers — flagged as its own item
because a less careful design could default to "point instance → overlay stack" by pattern alone and
get it wrong.

---

## 11. Proposed file/ticket breakdown

**Enumeration only, dependency order — none of these tickets is written here.** Highest existing
work-order is STEP227; proposed 228+.

| # | Ticket | Layer | One-line scope |
|---|---|---|---|
| **228** | `NavmeshBlocker_PARAMS` (name TBD by PARAMS Expert) | **PARAMS** | `NavLayerKind`, `NavmeshBlockerRectangle`, `NavmeshBlockerLayer`, `NavmeshBlockerLayerBundle` — §4's proposed shape, ratified/corrected by the PARAMS/Format Experts, not built as proposed here without their sign-off. |
| **229** | `NavmeshBlockerLayerBundle_IO` / import-export | **IO** | `.sanmap` round-trip for the new PARAMS type — new wire keys, Format Expert's call. Independent of the Lua-export question (out of scope, §2). |
| **230** | `RectangleDragGesture_UI<Accessor>` template + `AreaDragGesture_UI`/`NavmeshBlockerDragGesture_UI` instantiations | **UI** | RESOLVED per `ARCH_22_16`: builds the shared accessor-parameterized template (get/set `originX`/`originZ`/`sizeX`/`sizeZ`, each instantiation keeping its own named handle-radius/minimum-extent constant), refactors `AreaDragGesture_UI` into a thin instantiation as part of this same ticket (not left as dead duplicate code), and adds `NavmeshBlockerDragGesture_UI` as the second thin instantiation — mirroring `PropDragGesture_UI`/`DecalDragGesture_UI`'s existing `Traits`-shim-over-`InstanceDragGesture_UI<Traits>` posture (§21.3). The ported-byte-identical-copy fallback the original ticket text described is superseded and must not be built. Blocked on 228. |
| **231** | `MapCanvas_NavmeshBlockerDragDispatch_UI` / `MapCanvas_NavmeshBlockerDraw_UI` | **UI** | The dispatch/eligibility/draw-pass pair mirroring `MapCanvas_AreaDragDispatch_UI`/`MapCanvas_AreaDraw_UI` (§6), including the new `ApplicationPanel::Navmesh` entry. Depends on 230 and on 10's `PreviewLayerKind::NavmeshBlockers` compositing decision landing (or a placeholder immediate-mode-only draw pass if that lands later). |
| **232** | `NavmeshTab_TypeSections_UI` | **UI** | The fixed, water-gated six-section outer loop (§5.1) — the Navmesh-specific analogue of `MarkersTab_TypeSections_UI.h`, deliberately NOT reusing its dynamic-enumeration function (§4.1's closed-enum divergence). Depends on 228. |
| **233** | `NavmeshBlockerLayerBundle_UI` (Group tree) | **UI** | `TreeListWidget_UI<NavmeshBlockerLayerBundle, int>` instantiation + node body (rename/delete/reparent), mirroring `MarkersTab_Bundles_UI.h` minus its `Kind`-discriminated leaf key (§5.2). Depends on 228, 232. |
| **234** | `NavmeshBlockerLayer_UI` (Layer list + row body) | **UI** | `DraggableListWidget_UI<NavmeshBlockerLayer>` instantiation, row body (name/tint/lock/hide/grid-snap, NO symmetry section per §5.3), instance-row list with §5.4's Ctrl/Shift multi-select. Depends on 228, 232, 233. |
| **235** | Perf pass (VirtualList / spatial index, per §9) | **UI-Optimization** | Not scoped here — routed. Depends on real measurement against an ingested batch, per this Expert's own charter boundary. |

232→233→234 is a strict chain; 230→231 is a parallel chain merging with 232's own `ApplicationPanel`
addition; 228 gates everything; 229 and 235 are independent once 228/234 land respectively.

---

## ⚠️ Flagged for the ARCH Expert

1. **RESOLVED — §6's promotion recommendation.** `ARCH_22_16_RectangleDragGesturePromotion.md` rules
   promotion BINDING: `AreaDragGesture_UI`'s hit-test/resize/aspect-lock/center-move core is
   generalized into a new accessor-parameterized `RectangleDragGesture_UI<Accessor>` template
   (accessor-callback-parameterized over `originX`/`originZ`/`sizeX`/`sizeZ`, each instantiation
   keeping its own named handle-radius/minimum-extent constant per Constitution §8), mirroring how
   `PropDragGesture_UI`/`DecalDragGesture_UI` already sit as thin `Traits` shims over
   `InstanceDragGesture_UI<Traits>` (§21.3). `AreaDragGesture_UI` and the new
   `NavmeshBlockerDragGesture_UI` both become thin instantiations of it — Areas' own files are
   refactored to the thin-shim shape as part of ticket 230 itself, not left as dead duplicate code.
   The "port a byte-identical ~140-line copy" fallback this document originally described as the
   default is superseded by this ruling and must not be built instead.
2. **CONFIRMED — §4.1's `Params::NavLayerKind` enum vs. `ARCH_19_14_TypeSectionUiDerived.md`'s "no
   new `Params::MarkerTypeSection` struct" precedent.** `ARCH_22_15_NavmeshTabParamsShape.md` point 1
   rules this divergence correctly reasoned, not accidental drift: `markerTypeName`'s open-string
   shape exists specifically because a designer can invent any Bundle "type" freely, while a nav
   layer is the structural opposite — exactly six engine-fixed values, two conditionally absent,
   never designer-extensible. `enum class Params::NavLayerKind { Land, Amphibious, Hover, Air, Sea,
   Submarine }` is confirmed the correct shape; §19.14's own ruling stands unmodified and
   ungeneralized beyond its own open-string domain — this is the correct application of the
   closed-vs-open distinction §19.14 already implies, not a second precedent diluting it.
3. **RESOLVED — §7's Z-order batch-insert cost question** (Q4 below). `ARCH_22_17_ZOrderBatchInsertCost.md`
   rules Navmesh adopts the same "index 0 is topmost, continuously size-sorted" convention via its
   own **independent, parallel** `InsertMapAreaSortedBySize`-shaped function scoped to whichever
   vector(s) hold `NavmeshBlockerRectangle` — never touching `recipe.areas`, a wholly separate array
   and convention instance, not a shared cross-domain function. The `O(N^2)` batch-insert cost is
   ruled NOT to require a bulk-sort-then-batch-insert path, on a rough-estimate basis (Constitution
   §7's basis-tag discipline, not benchmarked): at the spec's own observed range (88 to 815
   rectangles per `NAVMAP_MODIFIER_BLOCKER_SPEC.md` §7), the worst case is on the order of
   `815² ≈ 6.6×10^5` simple float comparisons for the whole batch — sub-millisecond on any realistic
   target hardware, for a human-triggered, author-time, rare action, never per-frame and never inside
   a regeneration DAG. This also matches existing precedent exactly:
   `MapImporter_Areas_IO.cpp::ReadAreasJson` already uses the identical per-item incremental-insert
   pattern for a full from-scratch batch load without a bulk-sort special case. A bulk-append-plus-
   single-`std::sort` path remains available as a future benchmark-backed follow-up for the
   batch-ingestion path specifically (never for interactive single-rectangle create-by-drag), but is
   not built now on no measurement.
4. **RESOLVED (finding confirmed, recommendation declined) — the `MapArea`/`NavmeshBlockerRectangle`
   symmetry-limitation cross-reference.** `ARCH_22_15_NavmeshTabParamsShape.md` point 4 confirms §5.3's
   symmetry-drop is correct for the stated geometric reason (quarter-turn requires an extent-swap
   this codebase's orbit machinery was never built to do; `Radial` produces genuinely non-axis-aligned
   geometry `NavmapModifierTemplate` cannot represent at all), and confirms this finding applies to
   `Params::MapArea` identically. ARCH **declined** this document's recommendation to add a standing
   cross-reference note to `ARCH_13_RadialSymmetry.md`/§16, on the stated basis that no ticket
   currently proposes rectangle symmetry for either domain — a future ticket that does must re-derive
   this from source, with `ARCH_22_15` point 4 itself serving as the shortcut/pointer rather than a
   pre-emptive standing note.
5. **RESOLVED — `ARCH_22_09_OwnershipScopeRuling.md`'s gate.** `ARCH_22_10_MeshIngestionOwnershipRuling.md`
   rules this document, together with the Format/Generator/Compute-Optimization Experts' parallel
   `DESIGN_NavmeshBlockerMeshIngest_R1.md` / `DESIGN_NavmeshBlockerMaskGeneration_R1.md` /
   `DESIGN_NavmeshBlockerGeometryMath_R1.md`, jointly ARE the "real, separately-scoped design consult"
   §22.9 required. §22.9's gate is now satisfied for the file/ticket set this document's §11
   enumerates (and the parallel Format/Generator tables), scoped exactly to those tickets —
   corrected/superseded wherever `ARCH_22_11`-`ARCH_22_17` diverge from this document's own open
   questions (as items 1-4 above do). §22.9's own text stands as historical, correctly describing the
   architecture's state before these four consults existed, not retracted.

---

## ❓ Open questions

**Q1 — RESOLVED, `ARCH_22_15_NavmeshTabParamsShape.md` point 2.** Tagged-vector storage: one shared
`recipe.navmeshBlockerLayers` / `navmeshBlockerLayerBundles` / `navmeshBlockerRectangles`, each entry
carrying its own `navLayerKind` tag, with the Tab building a per-`NavLayerKind` filtered view exactly
as `MarkerLayerBundle` already does (`ARCH_19_15_TypeSectionTreeComposition.md`) — not six parallel
per-`NavLayerKind` vectors. ARCH formally closed this rather than leaving it open twice over (both
this document and the Generator doc had independently recommended the same default). This document's
own tab design (§5) is unaffected either way, as already noted.

**Q2 — RESOLVED, `ARCH_22_15_NavmeshTabParamsShape.md` point 5.** Confirmed: Move only, ported
verbatim (translate every member rectangle's origin by an offset); Rotate is dropped entirely for
this domain, not merely disabled in UI — same reasoning as §5.3's symmetry finding (a rotated
axis-aligned rectangle is not representable at all).

**Q3 — Re-running the mesh-intersection ingestion pipeline against a Layer a designer has since
hand-edited: overwrite, merge-additive, or refuse?** Explicitly out of scope for this UI design (§2)
— routed to whoever designs that pipeline (Generator Expert), noted here only so the UI's own
provenance label (§8) isn't mistaken for having already answered it.

**Q4 — RESOLVED, `ARCH_22_17_ZOrderBatchInsertCost.md`.** Navmesh adopts
`InsertMapAreaSortedBySize`'s shape as-is (`O(N)` per insert), via its own independent per-item
size-sorted convention scoped to `NavmeshBlockerRectangle`'s own vector(s), never touching
`recipe.areas`. The `O(N^2)` batch-insert cost is ruled negligible in practice — a rough-estimate
basis (Constitution §7), not benchmarked: worst case ~`815^2 ≈ 6.6×10^5` float comparisons for the
spec's own largest observed batch (815 rectangles, `NAVMAP_MODIFIER_BLOCKER_SPEC.md` §7),
sub-millisecond for a rare, human-triggered, author-time action never on the per-frame or
regeneration-DAG path. No bulk-sort path is required; one may be added later only as a real,
benchmark-backed follow-up.

**Q5 — Does the Navmesh Tab belong in `ApplicationPanelGroup::Environment`** (alongside Water/
Atmosphere/Areas, `Application_Panels_UI.h`), **or does it warrant its own group?** A UX-only call,
not attempted here beyond noting Environment is the closest existing precedent (Water-adjacent,
Areas-adjacent — both real dependencies of this feature per §3 and the "reuse Areas' interface"
instruction).
