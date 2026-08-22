# STEP55 — Retire the primary-toolbar "Regenerate" button, collapse to one trigger path

**Layer:** UI. **Domain:** `Application` shell (`DrawCanvasWindow`), `MapCanvas`, `SystemTab`.
**Sequence:** Phase 4.2, `work_orders/SEQUENCE_PreviewOverlayLayering.md`. Cites `ARCH_14_07_ViewToolbar.md` §14.7.

## Problem
`ARCH_14_07_ViewToolbar.md` §14.7 ("'Regenerate' is retired from the primary toolbar"):

> `Pipeline::PreviewDriver` already auto-derives refresh tier from parameter hashes
> (`NotifyParametersChanged()`); a manual full-regen button is the exact anti-pattern that system
> exists to replace. `MapCanvas::RequestRegeneration()` and `PreviewDriver::RequestMapUpdate()` are
> currently **two rival trigger paths** — per hit-list #3's "retire rival toggles" (applied here to
> a UI-level trigger, not a compute backend), these **must collapse to one call path.** Keep exactly
> one debug/System-panel affordance calling `RequestMapUpdate()` directly, for the one legitimate
> manual case `PreviewDriver`'s own docstring already names ("a change no parameter hash can see: a
> resize, a recipe reload, new stratum art") — not on the View toolbar.

The named docstring is `PreviewDriver_PIPELINE.h:41`, directly above `RequestMapUpdate()`:
```cpp
// A change no parameter hash can see — a resize, a recipe reload, new stratum art.
void RequestMapUpdate() { bNeedsMapUpdate = true; }
```

Today's wiring, confirmed by reading the real call sites:
- `Application::DrawCanvasWindow` (`src/ui/Application_Draw_UI.cpp:47`) draws the primary "Map
  Preview" toolbar's `ImGui::Button("Regenerate")`, calling `canvas.RequestRegeneration()`.
- `MapCanvas::RequestRegeneration()` (`src/ui/MapCanvas_UI.cpp:55-58`) increments a debug counter
  (`regenerationRequestCount`) and invokes the injected `regenerationCallback`.
- `Application::WireCallbacks()` (`src/ui/Application_UI.cpp:61`) binds that callback to
  `[this] { previewDriver.RequestMapUpdate(); }` — a pure pass-through today, but the two named
  APIs (`MapCanvas::RequestRegeneration()` and `PreviewDriver::RequestMapUpdate()`) are the "two
  rival trigger paths" ARCH is calling out: two call points a future caller could reasonably reach
  for, only one of which is the real door. Collapsing them to one name removes the ambiguity even
  though the current indirection happens to behave correctly.
- Separately, `SystemTab_UI.cpp` already has its own file-local `RequestRegeneration(Pipeline::
  PreviewDriver*)` free function (`SystemTab_UI.cpp:21-23`) that calls `previewDriver->
  RequestMapUpdate()` directly — but every existing call site of it (`DrawBackendSettings`'s two
  `Combo_UI`s, the determinism `Checkbox_UI`) fires automatically on a settings edit. None of them
  is the manual, human-clicked affordance the docstring's exception describes (resize / recipe
  reload / new stratum art — cases with no settings edit to hang a trigger off). That affordance
  does not exist yet anywhere in the System panel; today it only exists, misplaced, as the primary
  toolbar's "Regenerate" button.

## Fix

