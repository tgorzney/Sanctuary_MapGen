# STEP54 — Retire "Regenerate," add the click-to-open "View" popup (two non-crossing layer sections)

**Layer:** UI. **Domain:** `Application` (canvas-window chrome), `PreviewCompositeSettings::fieldLayers`,
`overlayLayers` (STEP51). **Accuracy class:** Visual — presentation-only, no simulation math.
**Backend policy:** CPU-only imgui chrome; no `Sys::DispatchPolicy`/GPU-backend decision anywhere
in this ticket. **Sequence:** Phase 4.1, `work_orders/SEQUENCE_PreviewOverlayLayering.md`.
**ARCH rules invoked:** §14.7 (binding law this implements), §14.2 (data model + "toolbar never
adds/removes/blends/hides only" contract), §14.8 (dirty-tier classification), §1.5 (file split/size
ceilings), §8 (no bare literals).

**Depends on STEP51** (`OverlayLayer_UI`/`overlayLayers`) — confirmed **not yet in `src/`**
(`grep -rn "OverlayLayer_UI|overlayLayers" src` returns nothing as of this writing). This
work-order cannot be dispatched to the Coder until STEP51 lands `overlayLayers` somewhere
reachable from `Application`. **Ideally also depends on STEP53** (Phase 3's screen-space icon
draw pass) so the popup has a real render to preview live — not a hard blocker (the popup can be
built and tested against `overlayLayers` state alone), but the acceptance test's "opacity actually
changes what's drawn" checks have nothing to observe until STEP53 exists.

## Problem
`Application::DrawCanvasWindow()` (`src/ui/Application_Draw_UI.cpp:45-56`) draws the Map Preview
toolbar's only control today:
```cpp
if (ImGui::Button("Regenerate")) canvas.RequestRegeneration();
```
`MapCanvas::RequestRegeneration()` (`src/ui/MapCanvas_UI.cpp:55-58`) fires a caller-injected
callback wired in `Application::WireCallbacks()` (`src/ui/Application_UI.cpp:61`) straight to
`previewDriver.RequestMapUpdate()` — a manual full-regen trigger existing alongside
`Pipeline::PreviewDriver`'s own automatic hash-derived tiering (`NotifyParametersChanged()`),
which ARCH_14_07_ViewToolbar.md §14.7 names as "the exact anti-pattern that system exists to replace." §14.7 rules this
button retired from the primary toolbar and its slot replaced with a "View" button that opens a
click-to-open (not hover-triggered) popup listing the two composited/screen-space layer stacks so
their Z-order, per-terrain-layer blend mode, and per-overlay-layer opacity can be edited from one
place — something no UI surface exposes today: `PreviewCompositeSettings::fieldLayers`
(`src/ui/PreviewComposite_Settings_UI.h:63`) has **zero** call sites drawing its `blendMode` or
reordering it anywhere in `src/` (confirmed by grep — every `fieldLayers` hit is either
`TerrainOverlayTab_UI.h`'s find-by-kind helper or a test constructing layers directly), and
`overlayLayers` does not exist in the codebase yet at all (STEP51).

**Out of explicit scope for THIS ticket** (§14.7 splits it into 4.1/4.2 in the sequence doc, and
the dispatch brief for this ticket names only the popup): collapsing
`MapCanvas::RequestRegeneration()`/`PreviewDriver::RequestMapUpdate()` into one call path and
adding the one legitimate System-panel manual-refresh affordance §14.7 names (resize/recipe-reload/
new-stratum-art) is **STEP 4.2, not this ticket.** Removing the "Regenerate" button here does
leave the primary toolbar with **zero manual trigger** until 4.2 lands its replacement — flagged,
not silently absorbed into this ticket's scope; `MapCanvas::RequestRegeneration()` itself,
`SetRegenerationCallback`, and `MapCanvas_Picking_UI_Test.cpp`'s existing use of it are untouched
by this work-order (they simply lose their only live UI caller until 4.2).

## Fix

### 1. Swap the toolbar button (`Application_Draw_UI.cpp`)
```cpp
void Application::DrawCanvasWindow() {
    ImGui::Begin("Map Preview");
    if (ImGui::Button("View")) ImGui::OpenPopup("ViewLayersPopup");
    if (ImGui::BeginPopup("ViewLayersPopup")) {
        DrawViewLayersPopup();
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    if (canvas.HasSelection()) ImGui::Text("Selected entity: %u", canvas.SelectedEntityIdentifier());
    else                       ImGui::TextUnformatted("Selected entity: none");
    /* ...unchanged below... */
}
```
`ImGui::BeginPopup` is click-to-open/click-elsewhere-to-close by construction — no hover logic
anywhere, which is exactly what §14.7 requires ("hover-close would fight a drag-reorder gesture").
Declare `void DrawViewLayersPopup();` alongside `Application`'s other `Draw*` aspect methods in
`src/ui/Application_UI.h`'s private section (next to `DrawCanvasWindow()`), implemented in a new
aspect file per the header's own "Aspect .cpp units" convention (`Application_UI.h:8-14` already
lists `_Draw_UI` / `_LeftColumn_UI` / `_Panel{...}_UI` as siblings of this exact shape) —
`Application_ViewLayersPopup_UI.cpp`, kept under the ARCH_01_05_FileSizeCeilings.md §1.5 soft-100/hard-150-line ceiling on
its own.

