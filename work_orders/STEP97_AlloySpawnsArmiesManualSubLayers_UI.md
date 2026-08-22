# STEP97 — Alloy/SpawnsArmies gain Manual overlay sub-layers (corrects STEP51's "zero Manual sub-layers" design)

**Layer:** UI. **Domain:** `SeedMarkerDomains` in the not-yet-coded `Application_OverlaySetup_UI.cpp`
(STEP51's own design target). **Ratifying authority:** `ARCH_14_02_DataModel.md` §14.2's binding
sub-layer → data mapping table, current text quoted verbatim below. **Corrects:** the "zero Manual
sub-layers for Alloy/SpawnsArmies" design `STEP51_OverlayLayerDataModel_UI.md` shipped in its
`SeedMarkerDomains` reference implementation — this ticket does not edit that file.

## ⚠️ Verified premise correction — read before dispatching

This ticket was requested on the premise that STEP51 is "already shipped, confirmed IMPLEMENTED"
(`work_orders/IMPLEMENTATION_STATUS.md` line 201) and that only STEP60 stands between here and a
coder task. **Both halves of that premise fail verification against the real repo:**

1. **STEP51 itself has no trace in `src/` on any branch.** `grep -rn` across `src/` for
   `OverlayLayer_UI`, `OverlayDomainKind_UI`, `OverlaySubLayerRef_UI`, `OverlayLayerSettings`,
   `SeedMarkerDomains` — zero matches, all four. The two files STEP51's own "Files touched" section
   names, `src/ui/OverlayLayer_Settings_UI.h` and `src/ui/Application_OverlaySetup_UI.cpp`, do not
   exist (confirmed via Glob). `git log --all --oneline -- <both paths>` returns nothing on any of
   the 9 local branches (`SanGen-v2`, `SanGen-v3`, `master`, the `worktree-agent-*` branches) —
   this was never committed anywhere, not merely uncommitted-in-working-tree.
2. **`IMPLEMENTATION_STATUS.md`'s "confirmed IMPLEMENTED" claim directly contradicts a second
   tracking document.** `work_orders/SEQUENCE_PreviewOverlayLayering.md` line 33 (Phase 2.1's own
   row) states STEP51's status as **`DRAFTED`**, not implemented. Ground truth (empty `src/` grep +
   empty `git log --all`) sides with `SEQUENCE_PreviewOverlayLayering.md`: STEP51 is drafted
   work-order prose only, never coded.

