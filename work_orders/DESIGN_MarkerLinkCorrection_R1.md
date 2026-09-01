# Design Output — Markers Tab Link Mechanic Correction, Round 1

*Companion to `work_orders/BRIEF_MarkerLinkCorrection_R1.md`. Covers the brief's 6 "what this brief
needs designed" items for the resolver-contract/body-composition/signature-widening design (SanGen
UI Expert scope). ARCH amendment to `ARCH_19_29`/`ARCH_19_31` for `MarkerTransform::linkIdentifier`
is a parallel, separate ARCH Expert deliverable — this doc assumes it lands as described in the
brief and does not re-derive it.*

## 1. Resolver contract — the real finding first: today's cascade is UI-mirror-only, zero runtime effect

Read every consumer of the 7 governed fields, not just the 8 `Effective*` resolvers in
`MarkersTab_MarkerLinkResolvers_UI.h` (+2 in `MarkersTab_ManualLayerHelpers_UI.h` —
`EffectiveManualMarkerLayerColorOverrideEnabled`/`EffectiveManualMarkerLayerColor`, STEP239). Result,
confirmed by grepping every call site of all 10:

⚠️ **Every single call site of the 10 `Effective*` resolvers is inside
`MarkersTab_ManualLayerRowBody_UI.cpp` / `MarkersTab_BundleHeaderExtras_UI.cpp` /
`MarkersTab_Bundles_UI.cpp` / `MarkersTab_ManualLayers_UI.cpp` — i.e. the Marker-Type section's own
Layer/Bundle row display** (the `bLinked` disable-and-mirror pattern, e.g.
`MarkersTab_ManualLayerRowBody_UI.cpp:183-212`). **Zero non-UI-row-display consumer ever calls any
of these 10 functions.** Every real runtime consumer of the 7 governed fields reads the raw
`MarkerInstanceLayer` field directly, with no Link-awareness at all:

| Governed field | Real runtime consumer(s) | Reads |
|---|---|---|
| Hidden | `ResolveMarkersManual` (`MapCanvas_IconLayer_CullManual_UI.cpp:145-148`) | `input.recipe->markerLayers[subLayerArrayIndex].bHidden` |
| Icon Scale | `ResolveMarkersManual` (same file, line 155-157); `ManualMarkerDotRadius` (`MapCanvas_MarkerRosterDraw_UI.cpp:19-25`) | `layer.iconScale` |
| Color override + color | `ResolveMarkersManual` (line 151-163); `ManualMarkerTint` (`MapCanvas_MarkerRosterDraw_UI.cpp:34-52`) | `layer.bColorOverrideEnabled`, `layer.color[]` |
| Grid Snap pair | `QuantizeMarkerPositionToLayerGrid` (`MarkersTab_ManualLayerHelpers_UI.h:57-65`) | `layer.bGridSnapEnabled`, `layer.gridSnapSizeWorldUnits` |
| Symmetry pair | `ResolveEffectiveMarkerSymmetry` (`MarkersTab_ManualLayerHelpers_UI.h:76-90`) | `layer.bSymmetryEnabled`, `layer.symmetry` |
| Locked | `IsMarkerInstanceLayerLocked` (`MarkersTab_ManualLayerHelpers_UI.h:36-40`) | `layer.bLocked` |
| Name | (row label only — no runtime consumer needed) | n/a |

This is independent of, and pre-dates, this correction — worth folding into the same ticket as the
`NotifyPlacementChange` gap (brief item 6), since it's the same class of "cascade exists on paper,
never reaches the render/gesture layer" bug: **toggling a Link's Icon Size/Grid/Symmetry/Hidden/
Locked/Color today changes nothing about the actual preview, drag behavior, hit-test, lock gating,
or grid snap** — only the disabled row mirror. `ApplyAddLinkAction` never copies any of the Link's 6
non-name fields into the freshly-minted Layer either (`MarkersTab_Links_UI.cpp:48-54` sets only
`name`/`layerId`/`parentBundleIdentifier`/`markerTypeName`/`linkIdentifier`), so the new Layer keeps
struct-default values regardless of what the Link's own header controls say.