### 2. The popup body — two independent `DraggableList` sections, one shared apply rule
```cpp
// Application_ViewLayersPopup_UI.cpp
#include "Application_UI.h"
#include "DraggableListWidget_UI.h"
#include "Combo_UI.h"
#include <cstdio>
#include <imgui.h>

namespace SanmapGen {
namespace Ui {
namespace {

// Applies ONLY Reorder + ToggleVisibility. Delete is deliberately NOT wired here — adding/
// removing a whole layer is domain-tab authoring, never the View toolbar (§14.2: "Sub-layer
// authoring (add/remove/toggle) lives in each domain's own tab... never the View toolbar, which
// only orders/blends/hides whole OverlayLayer_UIs"). ToggleLock/Select are inert by construction:
// neither PreviewFieldLayer nor OverlayLayer_UI carries a lock or "selected" concept, so those
// signal kinds fall through unapplied — the exact contract DraggableListWidget_UI.h's own
// ApplyDraggableListSignal doc states ("every other kind belongs to state this widget does not
// own and is left alone"). Templated: identical shape for PreviewFieldLayer and OverlayLayer_UI,
// both of which carry a plain `bEnabled` field.
template <typename T>
bool ApplyViewLayerSignal(std::vector<T>& items, const DraggableListSignal& signal) {
    if (signal.kind == DraggableListSignalKind::ToggleVisibility) {
        if (signal.sourceRowIndex < 0 || signal.sourceRowIndex >= static_cast<int>(items.size()))
            return false;
        T& item = items[static_cast<std::size_t>(signal.sourceRowIndex)];
        item.bEnabled = !item.bEnabled;
        return true;
    }
    if (signal.kind != DraggableListSignalKind::Reorder) return false;   // Delete/Lock/Select: no-op
    return ApplyDraggableListSignal(items, signal);
}

const char* const previewLayerKindNames[] = {
    "HeightRamp", "StratumSplat", "Flow", "Accumulation", "Water", "Slope"
};
// PreviewBlendMode's own enum order (PreviewComposite_Settings_UI.h) — distinct from
// Params::HeightBlendMode's label set (LayersTab_UI.cpp's blendModeNames); do not merge the two.
const char* const previewBlendModeNames[] = {
    "Replace", "AlphaBlend", "Add", "Multiply", "Maximum", "Minimum"
};

// Terrain (composited) section — reorder + per-row blend-mode Combo_UI (real GPU blend-equation
// switch into the composite shader, §14.7 "unchanged by this ruling"). Returns true if the RECIPE-
// adjacent composite settings moved (needs a driver notification), matching
// Application_LeftColumn_UI.cpp:57-60's existing "mutate PreviewCompositeSettings then
// NotifyParametersChanged()" precedent — this IS presentation state, not PARAMS, but the driver
// still derives its own tier (B, full recomposite, §14.8) from that one call.
bool DrawTerrainSection(std::vector<PreviewFieldLayer>& fieldLayers) {
    char rowLabel[48] = { 0 };
    const DraggableListSignal signal = DraggableList<PreviewFieldLayer>::Render(
        "ViewListField", fieldLayers,
        [&](int rowIndex) {
            const PreviewFieldLayer& layer = fieldLayers[static_cast<std::size_t>(rowIndex)];
            const int kindIndex = static_cast<int>(layer.kind);
            const char* const kindName = (kindIndex >= 0
                && kindIndex < IM_ARRAYSIZE(previewLayerKindNames)) ?
                previewLayerKindNames[kindIndex] : "Unknown";
            std::snprintf(rowLabel, sizeof(rowLabel), "%s", kindName);
            DraggableListRow row;
            row.label    = rowLabel;
            row.bVisible = layer.bEnabled;
            return row;
        },
        [&](int rowIndex) {
            PreviewFieldLayer& layer = fieldLayers[static_cast<std::size_t>(rowIndex)];
            int blendIndex = static_cast<int>(layer.blendMode);
            if (ImGui::Combo("Blend Mode", &blendIndex, previewBlendModeNames,
                             IM_ARRAYSIZE(previewBlendModeNames)))
                layer.blendMode = static_cast<PreviewBlendMode>(blendIndex);
        });
    return ApplyViewLayerSignal(fieldLayers, signal);
}

// Overlays (screen-space) section — reorder + per-row opacity, §14.2/§14.13 item 5: opacity, not
// blend mode, and every overlay layer shares ImGui's one global blend equation. §14.8's Tier C
// ("every overlay layer, every frame... opacity... zero GPU recompute") means there is nothing
// expensive here to DEFER, unlike LayersTab_UI.cpp's dial-wrapped scalars: RtToggleWidget_UI's
// realtime-toggle machinery exists to spare an EXPENSIVE recompute during a drag, and an overlay
// redraw is already unconditional and cheap every frame regardless of RT state. A direct
// ImGui::SliderFloat with instant commit is therefore correct here, not a missing dial — flagged
// explicitly because it diverges from LayersTab_UI.cpp's own "no raw SliderFloat here" file-header
// convention, which is scoped to controls that DO gate an expensive stage re-run.
bool DrawOverlaySection(std::vector<OverlayLayer_UI>& overlayLayers) {
    const DraggableListSignal signal = DraggableList<OverlayLayer_UI>::Render(
        "ViewListOverlay", overlayLayers,
        [&](int rowIndex) {
            const OverlayLayer_UI& layer = overlayLayers[static_cast<std::size_t>(rowIndex)];
            DraggableListRow row;
            row.label    = layer.name.empty() ? "Overlay" : layer.name.c_str();
            row.bVisible = layer.bEnabled;
            return row;
        },
        [&](int rowIndex) {
            OverlayLayer_UI& layer = overlayLayers[static_cast<std::size_t>(rowIndex)];
            ImGui::SliderFloat("Opacity", &layer.opacity, 0.0f, 1.0f);
        });
    return ApplyViewLayerSignal(overlayLayers, signal);
}

} // namespace

void Application::DrawViewLayersPopup() {
    ImGui::TextUnformatted("Reorder within a section only: a terrain layer and an overlay layer "
                           "can never cross (not renderable, see ARCH_14_07_ViewToolbar.md \xc2\xa714.7).");
    ImGui::SeparatorText("Terrain (composited)");
    const bool bTerrainMoved = DrawTerrainSection(composite.Settings().fieldLayers);
    if (bTerrainMoved) previewDriver.NotifyParametersChanged();

    ImGui::SeparatorText("Overlays (screen-space)");
    DrawOverlaySection(/* wherever STEP51 hosts overlayLayers, e.g. hostedSettings.overlayLayers */);
    // No previewDriver call for the overlay section: §14.8 Tier C redraws every frame from current
    // state unconditionally, so reorder/opacity here trip no dirty flag at all — STEP53's draw pass
    // simply reads overlayLayers live. If STEP51/STEP53 land it differently (e.g. a real per-frame
    // C2-cache invalidation hook, §14.8), this call site is where that hook goes; not invented here.
}

} // namespace Ui
} // namespace SanmapGen
```

