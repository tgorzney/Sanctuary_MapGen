[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.33. **Only the ARCH Expert writes this file.**

### 19.33 Link membership corrected to instance-tier tagging — CORRECTED 2026-08-31, direct human ruling, amends §19.29 (retracts its `MarkerTransform` exclusion) and §19.31 (widens the resolver mechanism); ruled sound with one refinement

Responds to `work_orders/BRIEF_MarkerLinkCorrection_R1.md` ("RULED by the human, this session"),
correcting STEP239-242's shipped mechanism ahead of the Round-1 UI correction
(`DESIGN_MarkerLinkCorrection_R1.md`, not yet authored). Grounded against the live code
(`src/params/MarkerLink_PARAMS.h`, `MarkerInstance_PARAMS.h`, `MarkerLayerBundle_PARAMS.h`,
`src/ui/MarkersTab_Links_UI.h/.cpp`, `MarkersTab_MarkerLinkResolvers_UI.h`,
`src/io/MapExporter_Markers_IO.cpp`, `MapImporter_MarkerGroups_IO.cpp`,
`MapImporter_MarkerLayerBundle_IO.cpp`, `MapImporter_MarkerLink_IO.cpp`), not merely the brief's own
restatement.

#### The reversal, why it's now correct
`ARCH_19_29`'s original ruling — *"Neither field is added to `MarkerRuleLayer` or `MarkerTransform`...
a Link never needs to reach past the Layer down to the raw transform for anything this ticket
requires"* — was true only because the shipped `ApplyAddLinkAction` (`MarkersTab_Links_UI.cpp:24-59`)
minted a **freshly-created, Link-exclusive** `MarkerLayerBundle`+`MarkerInstanceLayer` per represented
type and moved the selected instances onto it via `ReassignManualInstanceLayers`. Under that shape,
"every instance on this Layer" and "every instance in this Link" were the same set by construction —
resolving the 7 governed fields (`ARCH_19_31`) at the Layer tier was sufficient and correct.

**The human has now ruled (relayed via `BRIEF_MarkerLinkCorrection_R1.md`) that "+Link" must stop
minting any Bundle/Layer and must stop moving instances at all** — existing grouping/layering stays
completely untouched; a Link becomes a pure per-instance tag. That guarantee (Layer membership ==
Link membership) no longer holds once instances stay on their real, pre-existing, possibly-mixed
Layers. §19.29's exclusion sentence's own stated reason for existing is gone; its conclusion no
longer follows. **The reversal is architecturally required, not merely requested — ratified.**

#### Ruled: architecturally sound, with one refinement (Name is NOT extended to the instance tier)

**Sound, no refinement needed, for six of the seven governed fields** — `bColorOverrideEnabled`+
`color`, `bHidden`, `iconScale`, the grid-snap pair, the symmetry pair, `bLocked`. Each of these is a
genuine per-marker rendering/interaction property consumed at exactly the granularity a
`MarkerTransform` sits at (canvas draw, hit-test, drag-gate, preview compositor all already operate
per-transform, resolving up to a Layer only to fetch a shared setting) — checking the transform's own
tag first, falling back to the Layer's own already-correct resolution (`§19.31`, itself already
`layer.linkIdentifier >= 0` → Link, else the Layer's own field) when the instance itself carries no
tag, is a clean, deterministic two-step chain with no ordering hazard. **This is also the intended new
capability, not a side effect to guard against**: a single physical Layer can now legitimately show
DIFFERING resolved colors/hidden/lock states across its own markers, some Link-tagged, some not, or
tagged into different Links — that is the entire point of moving from Layer-exclusive to
per-instance tagging. A resolver, or a coder, must not "promote" any instance-tier tag up to the
whole Layer, and must not treat a Layer as wholly Link-governed just because *some* member instance is.

**Not sound as literally stated for `name` — real problem, flagged, refinement required.** The
brief's ask applies the identical "instance tag first" rule to all 7 fields, including `name`. This
doesn't have a coherent target: `EffectiveMarkerLayerBundleName`/`EffectiveManualMarkerLayerName`
(`MarkersTab_MarkerLinkResolvers_UI.h:30-46`) mirror the Link's `name` onto a **Bundle's or Layer's
own row-label field** — a Section/Group-tier display string. `MarkerTransform::name`
(`MarkerInstance_PARAMS.h:73`) is a pre-existing, semantically unrelated field: **the individual
marker's own proper name** (the file's own comment: `e.g. "Mex 0"`), wire-keyed as the folded-in inner
dictionary key, round-tripped through `MakeNamesUnique`. There is no "the instance's own Section-row
name" concept to resolve into at the transform tier — inventing an `EffectiveMarkerTransformName`
that mirrors the Link's single shared name onto every tagged instance's OWN identity name would
silently erase what distinguishes "Mex 0" from "Mex 1" from "Mex 2" the moment all three joined the
same Link, a real functional regression, not a faithful "one more field, same mechanism" extension.
**Ruled: `name` is excluded from the new instance-tier check.** Name propagation continues to exist
**only** at the Bundle/Layer tier, exactly as `§19.31` already ratified, for whatever Bundle/Layer
entities still carry a `linkIdentifier` (see "dead-write, live-read" below — going forward that means
backward-compat legacy data only). The Link's own Section header (`link.name`, already the one
editable surface, `MarkersTab_Links_UI.cpp:64`) remains the only "name" a Link visibly contributes;
nothing about the correction's Links-Section UI (brief's point 2 — per-type grouping, per-instance
rows) needs or implies a per-instance name override. **6 of the 7 governed-field resolvers gain the
instance-tier check; the 7th (Name) does not, and stays exactly as `§19.31` already specified.**