**Consumer tier**: all of the above are `src/ui/*` — zero PIPELINE/PROC sites. Confirmed by grep: no
file under `src/pipeline/` references `markerLayers`/`MarkerInstanceLayer` at all. Per ARCH §14,
markers are never baked into the composite; the two real render consumers
(`MapCanvas_MarkerRosterDraw_UI.cpp`, `MapCanvas_IconLayer_CullManual_UI.cpp`) are both screen-space
overlay draw, UI tier. No Compute/Generator Expert sign-off needed on item 2's consumer audit.

⚠️ **New finding, not in the brief**: two of the five real consumers sit behind **shared generic
templates instantiated for Markers/Props/Decals** (ARCH §21.3/§21.5), not Markers-only files. This
changes the blast radius of "add the instance-tier check" non-trivially — see §1.3 below.

### 1.1 The new instance-tier resolvers — one wrapper per governed field, living beside the existing 10

Each existing `Effective*` function is **unchanged** — it becomes the permanent Layer/Bundle-tier
fallback (also the sole path for legacy on-disk data, §6 below). A new sibling set wraps each one,
checking `MarkerTransform::linkIdentifier` first:

```cpp
// New file: MarkersTab_MarkerLinkTransformResolvers_UI.h (see §1.2 for why a new file, not an
// addition to the already-near-ceiling MarkersTab_MarkerLinkResolvers_UI.h).
inline bool EffectiveMarkerTransformHidden(const Params::MarkerTransform& transform,
                                            const Params::MarkerInstanceLayer& layer,
                                            const std::vector<Params::MarkerLink>& links) {
    if (transform.linkIdentifier >= 0)
        for (const Params::MarkerLink& link : links)
            if (link.identifier == transform.linkIdentifier) return link.bHidden;
    return EffectiveManualMarkerLayerHidden(layer, links);   // falls through to the Layer-tier
}                                                             // (legacy) resolution, unchanged.
```

Same shape, one per governed field:
- `EffectiveMarkerTransformIconScale(transform, layer, links)` → falls back to
  `EffectiveManualMarkerLayerIconScale`
- `EffectiveMarkerTransformGridSnapEnabled` / `EffectiveMarkerTransformGridSnapSizeWorldUnits` →
  `EffectiveManualMarkerLayerGridSnapEnabled` / `...GridSnapSizeWorldUnits`
- `EffectiveMarkerTransformSymmetryEnabled` / `EffectiveMarkerTransformSymmetry` → the
  Symmetry-pair equivalents
- `EffectiveMarkerTransformLocked` → `EffectiveManualMarkerLayerLocked`
- `EffectiveMarkerTransformColorOverrideEnabled` / `EffectiveMarkerTransformColor` → the two
  STEP239 helpers in `MarkersTab_ManualLayerHelpers_UI.h`

**Name has no transform-tier wrapper.** Under the correction nothing binds a Layer/Bundle to a Link
anymore going forward (§2 below), so the Layer/Bundle-row label resolvers
(`EffectiveManualMarkerLayerName`, `EffectiveMarkerLayerBundleName`) stay exactly as-is, serving only
the legacy-fallback case; there is no per-instance name display anywhere in the tree that would need
a transform-tier equivalent.

### 1.2 File placement

`MarkersTab_MarkerLinkResolvers_UI.h` is already ~120 lines carrying 8 resolvers, explicitly called
out in its own header comment as near ARCH §1.5's soft ceiling. Adding 8 more (7 wrappers + the
color-override pair counts as 2) pushes it over. Recommend a new sibling file,
`MarkersTab_MarkerLinkTransformResolvers_UI.h`, same namespace, same aspect-split precedent this
whole feature already uses (mirrors `MarkersTab_Links_UI.h`/`MarkersTab_LinksHeaderExtras_UI.cpp`'s
own split). Not an ARCH-boundary question — flagging only so a STEP ticket doesn't silently blow the
ceiling by editing the existing file in place.

### 1.3 The real consumer-site changes — three are trivial, two need to go through the ARCH Expert first

**Trivial (Markers-only file, transform already in local scope, no shared-template contract to touch):**

- `MarkersTab_ManualInstance_UI.cpp:129,143` (`DrawSelectedMarkerInstance`) — already holds
  `transform` and `markerLayers`; add a `links` parameter, replace the two calls with
  `EffectiveMarkerTransformLocked(transform, ..., links)` / a link-aware grid-quantize wrapper.
