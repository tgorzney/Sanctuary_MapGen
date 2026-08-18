# UI_FRAMEWORK_SPEC — imgui-bypass & the 100k-entity UI

Source: `gui/UIHelpers.h`, `gui/widgets/VirtualListRenderer.h`,
`gui/PreviewRenderer.*`, `gui/widgets/Widget_MapCanvas.*`, the `Tab_*` files, and
the DOP structures in `Parameters.h`. SanGen must render/scroll/click 100k+
entities responsively; imgui is used, but bypassed on the hot paths.

## The bypass toolkit (keep & formalize)
1. **Direct `ImDrawList` custom widgets.** Custom controls are drawn with
   `GetWindowDrawList()->AddRectFilled(...)` + an `InvisibleButton` for hit-testing,
   instead of imgui's built-in widgets — e.g. the dual-handle `RangeSliderFloat`
   (`UIHelpers.h`). Faster and fully styleable.
2. **`ImGuiListClipper` virtualization.** `VirtualListRenderer<T>` renders only the
   visible rows of a huge list (`clipper.Step()` → `DisplayStart..DisplayEnd`), over
   a contiguous/SoA array. This is how 100k-item dropdowns/lists scroll cheaply.
3. **GPU preview, not imgui, for the map.** `PreviewRenderer` draws the 100k+ entity
   preview on the GPU (compute/shaders into a texture). Entities never go through
   imgui draw loops.
4. **O(1) picking via an entity-ID buffer.** The preview also renders a per-pixel
   **`EntityIDBuffer`**; a click reads back the ID under the cursor for O(1)
   selection instead of testing 100k items.
5. **O(1) marker hit-testing via the spatial grid.** Interactive markers use the
   32×32 `MarkerSpatialGrid` (`Parameters.h`) — a click hashes to a chunk and tests
   only that chunk's markers.
6. **SoA segregation.** Props live in the flat `StaticPropsList` (`PropInstance` SoA),
   never in an imgui or per-item loop — the reason 100k props don't tank the UI.
7. **RT (realtime) toggles.** Widgets (e.g. the range slider) carry a per-control
   "RT" flag: while off, dragging updates the value but defers the expensive
   recompute until mouse-release — keeping FPS high during slider scrubbing.

## Interaction / dirty-flag model
- Two-tier dirty flags: **`bNeedsMapUpdate`** (full regen: heightmap/erosion/flow/
  placement) vs **`bNeedsPreviewRender`** (recolor/recomposite only). A control sets
  whichever it truly affects, so cheap visual tweaks don't trigger a full regen.
- Reorder/enable/lock/delete for the layer stacks is handled by the templated
  `RenderDraggableLayerList<T>` (drag-drop payloads, per-layer vis/lock/delete).
- Regen is dispatched async off these flags (main loop), not inline in the widget.

## Known issues to fix in v2 (from the code survey)
- **Preview shadow-reimplements the sim.** `PreviewRenderer` recomputes slope/flow/
  marker filtering in its own shaders, independent of the CPU bake → "preview truth
  ≠ bake truth." Unify on one source of truth (WYSIWYG). *(Cross-ref
  LAYER_SYSTEM/PARAMS/SIM specs; this is a hit-list item.)*
- **`Widget_MapCanvas` is a god-widget** (~720 lines): UI + hit-testing + triangle
  height-interpolation geometry + unit-grid spawning + symmetry spawn + army
  creation + context menus. Split per the layer model (UI vs geometry vs entity ops).
- **Dirty-flag assignment is inconsistent per widget** (some controls set only
  `bNeedsPreviewRender` when they actually change sim inputs). Formalize which flag
  each parameter sets (ideally derive it from the dependency DAG, not by hand).

## Universal widget library
The bypass toolkit above is not per-tab copy-paste — it is **one shared widget
library** every tab draws from: RangeSlider, VirtualList<T>, DraggableList<T>,
gradient editor, icon-grid / atlas-thumbnail button, labelled dial, RT-toggle
wrapper, **`ConfirmDialog_UI`**, etc. One optimized implementation, consistent
look, DRY. New tabs compose these; they do not hand-roll imgui. Widgets that show
icons/thumbnails read from the resident atlas (ASSET_LOADING_SPEC), never from
files.

- **`ConfirmDialog_UI`** — generic, reusable OK/Cancel confirm-modal (title +
  pre-formatted body text + two labelled buttons; caller owns a one-per-site
  `ConfirmDialogState`, no ESC/backdrop dismissal by default —
  `bClosableWithoutChoice` opts in). Added to the library the same session that
  ratified `ENTITY_AUTHORING_PARAMS_SPEC.md`'s blueprintPath "warn, never block"
  export ruling — its first consumer is the Files-tab export-warning flow
  (`work_orders/STEP5_PropsDecalsValidation_UI.md`), but the widget itself is a
  general primitive, not bespoke to that flow: no confirm-dialog widget existed in
  the toolkit before this addition. Follows the existing `ColorSwatch_UI.cpp`
  draw/state split (`WidgetHelpers_UI.h` "THE SPLIT").

## v2 guidance
- Keep the bypass toolkit (1–7); make it the standard, not ad-hoc per tab.
- Formalize the two-tier dirty flags off the dependency DAG (PARAMS_PIPELINE_SPEC).
- One source of truth for preview vs bake (kill the shadow logic).
- Every widget respects §8 tweakability and the accuracy/dispatch contexts (Preview =
  fast/GPU, idle → escalate to accurate).
- Load all icons/textures through the asset-safety layer (Constitution §6) — never
  raw into the 100k lists.