### 1. Remove the button from the primary Map Preview toolbar
`Application::DrawCanvasWindow` (`src/ui/Application_Draw_UI.cpp:45-56`):
```cpp
void Application::DrawCanvasWindow() {
    ImGui::Begin("Map Preview");
    if (ImGui::Button("Regenerate")) canvas.RequestRegeneration();
    ImGui::SameLine();
    if (canvas.HasSelection()) ImGui::Text("Selected entity: %u", canvas.SelectedEntityIdentifier());
    else                       ImGui::TextUnformatted("Selected entity: none");
    ...
```
Delete the `ImGui::Button("Regenerate")` line and its now-orphaned `ImGui::SameLine()` (it only
existed to keep the button and the selection text on one row; with the button gone the selection
text is the row's first item). Nothing else in this function changes — the region-sizing logic
from STEP44 is untouched.

This is **not gated on STEP54** (the View popup). The two are separate lines in the same function
with no shared state or ordering dependency: STEP54 adds a "View" button/popup elsewhere in this
toolbar; this ticket deletes the "Regenerate" button. Either can land first. (STEP54's own
rationale for not carrying an imperative regen action — it is presentation-only, per ARCH_14_07_ViewToolbar.md §14.7's
popup design — is *why* this button has nowhere else to go on the primary toolbar, but building
that popup is not a prerequisite for deleting a now-orphaned button.)

### 2. Collapse `MapCanvas::RequestRegeneration()` — remove it, not repoint it
**Finding:** zero remaining distinct callers once (1) lands. Its production call site was
exclusively the toolbar button just removed; its only other reference is the acceptance test
(`MapCanvas_Picking_UI_Test.cpp:69-73`), which exists solely to prove the callback plumbing this
ticket is retiring. `Application::WireCallbacks()`'s binding
(`canvas.SetRegenerationCallback([this] { previewDriver.RequestMapUpdate(); })`) was already a
pure pass-through — no MapCanvas-side transformation, filtering, or state beyond the debug counter
survives its removal. Per the ARCH ruling above ("these must collapse to one call path") and the
instruction to state the reasoning: **remove**, do not keep a pass-through. A pass-through with one
caller is exactly the second name the ARCH ruling says must not exist.

Remove from `src/ui/MapCanvas_UI.h`:
- `SetRegenerationCallback()` (lines 38-40)
- `RequestRegeneration()` declaration (line 52)
- `RegenerationRequestCount()` (line 65) — reads a counter that only existed to test the callback
  being removed; no other reader exists (confirmed by the same grep as above)
- `regenerationCallback` member (line 76)
- `regenerationRequestCount` member (line 82)

Remove from `src/ui/MapCanvas_UI.cpp`:
- `MapCanvas::RequestRegeneration()` and its preceding comment (lines 53-58)

Remove from `src/ui/Application_UI.cpp`:
- `canvas.SetRegenerationCallback([this] { previewDriver.RequestMapUpdate(); });`
  (`WireCallbacks()`, line 61)

Update `Application::WireCallbacks()`'s own header comment (`Application_UI.cpp:56-58`, "The three
injected seams that keep the layer graph downward-only") — it names three; after this removal only
two remain (`previewDriver.SetPreviewCompositeCallback`, `canvas.SetSelectionChangedCallback`).
Reword to "two."

Update `MapCanvas_UI.h`'s class-level header comment (lines 6-12), which currently states "the
ONLY way this widget can cause work to happen is the injected regenerate callback
(`Pipeline::PreviewDriver::SetPreviewCompositeCallback` is the same pattern)". After this ticket
`MapCanvas` causes **no** work to happen at all — it is draw + pick + pan/zoom only, a strictly
presentational widget (reinforcing, not just satisfying, "UI never simulates," ARCH_03_ModuleBoundaries.md §3.2). Reword
that sentence to state the widget causes no work and drop the now-inaccurate callback-pattern
comparison; keep the rest of the comment (the picking/`Picking_UI` description) unchanged.

### 3. Add the one legitimate manual affordance to the System panel
Add exactly one `ImGui::Button` to `SystemTab_UI.cpp`'s `DrawSystemTab()` (drawn today from
`Application::DrawPerformancePanel()`, `Application_PanelSystem_UI.cpp:37`, which already has
`previewDriver` in scope) that calls the file's existing local `RequestRegeneration(previewDriver)`
helper (`SystemTab_UI.cpp:21-23`) — reused as-is, not duplicated; it already does exactly
`if (previewDriver != nullptr) previewDriver->RequestMapUpdate();`. This is the ticket's one
debug/System-panel affordance for the docstring's named manual case. Suggested placement: its own
row, separated from the automatic backend/context settings above it, with wrapped text naming the
exception it exists for (mirroring how `DrawBackendSettings` already documents itself):
```cpp
ImGui::Separator();
ImGui::TextWrapped("Force a full regeneration for a change no parameter hash can see: a resize, "
                   "a recipe reload, or new stratum art.");
if (ImGui::Button("Force Regenerate")) RequestRegeneration(previewDriver);
```
Exact label text and placement within `DrawSystemTab` are the coder's call; the binding contract
is: exactly one manual button, in the System panel (not the View toolbar, not the primary Map
Preview toolbar), calling `RequestMapUpdate()` (directly or via the existing local helper) with no
new rival free function introduced.