#### New field — ratified

```cpp
// MarkerInstance_PARAMS.h — MarkerTransform gains:
int linkIdentifier = -1;   // ARCH §19.33 — the primary, instance-tier resolution key for the six
                            // governed fields listed above (never Name, see the refinement above).
                            // -1 = not Link-bound, the same sentinel this whole struct family already
                            // uses (layerIndex's absence-sentinel aside, this mirrors
                            // MarkerLayerBundle::linkIdentifier / MarkerInstanceLayer::linkIdentifier,
                            // §19.29, applying that exact two-tier-already/now-three-tier pattern).
```
No collision with `MarkerTransform`'s existing bare-int fields (`layerIndex`,
`symmetryGroupIdentifier`, `instanceIdentifier`) — distinct purpose, distinct name, per §1.9's
qualified-noun convention this file already follows. **Additive, no `SanGenVersion` bump** — same
precedent class as `instanceIdentifier` (§19.16) landing on this exact struct.

**Wire key and IO home — ruled by direct analogy to the already-shipped `instanceIdentifier` on this
exact struct** (`MapExporter_Markers_IO.cpp:39`, `MapImporter_Markers_IO.cpp:82-83`): wire key
**`"LinkIdentifier"`**, read/written directly on the transform's own JSON object (nested at
`markers[type].transforms[name]`, the same object `InstanceIdentifier`/`layerIndex`/
`symmetryGroupIdentifier`/`iconNameOverride`/`alias` already live on) — in the SAME two files that
already own every other `MarkerTransform` scalar, `MapExporter_Markers_IO.cpp` and
`MapImporter_Markers_IO.cpp`, not the two `MarkerLink`-specific IO files (those own the `MarkerLinks`
array and the Bundle/Layer-tier merged fields only). Absent key → `-1`, no legacy-backfill counter
needed (unlike `instanceIdentifier`, this field needs no cross-import uniqueness, only an
absence-default, the same posture `parentBundleIdentifier`/`assemblyIdentifier`/the two existing
`linkIdentifier` fields already have). **This wire-key/file-home ruling is offered with high
confidence from direct, identically-shaped precedent on the same struct — the Format Expert and IO
Architecture Expert should still give it a formal one-line confirmation per the brief's own routing
instruction before a coder ticket is cut, since wire-shape ground truth is their domain, not
re-derived here as a rubber stamp.**

#### Resolver-contract change — ratified, binding