### 3. Cross-section rejection — cite the exact mechanism, not a re-derivation
`DraggableList<T>::Render`'s first argument (`"ViewListField"` / `"ViewListOverlay"` above) IS the
imgui drag-drop **payload identifier** passed straight through to `DetectRowDragAndDrop`
(`DraggableListWidget_UI.h:75-76,122-138`):
```cpp
const char* const payloadIdentifier =
    (std::strlen(listIdentifier) < 32u) ? listIdentifier : "SanGenDraggableListRow";
...
ImGui::SetDragDropPayload(payloadIdentifier, &rowIndex, sizeof(int));   // drag source
...
const ImGuiPayload* const payload = ImGui::AcceptDragDropPayload(payloadIdentifier);  // drop target
```
`ImGui::AcceptDragDropPayload` only accepts a payload whose type string matches the target's own —
a drag started under `"ViewListField"` dropped onto a `"ViewListOverlay"` row's target simply never
matches `payload != nullptr`, so `RecordSignal` never fires for that drop. Both chosen identifiers
are 13/15 characters — well under the 32-character fallback threshold — so neither collapses onto
the shared `"SanGenDraggableListRow"` fallback (a 32-character-or-longer name on EITHER section
would silently reunite them onto that one shared fallback string and reopen exactly the crossing
this design forbids — worth stating because it is the one way this "no new code" mechanism could
regress invisibly). **No new validation code is written for this** — exactly ARCH_14_07_ViewToolbar.md §14.7's own
claim ("cross-section drops structurally fail to match — no new validation code needed").