- `MapCanvas_MarkerRosterDraw_UI.cpp` (`ManualMarkerTint`, `ManualMarkerDotRadius`) — both already
  iterate one `transform` at a time inside `DrawManualMarkerRoster`'s loop
  (`MapCanvas_MarkerRosterDraw_UI.cpp:100-125`); thread `transform.linkIdentifier` and
  `recipe.markerLinks` down (`DrawManualMarkerRoster` and `MapCanvas::DrawManualMarkerDragPass`
  both already receive/can receive a `recipe`-adjacent pointer — `manualMarkerDragRecipe` is
  already threaded into that call site, `MapCanvas_MarkerDrag_UI.cpp:45-49`).
- `MapCanvas_IconLayer_CullManual_UI.cpp` (`ResolveMarkersManual`) — already iterates `transform`
  per instance in its inner loop (line 179-233); the three raw reads at lines 145-163 are
  currently hoisted **once per Layer, outside the transform loop** — this correction forces them
  to move **inside** the per-transform loop (since different transforms on the same Layer can now
  resolve differently), a real, non-cosmetic restructuring of this function, not just a resolver
  swap. Flag this explicitly for the STEP ticket: it's a perf-shape change (hoisted-once →
  per-instance), not just a rename.

⚠️ **Needs ARCH Expert review before a coder touches it — two shared cross-domain generics, ARCH §21.3/§21.5:**

1. **`IsMarkerInstanceLayerLocked`/`QuantizeMarkerPositionToLayerGrid`/`ResolveEffectiveMarkerSymmetry`**,
   consumed through `InstanceDragGesture_UI.h`'s `Traits` contract (`MarkerDragTraits`,
   `MarkerDragGesture_UI.h:45-80`) — the SAME generic template is instantiated for Props/Decals
   (`PropDragGesture_UI.h`/`DecalDragGesture_UI.h`, presumably `PropDragTraits`/`DecalDragTraits`).
   The generic template calls these three Traits methods deep inside its own loops
   (`InstanceDragGesture_UI.h:110,126,166,185,200,310,318`) over **sibling** transforms mid-drag,
   not just the initially-clicked one — a Markers-only wrapper sitting outside the template cannot
   intercept those calls. Fixing this for real (not just at gesture-start) requires widening the
   shared `Traits` contract itself — e.g. a new `Traits::InstanceLinkIdentifier(const Transform&)`
   hook (Markers: `return transform.linkIdentifier;`; Props/Decals: `return -1;`) plus threading a
   `links` vector through all four `InstanceDragGesture_UI.h` entry points (Props/Decals
   pass/ignore an always-empty one). This is a real ARCH §21.3 amendment, not a UI-file text edit —
   route to the ARCH Expert before a STEP ticket touches it.
2. **The `isLayerLocked` predicate in `ManualInstanceHitTest_UI.h`**
   (`HitTestManualInstances<GroupT>`/`CollectManualInstancesInWorldRegion<GroupT>`,
   `ManualInstanceHitTest_UI.h:43-61`) — a `std::function<bool(int layerIndex)>` shared verbatim
   across the same three domains (ARCH §21.5), called by `MapCanvas_ManualDragDispatch_UI.cpp:25-27`
   and `MapCanvas_SelectionGesture_UI.cpp:142-144`. The callback only ever receives `layerIndex`,
   never the transform, so it structurally cannot see `transform.linkIdentifier`. Minimal-diff
   recommendation: widen the predicate to take the transform itself
   (`std::function<bool(const typename GroupT::TransformType&)>`, or equivalently pass both
   `layerIndex` and the transform) — Props/Decals lambdas simply don't reference the extra data, no
   new logic for them, but it's still a signature change to a ratified §21.5 generic. Same routing:
   ARCH Expert sign-off first.

Neither of these is a PIPELINE/PROC-tier concern (both are `src/ui/`), but both are cross-domain UI
generics whose contract the ARCH Expert, not this design doc, should formally re-ratify — flagging
per the brief's "flag, don't silently assume" instruction rather than waving them through as
ordinary UI edits.

## 2. `ApplyAddLinkAction` / `DeleteMarkerLink` / no-op guard — corrected shapes

### 2.1 `ApplyAddLinkAction` — mint the Link, tag transforms directly, nothing else

