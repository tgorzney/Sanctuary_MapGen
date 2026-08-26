# BRIEF — Markers UI Correction Round 2

Human-issued corrections after reviewing the shipped STEP121-126 work. Grounded in direct
investigation of the live code (not assumption) before drafting. Every item below is either a
confirmed bug (root cause verified by direct read) or a structural correction the human has now
stated twice; nothing here should be re-interpreted or narrowed without asking first — that failure
mode is exactly what caused this correction round.

## 1. Global section — not actually one line per type

**Confirmed root cause:** `DrawGlobalScaleRow` (`MarkersTab_Globals_UI.cpp:61-79`) has zero
`ImGui::SameLine()` calls. The category label sits on its own line above a 3-column block, and the
tallest column (`DrawSliderScalar`, `SliderScalar_UI.cpp:34-53`) is itself 3 lines tall (label /
track / value+toggle). Net: ~4 lines per type, not 1.

**Fix:** redraw as a genuine single ImGui line per type: `[Icon Button] [Label] [Scale slider,
compact] [Color swatch]`, all `SameLine()`-chained. Requires a compact slider variant (no own label
line, no own value-line) or manual layout bypassing `DrawSliderScalar`'s multi-line shape — this is
a real widget-composition question, not a one-line tweak. Route through design.

## 2. "Layer Icon Scale" (Manual Marker Layers block) — remove, confirmed dead

**Confirmed root cause:** `DrawManualMarkerLayerBlockSettings` (`MarkersTab_ManualLayers_UI.cpp:52-
59`) writes only to `ManualMarkerLayersState::layerIconScale`, a UI-scratch field with zero readers
anywhere in the codebase (grep-confirmed). The real per-layer scale is `Params::MarkerInstanceLayer::
iconScale`, already wired (STEP122). This field composes into nothing and never has.

**Fix:** delete the control and the dead `layerIconScale`/`layerIconScaleToggle` state fields. No
design round needed — pure removal of confirmed-dead code.

## 3. "Item Scale" (Global row) — rename + new default

**Confirmed:** label is literally `"Item Scale"` (`MarkersTab_Globals_UI.cpp:73`), bound to
`GlobalMarkerSettings::scaleAlloy/Plasma/Spawn`, default `0.17f` (`GlobalMarkerSettings_PARAMS.h:21-
23`).

**Fix:** rename label to `"Icon Scale (Global)"`; change default to `0.50f` for all three fields.
Simple field-level edit, no design round needed.

## 4. Remove "(Unassigned)" section

**Confirmed:** this section was MY OWN invention during STEP125 drafting (a bootstrap ruling never
asked for), not part of any human specification. `EnumerateMarkerTypeSectionNames`
(`MarkersTab_TypeSections_UI.cpp`) unconditionally appends `""` as a final bucket.

