[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.31. **Only the ARCH Expert writes this file.**

> **⚠️ FURTHER CORRECTED 2026-08-31 — see `ARCH_19_33_LinkMembershipInstanceTierCorrection.md`.**
> This section's resolver contract (the `Effective*` functions below) is **widened, not replaced**:
> six of the seven governed fields below (every one except `name`) now resolve against the owning
> `MarkerTransform::linkIdentifier` FIRST, falling back to this section's own Layer-tier resolution
> (unchanged, described below) only when the instance itself carries no tag. `name` is explicitly
> excluded from that widening — see `§19.33`'s own reasoning (a `MarkerTransform`'s `name` is the
> marker's own proper identity, e.g. "Mex 0", not a Section/Group row label, and has no coherent
> "instance tag" target to resolve into). `§19.33` also restates Delete-Link semantics (a third walk,
> clearing `MarkerTransform::linkIdentifier`) and corrects `ApplyAddLinkAction` to stop minting any
> `MarkerLayerBundle`/`MarkerInstanceLayer` at all. This file's own text is left unedited beneath this
> notice for historical record of the 2026-08-31 same-day correction/amendment described below —
> `§19.33` is authoritative on any conflict with what follows.

### 19.31 Propagated-property ruling — ONE uniform mechanism (read-and-resolve, master/slave) — CORRECTED 2026-08-31 by direct human ruling, superseding this section's original two-mechanism text — FURTHER AMENDED 2026-08-31 to add `bLocked` to the governed field set (see the follow-up note immediately below and the inline additions throughout)

**This is a correction to a previously ratified rule, not a first ratification.** The original text
(preserved at the bottom of this file, "SUPERSEDED") ruled two genuinely different mechanisms —
read-and-resolve for color-override/`bHidden`, a separate one-shot cascade-write-on-rename for
Name. **Direct human ruling overturns that split.** The human's own words, ground truth, not
reinterpreted: *"A link should have operated the same as a section and had all same functions
including hide... when a group is part of a link, the link is the 'master' and the Section Group
associated with the link is the 'slave' — the settings on a linked group would be disabled."*

**Follow-up amendment, same day.** This section's first pass left `bLocked` explicitly un-ruled
(quoted verbatim below, in the "Explicitly NOT re-opened"/un-ruled record further down) because
neither the human's correction note nor that task named it. A second, direct human ruling — relayed
verbatim via a peer session — closes that gap: *"Everything should be cascaded down to the Groups in
the Link."* `bLocked` is now IN scope, governed by the exact same read-and-resolve/master-slave
mechanism as `bHidden`/`iconScale`/grid-snap/symmetry — mandatory-when-linked, no per-field enable
flag, identical to the design choice already made for those six fields. Every place below that lists
the governed field set, the `Params::MarkerLink` struct, the resolver surface, and the UI-composition
implication has been updated in place to include it (this is additive text over the already-corrected
ruling above, not a second reversal — the read-and-resolve mechanism itself is unchanged, only the
governed field COUNT grows from six to seven).

#### The uniform rule
While `linkIdentifier >= 0` at a given tier, **every** Section/Group-equivalent setting that tier
carries goes fully inert (UI control disabled) and **mirrors the Link's own value live, by
read-and-resolve** — never independently editable, never written back up, and never seeded by a
one-shot copy. This applies identically to every governed field, **including Name** — Mechanism B
(the original design/ratification's cascade-write-on-rename) is **retracted for every field, not
just narrowed**. There is no longer a "Name is different because it has no resolve-elsewhere
concept" carve-out: Name gets exactly the same resolve-elsewhere concept every other governed field
already has, because the Link itself is now the concept it resolves from.

**Why read-and-resolve, restated (unchanged from the original ruling's own reasoning, now applied
uniformly rather than partially):** write-through-and-copy duplicates state in N+1 places (the Link
plus every bound Group/Layer) with an explicit re-sync step on every edit — the identical
"don't duplicate membership/state truth in two places that can drift" objection already used to
reject a forward-reference shape for Assembly (`ARCH_19_05_AssemblyReferencesBundle.md`). Applying
this to Name too is not a new argument — it is the same argument the original ruling withheld from
Name for a reason ("Name is always freely editable, never inert") that the human's own "slave... the
settings on a linked group would be disabled" framing directly contradicts: the human's model has
Name behaving like every other master/slave setting, not as new-source-of-truth precedent.

#### Governed fields, by tier — every field lives at whichever tier already carries it (§19.29's
already-established independent two-tier `linkIdentifier`, applied consistently rather than
special-cased)

