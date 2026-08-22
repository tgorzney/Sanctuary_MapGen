# STEP81 — Manual Marker Layers tab + the per-instance Layer picker

*Constitution §7. Executor: SanGen Coder. Authored by the SanGen UI Expert.
This is **Phase 2** of `work_orders/GAP_MarkerLayerAndSymmetry_PARAMS.md` ("Deferred UI work",
lines 61-65) — the ticket `STEP60_MarkerInstanceLayer_PARAMS.md` explicitly deferred twice, in its
"Out of scope" bullets 1 and 2 (`STEP60:248-257`).*

**Why two concerns in one ticket.** (b), the per-instance Layer picker STEP49 deferred, is a
`Combo_UI` over `recipe.markerLayers` — a list that has no authoring UI until (a) creates one.
STEP49's own out-of-scope wording names the dependency ("blocked on `Params::MarkerInstanceLayer`
not existing yet… No layer `Combo_UI` in this ticket", `STEP49:80-82`) and
`GAP_MarkerLayerAndSymmetry_PARAMS.md:62-65` bundles them in a single Phase-2 bullet for the same
reason. Splitting them would ship a layer list nothing can reference for one ticket's duration.

---

## ⚠️ Blocking dependencies — VERIFIED ABSENT FROM DISK, not assumed

Every item below was checked against the working tree while authoring this ticket. **This ticket is
not coder-dispatchable until STEP60 and STEP49 have both landed**, in that order:

| Required by this ticket | Ticket that creates it | State on disk |
|---|---|---|
| `Params::SymmetrySetting` | STEP60 §1 (via ARCH_16_01_NewParamsShapes.md §16.1) | **Absent** — `src/params/Symmetry_PARAMS.h` defines `SymmetryAxis`, `SymmetryDetection`, `SymmetryBlend` only |
| `Params::MarkerInstanceLayer` | STEP60 §1 | **Absent** — `src/params/MarkerInstance_PARAMS.h:17-29` holds only `MarkerTransform`/`MarkerInstanceGroup` |
| `MarkerTransform::layerIndex` | STEP60 §1 | **Absent** — `MarkerInstance_PARAMS.h:17-21` has `name`/`transform`/`alias`, no `layerIndex` |
| `MapRecipe::markerLayers` | STEP60 §2 | **Absent** — `src/params/MapRecipe_PARAMS.h:99-108` has `markers`, `propLayers`, `decalLayers`, no marker equivalent |
| `Ui::NextMarkerLayerId()` | STEP60 §2 | **Absent** — no `src/ui/MarkerLayerId_UI.h` exists |
| `Ui::DrawManualMarkers` + `ManualMarkersState` | STEP49 | **Absent** — no `src/ui/MarkersTab_Manual_UI.h`/`.cpp` exists |

Part (b) touches STEP49's files directly and cannot be written against a file that does not exist.
Part (a) can technically be built the moment STEP60 lands, but the two ship together — see
"Landing order" at the end.

## Root problem

STEP60 gave `MarkerInstanceLayer` a full PARAMS + IO home — the type, `layerId`, `layerIndex`, the
`MarkerGroups` wire array, the import-time clamp — and shipped it with **zero authoring UI**, by its
own design ("no rendering/overlay consumer", `STEP60:46`). Today the only way a `layerIndex` value
can change is a hand-edited `.sanmap` or a future importer; the only thing standing between that and
a corrupt recipe is `ClampMarkerLayerIndex` at import time (STEP60 §4). A designer cannot create a
marker layer, cannot reorder one, cannot set its symmetry, and cannot assign a hand-placed marker to
one.

Props and Decals have had exactly this UI since ARCH_12_ManualPropDecalLayers.md §12 (`src/ui/PropsTab_Manual_UI.h`/`.cpp`).
Markers are the third domain and the only one still missing it.

## Target files

**New:**
- `src/ui/MarkerLayerIndexRepair_UI.h` — the two pure `layerIndex` repair functions (see "File-size
  ruling" below for why these are split out rather than living in the tab header the way the Props
  precedent puts them).
- `src/ui/MarkersTab_ManualLayers_UI.h` / `.cpp` — the tab block itself.
- `src/ui/MarkerLayerIndexRepair_UI_Test.cpp` — unit coverage for the two repair functions.

