# STEP104 — Heightmap Layer Editor: inline per-layer settings, right-aligned "Add GeoLayer"

**Layer:** UI. **Domain:** `LayerEditor_UI.*`, `LayerEditor_Group_UI.cpp`, `Section_UI.*`.
**Sequence:** independent of STEP99-103 (the baked-layer work) — pure layout rework,
no PARAMS/DATA/IO change. Human-requested (real usage feedback, not a drafted design
doc).

## Root problem, part 1 — settings render once at the bottom, not under the layer

Confirmed by direct read: `DrawLayerEditor` (`src/ui/LayerEditor_UI.cpp`) draws the
WHOLE GeoLayer/Layer list first (`DrawGeoLayerList`), then — entirely separately,
after a `Separator()`, using `SelectedLayerEditorLayer(layerStack, state)` (which
walks `state.selectedGeoLayerIndex`/`selectedLayerIndex`) — draws
`DrawSelectedLayerPanels` exactly once for whichever layer happens to be selected.
Each Layer's own row, inside `DrawGroupLayerList` (`LayerEditor_Group_UI.cpp`), is a
real `ImGui::CollapsingHeader` (via `DraggableList<Params::Layer>::Render`,
`DraggableListWidget_UI.h`) that currently only shows a name field + Import RAW/
Duplicate/Bake row actions in its body — never the layer's actual settings. This
makes it hard to tell which layer's settings are showing (the human's own report:
"I can't really tell... only Stratum Index changes show since they might all have
the exact same settings").

## Root problem, part 2 — "Add GeoLayer" can't sit beside the section header

`Section_UI::DrawSectionBegin` (`src/ui/Section_UI.cpp`) draws the "GeoLayers"
collapsing header as a single `ImGui::InvisibleButton("##header", ImVec2(barWidth,
barHeight))` where `barWidth = ImGui::GetContentRegionAvail().x` — the header's own
hit-region claims the ENTIRE row width, unconditionally. `DrawGeoLayerList`
(`LayerEditor_UI.cpp`) currently draws `ImGui::SmallButton("Add GeoLayer")` as a
separate, first widget on its OWN line, above the list — not beside the "GeoLayers"
header bar at all, because there is no room left on that bar's line for anything
else. This is not a z-order/overlap bug in the button itself; the header simply
occupies the whole line, leaving nothing for a caller to compose a button onto
without either overlapping the header's own hit-test area or growing the row.

## Fix — part 1: move settings into each Layer's own row body