Consequence: this ticket cannot cite real `file:line` for `SeedMarkerDomains` (Constitution §7
requires target files; instruction requires "citing real `file:line`") because that function has
never existed in `src/`. Every citation below to STEP51's `SeedMarkerDomains` is a citation to
STEP51's *work-order markdown* (`work_orders/STEP51_OverlayLayerDataModel_UI.md`), not to live code
— flagged explicitly rather than presented as a code citation. This is reported, not silently
patched around: **STEP51 needs to actually be dispatched to the Coder before this ticket's fix has
anything to attach to.** `work_orders/IMPLEMENTATION_STATUS.md` line 201's "confirmed IMPLEMENTED"
line is itself wrong and should be corrected in a separate follow-up (out of scope here — that file
is a shared audit ledger, not this ticket's to rewrite).

## Root problem
`ARCH_14_02_DataModel.md` §14.2's sub-layer → data mapping table, current ratified text:
> "Alloy / Spawns-Armies | ⚠️ was blocked — `Params::MarkerInstanceLayer` now exists (ARCH §16);
> this row's data mapping updates to match §16.1's `recipe.markerLayers[i]`/
> `recipe.markerRuleLayers[i].rules[j]` shape, superseding the placeholder 'single undifferentiated
> Manual bucket' text below."

Once STEP51 lands (as coded, not just drafted), its `SeedMarkerDomains` must seed Manual sub-layer
refs for Alloy/SpawnsArmies from `recipe.markerLayers[i]`, matching the pattern already used for
Props/Decals (`PushManualRefs` before `PushProceduralRefs`, `STEP51_OverlayLayerDataModel_UI.md`
lines 249–252/257–260) instead of emitting zero Manual refs (STEP51's shipped-design text, same
file lines 158–163/210–224).

## ⚠️ Second blocking gap found during verification — not resolvable by this ticket alone
Props/Decals can route each `recipe.propLayers[i]`/`recipe.decalLayers[i]` entry wholesale to its
one domain because Props *is* one whole overlay domain and Decals *is* one whole overlay domain —
no per-entry split is needed. Alloy/SpawnsArmies are **not** one domain; they are a *category split*
of a single underlying marker concept (`MarkerCategory::Spawn` vs. everything else,
`MarkerRule_PARAMS.h:14`). The Procedural side can split cleanly because `MarkerRule` carries its
own `category` field per rule (`MarkerRule_PARAMS.h:20`) — each rule routes individually.

`Params::MarkerInstanceLayer` (§16.1's ratified shape, `ARCH_16_01_NewParamsShapes.md` lines 25–30)
carries **no category field** — `name`, `color[4]`, `iconScale`, `layerId`, `symmetry`, nothing that
says Spawn-vs-rest. The manual roster's own category-like signal lives one level away and on a
different axis: `MarkerInstanceGroup::bResource`/`name` (`src/params/MarkerInstance_PARAMS.h:23-28`,
comment at line 24-26 confirms `name` is "free-form std::string, NOT MarkerCategory") is keyed by
marker TYPE, while `recipe.markerLayers[i]` (via `MarkerTransform::layerIndex`, once STEP60 adds it)
is an independent, mapmaker-chosen DISPLAY bucket. A single manual layer's transforms can span
multiple `MarkerInstanceGroup`s of mixed type — nothing in the ratified shapes prevents a mapmaker
from putting a Spawn point and a Mass point in the same authored layer. There is therefore **no
principled way to route one `recipe.markerLayers[i]` entry wholesale to Alloy XOR SpawnsArmies** the
way `PushManualRefs`/`PushProceduralRefs` does for Props/Decals — the Manual side has no per-entry
discriminator to filter on, unlike the Procedural side's per-rule `category`.

This is an ARCH-level gap, not a coding decision this UI-layer ticket is positioned to invent an
answer for (Constitution: only the ARCH Expert writes ARCH; this expert authors work-orders, not
data-shape rulings). Candidate resolutions exist — e.g. route each manual *transform* individually
by inspecting its owning `MarkerInstanceGroup`'s type/`bResource` rather than by `layerIndex`
wholesale, which would mean a single `recipe.markerLayers[i]` entry could contribute refs to *both*
Alloy and SpawnsArmies' sub-layer lists simultaneously and would need a different `index` semantics
than the layer-position index Props/Decals use — but picking between candidates is an ARCH Expert
call, not asserted here.

## Explicit sequencing — this ticket is blocked on three unbuilt prerequisites, verified independently
1. **`STEP51_OverlayLayerDataModel_UI.md`** — the overlay data model itself (`OverlayLayer_UI`,
   `SeedMarkerDomains`, `Application_OverlaySetup_UI.cpp`). Verified NOT in `src/`, NOT in any
   branch's git history (see premise-correction section above), despite
   `work_orders/IMPLEMENTATION_STATUS.md` line 201 calling it "confirmed IMPLEMENTED."
2. **`STEP60_MarkerInstanceLayer_PARAMS.md`** — adds `Params::MarkerInstanceLayer` and
   `MapRecipe::markerLayers`. Verified NOT in `src/`: `grep -rn "MarkerInstanceLayer\|markerLayers"
   src/` returns zero matches; live `src/params/MarkerInstance_PARAMS.h` (read in full) has only
   `MarkerTransform`/`MarkerInstanceGroup`, no layer type, no `layerIndex` field.
   `work_orders/HANDOFF_TRACK_MarkerLayerSymmetry.md` line 17 itself calls it "DRAFT, complete" —
   matches the code-level finding.
3. **`STEP66_MarkerRuleLayer_PARAMS.md`** — the `MapRecipe::markerRules` → `markerRuleLayers` rename
   §16.1 depends on. Verified NOT in `src/`: live `src/params/MapRecipe_PARAMS.h:56` still declares
   the pre-rename flat `std::vector<MarkerRule> markerRules;` — no `markerRuleLayers` anywhere under
   `src/` (zero grep matches). `Params::SymmetrySetting` (the wrapper §16.1 also needs,
   `ARCH_16_01_NewParamsShapes.md` lines 6-10) is likewise absent from the live
   `src/params/Symmetry_PARAMS.h` (read in full: `SymmetryAxis`, `SymmetryDetection`,
   `SymmetryBlend` only).

No placeholder behavior is specified against any of these three — per instruction, and because
the open routing question above means even a fully-landed STEP51+60+66 does not by itself make this
ticket's fix well-defined; the ARCH-level gap must close first, or the fix's `index` semantics named
here would have to be redone.

## What changes, once unblocked (the shape of the fix — not yet dispatchable)
In `SeedMarkerDomains` (`Application_OverlaySetup_UI.cpp`, once STEP51 is coded), matching the
existing Props/Decals precedent's call shape (STEP51 markdown lines 249-252):
- Before the existing `recipe.markerRuleLayers` walk (STEP51 markdown lines 215-223, `flatIndex`
  loop), push Manual refs for whichever domain(s) the ARCH Expert's routing ruling (above) assigns
  each `recipe.markerLayers[i]` entry — or entry-fragment — to.
- `OverlayLayer_UI`/`OverlaySubLayerRef_UI` themselves need **no field changes** — §14.2's binding
  shape (`ARCH_14_02_DataModel.md` lines 4-20) is unchanged by this ticket; only `SeedMarkerDomains`'
  seeding logic and (pending the ARCH ruling) possibly `OverlaySubLayerRef_UI::index`'s per-domain
  resolution convention are affected.
- Extend STEP51's own test plan (its Verify section, item 2/3) with new assertions once real: a
  recipe with `markerLayers` entries produces the correct Manual refs on Alloy and/or SpawnsArmies,
  per whatever routing rule the ARCH Expert ratifies.

## Out of scope
- Editing `STEP51_OverlayLayerDataModel_UI.md` — corrected here as a successor ticket per
  dispatching instruction, its own file stays untouched.
- Dispatching STEP51, STEP60, or STEP66 to the Coder — this ticket only names them as blockers.
- Resolving the Manual-layer-to-domain routing question — routed to the ARCH Expert, not decided
  here.
- Correcting `work_orders/IMPLEMENTATION_STATUS.md` line 201's stale "confirmed IMPLEMENTED"
  claim — a separate, small follow-up against a shared ledger file, not this ticket's target.
- Any rendering consumer (`MapCanvas_IconLayer_UI.cpp`, §14.9) — unaffected either way, same
  out-of-scope posture STEP51 itself states.

## Files touched (once dispatchable — none today)
- `src/ui/Application_OverlaySetup_UI.cpp` — `SeedMarkerDomains`, once it exists via STEP51.
- Possibly `src/ui/OverlayLayer_Settings_UI.h` — only if the ARCH routing ruling requires an
  `index`-semantics change on `OverlaySubLayerRef_UI` for the Manual/Alloy/SpawnsArmies case; not
  assumed here.
- `src/ui/OverlayLayer_Settings_UI_Test.cpp` — new assertions, once the above lands.

## Verify
Not acceptance-testable today — no target function exists. Once STEP51+60+66 are coded and the ARCH
Expert has ruled on the routing question, acceptance is: a `MakeDefaultMapRecipe()`-derived fixture
extended with non-empty `recipe.markerLayers` produces the Manual `OverlaySubLayerRef_UI` entries the
ratified routing rule predicts, alongside the already-correct Procedural entries STEP51's existing
test plan (item 3) covers — full solo rebuild + the `OverlayLayer_Settings_UI_Test` suite green.
No performance estimate applies (Constitution §7) — same posture STEP51 itself states: compile-time
struct + `O(layerCount)` seeding, not a hot path.
