[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.23. **Only the ARCH Expert writes this file.**

### 19.23 `TreeListWidget_UI<T,LeafKeyT>::Render` header-extra contract — TWO callbacks, not one; ratified as designed
Responds to `DESIGN_MarkersUICorrectionRound2_R1.md` item 7(a) ("Markers UI Correction Round 2").
**Ratified as designed — the two-callback shape is correct, not a copy of `DraggableList<T>::Render`'s
single-callback shape.**

**Why two, deliberately, not one.** `DraggableList<T>` (§19.7's sibling widget family) has exactly
one row kind, so its STEP123 header-extra slot is one callback,
`DrawRowHeaderExtraFunction drawRowHeaderExtra(int rowIndex)`
(`DraggableListWidget_RowLayout_UI.h:104-115`, confirmed by direct read). `TreeListWidget_UI<T,
LeafKeyT>` has TWO distinct row kinds with two distinct identity types — a Node (`int
nodeIdentifier`) and a Leaf (`const LeafKeyT&`), per its own existing `RenderNode`/`RenderLeaf` split
(`TreeListWidget_RowLayout_UI.h`, confirmed by direct read — `Render`'s current signature already
carries 7 distinct callback template parameters, confirming the "existing 7-callback overload"
framing). Forcing one shared callback would need either a type-erased variant parameter (real
overhead for a tens-of-rows authoring-scale widget, §19.7's own "not virtualized on purpose" posture)
or silently wiring only one row kind. Two callbacks, each keyed by that row kind's own real identity
type, is the correct generalization — a deliberate, permanent divergence from `DraggableList`'s
shape, not an inconsistency a future reader should try to unify.

**Contract, ratified verbatim as designed:**
```cpp
// existing 7-callback overload — signature UNCHANGED, becomes a thin delegator (arity alone
// disambiguates, the exact STEP123 DraggableList precedent):
return Render(..., /*drawNodeHeaderExtra=*/[](int) {}, /*drawLeafHeaderExtra=*/[](const LeafKeyT&) {},
              /*headerExtraWidthPixels=*/0.0f, state, selectedNodeIdentifier);

// new overload, additive:
template <..., typename DrawNodeHeaderExtraFn, typename DrawLeafHeaderExtraFn>
static TreeListSignal<LeafKeyT> Render(..., DrawNodeHeaderExtraFn drawNodeHeaderExtra,
                                        DrawLeafHeaderExtraFn drawLeafHeaderExtra,
                                        float headerExtraWidthPixels, TreeListState& state,
                                        int selectedNodeIdentifier = -1);
```
Threaded into `TreeListDetail::RenderNode`/`RenderLeaf`, drawn after the click/drag-drop detection,
before the `if (bExpanded)` body — `headerExtraWidthPixels == 0.0f` draws nothing and reserves
nothing, byte-identical to today's layout, the same convention `DraggableList`'s own slot uses.

**The offset-collision bug class (items 8/9 of the same correction round) does not apply here, and
this contract is structurally immune to it by construction.** `RenderCollapsibleRow`'s bug was
computing the header-extra offset AND the affordance strip's start offset from the SAME
independently-re-derived `rowAvailWidthPixels - headerExtraWidthPixels - ...` formula, landing both
at the same X. Tree rows carry no affordance strip today — nothing for the new control to collide
with. Binding guidance for any FUTURE tree-row affordance-strip work (recorded so it is not
re-learned a second time): position the next element relative to where the previous one's own draw
call actually left the cursor, never by re-deriving the same subtraction a second time from a value
computed for a different control.

**Scope note.** This ruling is the widget-library contract only. Which concrete controls get wired
into `drawLeafHeaderExtra` for the Bundle tree (item 7(b) — Color Override) and which share that same
slot (item 10 — Symmetry, §19.24) is ordinary call-site work once this slot exists, not a further
ARCH ruling.