```cpp
// MarkersTab_Links_UI.cpp — replaces the current body (MarkersTab_Links_UI.cpp:24-59).
void ApplyAddLinkAction(Params::MapRecipe& recipe, const std::vector<int>& selectedManualInstanceIdentifiers) {
    if (selectedManualInstanceIdentifiers.empty()) return;
    if (IsAnyManualInstanceSelectionAlreadyLinked(recipe.markers, selectedManualInstanceIdentifiers)) return;  // ruling 2

    Params::MarkerLink link;
    link.identifier = NextMarkerLinkId(recipe.markerLinks);
    link.name       = "Link " + std::to_string(link.identifier);
    recipe.markerLinks.push_back(link);

    TagManualInstancesWithLink(recipe.markers, selectedManualInstanceIdentifiers, link.identifier);
}
```

No `PartitionSelectedManualInstancesByType` call, no `MarkerLayerBundle`/`MarkerInstanceLayer`
minted, no `ReassignManualInstanceLayers` call — existing layering/grouping is completely untouched,
per the human's ruling.

`TagManualInstancesWithLink` mirrors `ReassignManualInstanceLayers`'s exact walk
(`MarkersTab_ManualInstanceSelection_UI.cpp:35-41`) field-for-field, one field over:

```cpp
// New, MarkersTab_ManualInstanceSelection_UI.h/.cpp — sibling of ReassignManualInstanceLayers,
// same file (same "instance-identifier-keyed mutation over markers" home).
void TagManualInstancesWithLink(std::vector<Params::MarkerInstanceGroup>& markers,
                                const std::vector<int>& taggedIdentifiers, int linkIdentifier) {
    for (Params::MarkerInstanceGroup& group : markers)
        for (Params::MarkerTransform& transform : group.transforms)
            if (IsManualInstanceSelected(taggedIdentifiers, transform.instanceIdentifier))
                transform.linkIdentifier = linkIdentifier;
}
```

### 2.2 `DeleteMarkerLink` — clear the transform-tier tag, keep the legacy Layer/Bundle-tier clear as a fallback

```cpp
// MarkersTab_Links_UI.cpp — replaces MarkersTab_Links_UI.cpp:13-22.
void DeleteMarkerLink(int linkIdentifier, std::vector<Params::MarkerLink>& links,
                      std::vector<Params::MarkerInstanceGroup>& markers,             // NEW param
                      std::vector<Params::MarkerLayerBundle>& bundles,               // kept — legacy-only, §6
                      std::vector<Params::MarkerInstanceLayer>& markerLayers) {      // kept — legacy-only, §6
    for (Params::MarkerInstanceGroup& group : markers)
        for (Params::MarkerTransform& transform : group.transforms)
            if (transform.linkIdentifier == linkIdentifier) transform.linkIdentifier = -1;
    // Legacy fallback — clears any pre-correction Layer/Bundle-tier tag a surviving old .sanmap
    // might still carry (§6). A dead branch for every Link minted by the corrected ApplyAddLinkAction,
    // since nothing sets these anymore, but a real branch for imported legacy data.
    for (Params::MarkerLayerBundle& bundle : bundles)
        if (bundle.linkIdentifier == linkIdentifier) bundle.linkIdentifier = -1;
    for (Params::MarkerInstanceLayer& layer : markerLayers)
        if (layer.linkIdentifier == linkIdentifier) layer.linkIdentifier = -1;
    for (auto it = links.begin(); it != links.end(); ++it)
        if (it->identifier == linkIdentifier) { links.erase(it); break; }
}
```

Signature grows by one parameter (`markers`) — `DrawMarkerLinksSection` already has `recipe` in
scope, so the call site just adds `recipe.markers`.

### 2.3 No-op guard — `IsAnyManualInstanceSelectionAlreadyLinked`

Mirrors `IsManualInstanceSelectionEntirelyType`'s shape
(`MarkersTab_ManualInstanceSelection_UI.cpp:43-61`) but is an ANY-match, not an ALL-match, and
doesn't take a type name:

```cpp
// New, MarkersTab_ManualInstanceSelection_UI.h/.cpp.
// True the moment any resolved selected identifier already carries a real linkIdentifier (>= 0) —
// an unresolved/stale identifier is simply skipped (Constitution §6), never itself a reason to block.
bool IsAnyManualInstanceSelectionAlreadyLinked(const std::vector<Params::MarkerInstanceGroup>& markers,
                                               const std::vector<int>& selectedIdentifiers) {
    for (const int identifier : selectedIdentifiers)
        for (const Params::MarkerInstanceGroup& group : markers)
            for (const Params::MarkerTransform& transform : group.transforms)
                if (transform.instanceIdentifier == identifier && transform.linkIdentifier >= 0)
                    return true;
    return false;
}
```

Note this checks the **instance's own** `linkIdentifier` only — under the correction that's the sole
membership signal for new Links; a legacy Layer-tier tag (§6) never makes an instance itself
"already linked" for this specific guard's purposes (a legacy-tagged Layer's instances are not
retroactively tagged at the transform tier by this ticket — see §6's explicit no-migration
recommendation).

## 3. `DrawMarkerLinksSection` — widened signature and hierarchical body

### 3.1 Signature

```cpp
// MarkersTab_Links_UI.h — replaces MarkersTab_Links_UI.h:167.
void DrawMarkerLinksSection(Params::MapRecipe& recipe, MarkerLinksState_UI& state,
                            int& selectedManualInstanceIdentifier,
                            std::vector<int>& selectedManualInstanceIdentifiers,
                            int& manualInstanceSelectionAnchorIdentifier,
                            const std::function<void(int clickedInstanceIdentifier,
                                                     const std::vector<int>& selectedInstanceIdentifiers)>&
                                selectManualMarkerInstanceCallback,
                            Pipeline::PreviewDriver* previewDriver);   // item 5 wiring, see §5
```

Every one of these is already a live local (`state.selectedManualInstanceIdentifier` etc.) or
parameter (`selectManualMarkerInstanceCallback`, `previewDriver`) at `DrawMarkersTab`'s relocated
call site (§4) — no new state needs to be invented, this is exactly `DrawManualMarkerLayerListBody`'s
own established parameter set (`MarkersTab_UI.cpp:465-471`), one call up the tree.

### 3.2 Membership query — the new pure function the hierarchical body needs

"Every instanceIdentifier currently tagged to Link X, grouped by canonical type name" — a new pure
function, sibling of `PartitionSelectedManualInstancesByType` (same file, same shape, keyed by
`transform.linkIdentifier == X` instead of "is in `selectedIdentifiers`"):

```cpp
// New, MarkersTab_ManualInstanceSelection_UI.h/.cpp.
std::unordered_map<std::string, std::vector<std::pair<int, int>>> PartitionLinkedManualInstancesByType(
        const std::vector<Params::MarkerInstanceGroup>& markers, int linkIdentifier) {
    std::unordered_map<std::string, std::vector<std::pair<int, int>>> byType;
    for (int groupIndex = 0; groupIndex < static_cast<int>(markers.size()); ++groupIndex) {
        const Params::MarkerInstanceGroup& group = markers[static_cast<std::size_t>(groupIndex)];
        for (int transformIndex = 0; transformIndex < static_cast<int>(group.transforms.size()); ++transformIndex) {
            if (group.transforms[static_cast<std::size_t>(transformIndex)].linkIdentifier != linkIdentifier) continue;
            byType[Params::CanonicalMarkerTypeSectionName(group.name)].push_back({groupIndex, transformIndex});
        }
    }
    return byType;
}
```

Returns `(groupIndex, transformIndex)` pairs, not bare identifiers — matching
`DrawBaseSectionManualInstanceList`'s own item type (`std::pair<int,int>`, `MarkersTab_UI.cpp:180-229`)
exactly, since that's what `DrawManualInstanceRow`/`DrawSymmetryClusterInstanceList` already consume.

### 3.3 The hierarchical body — verbatim reuse of `DrawBaseSectionManualInstanceList`'s own machinery