**Fix:** remove the unconditional append. Open question requiring a ruling: what happens to a
Bundle/Layer whose `markerTypeName` is legitimately empty (legacy import, or a brand-new "Add
Group"/"Add Layer" click before any Type section exists to seed it from)? Since every "Add Group"/
"Add Layer" affordance now lives INSIDE a specific Type section (STEP125's own design), a
freshly-created entry always inherits that section's type — the empty-type case should only occur
for legacy/imported data. Route through design: does legacy-empty data get silently absorbed into a
specific bucket (which one?), or does the tab need a minimal "assign a type" affordance for orphaned
legacy entries? Do not silently invent an answer — this is exactly the kind of unstated-default
decision that caused the last round of corrections.

## 5. Remove "Ungrouped Procedural Rules" / "Ungrouped Manual Marker Layers" as separate sections

**Human's exact words:** "Ungrouped markers get listed individually after all the groups" — this
means individual rows listed directly under the Type section after the Group tree, NOT corralled
into their own separately-headed/collapsible sub-section. STEP125 built two more labeled
`DrawSectionBegin("Ungrouped ...", ...)` sub-sections nested inside each Type section — that is the
thing being corrected, not just their position.

**Fix:** flatten. Within each Type section, after the Group/Bundle tree, list every ungrouped
`MarkerRuleLayer`/`MarkerInstanceLayer` of that type as plain rows with no enclosing section header.
Route through design (touches `MarkersTab_TypeSections_UI.cpp`'s composition directly).

## 6. Full hierarchy re-verification

After 4 and 5 land, re-verify the complete Type → Group → Layer → Instance nesting against the
human's original diagram end to end before calling this closed. Not a separate ticket — a acceptance
gate on the tickets from 4/5/13.

## 7. Remove Color Override from the row body (list)

**Confirmed:** deliberate duplication exists (`MarkersTab_ManualLayerRowBody_UI.cpp:36-41` body copy,
lines 87-104 header copy) because `TreeListWidget_UI` (the Bundle/Group tree) has NO header-extra
mechanism at all — confirmed by direct read of `TreeListWidget_UI.h`/`TreeListWidget_RowLayout_UI.h`:
no `drawRowHeaderExtra`/`headerExtraWidthPixels` parameters, no affordance strip whatsoever on tree
nodes/leaves.

**Fix, two-part, sequenced:**
(a) Give `TreeListWidget_UI` a header-extra slot, mirroring `DraggableList<T>::Render`'s own
    3-callback overload (STEP123's precedent) — a real shared-widget contract change, needs a design
    round + ARCH sign-off (mirrors how the original `DraggableList` header-extra slot itself was
    ratified).
(b) Once (a) lands, wire the Bundle tree's Manual-layer leaf-body to use it, then delete the body
    copy from `DrawLayerRowBody` entirely (the reason it was kept goes away).

## 8/9. Row header spacing bug — empty gap + lock-toggle/swatch overlap

**Confirmed root cause, single bug:** `RenderCollapsibleRow` (`DraggableListWidget_RowLayout_UI.h:24-
47`) computes the header-extra control's own start offset AND (indirectly, by passing an
already-reduced width into) `DrawRowAffordances`'s start offset
(`DraggableListWidget_RowAffordances_UI.h:46-50`) using the exact same formula — both land at the
same absolute X. The affordance strip (lock/delete) draws ON TOP of the header-extra control instead
of to its right, and the strip's own right edge falls `headerExtraWidthPixels` (90px) short of the
row's true right margin, leaving that as empty space.

**Fix:** `DrawRowAffordances`'s start offset must be computed relative to WHERE the header-extra
control finishes drawing (its own real right edge), not by re-deriving the same
"totalWidth - headerExtraWidthPixels - stripWidth" formula the header-extra placement already used.
This is a real widget bug in shared code (`DraggableListWidget_RowLayout_UI.h`/
`_RowAffordances_UI.h`) — affects every current and future `headerExtraWidthPixels` caller, not just
this row. No design round needed — this is a straightforward arithmetic bug fix once named, but
flag for a coder ticket with a real regression test (a synthetic-frame geometry assertion, not visual
inspection).

## 10. Symmetry toggle on the row header

Add a Symmetry on/off toggle to the Manual Marker Layer row header (same header-extra slot family as
Color Override), positioned LEFT of the existing lock/delete affordance strip, per the human's
explicit placement instruction. Depends on 7(a)'s header-extra-slot work landing first if this is to
go on Bundle-tree rows too (confirm scope: manual-only rows, per item 7's split, or does the toggle
also need to reach bundled layers via the same new tree header-extra slot — almost certainly yes,
since symmetry is a per-Layer property regardless of Bundle membership). Route through design
alongside item 7.

## 11. Selection-in-list doesn't highlight on canvas

**Confirmed root cause — NOT a broken wire, a wrong target.** The STEP126 wiring
(`selectedManualInstanceIdentifier` → `SetManualMarkerSelectionSource` →
`ComputeManualMarkerSelectionHighlight` → `DrawManualMarkerRoster`'s tint) is fully intact and does
recompute every frame. But it only recolors a small auxiliary dot roster
(`kManualMarkerBaseDotRadiusScreenPixels = 6.0f`, scaled by the now-`0.17f` global scale to
~1px radius — effectively invisible) drawn ALONGSIDE the real marker icon sprites. The actual visible
icon a user looks at is rendered by a completely separate pipeline
(`MapCanvas::DrawOverlayIconLayerPass` → `instance.bSelected` →
`MapCanvas_IconLayer_Draw_UI.cpp`'s `outSelected`/`outNonSelected` split), driven exclusively by
canvas click-picking (`selectedEntityIdentifier`/`HasSelection()`), with ZERO connection to
`selectedManualInstanceIdentifier`.

**Fix:** requires a real architectural decision — should list-selection and canvas-click-selection
converge on ONE selection representation (list-click sets the same state canvas-click sets, so the
REAL icon sprite gets the select treatment), or does the dot-roster highlight need to become visible
in its own right (e.g., a distinct outline/ring independent of icon scale)? The former is almost
certainly correct (avoids two parallel, ever-diverging "selected" concepts) but changes how canvas
click-picking and the Markers tab talk to each other — route through design, do not silently pick an
approach.

## 12. Symmetry grouping in the instance list

**Human's exact words:** "Fix Symetry did not group the instances by symetry group - instances that
are symetrical are supposed to be group in UI, and then non-symetrical list after the groups, and the
groups would be collapsable." `MarkerTransform::symmetryGroupIdentifier` already exists
(STEP68) and is populated by drag-materialize and the `MarkerSymmetryFixCommand_UI.cpp` repair tool,
but the Layer's own instance list (STEP126) currently lists every instance flat, ungrouped, ignoring
this field entirely.

**Fix:** partition a Layer's instance list by `symmetryGroupIdentifier` (0 = ungrouped/free,
non-zero = a real symmetry cluster); render each non-zero group as its own collapsible sub-list,
non-zero groups first, `symmetryGroupIdentifier == 0` instances listed flat after, as individual
rows, not their own group. Route through design — new UI shape, not a bug fix.

## 13. Procedural layers get no instance list / no selection

**Human's own original diagram explicitly shows this** (the "Procedural" layer in the worked example
lists `Alloy_05, Alloy_06` exactly like the Manual layers above it). STEP126's design round narrowed
this to manual-only, citing `Data::PlacementInstances`'s lack of cross-bake stable identity — and I
ratified that narrowing without checking it against the human's own diagram. **This was wrong and is
now overridden: build it.**

The identity-stability concern is real but solvable differently: selection here is a live,
UI-session-only concern (never persisted to `.sanmap`, resets on the next bake) — it does not need a
stable identity ACROSS bakes, only a valid one for the CURRENT bake's `Data::PlacementInstances`
snapshot. A per-frame positional index into the current snapshot (rebuilt every time the snapshot
changes, same shape as `ManualInstanceLayerIndex_UI` but keyed off array position instead of a
persisted field) is very likely sufficient. Route through design to confirm the exact mechanism and
whether symmetry-grouping (item 12) applies to procedural instances too (procedural symmetry is
handled differently — `ResolveEffectiveMarkerSymmetry` bakes multiple placements directly, there may
be no per-instance `symmetryGroupIdentifier` equivalent for procedural output; confirm before
assuming parity).

## 14. Select-color input control missing from UI

**Confirmed:** `selectColorAlloy/Plasma/Spawn/Default` (STEP124) are consumed only by
`MapCanvas_MarkerRosterDraw_UI.cpp`'s tint resolver — grep-confirmed zero appearances in any editing
surface (`MarkersTab_Globals_UI.*` or elsewhere). No control exists for the human to change them.

**Fix:** add a color-swatch input per type, most likely folded into the same Global-section row work
from item 1 (space is already tight there — this is exactly why item 1 needs a design pass rather
than a quick patch, since it's about to gain a 5th control per row: icon button, label, scale slider,
normal-color swatch, NOW select-color swatch). Route through design as part of item 1's redesign, not
a separate row.

## Delivery grouping recommendation

- **No design needed, straight to coder tickets:** 2 (delete dead field), 3 (rename+default), 8/9
  (header spacing bug + regression test).
- **Needs a design round (sangen-ui-expert), covering real widget/architecture decisions:** 1+14
  (Global row redesign, now 5 controls), 4 (Unassigned-bucket disposition), 5 (flatten ungrouped
  layers), 7 (TreeListWidget header-extra slot + Color Override body removal), 10 (Symmetry toggle
  placement, depends on 7), 11 (unify list-selection with canvas click-selection), 12 (symmetry
  grouping in instance list), 13 (procedural instance parity).
- **ARCH ratification required for:** 7(a) (new shared-widget contract, same class as the original
  `DraggableList` header-extra slot), 11 (selection-representation unification — a real
  cross-cutting rule), 13 (procedural per-frame index mechanism), 12 (new UI composition, if it
  introduces a new field/no PARAMS change likely, but the grouping UI shape itself should be
  recorded).
- Sequencing: 4/5 (hierarchy flattening) should land before 6's re-verification gate. 7(a) should
  land before 7(b)/10 (both depend on the new slot). 11 should be decided before 12/13 are built on
  top of it, since a wrong selection-representation choice would need both redone.
