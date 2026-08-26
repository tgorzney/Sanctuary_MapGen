[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.15. **Only the ARCH Expert writes this file.**

### 19.15 Type-section × Bundle-tree composition — filtered-copy `TreeListWidget_UI<MarkerLayerBundle>` per type, the cross-Type-section nesting cutoff, and `bRowSuppressed`'s two-predicate composition
Responds to `DESIGN_MarkerTypeSectionsAndInstanceSelection_R1.md` items 3–5 and Open Q6. Three
related rulings for the same Ticket B composition, one subsection.

**(a) Filtered-copy-per-instantiation — ratified, extends §19.7.** `DrawMarkerLayerBundleTree` gains
one parameter, `const std::string& markerTypeNameFilter`; it builds a fresh
`std::vector<Params::MarkerLayerBundle>` per type, containing only bundles whose `markerTypeName`
matches, and passes that filtered copy to `TreeListWidget_UI<MarkerLayerBundle>::Render`. Safe by
§19.7's own contract, confirmed by direct read: `Render`'s `nodes` parameter drives tree LAYOUT only
— every mutation path (`Reparent`/`Select` signal application, `MarkersTab_Bundles_UI.cpp:105-132`)
resolves the real `Params::MarkerLayerBundle&` by `identifier`-keyed lookup into the CALLER's real
`bundles` vector, never by position within whatever was passed to `Render`. Reads come from the
filtered copy; writes go through identifier lookups into the real vector regardless of which
per-type copy triggered them. "Add Group" inside a Type-section seeds
`bundle.markerTypeName = markerTypeNameFilter` at creation, mirroring
`parentBundleIdentifierForNewLayer`'s already-shipped threading pattern
(`MarkersTab_ManualLayers_UI.h`) exactly.

**(b) Cross-Type-section nested-Bundle cutoff — ratified, same shape as §19.6 one tier up, cited
explicitly rather than left implicit.** If a nested child Bundle's own `markerTypeName` differs from
its parent's, it is absent from the parent's filtered copy; `TreeListWidget_UI<T>::Render`'s own
dangling-parent-resolves-to-root rule (§19.7's generic contract) then renders that child as a ROOT
inside its own, different Type-section's tree, with zero new widget code. This is the UI-render-tier
analogue of §19.6's PARAMS-tier membership-walk cutoff — different machinery (a display consequence
of filtering vs. a recursive-collection early-exit), same governing principle: a Bundle whose own
scoping tag diverges from its ancestor's stops being organizationally subordinate to that ancestor at
that point, at every tier this pack defines a scoping tag for. Binding: this is the intended,
correct rendering of a legitimately re-typed nested Bundle, not a defect to patch around.

**(c) `bRowSuppressed` composing two independent predicates — signed off, stays within the field's
documented contract.** Each per-type "Ungrouped Procedural/Manual" `DraggableList` instantiation
(STEP120's existing lists, unchanged mechanism, repeated once per Type-section) suppresses a row
when EITHER it belongs to a Bundle OR it belongs to a different type:
```cpp
row.bRowSuppressed = (layer.parentBundleIdentifier != -1) || (layer.markerTypeName != thisSection.typeName);
```
`DraggableListWidget_Types_UI.h:32-38`'s own documented contract for `bRowSuppressed` — present a
FILTERED view without `DraggableList` itself gaining filtering logic — does not restrict the
predicate to a single condition; composing two independent boolean filters with `||` is ordinary
predicate usage, not a contract violation. Signed off as legal.

**Known, accepted, inherited quirk — recorded, not fixed here.** Each Type-section's `DraggableList`
still walks the SAME full, un-filtered backing vector under a different suppression predicate; a
reorder-drag issued from one Type-section's filtered view operates on real vector indices and can
silently land past a row belonging to a different, currently-invisible type. This is the exact
tradeoff `DraggableListWidget_Types_UI.h:37`'s own comment already documents for the single-predicate
case, now exercised across two composed predicates at once — a widened blast radius, explicitly
recorded here as an accepted decision, not a silently inherited one. A future ticket may narrow
reorder to the visible/filtered set if this becomes a real authoring complaint; not owed by Ticket B.
