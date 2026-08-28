[← ARCH index](ARCH.md) · [§21 ARCH_21_CanvasInteractionUnification](ARCH_21_CanvasInteractionUnification.md) · SanGen ARCH §21.7. **Only the ARCH Expert writes this file.**

### 21.7 File-size ceiling flag — `MapCanvas_UI.h`

**Recorded, not resolved here** — the same posture `ARCH_19_22_ManualLayersHeaderSplit.md` §19.22
and `ARCH_08_04_CoderScopeLaw.md` §8.4 already establish for a file that will cross ceiling as a
DIRECT, foreseeable consequence of a ratified change: the coder work-order building §21.1-§21.6
must re-measure `MapCanvas_UI.h` (261 lines today, confirmed by direct read) against
`ARCH_01_05_FileSizeCeilings.md` §1.5's ceilings once `SetManualPropDragSource`/
`SetManualDecalDragSource` (mirroring `SetManualMarkerDragSource`'s existing shape,
`MapCanvas_UI.h:107-115`), §21.1's widened selection-set declarations, and §21.2's new right-button
tracking fields all land. If over ceiling, split a companion header — this file's own directory
already has direct, ratified precedent for exactly this move, twice over
(`MapCanvas_MarkerHitTest_UI.cpp`/`MapCanvas_MarkerRosterDraw_UI.cpp`, both split out of
`MapCanvas_MarkerDrag_UI.cpp`/`.h` at STEP126 for the identical reason). No specific split shape is
ruled here — §8.4's scope law applies: measure first, split only if actually over, mirror the
established sibling-header convention this file's own directory already uses.
