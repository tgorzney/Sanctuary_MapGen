# STEP234 — Global Delete key deletes selected manual instances (Markers/Props/Decals)

**Layer:** UI. **Domain:** new `ManualInstanceDelete_UI.h/.cpp`, `Application_UI.h`,
`Application_Draw_UI.cpp` (or new `Application_DeleteKey_UI.cpp`), `MapCanvas_UI.h`
(`ClearSelection`), `MarkersTab_UI.h` (tab-local selection clear). **Sequence:** independent — no
file overlap with STEP235/STEP236.

Ratifies `work_orders/DESIGN_MarkerLink_R1.md` §0 (manual-only) + §1 (revised, universal).

## Session coordination

Check `ListAgents`/message peer sessions before touching EACH file listed above, not just once at
ticket start — a peer may start editing any of them mid-session. Confirm no conflict before each
edit.

## Problem

No keyboard shortcut exists anywhere in the UI. The one prior single-instance delete button
(Markers only) is dead code with no live call path. Props/Decals have no per-instance delete
mechanism at all, live or dead.

## Fix

1. New file `ManualInstanceDelete_UI.h` — plain template mirroring `HitTestManualInstances<GroupT>`'s
   shape (`ARCH_21_03_DragGestureGenericization.md`):
   ```cpp
   template<typename GroupT>
   int DeleteManualInstancesById(std::vector<GroupT>& instances, const std::vector<int>& identifiers,
                                 const std::function<bool(int layerIndex)>& isLayerLocked);
   ```
   Erases every transform whose `instanceIdentifier` is in `identifiers` AND whose layer is not
   locked. Returns count erased; locked/missing identifiers are a silent per-identifier no-op.
2. Three thin wrappers in the same file, binding each domain's existing lock predicate
   (`IsMarkerInstanceLayerLocked`, `IsPropInstanceLayerLocked`, `IsDecalInstanceLayerLocked`):
   `DeleteSelectedManualMarkerInstances`, `DeleteSelectedManualPropInstances`,
   `DeleteSelectedManualDecalInstances`.
3. New public method `MapCanvas::ClearSelection()` (`MapCanvas_UI.h`) — one line, wraps the existing
   private `ApplySelectionGesture(std::vector<OverlayInstanceKey_UI>{}, false, false)`.
4. New private `Application::ApplyGlobalDeleteShortcut()`, called from `RunOneFrame()` immediately
   after `DrawCanvasWindow()`, before `EndImguiFrame()`:
   - Return early if `ImGui::GetIO().WantTextInput`, `scenarioEditMode.IsActive()`, or
     `!ImGui::IsKeyPressed(ImGuiKey_Delete)`.
   - Read `canvas.SelectedInstanceKeys()`, skip any key with `bManual == false` (procedural — no
     persisted identity), partition the rest by `collection` (Markers/Props/Decals; ignore Units).
   - Call the matching wrapper(s) from step 2.
   - If anything was actually deleted: `canvas.ClearSelection()`; clear
     `tabState.markers.selectedManualInstanceIdentifier`/`selectedManualInstanceIdentifiers`/
     `manualInstanceSelectionAnchorIdentifier`; `previewDriver.NotifyParametersChanged()`.

## Verify

- New test for `DeleteManualInstancesById<GroupT>`: erases only the targeted identifiers, skips
  locked-layer members, no-ops on missing identifiers, returns correct count.
- New test for `ApplyGlobalDeleteShortcut` gating: `WantTextInput` blocks; `ScenarioEditMode` active
  blocks; empty/all-procedural selection is a no-op; mixed Marker+Prop+Decal selection deletes from
  all three.
- Existing suites (`MarkersTab_UI_Test`, `MapCanvas_SelectionSet_UI_Test`,
  `MapCanvas_GestureOwnership_UI_Test`) stay green.

## Out of scope

- Group/Layer/Bundle delete (already exists, unchanged).
- Procedural instances (no persisted identity — §0's ruling).
- The Link mechanic (`DESIGN_MarkerLink_R1.md` §3) — separate ticket, blocked on ARCH ratification.
