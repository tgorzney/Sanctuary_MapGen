[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.24. **Only the ARCH Expert writes this file.**

### 19.24 `Params::MarkerInstanceLayer::bSymmetryEnabled` — new field, ratified as designed
Responds to item 10. **Ratified as designed** — field name, placement, and wire-key confirmed
correct by direct read; no correction needed.

```cpp
// MarkerInstanceLayer (MarkerInstance_PARAMS.h) gains:
bool bSymmetryEnabled = true;   // default true — every pre-existing/legacy layer's configured
                                  // `symmetry` mask stays live post-migration. false gates the
                                  // EFFECTIVE mask to SymmetryAxis::None (radialSymmetryRepeatCount
                                  // inert) WITHOUT destructively clearing the axis checkboxes
                                  // already configured in `symmetry` itself — re-enabling recovers
                                  // the prior configuration with no re-authoring.
```
Precedent confirmed by direct read: shape mirrors `bColorOverrideEnabled`'s already-shipped field in
the same struct (`MarkerInstance_PARAMS.h:42-49`) — a bool gate beside the real data field it gates,
never mutating that field's own contents.

**Wire key: `"SymmetryEnabled"`.** Confirmed by direct read of the sibling fields' own IO code
(`MapExporter_Markers_IO.cpp:79-82`, `MapImporter_MarkerGroups_IO.cpp:40-43`): this struct's
established `b<Name>` → PascalCase-drop-`b` convention is exact and unbroken — `bLocked`→`"Locked"`,
`bGridSnapEnabled`→`"GridSnapEnabled"`, `bColorOverrideEnabled`→`"ColorOverrideEnabled"`.
`bSymmetryEnabled` follows the same rule without exception.

**Additive, no `SanGenVersion` bump** — same precedent class as this struct's other additive bool
fields (§19.3/§19.4's "no legacy risk, the field never previously existed" posture).

**Consumer gate — binding, not left for the coder to infer.** Every current reader of
`MarkerInstanceLayer::symmetry` (`ResolveEffectiveMarkerSymmetry`'s call sites in
`MarkerDragGesture_UI.cpp`, and `MarkerSymmetryFixCommand_UI.cpp`'s repair walk) must check
`bSymmetryEnabled` first and force the effective mask to `SymmetryAxis::None` when false, never
reading `symmetry`'s raw bitmask directly. This closes the field over its one stated purpose — a
soft, non-destructive on/off gate — without leaving a second place a future reader could bypass it.

**Placement, confirmed consistent with §19.23.** The row header-extra region carries TWO adjacent
controls once this lands — `[Symmetry toggle][Color Override]` — sharing one
`headerExtraWidthPixels` sized to their combined width; `drawLeafHeaderExtra`/`drawRowHeaderExtra`
each draw both in sequence, no further widget-library change beyond §19.23's slot itself. Reaches
Bundle-tree rows via §19.23's `drawLeafHeaderExtra`, and the flat/ungrouped `DraggableList` rows via
the already-shipped STEP123 slot — same field, same gate, two render paths, confirmed correct per
§19.9/§19.20's existing "a bundled layer has no reason to lack a control an ungrouped one has"
posture.