```cpp
// MarkersTab_LinksHeaderExtras_UI.cpp — replaces DrawMarkerLinkSummaryBody's current
// BulletText-only body (MarkersTab_LinksHeaderExtras_UI.cpp:174-195).
void DrawMarkerLinkBody(const Params::MarkerLink& link, Params::MapRecipe& recipe,
                        int& selectedManualInstanceIdentifier,
                        std::vector<int>& selectedManualInstanceIdentifiers,
                        int& anchorIdentifier,
                        const std::function<void(int, const std::vector<int>&)>& selectManualMarkerInstanceCallback) {
    const auto byType = PartitionLinkedManualInstancesByType(recipe.markers, link.identifier);
    if (byType.empty()) { ImGui::TextDisabled("(no instances)"); return; }

    for (const auto& typeAndInstances : byType) {
        const std::string& typeName = typeAndInstances.first;
        const std::vector<std::pair<int,int>>& instances = typeAndInstances.second;
        ImGui::PushID(typeName.c_str());
        ImGui::TextUnformatted(typeName.c_str());   // ruling 3 — a plain label, NOT a Section widget

        std::vector<int> rowOrder;                   // ruling 4 — full selection needs THIS list's own order
        for (const std::pair<int,int>& gt : instances)
            rowOrder.push_back(recipe.markers[static_cast<std::size_t>(gt.first)]
                .transforms[static_cast<std::size_t>(gt.second)].instanceIdentifier);

        ManualInstanceRowInteractionContext_UI interaction;
        interaction.primaryIdentifier   = &selectedManualInstanceIdentifier;
        interaction.selectedIdentifiers = &selectedManualInstanceIdentifiers;
        interaction.anchorIdentifier    = &anchorIdentifier;
        interaction.rowOrder            = &rowOrder;
        interaction.selectManualMarkerInstanceCallback = selectManualMarkerInstanceCallback;

        DrawSymmetryClusterInstanceList<std::pair<int,int>>(instances,
            [&](const std::pair<int,int>& gt) {
                return recipe.markers[static_cast<std::size_t>(gt.first)]
                    .transforms[static_cast<std::size_t>(gt.second)].symmetryGroupIdentifier;
            },
            [](int groupIdentifier, int) { return groupIdentifier != 0; },
            [&](const std::pair<int,int>& gt) { DrawManualInstanceRow(recipe.markers, gt, interaction); });
        ImGui::PopID();
    }
}
```

This is `DrawBaseSectionManualInstanceList`'s own `rowOrder`/`interaction`/
`DrawSymmetryClusterInstanceList`/`DrawManualInstanceRow` block (`MarkersTab_UI.cpp:207-229`), copied
verbatim one tier over, with the item source swapped from "un-Layered instances of type X" to
"Link-tagged instances of type X." Selection-sync (point 3's "click anywhere, highlight everywhere")
is free: same three state fields, same callback, same `ManualInstanceRowInteractionContext_UI`
plumbing already round-tripping through `MapCanvas::SyncManualMarkerSelection` — nothing new to wire.

`DrawMarkerLinksSection`'s own outer loop (`MarkersTab_Links_UI.cpp:61-79`) swaps its call from
`DrawMarkerLinkSummaryBody(...)` to `DrawMarkerLinkBody(link, recipe, selectedManualInstanceIdentifier,
selectedManualInstanceIdentifiers, manualInstanceSelectionAnchorIdentifier,
selectManualMarkerInstanceCallback)`.

## 4. Relocated call site and "+Link" button placement

Move the call in `MarkersTab_UI.cpp` from its current position (line 490, last statement in
`DrawMarkersTab`) to immediately after `DrawMarkersTabGlobals(state.globals);` (line 282), passing
the widened parameter set (§3.1) — all already in local scope at that point, no new plumbing needed
in `DrawMarkersTab`'s own signature.

**"+Link" button placement — settled: stays in each Type-section header, not duplicated into the
Links section.** Reasoning:
- The button's job is "act on the CURRENT selection," which lives wherever the user is currently
  clicking rows — that's naturally the Type-section body (or, after this correction, also legally
  the Links section body itself, since rows there support the same Ctrl/Shift selection). Moving/
  duplicating the button into the Links section header buys nothing: a user who has just
  multi-selected instances inside a Link's own body already has an existing Link (that's what put
  those rows there) and would almost always hit the no-op guard (§2.3) — "+Link" is overwhelmingly
  a Type-section-body action, selecting UNLINKED instances there.