**Bundle tier (`MarkerLayerBundle`, `linkIdentifier >= 0`):**
- `name` — the ONLY field `MarkerLayerBundle` has that this ruling governs (no color/hidden/
  iconScale/grid-snap/symmetry/lock field exists on `MarkerLayerBundle` at all — those are
  Layer-tier fields, see below, and `MarkerLayerBundle` never gains parallel copies of them just to
  have something to resolve; §3.1's original finding that the human's "Group" mental model spans
  both tiers still holds and is not reopened here). **`bLocked` is explicitly confirmed NOT to join
  this list — see the dedicated ruling below, "Bundle-tier lock — ruled: no equivalent field."**

**Layer tier (`MarkerInstanceLayer`, `linkIdentifier >= 0`) — every governed setting that tier
carries:**
1. `name` — the Layer's own row label. Same read-and-resolve posture as the Bundle-tier `name`
   above; a Layer created directly under a Link-bound Bundle gets a bound `name` too (§19.29's
   independent-per-tier convention), not a derived one.
2. `bColorOverrideEnabled` + `color[4]` — unchanged in substance from the original ruling's
   Mechanism A; still read-and-resolve, now merely regrouped under one uniform label instead of a
   named "Mechanism A."
3. `bHidden` — unchanged in substance from the original ruling (it already correctly ruled `bHidden`
   propagates); the original text's "this is a genuine exception to pure read-and-resolve... the
   cascade is a write" framing is **retracted**. `bHidden` is pure read-and-resolve like every other
   field now — the earlier "no `bHidden` field on `MarkerLink`, hide is expressed as a cascading
   action" design is superseded (see the new field below).
4. `iconScale` — the human's "all same functions" is not a narrow pointer at color/visibility; icon
   size is as much a Section/Group-level "function" as color or visibility, with no principled
   reason to exclude it once the master/slave framing is taken at face value. Read-and-resolve: a
   bound Layer's icon-size control goes inert, displays the Link's own `iconScale`.
5. `bGridSnapEnabled` + `gridSnapSizeWorldUnits` — same reasoning as `iconScale`. Both fields
   together (the pair is one functional unit — a size with no enabling toggle is meaningless and
   vice versa), read-and-resolve as a pair.
6. `bSymmetryEnabled` + `symmetry` (the `Params::SymmetrySetting` sub-record) — same reasoning.
   Read-and-resolve as a pair, mirroring `ResolveEffectiveMarkerSymmetry`'s own existing "resolve at
   read time" idiom (`MarkersTab_ManualLayerHelpers_UI.h`) — this ruling adds a Link-resolution step
   ahead of that function's existing global/per-layer resolution, not a parallel mechanism.
7. **`bLocked`** — NEW (follow-up amendment, 2026-08-31, direct human ruling "everything should be
   cascaded down to the Groups in the Link"). Confirmed real per-Layer "function" (blocks
   drag/reposition/add/remove for every marker on the Layer, `MarkerInstanceLayer::bLocked`,
   `MarkerInstance_PARAMS.h:42`, landed STEP106). Read-and-resolve, mandatory-when-linked, no
   per-field enable flag — identical mechanism and identical design-choice reasoning as items 4–6
   above: the Lock toggle is not a propagation-gate layered on top of some other value, it IS the
   substantive on/off state the Layer already exposes, so once linked the Link owns it outright and
   the Layer's own Lock control goes fully inert (disabled), mirroring the Link's own value.

