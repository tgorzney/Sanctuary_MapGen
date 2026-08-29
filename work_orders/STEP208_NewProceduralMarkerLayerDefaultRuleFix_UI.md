# STEP208 — Newly created Procedural marker layer shows no settings (zero-rules gap)

**Layer:** UI. **Domain:** `src/ui/MarkersTab_UI.cpp` (Type-section "+ Layer" creation site),
`src/ui/MarkersTab_RuleLayerSettings_UI.cpp` (`DrawAddMarkerRuleLayerButton` creation site),
`src/ui/MarkersTab_UI_Test.cpp` and `src/ui/MarkersTab_RuleLayers_UI_Test.cpp` (acceptance test
extensions). Pure CPU/imgui + PARAMS default-construction — no GPU or compute-dispatch involvement, no
accuracy-class concern.

## Root problem
Two independent creation sites build a fresh `Params::MarkerRuleLayer` and push it with zero `rules`:

1. `src/ui/MarkersTab_UI.cpp`, the Type-section's own "+ Layer" header button, `buttons.
   bAddProceduralLayerClicked` handler (`MarkersTab_UI.cpp:336-345`).
2. `src/ui/MarkersTab_RuleLayerSettings_UI.cpp`, `DrawAddMarkerRuleLayerButton`
   (`MarkersTab_RuleLayerSettings_UI.cpp:57-69`, the Bundle-tree node's own "Add Procedural Layer Here"
   button and the ungrouped list's own "Add Layer" button both call this one function).

Neither site calls `layer.rules.push_back(...)`. Every generation setting (Gates/Quantity/Area/Focus/
Placement Gate/Transform/Template Picker, `DrawRuleSettings`,
`MarkersTab_RuleLayerSettings_UI.cpp:126-142`) is reachable ONLY per-rule, drawn inside a
`DraggableList<Params::MarkerRule>::Render(..., layer.rules, ...)` call in `DrawRuleLayerBody`
(`src/ui/MarkersTab_RuleLayers_UI.cpp:16-43`) — with `layer.rules` empty, that list has zero rows and
`DrawRuleSettings` is never invoked. This is not merely collapsed by default; it is structurally
unreachable — the layer shows `"0 rule(s) - select this layer to edit them"`
(`MarkersTab_RuleLayers_UI.cpp:21`) forever, with no affordance on that row to add one.

A working, separate "Add Rule" button already exists (`DrawMarkerRuleButtons`,
`MarkersTab_RuleLayerSettings_UI.cpp:78-98`) that pushes a plain default-constructed rule
(`layer->rules.push_back(Params::MarkerRule())`) — but this button is drawn tab-wide, gated on
`state.selectedRuleLayerIndex` already pointing at a real layer, so a user who just clicked "+ Layer"
must separately notice this OTHER button exists and click it a second time before anything becomes
editable. Neither layer-creation call site invokes this push, or an equivalent one, itself.

`DrawRuleLayerSettings` (`MarkersTab_RuleLayerSettings_UI.cpp:112-115`) — the layer-level settings that
draw unconditionally, independent of `layer.rules` — only draws the per-axis symmetry-override
checkboxes, and only `if (!layer.symmetry.bSymmetryUseGlobal)`, which defaults `true`
(`Params::MarkerRuleLayer::symmetry`, `src/params/MarkerRule_PARAMS.h:76`). So this section also renders
nothing for a freshly created layer today — flagged as possibly-intended existing behavior and left out
of scope below rather than guessed at.

## Fix approach
At both creation sites, after constructing `layer` and before `push_back`, push one default-constructed
`Params::MarkerRule` so the rule list has a row to render immediately:
```cpp
layer.rules.push_back(Params::MarkerRule());
```
placed identically to `DrawMarkerRuleButtons`'s own already-shipped, working convention — a plain
default-constructed rule, no field pre-seeded beyond what `Params::MarkerRule`'s own struct-default
member initializers already provide (`src/params/MarkerRule_PARAMS.h:20-63`: `bEnabled=true`,
`category=Generic`, `count=4`, etc. — do not invent or hand-tune any different default values). After the
push, `state.selectedRuleIndex = 0` already gets set by both call sites' existing code — confirm this
still correctly points at the one new rule after the fix (it does: index 0 of a 1-element vector).

## Explicit out-of-scope
- `DrawRuleLayerSettings`'s `!layer.symmetry.bSymmetryUseGlobal`-gated body rendering nothing by default
  — possibly intended existing behavior, not touched by this ticket. A separate ticket if the human wants
  it changed.
- No change to `DrawMarkerRuleButtons`'s own "Add Rule"/"Remove Selected Rule" buttons — they stay
  exactly as they are; this ticket only makes the FIRST rule appear automatically at layer-creation time.
- No change to the Manual-layer creation sites (`buttons.bAddManualLayerClicked`,
  `MarkersTab_UI.cpp:325-334`) — Manual layers have no `rules` concept at all, so there is no analogous
  gap there.
- No change to `Params::MarkerRule`'s own struct defaults (`MarkerRule_PARAMS.h`) — the fix consumes
  those defaults as-is, it does not tune them.

## Acceptance test
Extend `src/ui/MarkersTab_RuleLayers_UI_Test.cpp`'s existing `RunDrawAddMarkerRuleLayerButtonTypeSeedChecks`
(already drives `DrawAddMarkerRuleLayerButton` through a live headless-imgui click via
`ClickAddRuleLayerButton`) with an assertion that `seededLayers.back().rules.size() == 1` immediately
after the click (covers both the ungrouped-list "Add Layer" button and the Bundle-tree node's own "Add
Procedural Layer Here" button, since both call this same function). Add a new headless-imgui click-through
test in `src/ui/MarkersTab_UI_Test.cpp` for the Type-section's own "+ Layer" button
(`buttons.bAddProceduralLayerClicked`), mirroring the same click-frame technique, asserting
`recipe.markerRuleLayers.back().rules.size() == 1` post-click. Full solo rebuild + `ctest -C Debug`:
previously-passing suite stays green.
