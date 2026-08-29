# STEP205 — Ctrl/Shift multi-select clobbered by the canvas-sync path (Manual + Procedural marker lists)

**Layer:** UI. **Domain:** `src/ui/MapCanvas_UI.h`/`.cpp`, `src/ui/Application_UI.h`/`.cpp`,
`src/ui/MarkersTab_ManualLayerRowBody_UI.h`/`.cpp`, `src/ui/MarkersTab_ManualInstanceSelection_UI.h`,
`src/ui/MarkersTab_ManualLayers_UI.h`/`.cpp`, `src/ui/MarkersTab_Bundles_UI.h`/`.cpp`,
`src/ui/MarkersTab_TypeSections_UI.h`/`.cpp`, `src/ui/MarkersTab_UI.h`/`.cpp`,
`src/ui/MarkersTab_RuleLayers_UI.h`/`.cpp`, `src/ui/MarkersTab_RuleLayerInstances_UI.cpp`,
`src/ui/ProceduralInstanceRuleIndex_UI.h`. Pure CPU/imgui — no GPU or compute-dispatch involvement,
no accuracy-class concern.

## Root problem
A Markers-tab instance-list row click is supposed to update BOTH the tab-local multi-select set and
the canvas's real selection (ARCH §19.25 item 5, extended by §21.1). It does not stay in sync:

1. **`DrawManualInstanceRow`** (`src/ui/MarkersTab_ManualLayerRowBody_UI.cpp:34-47`) computes
   `bCtrl`/`bShift` from `ImGui::GetIO()` and correctly threads them into
   `ApplyManualInstanceSelectionClick(...)` (line 38-40), which writes the real Ctrl-toggle/Shift-range
   multi-select into `tabState.markers.selectedManualInstanceIdentifiers`. The SAME click then
   unconditionally calls `interaction.selectManualMarkerInstanceCallback(instanceIdentifier)` (line
   45-46) with no modifier information at all.
2. That callback is bound in `Application::WireCallbacks()` (`src/ui/Application_UI.cpp:157-159`) to
   `canvas.SelectManualMarkerByInstanceIdentifier(instanceIdentifier)`
   (`src/ui/MapCanvas_UI.cpp:47-50`), which calls the inline `SetSelection(const OverlayInstanceKey_UI&)`
   wrapper (`src/ui/MapCanvas_UI.h:238`) — `ApplySelectionGesture(key, /*bCtrlHeld=*/false,
   /*bShiftHeld=*/false)`, an unconditional **Replace**.
3. `MapCanvas::ApplySelectionGesture`'s Replace branch (`src/ui/MapCanvas_UI.cpp:76-88`) fires
   `selectionChangedCallback` with the new one-element set, which `Application::WireCallbacks()`'s
   closure (`src/ui/Application_UI.cpp:85-101`) uses to `.clear()` and rebuild
   `tabState.markers.selectedManualInstanceIdentifiers` from scratch — stomping the multi-select
   `ApplyManualInstanceSelectionClick` just wrote, within the same click/frame.