**Explicitly NOT re-opened by this correction, still independent per-Group exactly as the original
ruling found, restated for the record — no rationale for cross-type propagation was offered or
exists for any of these:** `parentBundleIdentifier` (per-type nesting position — structural, not a
user-facing "setting" in the Section/Group sense the human's ruling addresses), `markerTypeName`
(definitionally different per Group by construction), and membership itself (the entire point of a
*per-type* Group is to hold that type's own instances — a Link spans types precisely by having one
Group *per* type, each with its own distinct membership). **⚠️ Membership itself is reopened by
`§19.33`, not this section** — a Link's real membership is now per-instance tagging on
`MarkerTransform`, not per-type Group membership; this sentence's own "a Link spans types by having
one Group per type" framing describes the RETRACTED mechanism, see `§19.33`.

**`bLocked` is no longer in the "left un-ruled" state.** The original text's flag on this field
("Needs its own explicit human/ARCH ruling before a coder treats it either way") is now resolved by
the follow-up ruling above (governed-field item 7). The original flagging text is preserved verbatim
in the SUPERSEDED-adjacent historical note at the end of this section's "Governed fields" material
for the record of how the question was raised, but it is no longer live law — item 7 above governs.

#### Bundle-tier lock — ruled: no equivalent field (answers the follow-up task's open question)
**`MarkerLayerBundle` does NOT get a `bLocked`-equivalent field, and locking stays a Layer-tier-only
concept.** Checked against ground truth before ruling, per the Constitution's "do not guess": grepping
`MarkerLayerBundle_PARAMS.h` shows its full field set is `identifier`, `name`,
`parentBundleIdentifier`, `markerTypeName`, `assemblyIdentifier`, `linkIdentifier` — **no lock-like
field exists on `MarkerLayerBundle` today**, and no other section of this ARCH has ever proposed one.

This is decided by the exact precedent this section already set for `iconScale`/grid-snap/symmetry,
**not** the precedent set for `name`. `name` needed independent resolvers at both tiers because
BOTH `MarkerLayerBundle` and `MarkerInstanceLayer` already, structurally, independently carry their
own pre-existing `name` field — two real fields, one ruling, applied at both places it already lives.
`bLocked`, like `iconScale`/`bGridSnapEnabled`/`gridSnapSizeWorldUnits`/`bSymmetryEnabled`/`symmetry`,
exists on exactly one tier (`MarkerInstanceLayer`) and nowhere else — and this section's own governing
principle is explicit that a governed field "lives at whichever tier already carries it," with the
Bundle-tier text above stating outright that `MarkerLayerBundle` "never gains parallel copies of
[Layer-tier fields] just to have something to resolve." Locking, like color/icon-size/grid-snap/
symmetry, is a per-Layer authoring concern (it blocks drag/reposition/add/remove for the markers ON
that layer) with no existing "lock this whole nested Bundle of Layers" concept in the codebase or in
any ratified design — inventing a Bundle-tier lock now, solely to have a second thing to resolve,
would be exactly the anti-pattern this section already forbade for the other four Layer-only fields.
If a future design wants Bundle-level locking as its own product feature, that is a new PARAMS field
and a new ARCH ruling on its own merits, not an automatic consequence of this Link-propagation
correction.

#### `Params::MarkerLink` field additions — mandatory-when-linked, no per-field enable flag

**Design choice: every governed field is added to `MarkerLink` as a plain mirror of its Layer-tier
counterpart's own shape — NOT gated behind a second, new "does this field propagate" enable flag.**
Reasoning: `bColorOverrideEnabled`, `bGridSnapEnabled`, and `bSymmetryEnabled` are not
propagation-gates layered on top of some other value — they ARE the substantive on/off state the
Layer itself already exposes (whether color is overridden at all, whether grid-snap is on, whether
symmetry is on). Once linked, the Link owns that same substantive state outright; inventing a
parallel "is this field under Link control" flag on top would (a) contradict the human's own
"disabled" framing — a slave has no partial-control knob, its whole setting is inert — and (b) add a
mechanism the "all same functions"/"everything...cascaded" instructions never asked for. `MarkerLink`
becomes, functionally, a peer struct carrying the same setting fields `MarkerInstanceLayer` carries
(minus the structural/membership fields §19.29 and this section already keep independent), consumed
identically by the Section/Group UI a Link now IS one of. `bLocked` is added on that identical basis
in the follow-up amendment below — it is exactly as substantive and exactly as binary as the other
six, no different treatment warranted.

```cpp
// MarkerLink_PARAMS.h — Params::MarkerLink, amended. `identifier`/`name`/`bColorOverrideEnabled`/
// `color[4]` unchanged from ARCH_19_28. Fields below added by the ARCH §19.31 correction,
// 2026-08-31 (original correction) and its same-day follow-up amendment (`bLocked`), each a plain
// mirror of its MarkerInstanceLayer counterpart's own default:
struct MarkerLink {
    int identifier               = -1;
    std::string name;
    bool  bColorOverrideEnabled  = false;
    float color[4]               = {1.0f, 1.0f, 1.0f, 1.0f};

    bool  bHidden                 = false;   // retracts ARCH_19_28's "no bHidden field, hide
                                              // is a cascading action" text; bHidden is a plain
                                              // read-and-resolve field like color, requiring the
                                              // field to exist here as the resolve-from source.
    float iconScale                = 1.0f;   // mirrors MarkerInstanceLayer::iconScale.
    bool  bGridSnapEnabled         = false;  // mirrors MarkerInstanceLayer::bGridSnapEnabled.
    float gridSnapSizeWorldUnits   = 1.0f;   // mirrors MarkerInstanceLayer::gridSnapSizeWorldUnits.
    bool  bSymmetryEnabled         = true;   // mirrors MarkerInstanceLayer::bSymmetryEnabled.
    Params::SymmetrySetting symmetry;        // mirrors MarkerInstanceLayer::symmetry. Requires
                                              // MarkerLink_PARAMS.h to #include "Symmetry_PARAMS.h".
    bool  bLocked                  = false;  // NEW — follow-up amendment 2026-08-31. Mirrors
                                              // MarkerInstanceLayer::bLocked. Governs the same
                                              // read-and-resolve/master-slave mechanism as every
                                              // field above; no MarkerLayerBundle-tier counterpart
                                              // exists to mirror (see "Bundle-tier lock" ruling above).
};
```
`ARCH_19_28_MarkerLinkParamsType.md`'s struct listing and its "no `bHidden` field... deliberate
asymmetry" paragraph are superseded by this text; that file is not rewritten in place (kept for
historical record of the original ratification) but this section's text governs on conflict.