**True interleaving is out of scope by design, not a missing feature.** §14.7: a row cannot render
"under" a terrain layer without either re-baking overlay content into the composite texture (the
exact WYSIWYG/zoom-scaling bug this whole redesign — ARCH_14_PreviewOverlayLayering.md §14 — exists to kill) or rebuilding
`PreviewComposite` into an interleaved multi-target compositor; a control that visually SUGGESTED
interleaving without actually producing it would violate the WYSIWYG law by showing an order that
is not the real render order. This ticket does not attempt it, is not expected to, and no future
ticket should treat its absence here as an oversight without a fresh ARCH ruling.

## Files touched
- `src/ui/Application_Draw_UI.cpp` — swap the `Regenerate` button for `View` + `BeginPopup`
- `src/ui/Application_UI.h` — declare `void DrawViewLayersPopup();` in the private aspect-method list
- `src/ui/Application_ViewLayersPopup_UI.cpp` — new file: the popup body, both sections,
  `ApplyViewLayerSignal`, the two name tables
- Wherever STEP51 lands `overlayLayers` (candidate: `Application_HostedSettings_UI.h`, alongside
  `ApplicationHostedSettings` — its own header already frames itself as "the settings the rebuilt
  tabs edit that have no home in `Params::MapRecipe` yet," matching §14.5's "session-only UI
  presentation" classification for order/`bEnabled`/opacity) — this ticket only consumes the
  member STEP51 exposes; it does not choose its host.

## Verify
- New test: two `PreviewFieldLayer`s in `PreviewCompositeSettings::fieldLayers`, a synthetic
  Reorder `DraggableListSignal` targeting the terrain section only moves `fieldLayers`; a synthetic
  Reorder signal built against `"ViewListOverlay"`'s payload identifier and delivered to the
  terrain section's drop target is rejected (asserts `ApplyViewLayerSignal` never fires) — proves
  the cross-section rejection headlessly, not just by inspection.
- New test: `ApplyViewLayerSignal` with a `Delete` signal returns `false` and leaves the vector
  size unchanged, for both `PreviewFieldLayer` and `OverlayLayer_UI` — proves the "toolbar never
  adds/removes" rule is actually enforced, not just documented.
- New test: `ApplyViewLayerSignal` with a `ToggleVisibility` signal flips `bEnabled` and returns
  `true`; a `ToggleLock`/`Select` signal returns `false` and leaves the vector unmodified.
- Existing `ApplicationShell_*_UI_Test.cpp` / `ApplicationShell_DirtyTier_UI_Test.cpp` suite green
  with zero edits — the `Regenerate` button's removal touches no tested behavior (no existing test
  clicks it; `MapCanvas_Picking_UI_Test.cpp` drives `RequestRegeneration()` directly, not through
  imgui, and is unaffected).
- Manual/visual acceptance (once STEP53 lands so there's something real to preview): open "View,"
  drag a terrain row past an overlay row and confirm the drop is rejected (row order in both
  sections is visibly unchanged); drag within one section and confirm the reorder commits; change
  an overlay row's opacity slider and confirm the screen-space icon draw pass visibly fades without
  a full recomposite (`previewDriver.PreviewCompositeCount()` does not increment — Tier C, not B).
