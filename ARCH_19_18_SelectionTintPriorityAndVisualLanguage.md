[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.18. **Only the ARCH Expert writes this file.**

### 19.18 Selection tint — canonical priority order, and "selected replaces fill" as a visual language distinct from the drag-ghost
Responds to `DESIGN_MarkerTypeSectionsAndInstanceSelection_R1.md` Open Q3.

**Priority order — ratified, recorded as a canonical cross-cutting rule, not left as one function's
implementation detail.** Extends `DrawManualMarkerRoster`'s existing branch chain
(`MapCanvas_MarkerDrag_UI.cpp:119-127`, confirmed by direct read), highest to lowest:
1. Refused-drag red (`bThisGroupDragging && dragState.bSpawnCardinalityRefused`) — unchanged, wins
   over everything. An active-error signal must never be masked by a passive authoring aid.
2. **Selected highlight (new)** — full replacement of fill color, including army color and
   layer/type color.
3. Army color (`ManualSpawnArmyTint`, Spawn groups only).
4. Per-layer override / type-default color (`ManualMarkerTint`, unchanged).

`bLocked` carries no tint effect today (gates drag/reposition only, by its own doc comment); this
ruling gives it none — a locked+selected instance gets the select tint normally, no conflict.

**"Selected replaces fill" — ratified as its own named visual language, explicitly distinct from the
drag-ghost's unfilled-ring vocabulary; no shared meaning between the two.** Confirmed by direct
read: the live-drag ghost draws an UNFILLED ring (`drawList.AddCircle(...)`,
`MapCanvas_MarkerDrag_UI.cpp:144` — `AddCircle`, not `AddCircleFilled`), already meaning "unclaimed
drag slot." A selection highlight that reused that same unfilled-ring shape for "selected" would
collide two unrelated meanings onto one visual — a real semantic collision, not an aesthetic one.
Ratified: selection is communicated by REPLACING the marker's normal filled-dot color outright
(still `AddCircleFilled`, a different `tint`), never by an outline/ring. The two vocabularies stay
permanently non-overlapping: filled-dot color = identity/selection state; unfilled ring = ephemeral
drag-ghost slot.