- The one-frame lag is now real but cosmetic and self-resolving: mint a Link in a Type-section body,
  the Links section (now drawn *before* that body per §4's relocation) doesn't show it until the
  following frame. This is strictly better than today's ordering (Links section drawn *after* every
  Type-section, so today's lag is Links-section-only) and every OTHER visible surface already
  tolerates one-frame click-lag as a matter of course (e.g. `DrawSectionBegin`/`DrawSectionEnd`'s own
  collapse-state). Not worth adding a second button instance to avoid one frame of cosmetic lag.
- ❓ Flag for the human, not resolved unilaterally: if the intent is specifically "start a NEW Link
  from scratch with no pre-existing selection needed" (an empty-selection creation flow, unlike
  today's selection-driven mint), that's a materially different feature (mirrors how "+Group"/
  "+Layer" create their container regardless of selection, per `ApplyAddLinkAction`'s own header
  comment contrasting itself against that pattern) — out of scope for this correction unless the
  human asks for it explicitly.

## 5. `NotifyPlacementChange` wiring fix

`DrawMarkerLinksSection`'s per-Link loop already computes `bAnyCommitted` per Link
(`MarkersTab_Links_UI.cpp:66-70`) and drops it. Accumulate across the whole loop, call once after it
finishes — mirrors the Type-section loop's own "call once after the body" convention
(`MarkersTab_UI.cpp:479`, `NotifyPlacementChange(bRecipeMoved, previewDriver)`):

```cpp
void DrawMarkerLinksSection(Params::MapRecipe& recipe, MarkerLinksState_UI& state, /* §3.1 params */,
                            Pipeline::PreviewDriver* previewDriver) {
    bool bAnyLinkCommitted = false;
    for (Params::MarkerLink& link : recipe.markerLinks) {
        ImGui::PushID(link.identifier);
        if (DrawSectionBegin(...)) {
            bool bAnyCommitted = false;
            DrawMarkerLinkHeaderExtra(link, state, bAnyCommitted);
            if (state.renamingLinkIdentifier != link.identifier)
                DrawMarkerLinkBody(link, recipe, /* selection params */);
            bAnyLinkCommitted = bAnyLinkCommitted || bAnyCommitted;
            DrawSectionEnd();
        }
        ImGui::PopID();
    }
    if (state.pendingDeleteLinkIdentifier >= 0) { /* DeleteMarkerLink call, §2.2 signature */ }
    NotifyPlacementChange(bAnyLinkCommitted, previewDriver);
}
```

Needs `#include "PlacementRuleSections_UI.h"` (declares `NotifyPlacementChange`) and a forward
declaration/include for `Pipeline::PreviewDriver*` in `MarkersTab_Links_UI.h`, mirroring
`MarkersTab_UI.h`'s own existing include for the same type.

## 6. Backward-compat — old `.sanmap` files carrying the pre-correction Layer/Bundle-tier Link shape

**Recommendation: keep the Layer/Bundle-tier `linkIdentifier` fallback path alive indefinitely, no
one-time import migration.** Reasoning:
- The old shape (`MarkerLayerBundle::linkIdentifier`/`MarkerInstanceLayer::linkIdentifier`, an
  exclusive freshly-minted Layer/Bundle) and the new shape (`MarkerTransform::linkIdentifier` on
  individually-tagged instances, no exclusive Layer/Bundle) are not mutually exclusive on the wire —
  both fields keep existing on their respective structs. §2.1/§1.1's design already treats the
  Layer/Bundle-tier resolvers as a permanent fallback (checked when the transform's own
  `linkIdentifier < 0`), so an old file simply keeps resolving exactly as it does today: no
  instance-tier tag present (all `-1` on import, since the field didn't exist in the old `.sanmap`),
  falls through to the Layer's own `linkIdentifier`, which the old file DOES carry — **round-trips
  correctly with zero migration code**, for both read (rendering/resolution, once §1.3's real-consumer
  fix lands) and re-save (both fields serialize; nothing erases the old one).
- A one-time migration (rewriting every instance under an old Link-bound Layer to carry the new
  transform-tier tag, then clearing the Layer-tier tag) would actively **change** old files'
  behavior under the corrected resolution order in a way that's hard to justify: it would silently
  convert "every instance on this Layer, present and future" into "these specific instances, frozen
  at migration time" — a real behavior change disguised as a format upgrade, and exactly the kind of
  surprise Constitution §6 tells us not to spring on a re-saved file. Recommend explicitly against it.
- `DeleteMarkerLink` (§2.2) keeps clearing both tiers for exactly this reason — deleting a Link on a
  file that has never been re-saved since the old mechanism shipped must still fully un-tag it.
- `WarnDanglingMarkerLinkIdentifiers` (`MapImporter_MarkerLink_IO.cpp:67-81`) currently checks
  Bundle/Layer tiers only. Extend it with a third loop over
  `recipe.markers[].transforms[].linkIdentifier`, same shape, same soft-warn-never-repair posture
  (ARCH §19.29/§19.30) — a small, mechanical IO addition.

⚠️ **Needs IO Architecture Expert sign-off**: whether the new `MarkerTransform::linkIdentifier` wire
field lands in `MapExporter_Markers_IO.cpp`/`MapImporter_MarkerGroups_IO.cpp` (the existing
`MarkerTransform` read/write site, since instances live nested under
`MarkerGroups[i].Transforms[i]` — brief item 1) or gets its own file, mirroring how
`LinkIdentifier`'s existing Bundle/Layer-tier wire wiring was homed (`DESIGN_MarkerLink_R1.md` §3.8).
This design doc does not resolve that file-home question — it's the IO Architecture Expert's call,
not mine, and is orthogonal to whether a migration pass is warranted (it isn't).

## Summary of new/changed surfaces for the STEP ticket(s)

- **New file**: `MarkersTab_MarkerLinkTransformResolvers_UI.h` — 8 transform-tier wrapper resolvers
  (§1.1/1.2).
- **`MarkersTab_ManualInstanceSelection_UI.h`/`.cpp`**: + `TagManualInstancesWithLink`,
  `IsAnyManualInstanceSelectionAlreadyLinked`, `PartitionLinkedManualInstancesByType` (§2.1, 2.3, 3.2).
- **`MarkersTab_Links_UI.h`/`.cpp`**: `ApplyAddLinkAction` rewritten (§2.1), `DeleteMarkerLink`
  signature +`markers` param (§2.2), `DrawMarkerLinksSection` signature widened + relocated call
  site + `NotifyPlacementChange` wiring (§3.1, §4, §5).
- **`MarkersTab_LinksHeaderExtras_UI.cpp`**: `DrawMarkerLinkSummaryBody` replaced by
  `DrawMarkerLinkBody` (§3.3).
- **`MarkersTab_UI.cpp`**: `DrawMarkerLinksSection` call moved from end of `DrawMarkersTab` to right
  after `DrawMarkersTabGlobals` (§4).
- **`MarkersTab_ManualInstance_UI.cpp`**, **`MapCanvas_MarkerRosterDraw_UI.cpp`**,
  **`MapCanvas_IconLayer_CullManual_UI.cpp`**: real-consumer instance-tier fixes (§1.3, trivial tier).
- ⚠️ **`InstanceDragGesture_UI.h` (ARCH §21.3) / `ManualInstanceHitTest_UI.h` (ARCH §21.5)**:
  real-consumer instance-tier fixes, blocked on ARCH Expert amendment first (§1.3, needs-sign-off
  tier).
- **`MapImporter_MarkerLink_IO.cpp`**: `WarnDanglingMarkerLinkIdentifiers` +transform-tier loop (§6).
- Pre-existing, independent bug folded into this ticket per the brief: `ApplyAddLinkAction`'s
  newly-minted Layer/Bundle never copied the Link's 6 non-name fields into it either — moot going
  forward since no Layer/Bundle is minted anymore, but worth a one-line note in the STEP ticket so it
  isn't mistaken for something the correction needs to separately "fix."

## Who else this touches

- **ARCH Expert** — required, blocking, on two fronts: (1) the `MarkerTransform::linkIdentifier`
  reversal itself (already in motion, parallel consult per the brief), and (2) §1.3's two
  shared-generic amendments (§21.3 `InstanceDragGesture_UI.h` Traits contract, §21.5
  `ManualInstanceHitTest_UI.h` predicate signature) — new findings from this pass, not yet scoped by
  the brief.
- **IO Architecture Expert** — §6's wire-file-home question, and the
  `WarnDanglingMarkerLinkIdentifiers` extension.
- **Format Expert** — confirming the `MarkerGroups[i].Transforms[i].LinkIdentifier` JSON path (brief
  item 1, not re-derived here).
- **Compute/Generator Expert** — not needed; §1's audit found zero PIPELINE/PROC consumers.