**Modified:**
- `src/ui/MarkersTab_UI.h` — one include, one `MarkersTabState` member.
- `src/ui/MarkersTab_UI.cpp` — one call in `DrawMarkersTab` (`MarkersTab_UI.cpp:117-125`).
- `src/ui/MarkersTab_Manual_UI.h` / `.cpp` (STEP49's files) — part (b): the Layer picker and its
  label buffer.

## Layer & accuracy class

UI. Accuracy class: **Visual**. No compute, no math, no sim.

## Backend policy

N/A — no compute. No CPU/GPU dispatch decision exists in this ticket.

## ARCH rules invoked

- **Constitution §1** — UI sets PARAMS and owns no sim logic. This block writes
  `recipe.markerLayers` and repairs `recipe.markers`; it computes nothing.
- **Constitution §6** — validate, never trust: every index is range-checked before it is
  dereferenced or stored (`SelectedManualMarkerLayer`, the combo's resolve-before-store rule).
- **Constitution §8** — every limit is a named setting on the state struct, never a literal at a
  use site.
- **ARCH §1.5** (`ARCH_01_05_FileSizeCeilings.md`) — soft 100 / hard 150 lines per file, functions
  ≤ 40 lines, one primary type per file. Drives the file-size ruling below.
- **ARCH §8.4** (`ARCH_08_04_CoderScopeLaw.md`) — a coder never invents a missing type or field; it
  is reported. Drives the "Toggle" ruling and the Radial-bit finding below.
- **ARCH §12** (`ARCH_12_ManualPropDecalLayers.md`) — the Props/Decals manual-layer precedent this
  block mirrors.
- **ARCH §16.1** (`ARCH_16_01_NewParamsShapes.md`) — the ratified `MarkerInstanceLayer` /
  `MarkerRuleLayer` split, and the `SymmetrySetting` shared struct.
- **ARCH §16.5** (`ARCH_16_05_MarkerTransformFields.md`) — `layerIndex` vs.
  `symmetryGroupIdentifier` on `MarkerTransform`.
- **ARCH §14.13 item 3, Ruling 3** (`ARCH_14_13_OpenItems.md:84-104`) — manual props/decals are
  straight copy-through with **no** symmetry participation; its closing note records that manual
  **markers** are the deliberate, separately-ratified exception. See "Why markers get a symmetry
  control and props do not" below.

---

## ⚠️ Two distinctions the coder must not collapse

**1. `MarkerInstanceLayer` is NOT `MarkerRuleLayer`.** ARCH_16_01_NewParamsShapes.md §16.1 ratifies them as separate types in
separate domains, and `ARCH_16_01_NewParamsShapes.md:56-63` names the shape asymmetry explicitly so
it is "never assumed away": `MarkerRuleLayer` is a real **container** (owns
`std::vector<MarkerRule> rules`); `MarkerInstanceLayer` is **flyweight metadata only** — it owns no
`MarkerTransform` vector, because the instances live in `MarkerInstanceGroup.transforms` and
reference their layer by index. Both carry a `Params::SymmetrySetting symmetry`; they are not
interchangeable, and this ticket touches **only** `MarkerInstanceLayer`. The procedural side belongs
to `STEP80_MarkersTabRulesLayerSymmetry_UI.md` (see "Coordination").

**2. `layerIndex` is NOT `layerId`.** Per STEP60 §1/§2 Ruling 1 and `ARCH_14_13_OpenItems.md:41-51`,
the two coexist and serve different purposes:
- `MarkerTransform::layerIndex` — a **plain vector position** into `recipe.markerLayers`. It IS
  renumbered on reorder and clamped on delete. That renumbering is the entire point of part (a)'s
  repair functions.
- `MarkerInstanceLayer::layerId` — **stable identity**, `-1` sentinel for unassigned, minted once at
  creation by `NextMarkerLayerId()` and **never** renumbered by anything in this ticket. The repair
  functions must not touch it, read it, or be tempted to sort by it.

A repair function that renumbers `layerId`, or an Add button that sets `layerIndex` instead of
`layerId`, is the exact defect this section exists to prevent.

---

## Part (a) — `MarkersTab_ManualLayers_UI` (the Manual Marker Layers block)

`src/ui/PropsTab_Manual_UI.h`/`.cpp` is the precedent. Read both in full before starting. The
mapping is close but **not** total — every divergence is enumerated below, and none of them is
optional.

### File-size ruling (ARCH_01_05_FileSizeCeilings.md §1.5) — a deliberate divergence from the precedent

`PropsTab_Manual_UI.h` is 143 lines, i.e. sitting 7 lines under the **hard** ceiling and 43 over the
soft one. The marker header carries the same content plus a symmetry control, so mirroring the
precedent's file layout verbatim would land it at or past 150.

**Ruling: split `ClampMarkerLayerIndicesForRemovedLayer` and `RenumberMarkerLayerIndicesForReorder`
into their own header, `src/ui/MarkerLayerIndexRepair_UI.h`.** They are pure, imgui-free, and
independently testable — exactly the profile that already justified `MarkerLayerId_UI.h` being its
own single-function file (STEP60 §2). This keeps both new headers under the soft-100 ceiling and
gives the repair logic a test target that needs no window. This is a divergence from
`PropsTab_Manual_UI.h`'s file layout, not from its logic; the function bodies are otherwise
type-substituted mirrors. **Do not** retrofit the equivalent split onto the Props header — that is a
separate cleanup, out of scope here.

### `src/ui/MarkerLayerIndexRepair_UI.h` — the two repair functions

Type-substituted mirrors of `PropsTab_Manual_UI.h:98-108` and `:115-133`. **The nesting is
structurally identical** — `PropInstanceGroup.transforms[].layerIndex`
(`PropInstance_PARAMS.h:19,24`) and `MarkerInstanceGroup.transforms[].layerIndex`
(`MarkerInstance_PARAMS.h:23-29` once STEP60 adds the field) are the same two-level walk — so the
loop bodies transfer with no change beyond the type names.

```cpp
// src/ui/MarkerLayerIndexRepair_UI.h — the `MarkerTransform::layerIndex` repairs the Manual Marker
// Layers list runs on delete/reorder. Layer: UI. Pure, imgui-free, testable without a window —
// same headless posture as MarkerLayerId_UI.h (STEP60 §2) and UniqueNameList_UI.h.
// Both functions touch ONLY `layerIndex` (plain vector position). `MarkerInstanceLayer::layerId`
// is stable identity and is NEVER renumbered here (ARCH_14_13_OpenItems.md §14.13 item 3 Work-Order A; STEP60 §2).
#pragma once
#include <vector>
#include "../params/MarkerInstance_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// A removed layer CLAMPS every referencing transform to layer 0 rather than dropping the marker —
// the same deliberate divergence from `DropUnitRulesForRemovedArmy` that STEP22 ruling #5 made for
// props (`PropsTab_Manual_UI.h:92-97`): a marker losing its layer tag is still a real marker.
// Matches the clamp-to-0-on-out-of-range semantic STEP60 §4's import-side `ClampMarkerLayerIndex`
// already applies. Reports whether the recipe moved.
bool ClampMarkerLayerIndicesForRemovedLayer(std::vector<Params::MarkerInstanceGroup>& markers,
                                            int removedLayerIndex);

// The Reorder-signal counterpart: keeps every transform's `layerIndex` correct after
// `recipe.markerLayers` is reordered source -> target (the same erase-then-insert move
// `ApplyDraggableListSignal` performs, `DraggableListWidget_UI.h:42-60`). Identical shape/math to
// `RenumberPropLayerIndicesForReorder` (`PropsTab_Manual_UI.h:115-133`).
bool RenumberMarkerLayerIndicesForReorder(std::vector<Params::MarkerInstanceGroup>& markers,
                                          int sourceLayerIndex, int targetLayerIndex, int layerCount);

} // namespace Ui
} // namespace SanmapGen
```

Bodies: copy `PropsTab_Manual_UI.h:98-108` and `:115-133` verbatim, substituting
`PropInstanceGroup` → `MarkerInstanceGroup` and the parameter name `props` → `markers`. **Do not
re-derive the reorder arithmetic** — the three-branch shift in `PropsTab_Manual_UI.h:126-131` is
already correct and already matches `ApplyDraggableListSignal`'s reorder postcondition; re-deriving
it is how the legacy off-by-one that header's comment describes gets reintroduced.

**Naming note, to prevent a false collision report:** STEP60 §4 introduces
`ClampMarkerLayerIndex` (singular) in `src/io/MapImporter_Markers_IO.cpp`. That is the IO-layer
single-transform import clamp; this is the UI-layer whole-recipe delete repair. Different namespace,
different name, no conflict — do not merge or "deduplicate" them.

### `src/ui/MarkersTab_ManualLayers_UI.h` — state + inline helpers + the draw declaration

```cpp
struct ManualMarkerLayersState {
    SectionState       section;
    ColorSwatchOptions previewColorOptions;                     // picker only, no RGBA fields
    ScalarSliderRange  iconScaleRange{ 0.1f, 10.0f, 0.0f };     // same bounds as props (§8)

    bool           bUseGroupColor = false;                      // one tint for every layer
    float          groupColor[kColorSwatchChannelCount] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float          layerIconScale = 1.0f;
    RealtimeToggle groupColorToggle;
    RealtimeToggle layerIconScaleToggle;

    // ONE shared toggle set for the SELECTED row's own color/scale — `Params::MarkerInstanceLayer`
    // is a pure round-tripping type and cannot carry a `RealtimeToggle` member, exactly as
    // `PropInstanceLayer` cannot (`PropsTab_Manual_UI.h:55-60`).
    RealtimeToggle selectedLayerColorToggle;
    RealtimeToggle selectedLayerIconScaleToggle;

    SectionState   symmetrySection;                             // NEW vs. props — see below
    int            selectedLayerIndex = -1;
};
```

Inline helpers, direct mirrors — `SelectedManualMarkerLayer` (`PropsTab_Manual_UI.h:69-73`),
`EffectiveManualMarkerLayerColor` (`:76-79`), `ManualMarkerLayerRowLabel` (`:82-84`, fallback
`"Marker Layer"`), and `NextMarkerLayerName(int layerCount)` → `NextUniqueLabel("Marker Layer",
layerCount)` (`:90`, `UniqueNameList_UI.h:47-50`).

`ManualMarkerLayerRowLabel` **must be declared in this header, not made file-local in the `.cpp`** —
part (b)'s combo reuses it so the picker never shows a blank row for an unnamed layer.

Declaration — note the parameter list is **shorter** than
`DrawManualPropLayers` (`PropsTab_Manual_UI.h:137-139`):

```cpp
// `markers` is `recipe.markers`, repaired here when a layer is deleted or reordered.
// No `Data::PlacementInstances*` parameter — see divergence 1.
// No `Pipeline::PreviewDriver*` parameter — see the dirty-flag posture below.
void DrawManualMarkerLayers(ManualMarkerLayersState& state,
                            std::vector<Params::MarkerInstanceLayer>& markerLayers,
                            std::vector<Params::MarkerInstanceGroup>& markers);
```

### `src/ui/MarkersTab_ManualLayers_UI.cpp` — the composition

Mirror `PropsTab_Manual_UI.cpp`'s five file-local functions, minus one, plus one:

| Props function | Marker equivalent | Change |
|---|---|---|
| `DrawLayerSettings` (`:20-27`) | same | none — "Use Group Color" + "Layer Icon Scale" |
| `DrawLayerList` (`:30-41`) | same | `DraggableList<Params::MarkerInstanceLayer>::Render`, id `"manualMarkerLayers"` |
| `ApplyLayerListSignal` (`:49-68`) | same | calls the two repair functions from the new header |
| `DrawLayerListButtons` (`:72-79`) | same + `layerId` | **divergence 3** below |
| `DrawSelectedLayer` (`:84-100`) | same + symmetry | **divergence 2** below |
| `DrawTransformList` (`:104-123`) | — | **NOT PORTED — divergence 1** below |

`DrawManualMarkerLayers`'s body mirrors `PropsTab_Manual_UI.cpp:127-141` exactly, minus the
`DrawTransformList` call, ending with the same `if (bLayersMoved) MakeNamesUnique(markerLayers);`
repair.

**`ApplyLayerListSignal` ordering is load-bearing and must be copied, not reasoned about afresh:**
`RenumberMarkerLayerIndicesForReorder` runs **BEFORE** `ApplyDraggableListSignal` moves the layers
vector, because the renumber needs the pre-move layer count
(`PropsTab_Manual_UI.cpp:60-61` and its comment at `:44-46`);
`ClampMarkerLayerIndicesForRemovedLayer` runs **AFTER**, on the delete path
(`PropsTab_Manual_UI.cpp:63-64`). Then the selection is re-clamped (`:65-66`).

---

### The five divergences from the Props precedent

**Divergence 1 — no read-only transform list.** `PropsTab_Manual_UI.cpp:104-123`'s
`DrawTransformList` exists because the Props tab has nowhere else to preview the resolved
`Data::PlacementInstances` buffer. The Markers tab already has that list: `DrawPlacedMarkerList`
(`src/ui/MarkersTab_Placed_UI.h`, called at `MarkersTab_UI.cpp:123`). Porting `DrawTransformList`
would create a rival second view of the same buffer. **Do not port it.** Consequently
`ManualMarkerLayersState` carries no `transformListSection` / `transformRowHeight` /
`transformListHeight` (cf. `PropsTab_Manual_UI.h:44,63-64`), the `.cpp` includes no
`VirtualListWidget_UI.h` or `PlacementInstances_DATA.h`, and the header forward-declares no
`Data::PlacementInstances`. This is what buys the line budget for the symmetry section.

**Divergence 2 — the layer-level symmetry control (props has none).** `DrawSelectedLayer` gains,
after the icon-scale slider:

```cpp
if (DrawSectionBegin("Layer Symmetry", state.symmetrySection)) {
    DrawPlacementSymmetryAxes("markerLayerSymmetry", layer->symmetry.bSymmetryUseGlobal,
                              layer->symmetry.symmetryMask, nullptr);
    DrawSectionEnd();
}
```

`DrawPlacementSymmetryAxes` (`PlacementRuleSections_UI.h:112-113`,
`PlacementRuleSections_UI.cpp:17-25`) takes flat `bool&` / `int&` references, so
`SymmetrySetting`'s two mask fields bind directly — no wrapper, no new widget. This is the same
call shape the procedural marker rule already makes at `MarkersTab_UI.cpp:100-101`.

**Why markers get this control and props/decals do not.** `ARCH_14_13_OpenItems.md:84-104`
Ruling 3 establishes that manual props/decals are straight copy-through with **no symmetry
participation** — `PropTransform`/`DecalTransform` carry no symmetry fields at all, deliberately.
That same ruling's closing note (`:101-104`) records that manual **markers** diverge: `MarkerTransform`
DOES participate, via `symmetryGroupIdentifier` (ARCH_16_05_MarkerTransformFields.md §16.5), as "a deliberate, separately-ratified
exception the human required for markers specifically." So the presence of this control on the
marker tab and its absence on the props tab are both correct, and neither should be "harmonized."

**What the control means today, stated so it is not oversold.** `MarkerInstanceLayer::symmetry` has
**no consumer** — STEP60 §1's own field comment says so, and STEP60's out-of-scope list
(`:264-267`) confirms no ticket yet exists for the drag-follow half. This control gives the ratified
field its authoring surface and round-trips through STEP60's `MarkerGroups` IO. It does not place,
mirror, or move anything. Do not add a tooltip claiming otherwise, and do not add a "Place
Symmetric" button — `STEP61_ManualMarkerSymmetryAuthoring_UI.md` is **retired** (STEP60's amendment
banner, `:8-11`).

**Divergence 3 — the Add button mints a `layerId`.** `PropsTab_Manual_UI.cpp:72-79` sets only
`layer.name`, because STEP56's `layerId` retrofit is still unimplemented for props. Markers ship
with `layerId` from day one (STEP60's whole premise, `:20-25`), so:

```cpp
bool DrawLayerListButtons(std::vector<Params::MarkerInstanceLayer>& markerLayers,
                          ManualMarkerLayersState& state) {
    if (!ImGui::Button("Add Marker Layer")) return false;
    Params::MarkerInstanceLayer layer;
    layer.name    = NextMarkerLayerName(static_cast<int>(markerLayers.size()));
    layer.layerId = NextMarkerLayerId(markerLayers);   // MarkerLayerId_UI.h, STEP60 §2 — INCLUDE it,
                                                       // do not re-derive max-plus-one inline
    markerLayers.push_back(layer);
    state.selectedLayerIndex = static_cast<int>(markerLayers.size()) - 1;
    return true;
}
```

`NextMarkerLayerId` must be called **before** `push_back` — it scans for `max(layerId) + 1`, and a
default-constructed row already in the vector carries the `-1` sentinel, which is harmless but makes
the call order look optional when it is not (call it after and the new layer scans itself).

**Divergence 4 — TWO name-uniqueness regimes coexist in the markers domain. Do not cross-wire
them.** This is the single most likely mistake in this ticket:

- `MakeNamesUnique(recipe.markerLayers)` — **this ticket**. **Cosmetic.** `MarkerGroups` exports as
  a plain JSON array (STEP60 §3), so a duplicate name collides with nothing. Run for UX parity with
  the Armies/Areas/Props tabs, same as `PropsTab_Manual_UI.cpp:137-139`.
- `MakeNamesUnique(recipe.markers)` — **STEP49's** call (`STEP49:49-50`). **Load-bearing.**
  `MarkerInstanceGroup::name` is a real `.sanmap` dictionary key
  (`markers[group.name].transforms[transform.name]`), and STEP60's asymmetry section (`:78-88`)
  spells out that a collision silently drops an entry.

Both compile — `MakeNamesUnique` is constrained only to "has a `std::string name` member"
(`UniqueNameList_UI.h:19-42`) and every one of these types qualifies. The compiler will not catch a
swap. Part (a) touches **only** `markerLayers`; it must never call `MakeNamesUnique(markers)`.

**Divergence 5 — the requested "toggle" has no PARAMS home (ARCH_08_04_CoderScopeLaw.md §8.4 report, not an invention).**
The list's per-row visibility and lock affordances are drawn unconditionally by the shared widget
(`DraggableListWidget_UI.h:106-118`) and emit `ToggleVisibility` / `ToggleLock` signals. But
`MarkerInstanceLayer` — as ratified in `ARCH_16_01_NewParamsShapes.md:25-30` and as specified in
STEP60 §1 — carries **no `bEnabled` and no `bHidden`**. (Contrast `MarkerRuleLayer`, ARCH_16_01_NewParamsShapes.md §16.1
lines 14-21, which has both; that is STEP80's domain, not this one.)

**Ruling: the affordances stay inert, exactly as they already are for props.**
`ApplyLayerListSignal` handles `Select`, `Reorder` and `Delete` and lets the two toggle kinds fall
through to `ApplyDraggableListSignal`, which returns `false` for them
(`DraggableListWidget_UI.h:51`) — the identical posture at `PropsTab_Manual_UI.cpp:49-68`. Do **not**
invent a `bEnabled`/`bHidden` field on `MarkerInstanceLayer`, and do not hide the buttons (the shared
widget owns that strip; suppressing it per-caller is a widget change, not a tab change).
**Reported to the ARCH Expert as a field-request candidate, not fixed here** — a manual marker layer
arguably wants a hide toggle once an overlay consumer exists, but that is a PARAMS ratification plus
an overlay-layer consumer, neither of which exists.

---

### ⚠️ Pre-existing shared-widget defect: `DrawPlacementSymmetryAxes` silently strips the Radial bit

**Verified, affects this ticket directly, NOT introduced by it, and NOT fixed here.**

`DrawIndependentSymmetryAxes` repairs the mask on the way in via `ResolvedPlacementSymmetryMask`
(`PlacementRuleSections_UI.cpp:31-32`), which ANDs the mask against only the bits its 4-entry axis
table names (`PlacementRuleSections_UI.h:28-31,33-41,56-61`): `MirrorAcrossX`, `MirrorAcrossZ`,
`RotateHalfTurn`, `QuarterTurns`. `Params::SymmetryAxis::Radial` (`Symmetry_PARAMS.h:23`, bit 4) is
**not** in that table, so drawing the control **clears a Radial bit that was already set**.

Meanwhile STEP60 §3's `ReadMarkerGroupsJson` reads `"SymmetryMask"` as a free integer with no range
validation, and `radialSymmetryRepeatCount` round-trips through IO across many domains today with
**zero UI control anywhere in `src/ui/`** (confirmed by grep: no `radialSymmetryRepeatCount`
reference exists under `src/ui/` at all). So a `.sanmap` can legitimately arrive carrying a Radial
mask that this tab would silently destroy on first draw.

**Ruling: flag, do not fix, and do not let this ticket decide the fix.** Reasoning:
1. It is pre-existing and **universal** — every current caller is equally affected:
   `MarkersTab_UI.cpp:100`, `PropsTab_UI.cpp:86`, `PropsTab_Decals_UI.cpp:111`,
   `ArmiesTab_Units_UI.cpp:117`, and `SymmetryTab_UI` for the global mask. Markers are not being
   singled out, and this ticket introduces no new exposure class.
2. The fix is a change to the **shared** widget (extend the axis table to 5 and add a
   `radialSymmetryRepeatCount` slider bounded by `radialSymmetryRepeatCountMinimum`/`Maximum`,
   `Symmetry_PARAMS.h:30-31`). `STEP80_MarkersTabRulesLayerSymmetry_UI.md` needs the identical fix
   for `MarkerRuleLayer::symmetry`. One fix must serve both tickets; neither should land a private
   version.

**Action for the coder: report this to the human as a separate ticket candidate when this ticket is
dispatched. Do not absorb the shared-widget fix into this ticket.** Same posture STEP75 took with
the `NextArmyName` padding gap (`STEP75:43-49`). If the shared fix has already landed by
implementation time, this ticket needs no change — the call site is identical either way.

---

### Dirty-flag posture — silent, no `PreviewDriver`

`DrawManualMarkerLayers` takes **no `Pipeline::PreviewDriver*` parameter**, and the `.cpp` includes
no `PreviewDriver_PIPELINE.h`. Pass `nullptr` as `DrawPlacementSymmetryAxes`'s fourth argument;
`NotifyPlacementChange` is a null-guarded no-op (`PlacementRuleSections_UI.cpp:12-14`).

Justification: `recipe.markers` and `recipe.markerLayers` feed **no PROC stage** — STEP60 confirmed
zero `MarkerInstanceGroup` references anywhere under `src/proc/` (`STEP60:44-46`), and STEP49 adopts
the same silent posture for the same reason (`STEP49:72-77`). Notifying the driver for a change no
stage can consume is the "cheap tweak triggers a full regen" defect `PropsTab_Manual_UI.h` SCOPE
NOTE 1 (`:16-22`) exists to name. When the marker-overlay consumer eventually ships, these edits
become live for free by that design's own "manual sub-layers read `MapRecipe` directly" ruling.

Note the contrast to keep the file honest: the *procedural* rule stack in the same tab **does**
notify (`MarkersTab_UI.cpp:84`), because `recipe.markerRules` is hashed by the Placement stage. Two
blocks in one tab with opposite postures, both correct.

---

## Part (b) — the Layer picker on STEP49's per-instance editor

STEP49's "Selected instance editor" (`STEP49:53-68`) ends with "**No rotation/scale, no layerIndex
control**" and its out-of-scope list opens with "`layerIndex` / manual marker layers — blocked on
`Params::MarkerInstanceLayer` not existing yet… No layer `Combo_UI` in this ticket"
(`STEP49:80-82`). Part (a) removes that block.

**Retire the `layerIndex` half of both statements** — a factual supersession, the same kind of
correction STEP49 itself performed on the two stale "no PARAMS home" comments
(`STEP49:11-13`, and see the live one still at `MarkersTab_UI.h:22`). If STEP49's coder echoed the
"no layerIndex control" wording into a code comment in `MarkersTab_Manual_UI.h`, delete that clause.
**The rotation/scale half of the statement stands** — still out of scope, see below.

### The control

One `DrawCombo` (`Combo_UI.h:63-64`) in the selected-instance editor, after **Name** and before
**Position**, labelled `"Layer"`.

**Label buffer.** `ComboOptions::labels` is a borrowed `const char* const*`
(`Combo_UI.h:20-24`), and `recipe.markerLayers` is a vector of structs, so the labels must be
materialized. Add a reused member to STEP49's `ManualMarkersState`:

```cpp
std::vector<const char*> layerPickerLabels;   // rebuilt per frame; cleared, never reallocated
```

Rebuild it each frame with `clear()` + `push_back(ManualMarkerLayerRowLabel(layer))` over
`recipe.markerLayers` — `clear()` keeps the capacity, so this is amortized zero-allocation after the
first frames. Using `ManualMarkerLayerRowLabel` (part (a)'s header) rather than `layer.name.c_str()`
is what stops an unnamed layer from rendering as a blank, unpickable row.

### ⚠️ Do NOT bind `transform.layerIndex` directly to `DrawCombo`

`StepComboInteraction` resolves an out-of-range index to `-1` and **writes it back into the caller's
variable** (`Combo_UI.h:47-60`, via `ResolvedComboSelection` at `:29-33`). But `layerIndex`'s legal
domain is `[0, markerLayers.size())` with a `0` default and a clamp-to-0 convention — `-1` is not a
valid `layerIndex` value anywhere in the marker domain (STEP60 §1, §4; the sentinel `-1` belongs to
`layerId`, a different field — see the "two distinctions" section above). A direct bind lets an
empty `markerLayers` write `-1` into a live `layerIndex`, which STEP60's importer would later have
to warn about and repair.

**Ruling — mirror, gate, and store only a valid pick**, the same load/store mirror pattern
`MarkersTab_UI.h:78-100` already uses for `MarkerRule`'s int/range fields:

```cpp
// Drawn only when at least one layer exists; otherwise `transform.layerIndex` is left untouched
// (Constitution §6 — a widget never writes a value the PARAMS domain does not accept).
if (markerLayers.empty()) {
    ImGui::TextUnformatted("No marker layers yet — add one in Manual Marker Layers.");
} else {
    ComboOptions options;
    options.labels = state.layerPickerLabels.data();
    options.count  = static_cast<int>(state.layerPickerLabels.size());
    int pickedLayerIndex = transform.layerIndex;                 // mirror, not a direct bind
    DrawCombo("Layer", pickedLayerIndex, options);
    if (pickedLayerIndex >= 0) transform.layerIndex = pickedLayerIndex;   // store only a valid pick
}
```

The `>= 0` guard is the whole point: a stale `layerIndex` pointing past a list that shrank resolves
to `-1` inside the combo (so the closed row honestly shows `<none>`) without that `-1` reaching
PARAMS. The next delete/reorder repair, or STEP60's import clamp, brings the value back into range —
the same self-healing posture the rest of the domain uses.

**Scope: one picker on the selected instance only.** No multi-select, no bulk "assign all in this
group to layer N", no per-row layer badge in the instance list. Not asked for, not designed.

---

## Wiring into the tab

`src/ui/MarkersTab_UI.h` — include `MarkersTab_ManualLayers_UI.h`; add one member to
`MarkersTabState` (`MarkersTab_UI.h:39-75`):

```cpp
ManualMarkerLayersState manualLayers;
```

`src/ui/MarkersTab_UI.cpp` — one call inside `DrawMarkersTab` (`:117-125`):

```cpp
DrawManualMarkerLayers(state.manualLayers, recipe.markerLayers, recipe.markers);
```

**Ordering requirement.** This call must come **before** STEP49's `DrawManualMarkers(...)` call.
Part (b)'s picker reads the layer list this block authors, so authoring-before-reference keeps a
layer added this frame pickable on the same frame — the same read-after-populate ordering STEP60 §3
mandates for `ReadMarkerGroupsJson` before `ReadMarkersJson`. Final order in `DrawMarkersTab`:
globals → **manual marker layers** → manual markers (STEP49) → procedural rule stack → placed list.

Also delete the now-stale SCOPE NOTE 2 at `MarkersTab_UI.h:22` ("Editable MANUAL markers have no
PARAMS home at all") **if STEP49 has not already removed it** — STEP49 owns that deletion
(`STEP49:22-23`); check before touching, and do not double-report it as this ticket's change.

---

## Coordination — other agents own these; stay out

- **`work_orders/STEP80_MarkersTabRulesLayerSymmetry_UI.md`** — the PROCEDURAL side:
  `MarkerRuleLayer`, the two-level rule list, and moving `DrawPlacementSymmetryAxes` from the
  per-rule tier to the layer tier. Different type, different domain, different files. It landed on
  disk while this ticket was being authored; the boundary below was then **verified against the real
  file**, and its own out-of-scope section hands `MarkersTab_ManualLayers_UI`,
  `Params::MarkerInstanceLayer`, `recipe.markerLayers`, and the STEP49 layer picker explicitly to
  this ticket. The two agree — no negotiation needed.
  - **The one real merge point:** both tickets add a member to `MarkersTabState`
    (`MarkersTab_UI.h:39-75`) and both edit `MarkersTab_UI.cpp`. The members are distinct
    (`manualLayers` here, `selectedRuleLayerIndex` there) and the edits land in different functions
    (`DrawMarkersTab` here; `DrawRuleList`/`ApplyRuleListSignal`/`DrawRuleListButtons` there), so
    both changes are **purely additive** and do not conflict in either landing order.
  - STEP80 also renames `MapRecipe::markerRules` → `markerRuleLayers` (ARCH_16_01_NewParamsShapes.md §16.1, affecting
    `MapRecipe_PARAMS.h:56` and `MarkersTab_UI.cpp:81`) and splits `MarkersTab_UI.cpp` to stay under
    ARCH_01_05_FileSizeCeilings.md §1.5. **This ticket touches neither `markerRules`/`markerRuleLayers` nor any
    `MarkerRule`/`MarkerRuleLayer` type**, so if STEP80 lands first nothing here needs adjusting —
    re-confirm only that the `DrawManualMarkerLayers` call still sits in whichever file
    `DrawMarkersTab` ended up in after that split.
  - **Confirmed co-affected by the Radial-bit defect:** STEP80 moves the same
    `DrawPlacementSymmetryAxes` call to its layer tier and does not address the strip either. The
    shared fix serves both; neither ticket should land a private version.
  - Both tickets are blocked by the same Radial-bit widget defect above. Neither should fix it
    privately.
- **`work_orders/STEP79_MarkerRuleLayerProcConsumer_PROC.md`** — PROC-side consumer for the
  procedural rule layers. Not this ticket's layer, not this ticket's domain. No overlap: part (a)
  writes PARAMS the PROC layer does not read.

---

## Explicit out-of-scope

- **The drag-and-follow interaction.** `DESIGN_MarkerLayerSymmetry_R2.md` §1 (the gesture-start
  orbit match, the cached `{slot → MarkerTransform}` table, live per-frame recompute, the
  mid-drag cardinality rules) and §2 (the Spawn/Army orphan ruling) are a **separate, currently
  unwritten ticket**, blocked on STEP47/48/49 existing as real code. STEP60's out-of-scope list
  says the same (`:264-267`): the design is ratified as design (ARCH_16_MarkerLayerSymmetry.md §16), but no coder ticket
  exists for the drag/UI half. **This ticket ships the layer list, its symmetry setting, and the
  picker — not the live drag behaviour.** Nothing in these files may read or write
  `MarkerTransform::symmetryGroupIdentifier`.
- **`MarkerTransform::symmetryGroupIdentifier` itself** (ARCH_16_05_MarkerTransformFields.md §16.5) — no field, no control, no
  consumer here.
- **Any consumer for `MarkerInstanceLayer::symmetry`.** The control authors the field; nothing
  resolves an orbit from it.
- **Fixing `DrawPlacementSymmetryAxes`'s Radial-bit strip** or adding any
  `radialSymmetryRepeatCount` control — shared-widget change, co-owned with STEP80, reported above.
- **A `bEnabled`/`bHidden` field on `MarkerInstanceLayer`** — reported to ARCH, not invented
  (divergence 5).
- **Any rendering / overlay / compositor consumer** of `markerLayers`, `layerIndex`, `color`, or
  `iconScale`. `color`/`iconScale` are authored and round-tripped; nothing draws with them yet, the
  same posture props have had since ARCH_12_ManualPropDecalLayers.md §12.
- **`Data::PlacementInstances` correlation column** (`manualLayerId`) or any PROC resolution for
  manual markers — `ARCH_14_13_OpenItems.md`'s Work-Order B, unscheduled, and its props/decals half
  is not this ticket's either.
- **Rotation / scale editing and terrain-height snapping** on manual markers — still out of scope
  from STEP49, unchanged by this ticket.
- **Retrofitting the `MarkerLayerIndexRepair_UI.h` file split onto `PropsTab_Manual_UI.h`**, or
  changing any props/decals behaviour.
- **Anything under `src/proc/`, `src/io/`, or `src/params/`.** This is a pure UI ticket: STEP60
  already landed every PARAMS and IO change it needs. If a coder finds itself editing
  `MarkerInstance_PARAMS.h`, STEP60 has not landed and this ticket is not yet dispatchable.

## Files touched

- `src/ui/MarkerLayerIndexRepair_UI.h` — **new.** `ClampMarkerLayerIndicesForRemovedLayer`,
  `RenumberMarkerLayerIndicesForReorder`.
- `src/ui/MarkersTab_ManualLayers_UI.h` — **new.** `ManualMarkerLayersState`, the four inline
  helpers, `DrawManualMarkerLayers` declaration.
- `src/ui/MarkersTab_ManualLayers_UI.cpp` — **new.** The imgui composition.
- `src/ui/MarkerLayerIndexRepair_UI_Test.cpp` — **new.** Repair-function coverage.
- `src/ui/MarkersTab_UI.h` — include + `ManualMarkerLayersState manualLayers;`.
- `src/ui/MarkersTab_UI.cpp` — one `DrawManualMarkerLayers` call, ordered before STEP49's block.
- `src/ui/MarkersTab_Manual_UI.h` — STEP49's file: `std::vector<const char*> layerPickerLabels;` on
  `ManualMarkersState`; retire the "no layerIndex control" clause if present.
- `src/ui/MarkersTab_Manual_UI.cpp` — STEP49's file: the Layer `Combo_UI` block.
- The relevant `CMakeLists.txt` — register the new `.cpp` and the new test target alongside the
  existing UI sources/tests.

## Acceptance test

Full solo rebuild + `ctest -C Debug`, entire suite green, no pre-existing test's assertions altered.

**New unit tests — `MarkerLayerIndexRepair_UI_Test.cpp`** (no window needed; mirror the assertion
shape of the existing props coverage — locate it first and match it rather than inventing a style):
- `ClampMarkerLayerIndicesForRemovedLayer`: a `markers` fixture with transforms at
  `layerIndex` 0/1/2 across **two** `MarkerInstanceGroup`s (both groups must be walked). Removing
  layer 1 leaves 0 → 0, 1 → 0 (clamped, **not** dropped — the instance still exists), 2 → 1.
  Reports `true`. Removing with `removedLayerIndex < 0` is a no-op reporting `false`.
- `RenumberMarkerLayerIndicesForReorder`: assert **both** drag directions. Source 0 → target 2
  gives 0 → 2, 1 → 0, 2 → 1. Source 2 → target 0 gives 2 → 0, 0 → 1, 1 → 2. A source outside
  `[0, layerCount)` is a no-op reporting `false`; `clampedTarget == sourceLayerIndex` reports
  `false`. Post-condition must agree with `ApplyDraggableListSignal`'s own reorder result on a
  parallel vector — assert that agreement directly, since the two moves must stay in lockstep.
- **`layerId` is never touched**: build a `markerLayers` fixture with non-contiguous ids
  (e.g. `{5, 0, 9}`), run both repairs, assert every `layerId` is byte-identical afterwards. This is
  the regression guard for the `layerIndex`/`layerId` conflation this ticket warns about twice.
- `NextMarkerLayerId` still behaves after an add/delete cycle: from ids `{5, 0, 9}` it yields `10`;
  deleting the `9` row and adding again yields `6` (max-plus-one over what remains — id reuse is
  explicitly not a hazard, STEP60 §2 Ruling 1).

**Combo-binding test** (headless — `StepComboInteraction` needs no window, `Combo_UI.h:47-60`):
with an empty options list and a mirror seeded from `layerIndex = 0`, confirm the mirror resolves to
`-1` **and** that the `>= 0` store guard leaves a `transform.layerIndex` of `0` unmodified. This is
the specific defect part (b)'s ruling exists to prevent; it must be asserted, not just described.

**Manual acceptance (human, at the app):**
1. Add three marker layers; confirm names auto-seed uniquely and each gets a distinct `layerId`.
2. Assign a hand-placed marker to layer 2 via the new picker.
3. Reorder layer 2 to the top; confirm the picker still shows that same marker on that same layer
   (the renumber followed it) — this is the whole point of the repair pair.
4. Delete that layer; confirm the marker survives, now on layer 0, and is **not** deleted.
5. Set a non-default symmetry mask on a layer, export, re-import; confirm the mask round-trips via
   STEP60's `MarkerGroups` `SymmetryMask` key.
6. With zero layers, confirm the picker shows the hint line and writes nothing.

**Greps that must come back clean:**
- No `PreviewDriver` / `NotifyParametersChanged` reference in either new `.cpp` or `.h`
  (the same check STEP49's acceptance test specifies, `STEP49:94-95`).
- No `MakeNamesUnique(` call taking `markers` (as opposed to `markerLayers`) anywhere in
  `MarkersTab_ManualLayers_UI.cpp` — divergence 4.
- No `MarkerRule` / `MarkerRuleLayer` / `markerRules` / `markerRuleLayers` reference in any file
  this ticket creates — the STEP80 domain boundary.
- No `symmetryGroupIdentifier` reference anywhere in this ticket's files.
- Every new file under the ARCH_01_05_FileSizeCeilings.md §1.5 hard-150 ceiling, every function ≤ 40 lines. Report the actual
  line counts in the completion note; if any file exceeds the soft-100, say so explicitly rather
  than letting it drift.

## Landing order

`STEP60` → `STEP49` → **this ticket**. Part (a) alone becomes buildable as soon as STEP60 lands, but
part (b) edits STEP49's files, so dispatching this before STEP49 would leave the ticket half-done
with no clean seam. If schedule pressure forces a split, part (a) is the separable half — part (b)
is not.