This is a real, live gap, not a misreading of already-fixed code: `ARCH_21_01_MultiSelectRepresentation.md`
(§21.1) ratified the full `OverlayInstanceKeySet_UI`/`ApplySelectionGesture` multi-select machinery and
explicitly scoped `SelectManualMarkerByInstanceIdentifier` to keep calling the single-key Replace
overload "byte-identical to today's behavior," while separately flagging *"Threading real Ctrl/Shift
state from a Markers-tab list click into this same path... is a natural follow-up, explicitly NOT
required by this ratification."* STEP166-169 (already shipped, confirmed by direct read of
`MapCanvas_Draw_UI.cpp`/`MapCanvas_SelectionGesture_UI.cpp`) built every piece of infrastructure that
follow-up needs (`ApplySelectionGesture`'s two overloads, the ordered `OverlayInstanceKeySet_UI`) —
this ticket is exactly that deferred follow-up.

**Procedural sibling, verified — does NOT share the exact "clobber" defect, but has a related, worse
gap.** `SelectProceduralMarkerInstanceByArrayPosition` (`src/ui/MapCanvas_UI.cpp:54-57`) has the same
Replace-only shape, but `DrawProceduralInstanceRow` (`src/ui/MarkersTab_RuleLayerInstances_UI.cpp:23-33`)
— the Procedural list's own row — never reads `ImGui::GetIO().KeyCtrl`/`KeyShift` at all today, and
there is no tab-local plural-selection field for Procedural instances to stomp. So nothing gets
clobbered locally — but Ctrl/Shift are simply inert on a Procedural instance row today: the canvas's
already-built multi-select set (ARCH §21.1) is completely unreachable from that list. Since this ticket
is widening the shared callback type anyway, fixing both siblings together is the coherent,
minimal-blast-radius scope — not scope creep.

## Fix approach
1. **`MapCanvas`-side (the actual behavior change).** Give `SelectManualMarkerByInstanceIdentifier` and
   `SelectProceduralMarkerInstanceByArrayPosition` two new parameters, `bool bCtrlHeld = false, bool
   bShiftHeld = false` (defaults preserve every other existing call site's Replace-only behavior
   byte-identically). Both call `ApplySelectionGesture(key, bCtrlHeld, bShiftHeld)` directly instead of
   the modifier-blind `SetSelection` wrapper.
2. **Widen the shared callback type** from `std::function<void(int)>` to
   `std::function<void(int, bool bCtrlHeld, bool bShiftHeld)>` at every declaration/pass-through site in
   the chain (mechanical signature edit only, no logic change, at each):
   `Application_UI.h` (`selectManualMarkerInstanceCallback`/`selectProceduralMarkerInstanceCallback`
   fields), `Application_UI.cpp` (the two `WireCallbacks()` lambdas, which now forward the two bools into
   step 1's new parameters), `MapCanvas_UI.h` (the two method declarations),
   `MarkersTab_ManualInstanceSelection_UI.h` (`ManualInstanceRowInteractionContext_UI::
   selectManualMarkerInstanceCallback`), `MarkersTab_ManualLayerRowBody_UI.h`/`.cpp`
   (`DrawLayerRowBody`'s parameter), `MarkersTab_ManualLayers_UI.h`/`.cpp` (`DrawLayerList`'s parameter),
   `MarkersTab_Bundles_UI.h`/`.cpp` (both tree-walk pass-throughs), `MarkersTab_TypeSections_UI.h`/`.cpp`,
   `MarkersTab_UI.h`/`.cpp` (`DrawMarkersTab`'s parameter and its call sites),
   `ProceduralInstanceRuleIndex_UI.h` (`ProceduralInstanceListContext_UI::
   selectProceduralMarkerInstanceCallback`), `MarkersTab_RuleLayers_UI.h`/`.cpp`
   (`DrawRuleLayerListBody`'s parameter).
3. **The two real leaf-behavior edits.** `DrawManualInstanceRow` (`MarkersTab_ManualLayerRowBody_UI.cpp:
   45-46`) already has `bCtrl`/`bShift` in local variables from its own `ApplyManualInstanceSelectionClick`
   call (line 38-39) — reuse those exact two values in the widened callback call instead of re-reading
   `ImGui::GetIO()` a second time. `DrawProceduralInstanceRow`
   (`MarkersTab_RuleLayerInstances_UI.cpp:31-32`) gains its own `ImGui::GetIO().KeyCtrl`/`KeyShift` read
   at the click site (mirroring the Manual row's existing convention) and passes both through.
4. A named type alias for the widened `std::function` signature is a reasonable readability improvement
   but not required — this codebase's existing convention repeats the literal `std::function<...>`
   spelling at each site; either is acceptable.

## Explicit out-of-scope
- The Procedural instance list gains no new tab-local plural-highlight field — it stays exactly as
  session-only/no-persistent-highlight as `DrawProceduralInstanceRow`'s own header comment already
  documents. Only the CANVAS's real multi-select (already fully built, ARCH §21.1) becomes reachable
  from a Procedural list click too.
- No change to `MapCanvas_SelectionGesture_UI.cpp`'s `ApplyClickGesture`/`ApplyMarqueeGesture` or to
  `MapCanvas_Draw_UI.cpp`'s `ApplyPointerInput` — those already route through `ApplySelectionGesture`
  correctly (STEP166-169, confirmed by direct read); this ticket only fixes the SHELL-MEDIATED
  list-click landing point, a separate call path.
- No change to `MarkersTab_ManualInstanceSelection_UI.h`'s drag-and-drop reparenting mechanics.
- No widening of the multi-select set's VISUAL rendering beyond the primary — a separate ticket.
- No introduction of a shared named callback-type alias is mandated — a coder preference, not a ruling.

## Acceptance test
Extend `src/ui/MapCanvas_Picking_UI_Test.cpp`'s `RunManualMarkerSelectionChecks` and
`RunProceduralMarkerListSelectionChecks`: after establishing a baseline single selection via the
existing no-modifier calls, call `SelectManualMarkerByInstanceIdentifier(otherId, /*bCtrlHeld=*/true,
/*bShiftHeld=*/false)` and assert the ORIGINAL id is still present in `canvas.SelectedInstanceKeys()`
(toggled-in, not replaced) and the new id is now primary. Repeat for the Shift-union case and for the
Procedural sibling (`SelectProceduralMarkerInstanceByArrayPosition`). Extend
`src/ui/MarkersTab_ManualInstanceListRows_UI_Test.cpp` with a Ctrl-held click on a second row and assert
the callback fired with `(secondInstanceIdentifier, /*bCtrlHeld=*/true, /*bShiftHeld=*/false)` while
`tabState`'s own `selectedManualInstanceIdentifiers` still contains the first row's id — proving the
tab-local write and the canvas-sync write agree instead of racing. Full solo rebuild + `ctest -C
Debug`: previously-passing suite stays green.
