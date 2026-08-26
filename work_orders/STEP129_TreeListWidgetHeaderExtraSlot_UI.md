# STEP129 — TreeListWidget_UI header-extra slot (ARCH §19.23)

**Layer:** UI (shared widget). **Domain:** `TreeListWidget_UI.h`, `TreeListWidget_RowLayout_UI.h`.
**Sequence:** independent — no dependency. Unblocks STEP130 (7(b): Bundle-tree Color Override wiring
+ Symmetry toggle), which consumes this slot.

Ratifies `ARCH_19_23_TreeListHeaderExtraContract.md` verbatim.

## Fix

Add a new, additive `TreeListWidget_UI<T,LeafKeyT>::Render` overload taking two new header-extra
callbacks and a shared width, per ARCH §19.23's exact contract:

```cpp
// existing 7-callback overload becomes a thin delegator (arity alone disambiguates — the exact
// STEP123 DraggableList precedent for its own 2-callback -> 3-callback delegation):
template <typename T, typename LeafKeyT, ...>
static TreeListSignal<LeafKeyT> Render(..., TreeListState& state, int selectedNodeIdentifier = -1) {
    return Render(..., [](int) {}, [](const LeafKeyT&) {}, 0.0f, state, selectedNodeIdentifier);
}

// new overload, additive:
template <typename T, typename LeafKeyT, ..., typename DrawNodeHeaderExtraFn, typename DrawLeafHeaderExtraFn>
static TreeListSignal<LeafKeyT> Render(..., DrawNodeHeaderExtraFn drawNodeHeaderExtra,
                                        DrawLeafHeaderExtraFn drawLeafHeaderExtra,
                                        float headerExtraWidthPixels, TreeListState& state,
                                        int selectedNodeIdentifier = -1);
```

Thread `drawNodeHeaderExtra`/`drawLeafHeaderExtra`/`headerExtraWidthPixels` down into
`TreeListDetail::RenderNode`/`RenderLeaf` (`TreeListWidget_RowLayout_UI.h`). In each, after the
existing click/drag-drop detection and before the `if (bExpanded)` body:

```cpp
if (headerExtraWidthPixels > 0.0f) {
    const float rowAvailWidthPixels = ImGui::GetContentRegionAvail().x;
    ImGui::SameLine(rowAvailWidthPixels - headerExtraWidthPixels);
    drawNodeHeaderExtra(nodeIdentifier);   // RenderNode
    // or: drawLeafHeaderExtra(leaf);      // RenderLeaf
}
```

`headerExtraWidthPixels == 0.0f` (the delegator's default) must draw nothing and reserve nothing —
byte-identical layout to today for every existing call site. This is the ONLY thing this ticket
changes; no call site is rewired yet (that's STEP130).

Per ARCH §19.23's own scope note: do NOT wire any concrete control into these callbacks in this
ticket — that's STEP130's job. This ticket delivers the widget contract only.

## Verify

- New test (extend `TreeListWidget_UI_Test.cpp` / `ListWidgets_UI_Test`): a synthetic node + leaf
  fixture, `headerExtraWidthPixels = 0.0f` → assert the rendered layout (item rects) is byte-identical
  to calling the OLD 7-callback overload directly (regression proof the delegator is truly a no-op).
- New test: `headerExtraWidthPixels > 0.0f` with a synthetic `drawNodeHeaderExtra`/
  `drawLeafHeaderExtra` that draws a small marker widget (e.g. an invisible button of known size) →
  assert it appears at the expected right-aligned X for both a Node row and a Leaf row, and that it
  does not appear at all when `headerExtraWidthPixels == 0.0f` even if the callback is non-trivial
  (i.e. the width gate, not just an empty lambda, controls whether anything draws).
- Confirm every existing `TreeListWidget_UI<T,LeafKeyT>::Render` call site (grep for `::Render(`
  against this widget) still compiles unchanged and its own test suite stays green — this is a
  shared-widget change, treat it with the same cross-call-site rigor STEP123 used for `DraggableList`.