#### Resolver function surface required (read-and-resolve, one getter per governed field/pair,
mirroring the already-shipped `EffectiveManualMarkerLayerColor`/`...ColorOverrideEnabled` shape —
not a new pattern, an extension of the existing one to every governed field)

**⚠️ Widened again by `§19.33`** — six of the seven getters below (every one except
`EffectiveMarkerLayerBundleName`/`EffectiveManualMarkerLayerName`) gain a THIRD, instance-tier
resolution step consulted BEFORE the two steps shown here — see `§19.33` for the exact three-tier
contract and the new three-parameter resolver siblings. The two-parameter shapes below remain valid
for any call site that only has a Layer/Bundle in hand (no specific instance) — they are not deleted.

```cpp
// Bundle tier — new, no prior "Effective*" resolver existed at this tier before this correction:
const std::string& EffectiveMarkerLayerBundleName(const Params::MarkerLayerBundle& bundle,
                                                   const std::vector<Params::MarkerLink>& links);

// Layer tier — EffectiveManualMarkerLayerColorOverrideEnabled/EffectiveManualMarkerLayerColor
// already exist (STEP239) and need no change in shape, only continued use. New siblings:
const std::string& EffectiveManualMarkerLayerName(const Params::MarkerInstanceLayer& layer,
                                                   const std::vector<Params::MarkerLink>& links);
bool  EffectiveManualMarkerLayerHidden(const Params::MarkerInstanceLayer& layer,
                                       const std::vector<Params::MarkerLink>& links);
float EffectiveManualMarkerLayerIconScale(const Params::MarkerInstanceLayer& layer,
                                          const std::vector<Params::MarkerLink>& links);
bool  EffectiveManualMarkerLayerGridSnapEnabled(const Params::MarkerInstanceLayer& layer,
                                                const std::vector<Params::MarkerLink>& links);
float EffectiveManualMarkerLayerGridSnapSizeWorldUnits(const Params::MarkerInstanceLayer& layer,
                                                       const std::vector<Params::MarkerLink>& links);
bool  EffectiveManualMarkerLayerSymmetryEnabled(const Params::MarkerInstanceLayer& layer,
                                                const std::vector<Params::MarkerLink>& links);
const Params::SymmetrySetting& EffectiveManualMarkerLayerSymmetry(
    const Params::MarkerInstanceLayer& layer, const std::vector<Params::MarkerLink>& links);
// NEW — follow-up amendment, 2026-08-31:
bool  EffectiveManualMarkerLayerLocked(const Params::MarkerInstanceLayer& layer,
                                       const std::vector<Params::MarkerLink>& links);
// Every resolver: linkIdentifier >= 0 and resolves -> the Link's own field; else (unbound, or a
// dangling identifier, Constitution §6 soft-degrade) -> the Layer's/Bundle's own field, unchanged
// from today. Identical linkIdentifier-lookup shape for every one of these — a small shared
// private lookup-by-identifier helper is a legitimate casual-pass dedup, not a new pattern
// question; left to the coder ticket.
```