### 4. Update the now-invalid acceptance test
`MapCanvas_Picking_UI_Test.cpp` (`RunMapCanvasPickingChecks`) asserts the removed API
(lines 69-73):
```cpp
int regenerationCount = 0;
canvas.SetRegenerationCallback([&]() { ++regenerationCount; });
canvas.RequestRegeneration();
check(regenerationCount == 1 && canvas.RegenerationRequestCount() == 1,
      "the canvas asks for a regeneration through the injected callback and nothing else");
```
Delete this block — the API it exercises no longer exists. Update the file's own header comment
(line 3, "...and the only work the canvas can cause is the injected regeneration callback") to
match §2's finding: after this ticket the canvas causes no work at all, so this line should state
that instead (e.g. "...the canvas causes no work of its own — draw, pick, pan/zoom only").

## Out of scope
- STEP54 (the View popup itself) — not built or modified here; §1 above states the finding that
  this ticket does not need it to land first.
- Any change to `PreviewDriver::NotifyParametersChanged()`, the four-tier dirty model (§14.8), or
  `SystemTab_UI.cpp`'s existing automatic-trigger call sites (`DrawBackendSettings`, the
  determinism checkbox) — those already call `RequestMapUpdate()` correctly and are untouched.
- Retiring `Data::EntityIdBuffer`'s write path or anything else on Phase 1.4's blocked list — no
  relation to this ticket's trigger-path collapse.
- Renaming `PreviewDriver::RequestMapUpdate()` itself — it is the surviving name; §14.7 asks for
  collapse onto it, not a further rename.

## Files touched
- `src/ui/Application_Draw_UI.cpp` — `DrawCanvasWindow`: delete the Regenerate button + its
  `ImGui::SameLine()`
- `src/ui/MapCanvas_UI.h` — remove `SetRegenerationCallback`, `RequestRegeneration()` declaration,
  `RegenerationRequestCount()`, `regenerationCallback`, `regenerationRequestCount`; reword the
  class header comment's "injected regenerate callback" sentence
- `src/ui/MapCanvas_UI.cpp` — remove `RequestRegeneration()` definition
- `src/ui/Application_UI.cpp` — `WireCallbacks()`: remove the `SetRegenerationCallback` binding;
  reword its "three injected seams" comment to "two"
- `src/ui/SystemTab_UI.cpp` — `DrawSystemTab()`: add the one manual "Force Regenerate" button,
  calling the file's existing local `RequestRegeneration(previewDriver)` helper
- `src/ui/MapCanvas_Picking_UI_Test.cpp` — remove the regeneration-callback assertion block;
  reword the file's header comment to match the "canvas causes no work" finding

## Verify
- Full solo rebuild + `ctest -C Debug`, full suite green, with the edited
  `MapCanvas_Picking_UI_Test.cpp` still passing its remaining (picking) assertions unchanged.
- Grep confirms exactly one production call site of `RequestMapUpdate()` triggered by a manual
  button (the new System-panel one) plus the existing automatic call sites in
  `SystemTab_UI.cpp`/`Application_UI.cpp::ApplyExecutionPolicy` — and zero remaining references to
  `MapCanvas::RequestRegeneration`/`SetRegenerationCallback`/`RegenerationRequestCount` anywhere in
  `src/`.
- This is a pure wiring/removal change with no new pure-function logic (the new button is imgui
  chrome calling an existing helper, same category STEP44 already classified as needing no new
  unit test) — no new test is expected beyond the required edit to the invalidated assertion above.
  Manual on-screen confirmation that the toolbar no longer shows "Regenerate" and that the System
  panel's new button still forces a redraw is the human's own job (see memory: no interactive
  testing by AI).