In `DrawGroupLayerList`'s row-body lambda for `DraggableList<Params::Layer>::Render`
(`LayerEditor_Group_UI.cpp`), after the existing name/row-actions content, call the
same panel-drawing sequence `DrawSelectedLayerPanels` currently calls
(`DrawLayerEditorLayerSections`, `DrawLayerEditorSoilSection`,
`DrawLayerEditorErosionSections` — `LayerEditor_UI.cpp`'s current body) directly on
`layer` (the row's own `Params::Layer&`, already in scope), so each expanded row
shows its OWN settings inline, right below its own header — never another row's.

This should render unconditionally whenever the row's `CollapsingHeader` is open
(matching `DraggableList`'s own existing per-row expand/collapse state exactly —
confirm the real mechanics of how `DraggableList<T>::Render`'s row-body callback is
gated on open/closed state before implementing, per `DraggableListWidget_UI.h`).

Once this lands, `DrawSelectedLayerPanels`'s call site at the bottom of
`DrawLayerEditor` becomes dead weight for GeoLayer-hosted layers specifically —
remove that trailing call and the `Separator()`/`SelectedLayerEditorLayer` plumbing
that only fed it, UNLESS `selectedGeoLayerIndex`/`selectedLayerIndex` are also read
elsewhere for something other than driving this trailing draw (verify before
deleting — e.g. `LayerEditor_Action_UI.h`'s Import RAW/Bake actions may still need
to know which layer is "selected" for their own row-scoped buttons; if so, keep the
state, only remove the redundant trailing full-panel draw).

`DrawLayerEditorAdvancedSection` (`LayerEditor_Advanced_UI.cpp`) is currently
declared but never called from anywhere in this draw path — leave it exactly as
uncalled as it is today; this ticket only relocates what's already wired, it does
not wire up new panels.

The parallel `LayersTab_UI.cpp` (a separate, older GeoLayer/Layer list
implementation, not hosted inside the Heightmap tab) is explicitly OUT OF SCOPE —
see below.

## Fix — part 2: `Section_UI` gains an optional reserved-right-width slot

Add one new field to `SectionOptions` (`src/ui/Section_UI.h`):
```cpp
struct SectionOptions {
    bool  bDefaultOpen   = true;
    bool  bArrowShown    = true;
    float indentWidth    = 0.0f;
    float headerRounding = -1.0f;
    float reservedRightWidth = 0.0f;   // NEW — pixels of the header bar's right edge left
                                       // undrawn/unclickable, for a caller to compose a button
                                       // into via SameLine() immediately after DrawSectionBegin
                                       // returns. 0 = today's exact behavior, full-width header.
};
```
`DrawSectionBegin` (`src/ui/Section_UI.cpp`): subtract `options.reservedRightWidth`
from `barWidth` before both the `InvisibleButton` size and the filled-rect width —
this genuinely shrinks the header's own hit-test area, so it stops before the
button rather than overlapping it (the human's own stated preference, confirmed
achievable — no fallback-overlay needed):
```cpp
const float barWidth = ImGui::GetContentRegionAvail().x - options.reservedRightWidth;
```
(guard against a negative/degenerate width the same way `barWidth > 1.0f ? barWidth
: 1.0f` already guards the zero case.)

At the `HeightmapTab_UI.cpp` call site: pass a `SectionOptions` with
`reservedRightWidth` set to the "Add GeoLayer" button's own measured width (e.g.
`ImGui::CalcTextSize("Add GeoLayer").x + ImGui::GetStyle().FramePadding.x * 2.0f`,
plus a small fixed spacing constant — named, per Constitution §8, not a bare
literal) instead of the default `SectionOptions()`. Immediately after
`DrawSectionBegin("GeoLayers", state.geoLayerSection, geoLayerSectionOptions)`
returns `true`, call `ImGui::SameLine()` then draw the button — it will land in the
reserved gap at the header bar's right edge, same visual row as the "GeoLayers"
title and disclosure arrow, right-aligned, not overlapping. Remove the old
free-standing `ImGui::SmallButton("Add GeoLayer")` line from `DrawGeoLayerList`
(`LayerEditor_UI.cpp`) — the button now lives at the `HeightmapTab_UI.cpp` call
site, immediately after the section header, since that's the only place the header
bar and the button can share one draw call. `RecordLayerEditorAction(...)` (the
actual Add action) stays exactly as it is; only WHERE the button is drawn moves.

If, once implemented, measuring/reserving the button's exact width in advance
proves awkward for any real reason (font metrics not yet available at layout time,
etc.), the human has explicitly authorized the fallback: keep `reservedRightWidth =
0` (full-width header, today's behavior) and simply draw the button on the same
line via `ImGui::SameLine(ImGui::GetContentRegionAvail().x - buttonWidth)` positioned
on top of the header's already-full-width `InvisibleButton` — the LAST-drawn item at
a given screen position wins ImGui's hit-test, so a button drawn after the header
still receives clicks correctly even when visually overlapping. Prefer the
non-overlapping `reservedRightWidth` approach; only fall back to this if it's
genuinely not workable, and say so explicitly when reporting back.

## Files touched
- `src/ui/Section_UI.h` — new `SectionOptions::reservedRightWidth` field.
- `src/ui/Section_UI.cpp` — `DrawSectionBegin` subtracts it from `barWidth`.
- `src/ui/HeightmapTab_UI.cpp` — the "GeoLayers" section call site: pass the new
  option, draw "Add GeoLayer" via `SameLine()` right after `DrawSectionBegin`
  returns `true`.
- `src/ui/LayerEditor_UI.h` / `.cpp` — remove the old `Add GeoLayer` button from
  `DrawGeoLayerList` (moves to the call site above); remove the trailing
  `DrawSelectedLayerPanels` call + its now-dead supporting plumbing if confirmed
  unused elsewhere (see Fix part 1's caveat).
- `src/ui/LayerEditor_Group_UI.cpp` — `DrawGroupLayerList`'s row-body lambda gains
  the inline settings draw for its own `Params::Layer&`.
- Existing `LayerEditor_UI_Test.cpp` (and any test currently asserting on
  `DrawSelectedLayerPanels`'s call ordering, if one exists — check) updated to
  match the new draw order.

## ARCH rules invoked
- Constitution §8 — the reserved-width spacing/button-width figures are named
  constants, never bare literals at the call site.
- `UI_FRAMEWORK_SPEC` "Universal widget library" (cited in `Section_UI.h`'s own
  header comment) — `Section_UI` stays the one shared header implementation; this
  ticket extends it generically (any future section can reserve trailing width for
  its own button), not with a GeoLayers-specific carve-out.
- No PARAMS/DATA/IO/PROC change of any kind — pure UI layout.

## Explicit out-of-scope
- **`LayersTab_UI.cpp`** — the separate, older GeoLayer/Layer list implementation
  (not hosted inside the Heightmap tab). It duplicates the same list/settings
  pattern this ticket reworks for `LayerEditor_UI.cpp`'s copy, but is a distinct
  file with its own `DrawLayerControls`/`SelectedLayer` — not touched here. If it
  needs the identical rework, that's a follow-up ticket, not folded in silently.
- **`DrawLayerEditorAdvancedSection`** — stays uncalled, exactly as today. This
  ticket relocates existing wiring; it does not wire up new panels.
- **Any change to what settings a layer has, their ranges, or their defaults** —
  zero PARAMS change. This is purely where the same existing widgets render.
- **The GeoLayer row's own list widget** (`DraggableList<Params::GeoLayer>`, its
  `CollapsingHeader`, its right-aligned affordance strip) — unchanged. This ticket
  touches the section HEADER above the list (for the Add button) and each LAYER
  row's body WITHIN a GeoLayer (for inline settings) — not the GeoLayer row itself.

## Acceptance test
Extend `LayerEditor_UI_Test.cpp` (or add a case if a fitting one doesn't exist):
1. With a `LayerStack` containing 2 `GeoLayer`s, each with 2 `Layer`s of
   deliberately DIFFERENT noise settings (e.g. distinct `frequency` values):
   expanding one Layer's row and reading back what widgets/values render in its
   body shows THAT layer's own frequency, not another layer's — proves settings no
   longer bleed across the "last selected" global state.
2. Collapsing a Layer's row hides its settings; no settings render when no row in
   a GeoLayer is expanded.
3. `SectionOptions{reservedRightWidth = N}` on a synthetic header: the header bar's
   own drawn width/hit-rect is `contentRegionAvail.x - N`, confirmed via whatever
   headless-imgui-frame technique this codebase's existing `Section_UI`-adjacent
   tests already use (check for one; if none exists, mirror the nearest precedent,
   e.g. `MapCanvas_Render_UI_Test.cpp`'s style).
4. "Add GeoLayer" button, positioned per this ticket, still correctly fires
   `RecordLayerEditorAction`/adds a new GeoLayer when clicked — a real click-through
   test, not just a positioning assertion (regression coverage for the button's own
   function, unchanged from before this ticket, just moved).
5. Full solo rebuild + `ctest -C Debug`: previously-passing suite stays green,
   including any existing `Section_UI_Test`/`LayerEditor_Signals_UI_Test` coverage.

## Verify
No manual/interactive verification — automated test binaries and code reading only,
per the project's standing testing law. Full solo rebuild + `ctest -C Debug`: 100%
pass, zero unrelated test files edited or broken.
