# Design Output — Markers Tab Selection Actions & Link Grouping, Round 1

Continuation of `BRIEF_MarkerLink_R1.md` (read that first — its "already confirmed" section is
ground truth, not re-derived here). **FORMALLY RATIFIED** — `ARCH_19_28_MarkerLinkParamsType.md`
through `ARCH_19_32_MarkerSelectedScaleFields.md` confirm this design as-is (no corrections against
real code). All four items (§1-§4) are now coder-dispatchable; see `work_orders/STEP234-240` for
the cut tickets. Structured to answer the brief's four numbered asks in order, plus an explicit
scoping ruling (§0) the brief asked to be settled outright, mirroring `DESIGN_Assembly_R1.md`'s own
shape throughout.

## 0. Scoping ruling — Link membership is manual-instances-only (RULED, not left open)

**Confirmed, not a new rule — same reasoning as `DESIGN_Assembly_R1.md` §0 and `ARCH_19_09_ManualOnlyMembership.md`, applied one more time at the identical layer boundary.** A Link tags `MarkerLayerBundle`/`MarkerInstanceLayer` entries and `MarkerTransform` instances — all hand-authored, IO-round-tripped, PARAMS-resident. Procedural marker instances live in `Data::PlacementInstances`, PROC-regenerated every bake with no cross-bake stable identity (`ARCH_14_08_DirtyFlagTiers.md`; `ARCH_19_20_ManualOnlySelectionScope.md`'s still-binding sentence: `instanceIdentifier` is never repurposed for procedural identity). A persisted `linkIdentifier` needs exactly the stable identity `MarkerTransform::instanceIdentifier` (`ARCH_19_16_InstanceIdentifier.md`) already gives manual instances and procedural instances categorically lack. Not open.

**Corollary, stated explicitly since §3 depends on it:** the "+Link" button's cross-type selection source is `state.selectedManualInstanceIdentifiers` (`MarkersTab_UI.h:131`) — the same plural field item 1's Delete and item 2's reassignment both read — never the procedural per-frame `ruleIndex` selection (`ARCH_19_27_ProceduralInstanceSelectionMechanism.md`). If the live selection is a mix of manual and procedural entries (possible today — canvas/list selection converged onto one `OverlayInstanceKey_UI` representation, `ARCH_19_25_SelectionRepresentationUnification.md`), "+Link" (and Delete, §1) act on the manual subset only, silently ignoring any procedural member of the touched set — the same "soft, not a refusal" posture Constitution §6 already governs every other cross-tier mismatch in this format with.

## 1. Delete key — universal, shell-level, all three manual domains (REVISED — coder-dispatchable now, no ARCH blocker)

**Revised per direct human pushback**: *"The delete key needs to be a universal global thing... not just setup for markers."* Correct, and grounded in what's already shipped — `OverlayInstanceKeySet_UI` (`ARCH_21_01_MultiSelectRepresentation.md`) is *already* a unified Markers/Props/Decals selection surface; scoping Delete to `ApplicationPanel::Markers` would have been narrower than the selection model it reads from. This section supersedes the original Markers-only version in full.

### 1.1 Do Props/Decals have any existing per-instance delete mechanism? — No, live or dead

**Confirmed by direct read/grep: neither Props nor Decals has ANY per-instance delete affordance, live or dead — not even the dead-code equivalent of Markers' `DrawMarkerInstanceListButtons`.** What exists for Props/Decals is strictly **Layer-tier** delete, one tier up from an instance:
- `PropsTab_Bundles_UI.h/.cpp`: `DeletePropInstanceLayerOnly`/`DeletePropInstanceLayerCascade` (`PropsTab_Bundles_UI.cpp:63-80`) — erases a `PropInstanceLayer` row and either clamps or cascade-erases the transforms that referenced it. Mirrors `DeleteMarkerInstanceLayerOnly`/`Cascade` exactly.
- `DecalsTab_Bundles_UI.h/.cpp`: `DeleteDecalInstanceLayerOnly`/`DeleteDecalInstanceLayerCascade` (`DecalsTab_Bundles_UI.cpp:58-75`) — same shape.
- `ManualPropLayersState`/`ManualDecalLayersState` (`PropsTab_Manual_UI.h`) carry `selectedLayerIndex` — a **Layer** selection, not an instance one. There is no `selectedManualInstanceIdentifier(s)`-equivalent field anywhere in the Props or Decals tab state.
- Confirmed independently by `ARCH_21_01_MultiSelectRepresentation.md`'s own text: Props/Decals selection lives **only** in `MapCanvas`'s `OverlayInstanceKeySet_UI` today, with no tab-list-side mirror to keep in sync (unlike Markers' STEP141 `selectedManualInstanceIdentifiers`). This actually *simplifies* the delete design (§1.3) — there is no second, tab-local field to re-clamp for Props/Decals, only for Markers.

### 1.2 Is there a reusable cross-domain "erase by identifier" primitive? — Not yet, but the exact shape already exists as precedent, twice

**No existing erase-by-identifier primitive for any of the three domains** — Markers' own dead code erases by raw vector position (`MarkersTab_ManualInstance_UI.cpp:66-71`), not by `instanceIdentifier`, and nothing analogous exists for Props/Decals per §1.1.

**But the exact genericity shape this needs is already ratified and shipped, twice over, for the adjacent hit-test/region-select problem** (`ARCH_21_03_DragGestureGenericization.md`, confirmed live: `ManualInstanceHitTest_UI.h/.cpp`, `MapCanvas_SelectionGesture_UI.cpp`, `MapCanvas_ManualDragDispatch_UI.cpp` all exist and are wired):

```cpp
// ARCH_21_03's own already-shipped shape — plain (non-Traits) templates, because the algorithm
// touches only fields name-identical across all three group types:
template<typename GroupT>
bool HitTestManualInstances(const std::vector<GroupT>& instances, ...,
                            const std::function<bool(int layerIndex)>& isLayerLocked, ...);
template<typename GroupT>
void CollectManualInstancesInWorldRegion(const std::vector<GroupT>& instances, ...,
                                         const std::function<bool(int layerIndex)>& isLayerLocked, ...);
```

Erase-by-`instanceIdentifier` is **strictly simpler than hit-test** — it touches only `.transforms[].instanceIdentifier` and `.transforms[].layerIndex` (for the lock check), fields confirmed name-identical across `MarkerTransform`/`PropTransform`/`DecalTransform` since `ARCH_21_04_PropDecalInstanceIdentityFields.md` landed `instanceIdentifier` on all three. Per `ARCH_19_02_GenericitySplit.md`'s own standing rule — pure container/graph/UI mechanics with zero domain-field access gets one shared C++ template — this is squarely in the "one shared template" bucket, not the "three independent per-domain bodies" bucket `ResolvePropInstanceLayerId`/`ResolveDecalInstanceLayerId` occupy.

**Proposed, mirroring `HitTestManualInstances<GroupT>`'s own signature convention exactly, new sibling file `ManualInstanceDelete_UI.h`** (kept separate from `ManualInstanceHitTest_UI.h` — that file is query-only; this one mutates, a real file-boundary reason, not arbitrary):
```cpp
// Erases every transform across `instances` whose instanceIdentifier is in `identifiers` AND whose
// owning layer is NOT locked (isLayerLocked(layerIndex) == false) — mirrors §21.5's own
// predicate-injection idiom for the identical class of question ("does a lock gate this mutation").
// Returns the count actually erased (locked/missing identifiers are silently skipped, Constitution
// §6 — a partial delete due to a locked member is a soft degrade, not a refusal of the whole batch).
template<typename GroupT>
int DeleteManualInstancesById(std::vector<GroupT>& instances, const std::vector<int>& identifiers,
                              const std::function<bool(int layerIndex)>& isLayerLocked);
```
Per-domain thin wrappers, mirroring `HitTestManualMarkers`'s own "one-line wrapper over the template" closing convention (§21.3):
```cpp
int DeleteSelectedManualMarkerInstances(std::vector<Params::MarkerInstanceGroup>& markers,
                                        const std::vector<int>& identifiers);   // binds IsMarkerInstanceLayerLocked
int DeleteSelectedManualPropInstances(std::vector<Params::PropInstanceGroup>& props,
                                      const std::vector<int>& identifiers);     // binds IsPropInstanceLayerLocked
int DeleteSelectedManualDecalInstances(std::vector<Params::DecalInstanceGroup>& decals,
                                       const std::vector<int>& identifiers);    // binds IsDecalInstanceLayerLocked
```
`IsPropInstanceLayerLocked` (`PropsTab_Manual_UI.h:102-106`) and `IsDecalInstanceLayerLocked` (`DecalsTab_Manual_UI.h:78`, cited by `ARCH_21_05_LockedItemExclusionCorrection.md`) already exist and are exactly the predicates §21.5 already uses for hit-test/marquee lock-gating — reused here unchanged, not invented.

**Lock-gating the delete itself — a reasoned call, not routed as a question.** §21.5 rules locking gates *acquiring* selection, not an active-selection audit — an instance selected before its layer locked stays selected and still draws its select-tint. Whether Delete should still *refuse* to erase such an instance is a fresh question §21.5 doesn't answer directly. Recommendation: **yes, gate it** — deletion is a strictly more destructive mutation than drag-begin, which §21.5 already refuses for a locked layer; silently deleting a "protected" instance because it happened to be selected before the lock was applied would be a worse surprise than refusing. This mirrors the existing predicate-injection idiom exactly (§21.5's own mechanism), not a new concept.

### 1.3 Keypress-poll site — shell-level, `Application::RunOneFrame()`, not any one tab's draw call

**No `tabState.activePanel` gate at all.** Confirmed by direct read of `Application_Frame_UI.cpp:35-57` — `RunOneFrame()` is the one place that already sits outside every tab's own draw call, calling `DrawSettingsWindow()` (draws whichever tab is active) and `DrawCanvasWindow()` (the canvas, tab-independent) as two peer steps. A new private method, called from here, is domain-agnostic and tab-agnostic by construction — reading only `canvas.SelectedInstanceKeys()`, never `tabState.activePanel`:

```cpp
// Application_Draw_UI.cpp (or a new small Application_DeleteKey_UI.cpp, ARCH §1.5 sizing left to
// the coder) — new private method, declared in Application_UI.h alongside DrawCanvasWindow().
void Application::ApplyGlobalDeleteShortcut() {
    if (ImGui::GetIO().WantTextInput) return;              // a live rename/text field wins, unchanged
    if (scenarioEditMode.IsActive()) return;                // mirrors DrawCanvasWindow's own existing
                                                             // exclusivity gate (Application_Draw_UI.cpp:51) —
                                                             // Scenario Edit Mode owns canvas interaction
                                                             // exclusively; Delete is a canvas-selection
                                                             // action, so it defers the same way pan/drag do.
    if (!ImGui::IsKeyPressed(ImGuiKey_Delete)) return;

    const OverlayInstanceKeySet_UI& selected = canvas.SelectedInstanceKeys();
    std::vector<int> markerIdentifiers, propIdentifiers, decalIdentifiers;
    for (const OverlayInstanceKey_UI& key : selected.keys) {
        if (!key.bManual) continue;   // procedural instances have no persisted identity — §0's corollary
        switch (key.collection) {
            case PlacementCollectionKind_UI::Markers: markerIdentifiers.push_back(key.instanceIndex); break;
            case PlacementCollectionKind_UI::Props:   propIdentifiers.push_back(key.instanceIndex);   break;
            case PlacementCollectionKind_UI::Decals:  decalIdentifiers.push_back(key.instanceIndex);  break;
            default: break;   // Units — out of scope, mirrors §21's own closing note
        }
    }
    if (markerIdentifiers.empty() && propIdentifiers.empty() && decalIdentifiers.empty()) return;

    bool bAnyDeleted = false;
    if (!markerIdentifiers.empty())
        bAnyDeleted |= DeleteSelectedManualMarkerInstances(recipe.markers, markerIdentifiers) > 0;
    if (!propIdentifiers.empty())
        bAnyDeleted |= DeleteSelectedManualPropInstances(recipe.props, propIdentifiers) > 0;
    if (!decalIdentifiers.empty())
        bAnyDeleted |= DeleteSelectedManualDecalInstances(recipe.decals, decalIdentifiers) > 0;
    if (!bAnyDeleted) return;

    canvas.ClearSelection();   // NEW small public method, §1.4 — the staleness hazard
                               // DESIGN_Assembly_R1.md §2 already flagged for a held index-based
                               // selection surviving a structural mutation underneath it
    tabState.markers.selectedManualInstanceIdentifier = -1;             // Markers' own tab-local
    tabState.markers.selectedManualInstanceIdentifiers.clear();         // mirror — Props/Decals have
    tabState.markers.manualInstanceSelectionAnchorIdentifier = -1;      // no such mirror to clear (§1.1)
    previewDriver.NotifyParametersChanged();
}
```
Called from `RunOneFrame()` **after both `DrawSettingsWindow()` and `DrawCanvasWindow()`**, before `EndImguiFrame()` — so `io.WantTextInput` reflects every widget either draw call submitted this frame, not a partial state:
```cpp
DrawSettingsWindow();
ApplyExecutionPolicy();
ResolveIconSelections();
ServiceDirtyTier();
DrawCanvasWindow();
ApplyGlobalDeleteShortcut();   // NEW
EndImguiFrame();
```

### 1.4 New small public API surface needed — `MapCanvas::ClearSelection()`

**Gap found while grounding this**: `MapCanvas::ApplySelectionGesture` (the canonical selection-mutation entry point, `ARCH_21_01`) is **private** — the shell has no existing public way to clear `selectedInstanceKeys` after a delete. Leaving a stale key in place is not safe: it is exactly the index-based-staleness hazard `DESIGN_Assembly_R1.md` §2 already named ("deleting/inserting... shifts every later index... can silently point at the wrong instance"). Proposed, mirroring `MapCanvas::SetSelection`'s own "public method thinly wraps the private canonical entry point" shape (`MapCanvas_UI.h:336`) exactly:
```cpp
// MapCanvas_UI.h — new public method, one line
void ClearSelection() { ApplySelectionGesture(std::vector<OverlayInstanceKey_UI>{}, false, false); }
```
Trivial, additive, exposes existing private machinery under a new name — no new mechanism.

### 1.5 What's still scoped out, restated

Delete still targets **instances only**, manual-only (§0's ruling, unchanged) — it does not delete a selected Group/Layer/Bundle (those already have their own explicit "X" delete affordance in every domain's own Bundle tree, `DeleteMarkerLayerBundleCascade`/`DeletePropInstanceLayerCascade`/`DeleteDecalInstanceLayerCascade` and friends) and does not touch procedural instances (no persisted identity, §0). This is a scope boundary, not a gap.

**Revised ARCH flag for §1** (still no blocker, casual-pass tier only): `DeleteManualInstancesById<GroupT>` and its three per-domain wrappers mirror `HitTestManualInstances<GroupT>`'s already-ratified genericity shape near-verbatim — same casual naming/placement pass `ARCH_19_02` already blesses this class for, not a new-PARAMS question. `MapCanvas::ClearSelection()` is the same tier. The lock-gates-delete recommendation (§1.2) is a reasoned interaction-design call, worth a one-line ARCH confirmation since it extends §21.5's precedent to a new mutation kind it didn't originally cover. **Still coder-dispatchable as one ticket** — no new PARAMS, no new wire shape, no new stored concept; every field/function this section reads or extends already exists and is already ratified (§21.1/§21.3/§21.4/§21.5).

## 2. "+ Group" / "+ Layer" move current selection into the new container — coder-dispatchable now, no ARCH blocker

**Scoped to a same-Marker-Type selection, per the brief's own explicit carve-out** (cross-type is item 3/Link, not this). Concretely, in `MarkersTab_UI.cpp`'s existing `buttons.bAddGroupClicked`/`buttons.bAddManualLayerClicked` branches (lines 319-339):

```cpp
if (buttons.bAddGroupClicked) {
    // ...existing bundle.identifier/markerTypeName/parentBundleIdentifier construction, unchanged...
    recipe.markerLayerBundles.push_back(bundle);
    state.bundles.selectedBundleIdentifier = bundle.identifier;
    // NEW: if the current selection is non-empty AND entirely this type, move it into a fresh
    // Manual Layer under the new Group (a bare Group has no direct Instance membership of its own,
    // §148's own already-shipped "first Manual Layer" convention, MarkersTab_Bundles_UI.h:142-150 —
    // reused here, not reinvented: a NEWLY created Group always creates its first Layer alongside it
    // rather than deferring to the pending-create-on-drop mechanism that exists ONLY because a drop
    // target can't safely push_back mid-tree-walk; a button click has no such constraint).
}
if (buttons.bAddManualLayerClicked) {
    // ...existing layer.name/layerId/parentBundleIdentifier/markerTypeName construction, unchanged...
    recipe.markerLayers.push_back(layer);
    state.manualLayers.selectedLayerIndex = static_cast<int>(recipe.markerLayers.size()) - 1;
    // NEW: if the current selection is non-empty AND entirely this type, reassign it onto the new
    // Layer directly.
}
```

**"Entirely this type" test — new, small helper, needed before either branch above can gate correctly:**
```cpp
// MarkersTab_ManualInstanceSelection_UI.h — new function
// True only when every one of `selectedIdentifiers` resolves to a transform whose OWN group,
// folded through Params::CanonicalMarkerTypeSectionName (the same alias-folding
// DrawBaseSectionManualInstanceList/FindOrCreateMarkerInstanceGroupByName already apply,
// MarkersTab_UI.cpp:117-125,173), equals `typeName`. An empty selection is NOT "entirely this
// type" (callers gate the whole move on non-empty separately) — this predicate answers ONE
// question only: is a same-type reassignment legal, never "is there anything to reassign."
bool IsManualInstanceSelectionEntirelyType(const std::vector<Params::MarkerInstanceGroup>& markers,
                                           const std::vector<int>& selectedIdentifiers,
                                           const std::string& typeName);
```
When the selection spans types, both buttons still create the (now-empty) container exactly as today — no partial/silent move, no error dialog; the brief's own routing ("that's item 3, not this") is a UI-level "this button doesn't apply" no-op, not a refusal.

**Reassignment call, both branches, once the Layer exists (new or the Group's freshly-created first Layer):**
```cpp
ReassignManualInstanceLayers(recipe.markers, state.selectedManualInstanceIdentifiers, newLayerIndex);
```
Exactly `ReassignManualInstanceLayers`'s existing, already-shipped, pure, in-place contract (`MarkersTab_ManualInstanceSelection_UI.h:66-67`) — zero new PARAMS, zero new mechanism, the brief's own "reuse, don't invent" instruction satisfied literally.

## 3. The Link mechanic

### 3.1 What "Link" actually needs to be, given real gaps found by direct read

**Links must be a real, stored PARAMS concept — not a UI-derived tier like Type-sections. Confirmed by ARCH advisory ruling, not just this design pass's own reasoning.** ARCH's ruling: *"stored PARAMS concept, definitively — not a recommendation I'm merely accepting. §19.14's Type-section-is-UI-derived ruling turns on a specific fact that does not hold here: Type-sections dedupe an already-existing open string field (`markerTypeName`) with no data of their own. A Link is new source-of-truth state (name, id, color-override toggle+color) that exists nowhere else in the recipe — there is nothing to derive it from. §19.14's own reasoning therefore argues for a stored concept, exactly as the design states."* `ARCH_19_14_TypeSectionUiDerived.md` rules Type-sections UI-derived specifically *because* they dedupe an *already-existing* open-string field with no data of their own — a Link is the opposite case.

**A real, load-bearing gap: `MarkerLayerBundle` (the "Group") carries no color-override field of any kind today.** Confirmed by direct read of `MarkerLayerBundle_PARAMS.h:29-40` — its full field set is `identifier`/`name`/`parentBundleIdentifier`/`markerTypeName`/`assemblyIdentifier`. The color-override toggle+color the human describes ("Groups... would automatically have its override-color set... the override toggle would auto-toggle") lives one tier down, on `MarkerInstanceLayer::bColorOverrideEnabled`/`color[4]` (`MarkerInstance_PARAMS.h:25,53`, surfaced via `DrawManualMarkerLayerColorOverrideHeaderControl`, `MarkersTab_ManualLayerRowBody_UI.cpp:174-195`). The human's own mental model uses "Group" loosely for what the shipped Group→Layer→Instance hierarchy actually splits across two tiers — very plausible given how tightly the UI nests a Group with its one Layer today. Resolution below keeps the *tag* at the Group tier (organizational, matches the brief's own "creates a Group... carrying a back-reference to the Link's id") while keeping the *actual color-override read/write surface* where it already, correctly, lives — the Layer tier — rather than inventing a second, rival color-override field on the Bundle that would then need its own composition rule against the Layer's pre-existing one.

### 3.2 Propagation mechanism — RULED by ARCH advisory consult: read-and-resolve for color/visibility, a distinct one-shot cascade-write for Name

**Confirmed, not merely recommended.** ARCH's ruling: *"this is the identical 'don't duplicate membership/state truth in two places that can drift' objection already used to reject a forward-reference shape for Assembly, applied one tier over to state instead of membership. Read-and-resolve (not write-through-and-copy) is the correct mechanism for exactly that reason."* Write-through-and-copy would duplicate color-override state in N+1 places (the Link plus every per-type Group/Layer) with an explicit re-sync step on every edit — exactly the "duplicates membership truth in two places that can go out of sync" objection `ARCH_19_05_AssemblyReferencesBundle.md` already used to reject a forward-reference shape for Assembly, one tier over. Read-and-resolve needs no sync step, cannot drift, and is the exact shape this codebase already uses twice for adjacent problems:
- `Params::ResolvePropInstanceLayerId`-style shared resolvers (the brief's own citation).
- **A closer, UI-side precedent, confirmed by direct read: `EffectiveManualMarkerLayerColor`** (`MarkersTab_ManualLayerHelpers_UI.h:86-89`): `return state.bUseGroupColor ? state.groupColor : layer.color;` — the *exact* "resolve at read time, don't copy down" idiom, already composed with the SAME control (`DrawManualMarkerLayerColorOverrideHeaderControl`'s own `ImGui::BeginDisabled(state.bUseGroupColor)`, line 176) that Link's control needs to extend. `bUseGroupColor` itself is a different, ephemeral, session-only, single whole-tab toggle (not persisted, not named, not per-Link) — cited as the *shape* to reuse for composing "disable the per-Layer control, show the resolved-elsewhere value instead," not as the mechanism itself, which Link needs to be real, persisted, per-Link PARAMS.

### 3.3 Proposed `.sanmap`/PARAMS shape (strawman — names/casing pending ARCH ruling, §6)

```cpp
// MarkerLink_PARAMS.h — new file, sibling of MarkerLayerBundle_PARAMS.h (same "new tier, own file"
// reasoning §19.3 already used to keep MarkerLayerBundle out of both MarkerRule_PARAMS.h and
// MarkerInstance_PARAMS.h)
struct MarkerLink {
    int identifier               = -1;    // stable, spelled per §1.9, matches
                                            // MarkerLayerBundle::identifier/Assembly::identifier
    std::string name;
    // "Links would be where the color override is set" — the human's own words; this Link IS the
    // single source of truth these two fields resolve from, never copied down (§3.2).
    bool  bColorOverrideEnabled  = false;  // mirrors MarkerInstanceLayer::bColorOverrideEnabled
    float color[4]               = {1.0f, 1.0f, 1.0f, 1.0f};   // mirrors MarkerInstanceLayer::color
};
// MapRecipe gains: std::vector<MarkerLink> markerLinks;
```

**Back-reference — the SAME scalar-on-the-tagged-entity shape ARCH already ratified twice for this exact "does a lower tier belong to a higher, cross-cutting concept" question** (`assemblyIdentifier` on the Bundle, §19.5; `markerTypeName` on BOTH `MarkerLayerBundle` and `MarkerRuleLayer`/`MarkerInstanceLayer`, §19.3/§19.13 — a two-tier field is not a new pattern, it is this exact struct family's established convention, applied a third time):
```cpp
// MarkerLayerBundle gains:   int linkIdentifier = -1;   // organizational — which Link created this
                                                           // Group; drives the Links tier's own
                                                           // membership/ungroup walk (§3.6)
// MarkerInstanceLayer gains: int linkIdentifier = -1;    // the ACTUAL color-override resolution key
                                                           // (§3.2) — checked directly, not via a
                                                           // walk up parentBundleIdentifier, so a
                                                           // later re-nest of the Layer under a
                                                           // different Group never silently changes
                                                           // which Link governs its color
```
Both `-1` = not Link-bound, the same sentinel convention every other back-reference in this family already uses. **Neither field is added to `MarkerRuleLayer` or `MarkerTransform`** — Link membership is Layer-tier-and-above only (§0's manual-only ruling already excludes Procedural; a Link never needs to reach past the Layer down to the raw transform, since color-override itself has never been a per-transform field).

**Wire shape — Format Expert-ruled (confirmed, with one correction to the original strawman):**
```
MarkerLinks: [ N × {
    Identifier            (int)
    Name                  (string)
    ColorOverrideEnabled  (bool)
    Color                 ({r,g,b,a})
} ]
```
⚠️ **Correction from Format Expert ruling**: `Color` serializes as an **object `{r,g,b,a}`**, not a bare 4-element array — every existing color field backing a C++ `float color[4]` in a SanGen-owned PascalCase array (`PropGroups`/`DecalGroups`/`MarkerGroups.Color`) already uses this shape; the original strawman's "4 × float" was wrong.

**Terminology correction from Format Expert ruling**: there is no wire array literally named `MarkerInstanceLayer` — that's the C++ struct name. The wire array for that struct is `MarkerGroups` (per `SANMAP_FORMAT_SPEC.md` Correction 16). Concretely: `LinkIdentifier (int)` is added to both `MarkerLayerBundles[i]` and `MarkerGroups[i]` entries.

Per-tier back-reference: `LinkIdentifier` (PascalCase on wire, full word per §1.9's "Id" ban) merged into both — direct field injection, lowerCamelCase in C++/PascalCase on wire, the identical `assemblyIdentifier`/`AssemblyIdentifier` precedent, **confirmed by Format Expert ruling**. **Additive, no `SanGenVersion` bump** — confirmed: same precedent class as every prior addition in this family (Corrections 12/14/16/18); import validation: no range to check, a dangling `LinkIdentifier` degrades to "not Link-bound" (`-1`/absent, soft, logged) — confirmed, same posture as `AssemblyIdentifier`/`ParentBundleIdentifier`'s own dangling-reference rule.

### 3.4 CORRECTION (2026-08-31, direct human ruling — supersedes the ARCH_19_31 text below until re-ratified)

**The human's own words, ground truth, not to be re-interpreted:** *"A link should have operated
the same as a section and had all same functions including hide... if a link is toggled hidden, it
propagates that change to each Marker Section that has a linked group... when a group is part of a
link, the link is the 'master' and the Section Group associated with the link is the 'slave' — the
settings on a linked group would be disabled."*

This is a single, uniform mechanism for EVERY Section/Group-equivalent setting — not the split
ARCH_19_31 originally ruled (read-and-resolve for color/hidden vs. a separate one-shot
cascade-write for Name). While `linkIdentifier >= 0`, a bound Group's own controls for every one of
these settings go fully inert (disabled, resolving live from the Link) — **including Name**, which
is NOT independently re-editable while linked (this reverses ARCH_19_31's Name ruling). Scope is
"all same functions" as a Section/Group, meaning `Params::MarkerLink` needs to grow to carry every
one of `MarkerInstanceLayer`'s equivalent settings, not just color: `name`, `bColorOverrideEnabled`,
`color[4]`, `bHidden` (NEW — no such field exists on `MarkerLink` today), and — pending explicit
ARCH confirmation of scope — `iconScale`, `bGridSnapEnabled`/`gridSnapSizeWorldUnits`,
`bSymmetryEnabled`/`symmetry`. **FORMALLY RATIFIED** — `ARCH_19_31_PropagatedPropertyMechanisms.md` (+ `ARCH_19_28`,
`ARCH_19_MarkerLayerBundle.md`, `ARCH.md`) amended: one uniform read-and-resolve/master-slave
mechanism for every setting, Name included (the cascade-write mechanism is retracted). Extension
confirmed in scope: icon scale, grid-snap, symmetry — mandatory-when-linked, no per-field opt-out.
`Params::MarkerLink` gains `bHidden`, `iconScale`, `bGridSnapEnabled`, `gridSnapSizeWorldUnits`,
`bSymmetryEnabled`, `symmetry` (`Params::SymmetrySetting`), **and `bLocked`** (confirmed by direct
human follow-up: "everything should be cascaded down") — same read-and-resolve/master-slave
mechanism, `EffectiveManualMarkerLayerLocked` resolver, disabled-while-linked Lock toggle. **No
Bundle-tier `bLocked`-equivalent** — `MarkerLayerBundle` has no lock-like field today and none is
proposed; follows the iconScale/gridSnap/symmetry precedent (Layer-tier-only fields get no invented
Bundle-tier counterpart), not the Name precedent (which needed two tiers only because Name already
existed on both). Bundle-level locking, if ever wanted, is a fresh field + fresh ARCH ruling later.
See `STEP242` for the `bLocked` follow-up ticket (sequenced after `STEP241`, same files).

Name resolvers are needed independently at both tiers (`MarkerLayerBundle::name` and
`MarkerInstanceLayer::name`), each keyed off that tier's own `linkIdentifier` per the existing
§19.29 two-tier pattern — not one shared resolver. See `STEP241` for the concrete revision ticket.

### 3.4 [SUPERSEDED — see correction above] Propagated vs. independent properties — RULED by ARCH advisory consult

**Propagates via read-and-resolve (Layer's own field becomes an inert, read-only mirror while linked — never independently editable, never written back up):**
1. **Color-override enabled + color** — explicit in the human's own example. Resolved via a new function mirroring `EffectiveManualMarkerLayerColor`'s exact shape:
```cpp
// MarkersTab_ManualLayerHelpers_UI.h — new function, same file, same shape as
// EffectiveManualMarkerLayerColor (adjacent, not a rename of it — that function stays for the
// bUseGroupColor case, which is unrelated and still needed independent of Links)
bool EffectiveManualMarkerLayerColorOverrideEnabled(const Params::MarkerInstanceLayer& layer,
                                                    const std::vector<Params::MarkerLink>& links);
const float* EffectiveManualMarkerLayerColor(const Params::MarkerInstanceLayer& layer,
                                             const std::vector<Params::MarkerLink>& links);
   // linkIdentifier >= 0 and resolves -> the Link's own bColorOverrideEnabled/color;
   // else -> layer.bColorOverrideEnabled/layer.color, unchanged from today.
```
2. **`bHidden` (the Layer's own visibility toggle)** — ARCH confirms: *"the 'except when disabling etc.' phrasing reads most naturally as 'unlike an ordinary Section, a Link's disable does cascade' — inclusion, matching the design's reading."* Same resolver shape, one more field, same inert-mirror-while-linked posture as color.

**Propagates via a DIFFERENT mechanism — one-shot cascade-write-on-rename, not read-and-resolve. ARCH drew this distinction explicitly, don't conflate the two:**
3. **Name** — *"a Bundle's `name` has no alternate 'resolve elsewhere' concept the way color/hidden now do — it is always a real, freely-editable string. Renaming a Link performs an immediate write into every bound Bundle's `name` field..., but the Bundle's `name` stays independently editable afterward and is never locked. This is correct and does not reintroduce drift-risk, because Name is declared... a display convenience, not a field that must render byte-identically for correctness the way an override color does."* Concretely: unlike color/`bHidden`, a bound Group's `name` field is NOT read-only and NOT resolved live from the Link — a Link rename just performs a normal, one-time field write into every currently-bound Group's `name` (same "non-structural, safe mid-walk" posture the Group's own rename already has, `MarkersTab_BundleHeaderExtras_UI.cpp:203`); a human can still freely rename an individual Group afterward without that being blocked or immediately overwritten.

**Stays independent per-Group (never propagated) — ARCH confirms, "no rationale for cross-type propagation of any of these was offered or exists":** `parentBundleIdentifier` (each Group's own nesting inside its own Type-section), `markerTypeName` (definitionally different per Group), membership (the entire point of a *per-type* Group), `iconScale`, `bGridSnapEnabled`/`gridSnapSizeWorldUnits`, `bSymmetryEnabled`/`symmetry`.

### 3.5 UI composition — reuses the existing disable+resolve control, adds one OR'd condition

```cpp
// DrawManualMarkerLayerColorOverrideHeaderControl (MarkersTab_ManualLayerRowBody_UI.cpp:174-195) —
// ONE new condition added to an EXISTING BeginDisabled, not a new mechanism:
ImGui::BeginDisabled(state.bUseGroupColor || layer.linkIdentifier >= 0);
// ...swatch reads EffectiveManualMarkerLayerColor(layer, links) instead of layer.color directly,
// so a disabled-because-linked swatch still shows the TRUE (Link-resolved) color, exactly as
// bUseGroupColor's own disabled swatch already shows state.groupColor today.
```
The one *editable* surface for a Link's own color-override lives on the new Links tier (§3.6) — bound directly to `Params::MarkerLink::bColorOverrideEnabled`/`color`, using the same `DrawManualMarkerLayerColorOverrideHeaderControl`-shaped control (a small overload or a second, near-identical function taking a `MarkerLink&` instead of a `MarkerInstanceLayer&` — ARCH §19.2's "domain-touching-logic-vs-pure-mechanics genericity split" argues for a shared pure predicate/draw-shape here rather than two independent copies, flagged for the coder ticket to resolve per that established rule, not re-litigated here).

### 3.6 The "Links" tier — a per-Link `DrawSectionBegin` loop, sibling to the Type-section loop, NOT nested inside it

**Composition, answering the brief's own ❓ directly.** The human's framing — "a Link... is essentially no different than a Section and should have exact same options" — read literally against the actual widget library: draw one collapsible `Ui::DrawSectionBegin`/`DrawSectionEnd` pair **per `Params::MarkerLink`**, in a loop sibling to (drawn once, before or after) the Type-section loop — not a `TreeListWidget_UI`/`DraggableList` row, and not folded into `EnumerateMarkerTypeSectionNames`'s own enumeration (a Link is not a `markerTypeName` value and must never be mistaken for one). This is the SAME idiom the Type-section loop already uses (`MarkersTab_UI.cpp:275-286`'s `for (rowIndex...) DrawSectionBegin(typeName, ...)`), applied to `recipe.markerLinks` instead of the fixed 3-entry array.

⚠️ **Found while reading, not this brief's to fix: the LIVE `DrawMarkersTab` (`MarkersTab_UI.cpp`, the only call site actually wired from `Application_PanelEnvironment_UI.cpp:37`) still loops over the fixed 3-entry `markerGlobalScaleRowLabels`/`kMarkerGlobalScaleRowCount` for its OUTER Type-section enumeration itself**, not `EnumerateMarkerTypeSectionNames`'s dynamic list (`MarkersTab_TypeSections_UI.h`/`.cpp`'s `DrawMarkerTypeSections`, which implements §19.14/§19.15's dynamic ruling correctly but has zero live callers anywhere in the codebase — confirmed by grep). Either this is dead/unwired code from an incomplete migration, or the dynamic-enumeration ruling never actually shipped to the live tab despite ARCH recording it as ratified. This means "(Unassigned)"/custom-`markerTypeName` sections are likely not rendering in the shipped UI today, contrary to §19.14/§19.15's text. **Not entangled with the Links tier** (Links loop over `recipe.markerLinks`, an independent array, regardless of which Type-section loop is live) — flagged as a separate, pre-existing defect for ARCH/Coder attention, not blocking this design.

**Per-Link header — distinct hue, zero new widget code.** `DrawSectionBegin` already takes a `const WidgetStyle& style` parameter and already paints its header bar from `style.trackColor` (`Section_UI.cpp:56-58`, `ResolveWidgetColor(style.trackColor, ...)`) — every other `DrawSectionBegin` call site in this tab passes the default `WidgetStyle()` (`kThemeColor` = "follow imgui theme"). A distinct hue is one named style variant:
```cpp
// MarkersTab_Links_UI.h — new file, a named constant per Constitution §8 (a UI-chrome tweakable,
// not a PARAMS/recipe value, same tier as kMarkerLayerHeaderExtraCombinedWidthPixels)
inline WidgetStyle LinkSectionHeaderStyle() {
    WidgetStyle style;
    style.trackColor = /* a named PackedColor constant distinct from every colorAlloy/Plasma/Spawn
                          default and from kThemeColor's own resolved value */;
    return style;
}
```
used only at the Links loop's own `DrawSectionBegin(link.name.c_str(), ..., LinkSectionHeaderStyle())` call — every existing Type-section `DrawSectionBegin` call is untouched (default style, unchanged look).

**Per-Link header content:** double-click-to-rename (mirrors `DrawMarkerLayerBundleNodeHeaderExtra`'s own scratch-buffer rename exactly, `MarkersTab_BundleHeaderExtras_UI.cpp:151-231` — same reasoning: live-editing `link.name` directly would churn the header's own imgui id every keystroke), the Link's own color-override toggle+swatch (§3.5), an "X" delete (§3.7). **Body** (once expanded): a read-only summary — which per-type Groups are bound to this Link (walk `recipe.markerLayerBundles` for `linkIdentifier == link.identifier`, one per type by construction) and a live instance count per type, mirroring the Assembly design's own "3 markers, 1 prop, 2 decals selected" readout convention (`DESIGN_Assembly_R1.md` §3) one tier over. No move/rotate here — Link has no spatial concept of its own, unlike Assembly.

**New `+Link` button — every Type-section header, alongside `+Group`/`+Layer`, enabled whenever `state.selectedManualInstanceIdentifiers` is non-empty.** This is a genuine, reasoned interaction-design call (squarely mine, not routed as a question, mirroring how Assembly's design settled its own pivot-point call directly): the button's ACTION is cross-type and identical regardless of which Type-section's copy is clicked (it always acts on the whole tab-wide selection, never just the current section's own subset), so drawing an identical, always-available affordance in every section header — exactly where the brief asks for it — is correct even though only one click is ever needed. No section-scoping logic is needed on the button itself, unlike `+Group`/`+Layer`'s new same-type gate (§2).

```cpp
if (buttons.bAddLinkClicked) {
    Params::MarkerLink link;
    link.identifier = NextMarkerLinkId(recipe.markerLinks);   // mirrors NextMarkerLayerBundleId
    link.name = /* default editable name, e.g. "Link " + std::to_string(link.identifier) */;
    recipe.markerLinks.push_back(link);

    // Partition the live selection by type — new helper, same per-frame-index-build convention
    // ManualInstanceLayerIndex_UI.h/MarkerLayerBundleLeafIndex_UI already use:
    const auto byType = PartitionSelectedManualInstancesByType(recipe.markers,
                                                                state.selectedManualInstanceIdentifiers);
    for (const auto& [typeName, instanceIdentifiers] : byType) {
        Params::MarkerLayerBundle bundle;
        bundle.identifier       = NextMarkerLayerBundleId(recipe.markerLayerBundles);
        bundle.markerTypeName   = typeName;
        bundle.parentBundleIdentifier = -1;   // root within its own Type-section — a Link's Group
                                               // never nests under a "currently selected" Group the
                                               // way +Group/+Layer do (§2); there is no single
                                               // selected-Group frame of reference across types.
        bundle.linkIdentifier   = link.identifier;
        recipe.markerLayerBundles.push_back(bundle);

        Params::MarkerInstanceLayer layer;
        layer.name    = link.name;
        layer.layerId = NextMarkerLayerId(recipe.markerLayers);
        layer.parentBundleIdentifier = bundle.identifier;
        layer.markerTypeName         = typeName;
        layer.linkIdentifier         = link.identifier;
        recipe.markerLayers.push_back(layer);

        ReassignManualInstanceLayers(recipe.markers, instanceIdentifiers,
                                     static_cast<int>(recipe.markerLayers.size()) - 1);
    }
}
```
`PartitionSelectedManualInstancesByType` is new, small (mirrors `BuildManualInstanceLayerIndex`'s exact per-frame-map-build shape, `ManualInstanceLayerIndex_UI.h:22-33`, keyed by `Params::CanonicalMarkerTypeSectionName(group.name)` instead of `layerIndex`) — flagged as new surface for the coder ticket, not existing machinery.

### 3.7 Delete-Link semantics (restating the brief's already-decided ground truth, placed against the real data model)

Deleting a Link: for every `MarkerLayerBundle` with `linkIdentifier == deletedId`, clear `linkIdentifier` to `-1` (ungroup the LINK relationship only — the Group itself, and every Layer/Instance under it, is untouched, mirroring `DeleteMarkerLayerBundleGroupOnly`'s existing "erase container, promote/keep contents" posture exactly, just narrower: here even the Group survives, only the tag clears); for every `MarkerInstanceLayer` with `linkIdentifier == deletedId`, clear `linkIdentifier` to `-1` too (its `bColorOverrideEnabled`/`color` then read from its OWN fields again, unchanged from whatever they last resolved to — since the fields were never copied, there is nothing to "restore," they simply become live-editable again at their last-resolved values, a correct and unsurprising outcome of read-and-resolve). Erase the `Params::MarkerLink` entry itself last. No instance, Layer, or Group is ever erased by this action — exactly the brief's own restated ground truth, and a natural (not special-cased) consequence of §3.3's tagging shape.

### 3.8 IO file homes and wiring — IO Architecture Expert advisory ruling

**No new migration unit needed, confirmed independently of Format Expert's ruling.** All three additions are pure absent-safe field/key additions (`ReadJsonInteger`/`ReadJsonFloat` never fail on a missing key) — same precedent class as the already-shipped `AssemblyIdentifier`/`MarkerTypeName` additions, neither of which got a `<Domain>_Migrate_V<N>_IO` file or version bump either. `IO_MIGRATION_SPEC.md` §1 reserves a migration unit for reshaping a fragment forward; an old file simply lacking a new key isn't a shape mismatch, it's what absent-safe reading already makes version-transparent.

**File homes — split by what's genuinely new vs. a merged field on an existing section:**
- **New top-level `MarkerLinks` array + `Params::MarkerLink` → new file pair**, mirroring `MarkerLayerBundle`'s own precedent (a new tier gets its own PARAMS file and its own IO file): `src/io/MapExporter_MarkerLink_IO.h/.cpp`, `src/io/MapImporter_MarkerLink_IO.h/.cpp` (singular domain name, matching `MapImporter_MarkerLayerBundle_IO.cpp`'s naming against its plural wire array).
- **The two `LinkIdentifier` merged fields → into the EXISTING files that already own those sections**, not a new file — `MapExporter_Markers_IO.cpp` (both Bundle and Group JSON-building already live here, well under the ARCH §1.5 size ceiling), `MapImporter_MarkerLayerBundle_IO.cpp` and `MapImporter_MarkerGroups_IO.cpp` on the importer side (already split out by line count, both with headroom).
- **The three `GlobalMarkerSettings` scale fields → `MapExporter_MarkersStack_IO.cpp`/`MapImporter_MarkersStack_IO.cpp`**, alongside the existing `scaleAlloy/Plasma/Spawn` handling.

**Import-degrade posture confirmed, zero special-casing** — a dangling `LinkIdentifier` needs no manifest/runner involvement; it's a plain unlvalidated int read, identical in kind to `ParentBundleIdentifier`/`AssemblyIdentifier`'s own documented posture. Resolution to "not Link-bound" happens at the UI/resolver layer (§3.2's read-and-resolve), not in IO.

**Precision items flagged for the eventual coder ticket (none blocking, but should be stated explicitly rather than left to inference):**
1. `MarkerLinks` must be added to `Sanmap_KnownTopLevelKeys_IO` (or its equivalent) — otherwise it round-trips verbatim under `UnknownImport` instead of parsing into `recipe.markerLinks`.
2. No cycle-repair pass is needed for `LinkIdentifier` (unlike `MarkerLayerBundle`'s own parent-cycle repair) since it points into a flat, non-nesting list — but the work-order should state explicitly whether a dangling `LinkIdentifier` gets a logged warning at import (a small pass analogous to the Bundle-cycle repair) or is left entirely to the UI resolver's silent `-1` handling (§3.7 implies the latter — IO Architecture Expert flags this as needing one explicit sentence, not guessed).
3. No import ordering dependency between `MarkerLinks` and `MarkerGroups`/`MarkerLayerBundles` — `ReadMarkerLinksJson` populates its own flat list, callable anywhere alongside the other top-level-array readers.
4. New file pair needs the standard sibling test files (`MapExporter_MarkerLink_IO_Test.cpp`/`MapImporter_MarkerLink_IO_Test.cpp`) per this tree's per-file test convention — an ordinary IO round-trip test, not a migration test.

## 4. Icon-size / scale UI rework (Marker-Type section header)

### 4.1 Which control this is, confirmed by direct read

The brief's "existing icon-scale slider" in the Marker-Type section header is `DrawTypeSectionMarkerSettingsRow` (`MarkersTab_Globals_UI.h:135-138`, `.cpp`) — the icon-thumbnail / icon-color-swatch / "Selected"-label+select-color-swatch / Size-slider row, bound to `Params::GlobalMarkerSettings::scaleAlloy/Plasma/Spawn` via `ResolveGlobalMarkerScaleRowFields` (`MarkersTab_Globals_UI.h:105-116`), drawn once per Type-section via `DrawRightAlignedTypeSectionHeaderButtons` (`MarkersTab_UI.cpp:90`). **This is a DIFFERENT field from the per-Manual-Layer `iconScale`** (`MarkerInstanceLayer::iconScale`, edited via the Bundle-tree leaf's own `[Icon Size]` compact slider, `DrawMarkerLayerIconSizeHeaderControl`, `MarkersTab_ManualLayerRowBody_UI.cpp:215-223`) — the brief's item 4 heading explicitly scopes to "Marker-Type section header," so this design targets `GlobalMarkerSettings::scaleAlloy/Plasma/Spawn` only; the per-Layer `iconScale` control is untouched. **Flagged for human confirmation, per the brief's own ❓** — state this reading plainly rather than silently touching both.

### 4.2 The circular-slider widget already exists — confirmed by direct read, contrary to the brief's own suspicion

`LabelledDialWidget_UI.h`/`.cpp` (`DrawLabelledDial`) is a real, shipped, tested knob widget — ImDrawList arc + pointer, `DialRange{minimumValue, maximumValue, increment, pixelsForFullSweep}`, a `RealtimeToggle`, live consumers in `TerrainTab_UI.cpp`/`LayersTab_UI.cpp`/`PropsTab_Rules_UI.cpp`/`DecalsTab_Rules_UI.cpp`/`MarkersTab_Rules_UI.cpp`. `MarkersTab_UI.cpp`'s own file header even names it: *"Section/Checkbox/Combo/RangeSlider/Dial for the scalars."* **The brief's "no radial/knob widget exists" is not correct — a new PRIMITIVE is not needed.**

**What IS missing, confirmed by direct read: a compact, single-header-row-height variant.** `DrawLabelledDial` draws a `2×radius`-tall knob beside a `BeginGroup()`ed two-line label+field+RT stack (`LabelledDialWidget_UI.cpp:50-63`) — taller than the one-line header row this needs. `SliderScalar_UI.h` already has exactly this split for the LINEAR slider: `DrawSliderScalar` (3-line, full) vs. `DrawSliderScalarCompact` (STEP134, one line, caller-fixed track/field widths, optional RT button — `SliderScalar_UI.h:122-137`, already the control `DrawMarkerLayerIconSizeHeaderControl` uses today). **The Dial has no equivalent compact sibling yet — that gap is real, but it is a new *variant of an existing widget*, not a new widget-library primitive**, the same distinction the codebase already draws for its linear counterpart:

```cpp
// LabelledDialWidget_UI.h — new function, same relationship to DrawLabelledDial that
// DrawSliderScalarCompact already has to DrawSliderScalar
// No label line, no separate field line: a small knob (diameter = the header row's own frame
// height, via style.dialRadius) immediately beside a caller-fixed-width numeric field, on ONE
// line — mirrors DrawSliderScalarCompact's own composition (PushID -> fixed knob -> SameLine ->
// fixed-width DragFloat -> optional RT) exactly, substituting the knob's InvisibleButton+arc for
// the linear track.
WidgetChange DrawDialCompact(const char* label, float& value, const DialRange& range,
                             RealtimeToggle& realtimeToggle, float fieldWidthPixels,
                             const WidgetStyle& style = WidgetStyle(), const char* valueFormat = "%.2f",
                             bool bShowRealtimeToggle = true);
```
Low-risk, additive, new-function-in-an-existing-file — flagged for a casual ARCH pass (naming/placement only, same tier as `DESIGN_Assembly_R1.md` §6 item 3's "low-risk, `_UI`-suffix/naming-law conformant by construction" framing), not a blocker to building it.

### 4.3 Range clamp and the two-input reading

**Clamp to `[0.25, 2.0]`** applies to `GlobalMarkerSettings::scaleAlloy/Plasma/Spawn`'s own `DialRange`/`ScalarSliderRange` at the call site (`MarkersTabGlobals`'s equivalent of `iconScaleRange`, currently `ScalarSliderRange` shared with the Layer-tier control at `{0.1f, 10.0f, 0.0f}` — a NEW, separate, narrower range value scoped to just this row, not a change to the shared `iconScaleRange` field the per-Layer control still uses at its own, wider bounds). No PARAMS-level clamp is proposed (matches this format's existing "no range to validate on import, UI enforces at the edit surface" posture for every comparable scalar) — flagged for ARCH only insofar as the new field pair below needs a home, not because the clamp itself needs ratifying.

**Two separate icon-size inputs — RECOMMENDATION: per-Type-section pair (base + selected), doubling `GlobalMarkerSettings`' existing 3-field pattern, NOT a single tab-wide default+selected pair.** Reasoning: item 4's own heading scopes to the per-Type header row, which is ALREADY per-type (`scaleAlloy`/`scalePlasma`/`scaleSpawn`, three independent fields, not one shared default) — the natural, minimal-surprise extension is a same-shaped second field per type, not a new tab-wide concept the row has no other precedent for. This is also the reading that lets the "selected size" mean something concrete at render time (§4.4): *this marker type's* icon grows/shrinks when *an instance of it* is selected, not a single global multiplier applied indiscriminately across Alloy/Plasma/Spawn.

```cpp
// GlobalMarkerSettings gains, strict mirror of scaleAlloy/Plasma/Spawn — no 4th-field deviation
// needed here (unlike §19.17's selectColorDefault): ResolveMarkerGroupTypeScale's own unmatched-
// name fallback is already 1.0f, a genuine no-op multiplier that works identically for a "selected"
// scale as it does for the base one — no white-vs-white ambiguity a COLOR fallback has.
float scaleSelectedAlloy  = 0.50f;   // same default as scaleAlloy — "selected" starts equal to base
float scaleSelectedPlasma = 0.50f;
float scaleSelectedSpawn  = 0.50f;

// new resolver, strict mirror of ResolveMarkerGroupTypeScale
inline float ResolveMarkerGroupSelectedTypeScale(const std::string& groupName,
                                                 const GlobalMarkerSettings& settings) { /* ... */ }
```
**Wire key — Format Expert-ruled, corrected from the original strawman**: not `SelectedScaleAlloy`/etc. The shipped precedent is wire `MarkerScaleAlloy`/`Plasma`/`Spawn` → C++ `scaleAlloy`/`Plasma`/`Spawn` (the `Marker` prefix drops on the C++ side only, since the type already scopes it; wire keeps the full prefix). Strict mirror preserving that prefix position:
```
Wire:  MarkerScaleSelectedAlloy / MarkerScaleSelectedPlasma / MarkerScaleSelectedSpawn
C++:   scaleSelectedAlloy / scaleSelectedPlasma / scaleSelectedSpawn
```
The design's original `SelectedScaleAlloy` strawman would have broken the established `Marker<Field><Type>` wire template — rejected, use the corrected spelling above. **❓ Confirm with the human directly, per the brief's own instruction** — the "per-type base+selected pair" reading (this design's recommendation) vs. the brief's own alternative ("a single global default+selected pair") is a real fork this design pass cannot close from context alone; the wire spelling above assumes the per-type reading.

### 4.4 Render-consumer gap — real, flagged, not silently solved here

Confirmed by direct read: `MapCanvas_IconLayer_CullManual_UI.cpp:202` composes the candidate's rendered scale as `transform.transform.scaleX * groupTypeScale * layerIconScale` — computed BEFORE selection state is known (the selection-tint comparison against `bSelected` happens downstream, per §19.18's own priority-order ruling, at whatever site resolves the final draw tint — not this candidate-building loop). A "selected size" therefore needs the scale-composition to be selection-AWARE at whatever later stage already knows `bSelected` for tint purposes — folding `ResolveMarkerGroupSelectedTypeScale`'s result in at candidate-build time (this file, line 202) is the WRONG site, since selection isn't resolved yet there. **Exact site TBD by direct read at ticket time** (flagged, not guessed, per Constitution §8.4) — likely the same downstream stage §19.18/§19.19's tint-priority logic already occupies. This is authoring-scale (no throughput/batching concern — same O(candidates) cost class already paid for tint), so it stays within UI-layer ownership; no UI Optimization Expert consult needed for this specific addition.

## 5. ARCH advisory ruling received — items below are RULED (advisory), pending only formal write-up in a dedicated ARCH ratification session

An ARCH Expert consult has already run against this design (read-only advisory dispatch, per its own "read-only when dispatched as a subagent" convention — it did not write any ARCH.md/ARCH_NN_*.md file; that write happens in a separate dedicated session). Its rulings, summarized against each item originally flagged here:

1. **New PARAMS type `Params::MarkerLink`** (§3.3) — CONFIRMED as strawmanned. New file `MarkerLink_PARAMS.h`, sibling of `MarkerLayerBundle_PARAMS.h`, per §19.1/§19.3's "new tier gets its own file" pattern. `MapRecipe::markerLinks` confirmed.
2. **Two new `linkIdentifier` scalar fields** (`MarkerLayerBundle`, `MarkerInstanceLayer`) — CONFIRMED, `linkIdentifier` (never `LinkId`). The design's choice to give `MarkerInstanceLayer` its own independent field (rather than deriving via `parentBundleIdentifier` walk-up) is CONFIRMED, not just accepted — ARCH ties it to the identical already-twice-ratified two-tier back-reference pattern (`markerTypeName` on both `MarkerLayerBundle` §19.3 and `MarkerRuleLayer`/`MarkerInstanceLayer` §19.13, independently set, no walk-up derivation). If the two ever disagree (e.g. a re-nested Layer), that's soft silent degrade, same class as every other dangling back-reference in this family — not a structural error.
3. **New top-level wire array `MarkerLinks`** (§3.3) — CONFIRMED at the ARCH-shape level; final micro-spelling was Format Expert's call (§3.3 above already incorporates that ruling: `Color` as `{r,g,b,a}`, wire target is `MarkerGroups` not "MarkerInstanceLayer").
4. **Propagated-property list** (§3.4) — RULED, not left as a recommendation (see §3.4's revised text above for the full ruling, including the Name-uses-a-different-mechanism distinction).
5. **Two new `GlobalMarkerSettings` fields** — CONFIRMED, naming per §4.3 (Format-corrected wire spelling already applied there).
6. **New widget-library function `DrawDialCompact`** — APPROVED, casual pass, no further ruling needed.
7. ⚠️ **The hardcoded 3-entry Type-section loop defect** (§3.6) — CONFIRMED as real and independent by direct ARCH read: `MarkersTab_UI.cpp:275-276` loops the fixed array; `DrawMarkerTypeSections` (the function implementing §19.14/§19.15's ratified dynamic enumeration) has **zero call sites** anywhere. ARCH's own words: *"a real, confirmed conformance gap between ratified law... and the live tab — needs recording as a new Standing Recorded Defect in `sangen_arch_pack/INDEX.md` at the next ratification session, and a coder ticket to wire `DrawMarkerTypeSections` in as the live outer loop (or retire it if superseded — needs a decision, not assumed here)."* Not entangled with Link work; a separate, pre-existing defect for the human/ARCH Expert to schedule.

**What's genuinely still open after this ruling**: nothing content-wise on §3/§5 — every naming/shape/propagation question has an advisory answer. What remains is procedural: this advisory ruling needs to be formally written into new ARCH_19_XX section files in a dedicated ARCH ratification session (the same process that turned Assembly's own design-pass ❓s into `ARCH_19_05`/`ARCH_19_06`/`ARCH_19_08` etc.) before any of §3/§4's new-field half is actually coder-dispatchable. §1 and §2 (and §4's widget half) remain dispatchable now regardless, per their own sections.

## 6. Questions to relay — ARCH and Format both answered (advisory); two human-decision forks resolved provisionally below

**✅ ARCH Expert — answered.** See §5's revised text for the full ruling: `Params::MarkerLink` shape confirmed, independent (not walk-up-derived) `linkIdentifier` on `MarkerInstanceLayer` confirmed, propagated-property list ruled (color/`bHidden` via read-and-resolve, Name via a distinct one-shot cascade-write), `DrawDialCompact` approved, and the hardcoded Type-section loop flagged as a real, separate, pre-existing defect needing its own Standing Recorded Defect entry and coder ticket.

**✅ Format Expert — answered.** See §3.3 and §4.3's revised text: `Color` is `{r,g,b,a}` (not a bare array), the wire array for `MarkerInstanceLayer` is `MarkerGroups`, the new scale fields are `MarkerScaleSelectedAlloy/Plasma/Spawn` (wire) / `scaleSelectedAlloy/Plasma/Spawn` (C++) to preserve the established `Marker<Field><Type>` template, no `SanGenVersion` bump needed for any of the three additions, dangling-`LinkIdentifier` soft-degrade posture confirmed.

**Resolved provisionally, adopting this design's own stated recommendation (per direct instruction to proceed) — flag if either reading is wrong, easy to correct before coder dispatch:**
1. Item 4's two-input reading: **adopting the per-Type-section base+selected pair** (§4.3's recommendation — `scaleSelectedAlloy/Plasma/Spawn` alongside the existing `scaleAlloy/Plasma/Spawn`), not a single tab-wide default+selected pair.
2. Item 4's scope: **confirmed as targeting `GlobalMarkerSettings::scaleAlloy/Plasma/Spawn` (the Type-section header row) only** — the per-Manual-Layer `iconScale` control (a separate field, separate location, §4.1) stays untouched.

## 7. Who else this touches
- **ARCH Expert**: advisory ruling received (§5) — formal write-up into new `ARCH_19_XX` section files still needs a dedicated ratification session before §3/§4's new-field half is actually coder-dispatchable (procedural gate only, no content gap remains). Also owns scheduling the §3.6/§5 item 7 Standing Recorded Defect (hardcoded Type-section loop) into `sangen_arch_pack/INDEX.md`.
- **Format Expert**: advisory ruling received (§3.3, §4.3) — same procedural gate as ARCH, no open content question remains.
- **IO Architecture Expert**: advisory ruling received (§3.8) — no new migration unit needed, file homes named (new `MarkerLink_IO` file pair; merged fields go into existing `Markers`/`MarkerGroups`/`MarkersStack` IO files), plus four precision items (KnownTopLevelKeys wiring, dangling-reference logging decision, import ordering, test files) for the eventual coder ticket to state explicitly.
- **UI Optimization Expert**: not consulted this round — every new list/loop here is authoring-scale (a handful of Links, tens of Groups); §4.4's render-consumer change is a scalar-composition-site relocation, not a new throughput/batching concern. Flag only if Link-membership counts or per-frame Link resolution ever approach a regime where the read-and-resolve walk (§3.2) stops being O(a handful) per instance.
- **SanGen Coder**: §1 (now universal, all three manual domains) and §2 are dispatchable as their own small ticket(s) today, independent of §3/§4's ARCH gate. §4's widget half (`DrawDialCompact`) is approved and dispatchable too.

**Status summary**: §1 and §2 are coder-dispatchable now, no blocker. §4's widget half (`DrawDialCompact`) is approved and dispatchable. §3 (the Link mechanic) and §4's new-field half have a complete advisory design — UI Expert design pass, ARCH ruling, Format Expert ruling, all in agreement — but are not yet coder-dispatchable until that advisory ruling is formally written into ratified ARCH section files in a dedicated ARCH session. The separate hardcoded-Type-section-loop defect (§3.6) needs its own scheduling decision, unrelated to Link.

---

Files read to ground this design (all absolute paths under `D:\Projects\Sanctuary\Map Generator\`):
`work_orders\BRIEF_MarkerLink_R1.md`, `work_orders\DESIGN_Assembly_R1.md`,
`ARCH_19_MarkerLayerBundle.md` and §19.3/§19.4/§19.5/§19.7/§19.9/§19.12/§19.13/§19.14/§19.15/§19.16/
§19.17/§19.20/§19.23/§19.24/§19.25/§19.27, `ARCH_21_01_MultiSelectRepresentation.md`,
`src\ui\MarkersTab_UI.cpp`, `src\ui\MarkersTab_UI.h`, `src\ui\MarkersTab_ManualInstanceSelection_UI.h`,
`src\ui\MarkersTab_ManualInstance_UI.cpp`, `src\ui\MarkersTab_BundleHeaderExtras_UI.cpp`,
`src\ui\MarkersTab_Bundles_UI.h`, `src\ui\MarkersTab_BundleDelete_UI.h`,
`src\ui\MarkersTab_ManualLayerRowBody_UI.h`, `src\ui\MarkersTab_ManualLayerRowBody_UI.cpp` (partial),
`src\ui\MarkersTab_ManualLayerHelpers_UI.h`, `src\ui\MarkersTab_Globals_UI.h`,
`src\ui\MarkersTab_TypeSections_UI.h`, `src\ui\ManualInstanceLayerIndex_UI.h`,
`src\ui\LabelledDialWidget_UI.h`/`.cpp`, `src\ui\SliderScalar_UI.h`, `src\ui\Section_UI.h`/`.cpp`,
`src\ui\WidgetHelpers_UI.h`, `src\ui\Application_Draw_UI.cpp`,
`src\ui\MapCanvas_IconLayer_CullManual_UI.cpp`,
`src\params\MarkerLayerBundle_PARAMS.h`, `src\params\MarkerInstance_PARAMS.h`,
`src\params\GlobalMarkerSettings_PARAMS.h`.