Every one of the six resolvers below (`MarkersTab_MarkerLinkResolvers_UI.h`) gains a **new, first
parameter check against the owning `MarkerTransform::linkIdentifier`**, ahead of its existing
Layer-tier resolution (which is UNCHANGED — still `layer.linkIdentifier >= 0 → Link, else
layer`'s own field):

```cpp
// New shape, six resolvers (Name is excluded, see the refinement above) — mirrors the existing
// EffectiveManualMarkerLayer*'s exact "linkIdentifier lookup, else fall through" idiom, one more tier:
bool  EffectiveManualMarkerInstanceHidden(const Params::MarkerTransform& transform,
                                          const Params::MarkerInstanceLayer& layer,
                                          const std::vector<Params::MarkerLink>& links);
// ...EffectiveManualMarkerInstanceColorOverrideEnabled/Color, IconScale, GridSnapEnabled/
// GridSnapSizeWorldUnits, SymmetryEnabled/Symmetry, Locked — same three-parameter shape.
// Resolution order, identical for all six:
//   1. transform.linkIdentifier >= 0 AND resolves to a real Params::MarkerLink -> that Link's field.
//   2. else, layer.linkIdentifier >= 0 AND resolves to a real Params::MarkerLink -> that Link's field
//      (the EXISTING Layer-tier mechanism, §19.31, unchanged).
//   3. else -> layer's own stored field, unchanged from today.
// A dangling identifier at EITHER tier is Constitution §6's existing soft-degrade — falls through to
// the next step exactly as a -1/unbound value would, never a refusal, never a repair.
```
The existing two-parameter `EffectiveManualMarkerLayer*` resolvers (`Hidden`/`IconScale`/
`GridSnapEnabled`/`GridSnapSizeWorldUnits`/`SymmetryEnabled`/`Symmetry`/`Locked`, plus
`EffectiveManualMarkerLayerColorOverrideEnabled`/`Color` in `MarkersTab_ManualLayerHelpers_UI.h`) are
**not deleted** — they remain the correct call for any site that only has a Layer in hand and no
specific transform (e.g. a Layer-row header control in the Bundle tree, which by definition represents
the whole Layer, not one instance). The new three-parameter siblings are for **per-instance** call
sites. `EffectiveMarkerLayerBundleName`/`EffectiveManualMarkerLayerName` are unchanged (Name stays
two-parameter, per the refinement above).

**Binding consumer-site audit — every one of these must be widened to pass the owning `MarkerTransform`,
not resolve from `layerIndex` alone, before this correction is complete (named per the brief's own
ask, not left to inference):**
- Canvas draw (marker icon color/visibility/scale composition, `MapCanvas_IconLayer_CullManual_UI.cpp`
  and whatever preview-compositor mirror exists) — UI/PIPELINE-adjacent, confirm tier at dispatch time.
- Hit-test / drag-gate / marquee lock predicates (the `bLocked` consumers, `ARCH_21_03`/`ARCH_21_05`'s
  own `isLayerLocked(layerIndex)`-shaped predicates) — these currently resolve lock by `layerIndex`
  alone; they need the transform in hand too, or a widened predicate signature
  `isInstanceLocked(instanceIdentifier, layerIndex)`. This is a real, load-bearing signature change,
  not cosmetic — flag for the coder ticket explicitly. **RESOLVED 2026-08-31 by `§21.9`** (concretely:
  the predicate/`Traits` methods take the whole `Transform`, not an `instanceIdentifier`/`layerIndex`
  pair — a strictly better shape than the placeholder speculated here, since it needs no second lookup
  to read the transform's own `linkIdentifier`). `§21.9` is authoritative on the exact signatures.
- `ResolveEffectiveMarkerSymmetry` (`MarkersTab_ManualLayerHelpers_UI.h`) — the design's own §3.2
  citation already names this as needing "a Link-resolution step ahead of its existing global/per-layer
  resolution"; under this correction that step is transform-tier-first, not Layer-tier-first.
  **RESOLVED 2026-08-31 by `§21.9`**, together with its sibling `QuantizeMarkerPositionToLayerGrid` and
  the new `EffectiveManualMarkerInstanceLocked`/`IsMarkerInstanceLocked` pair for `bLocked`.
- Any non-UI/PIPELINE/PROC read site found during that audit needs Compute/Generator Expert sign-off
  per the brief's own routing — not assumed clear here.

#### No-op guard and `ApplyAddLinkAction`/`DeleteMarkerLink` — ruled consistent with the field-placement ruling above, no separate ARCH note needed

**"+Link" no-op guard** (brief: "if ANY selected instance already belongs to ANY existing link, do
nothing"): under this correction, membership is a direct field check —
`transform.linkIdentifier >= 0` — no Bundle/Layer walk required. This is a direct, mechanical
corollary of adding the field to `MarkerTransform`, not a new design decision; ratified as part of
this same section, no separate ruling needed.

**`ApplyAddLinkAction` (`MarkersTab_Links_UI.cpp:24-59`) — corrected shape, ratified:** mint the
`Params::MarkerLink` entry (unchanged), then set `transform.linkIdentifier = link.identifier` directly
on every selected instance. **Delete the Bundle/Layer-minting and `ReassignManualInstanceLayers` call
entirely** — no `Params::MarkerLayerBundle`, no `Params::MarkerInstanceLayer` is created by this
action any more; existing layering/grouping is untouched, exactly as ruled. `PartitionSelectedManualInstancesByType`
(already shipped, `MarkersTab_ManualInstanceSelection_UI.h`) is **retained** as a pure, read-only UI
helper for the Links-Section body's per-type grouping (brief's point 2) — it is no longer used to
drive any PARAMS mutation, only a display grouping. This is consistent with the field-placement ruling
above; no separate ARCH note needed.

**`DeleteMarkerLink` (`MarkersTab_Links_UI.cpp:13-22`) — gains a third walk, ratified:** in addition to
the two existing walks (clear `bundle.linkIdentifier`/`layer.linkIdentifier` to `-1` for backward
compat, see below — **kept, not removed**), add a walk over every `MarkerTransform` in
`recipe.markers[*].transforms` clearing `transform.linkIdentifier` to `-1` wherever it matches the
deleted Link's identifier. Same posture as the existing two walks: ungroups the LINK relationship
only, the instance itself is never touched/erased/moved. Erase the `Params::MarkerLink` entry last,
unchanged.

#### Backward compatibility — no migration needed, ruled

**A `.sanmap` written by the OLD (Layer-exclusive) mechanism needs no one-time migration and no
special-case read path.** Reasoning, checked directly against the resolver chain above, not assumed:
the new instance-tier check is purely additive and checked **first**, falling back to the
**unchanged** Layer-tier mechanism exactly as it already, correctly, resolves today. An old-mechanism
file has `bundle.linkIdentifier`/`layer.linkIdentifier` set (from when `ApplyAddLinkAction` used to
write them) and no `transform.linkIdentifier` (the field didn't exist, so it imports as `-1`, "not
tagged"). Under the new chain: step 1 (`transform.linkIdentifier >= 0`) is false for every such
transform → falls through to step 2, which is the exact same Layer-tier resolution this file already
relied on before this correction shipped. **The old data resolves identically, correctly, with zero
code written for it specifically** — this is the strongest point in favor of ruling the correction
sound: it is a pure superset of the existing mechanism, not a replacement requiring translation.

**The Bundle-tier and Layer-tier `linkIdentifier` fields (`§19.29`) are NOT retired, deprecated, or
removed by this correction.** Going forward, no live UI path sets them any more (`ApplyAddLinkAction`
no longer writes them, and no other shipped affordance writes them either, confirmed by grep) — they
become **dead-write, live-read**: nothing mints new Bundle/Layer-tier tags, but the resolver fallback
chain above still consults whatever value either field already holds, whether from an old-mechanism
save or (in principle) hand-edited data. Do not delete these fields, their IO round-trip, or their
resolver fallback step — doing so would break import of every already-shipped Link-bearing `.sanmap`
outright (a hard break, not a soft degrade), which Constitution §6 does not sanction for a field that
already round-trips correctly. If a future design ever wants to actively strip stale Bundle/Layer-tier
tags on import (a "compact on load" convenience), that is a new, separate ARCH ruling on its own
merits — not a consequence of this correction.

#### `§19.29`'s text, corrected

`§19.29`'s sentence *"Neither field is added to `MarkerRuleLayer` or `MarkerTransform`... a Link never
needs to reach past the Layer down to the raw transform for anything this ticket requires"* is
**retracted for `MarkerTransform` only.** The `MarkerRuleLayer` half of that sentence is **unchanged
and still correct** — procedural marker instances remain categorically out of Link scope
(`ARCH_19_09_ManualOnlyMembership.md`, restated by `work_orders/DESIGN_MarkerLink_R1.md` §0, neither
reopened nor touched by this correction). `§19.29`'s own two-tier independent-back-reference framing
(Bundle and Layer, no walk-up derivation) is **extended, not replaced** — `MarkerTransform` becomes a
third, independent tier in the identical pattern, consulted FIRST in the resolver chain, per the
contract above.

#### What this does NOT reopen

`§19.28`'s `Params::MarkerLink` field set (all 7 mirrored fields, `bLocked` included) is unchanged —
this correction is about WHERE membership is tagged and in what order it resolves, not what a Link
itself stores. `§19.30`'s `MarkerLinks` wire array shape is unchanged. `§19.32`'s
`scaleSelected*`/select-color fields are unrelated and untouched. The Bundle-tier "no `bLocked`-
equivalent field" ruling (`§19.31`) is unchanged and does not extend to asking whether `MarkerTransform`
needs one either — it already has one now, per this section.