#### UI composition implication
Every corresponding control gains the disabled-while-linked treatment already shipped for color
(`ImGui::BeginDisabled(state.bUseGroupColor || layer.linkIdentifier >= 0)`,
`MarkersTab_ManualLayerRowBody_UI.cpp`) — the same `|| <tier>.linkIdentifier >= 0` clause extends to
the rename control (Bundle tier AND Layer tier), the `bHidden` visibility toggle
(`MarkersTab_ManualLayers_UI.cpp`'s `ApplyLayerListSignal`/row-building), the Icon-Size control
(`DrawMarkerLayerIconSizeHeaderControl`), the Grid-Snap control
(`DrawMarkerLayerGridSnapHeaderControl`), the Symmetry toggle
(`DrawMarkerLayerSymmetryToggleHeaderControl`), and — NEW, follow-up amendment 2026-08-31 — the Lock
toggle (wherever `MarkerInstanceLayer::bLocked` is currently drawn/toggled) — each reading its own
`Effective*` resolver instead of the raw field, exactly as color already does. The Link's own tier
(§19.30's Links UI) remains the one editable surface for every one of these settings while a
Group/Layer is bound to it — bound directly to the corresponding new `Params::MarkerLink` field,
mirroring the existing `DrawMarkerLinkColorOverrideHeaderControl` shape for each new field, including
`bLocked`. **⚠️ `§19.33` widens this**: per-instance rows (in the Links-Section body and the
Marker-Type-section instance list alike) gain the SAME disabled-while-linked treatment keyed off
`transform.linkIdentifier >= 0`, one tier further down, for the six fields listed above.

#### Delete-Link semantics — unchanged in shape, restated because it now covers more fields
Deleting a `MarkerLink`: for every `MarkerLayerBundle`/`MarkerInstanceLayer` with matching
`linkIdentifier`, clear it to `-1` (ungroup the LINK relationship only — the Group/Layer/instances
themselves are untouched). Every governed field on the now-unbound Bundle/Layer simply reads from
its OWN stored value again, unchanged from whatever it last held before being bound (read-and-
resolve never copied anything onto it, so there is nothing to "restore" for any of the newly-added
fields either — the same correct, unsurprising consequence the original ruling already established
for color, now uniform across every field, `bLocked` included). Erase the `Params::MarkerLink` entry
itself last. No instance, Layer, or Group is ever erased by this action. **⚠️ `§19.33` adds a third
walk** — clearing `MarkerTransform::linkIdentifier` to `-1` for every instance tagged with the
deleted Link, on the identical "ungroup only, never erase" basis — since that field is now the
PRIMARY membership record going forward (the Bundle/Layer-tier walks above are retained for
backward-compat cleanup of any legacy-mechanism data only, see `§19.33`).

---

### [SUPERSEDED 2026-08-31 — see the corrected ruling above; kept for historical record only, not binding]

The original ratified text below split propagation into two mechanisms (read-and-resolve for
color/`bHidden`, a one-shot cascade-write-on-rename for Name) and explicitly excluded `iconScale`/
grid-snap/symmetry from propagation. Its first correction pass (same day) also left `bLocked`
explicitly un-ruled and flagged for a future decision — that flag has since been resolved by the
follow-up amendment above (`bLocked` is now governed, item 7 of the Layer-tier list). **All of this
is retracted by direct human ruling; do not implement anything below.**

<details>
<summary>Original text (retracted)</summary>

Responds to `work_orders/DESIGN_MarkerLink_R1.md` §3.4/§5 item 4. Ratified two distinct mechanisms
— read-and-resolve for color-override/`bHidden`, a separate one-shot cascade-write-on-rename for
Name — and ruled `iconScale`/`bGridSnapEnabled`/`gridSnapSizeWorldUnits`/`bSymmetryEnabled`/
`symmetry` stay independent per-Group, never propagated. See git history for the original file
contents if the exact original wording is ever needed; not reproduced verbatim here to avoid a
stale duplicate of law that no longer applies.

</details>
