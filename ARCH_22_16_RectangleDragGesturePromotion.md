[← ARCH index](ARCH.md) · [§22 ARCH_22_NavmapModifierBlockers](ARCH_22_NavmapModifierBlockers.md) · SanGen ARCH §22.16. **Only the ARCH Expert writes this file.**

### 22.16 `AreaDragGesture_UI`'s rectangle core is promoted to an accessor-parameterized template — the second proven consumer

**RULED: promote.** `AreaDragGesture_UI`'s hit-test/resize/aspect-lock/center-move algorithm is
generalized into a new accessor-parameterized template — `RectangleDragGesture_UI<Accessor>` (exact
name/file layout the implementing coder's call) — living alongside the existing substrate files.
`AreaDragGesture_UI` and the new `NavmeshBlockerDragGesture_UI` both become thin instantiations,
mirroring exactly how `PropDragGesture_UI`/`DecalDragGesture_UI` already sit as thin `Traits` shims
over `InstanceDragGesture_UI<Traits>` (§21.3) — the same genericity mechanism this pack already uses
for point-instance dragging, applied here to rectangle dragging for the first time.

**Reasoning, per this project's own "proven twice" discipline** (`ARCH_19_02_GenericitySplit.md`,
cited by the UI doc itself). `ARCH_21_08_AreaCanvasGesture.md` correction 2's original ruling that
Areas' algorithm should stay hand-written was correct **at the time it was written** — Areas was the
only consumer, and genericizing a single-consumer algorithm is premature abstraction this pack avoids
elsewhere too (§22.4's own "stay map-local until proven twice" language; `ARCH_15_10`'s "ported only
once shown universal" reasoning). `Params::NavmeshBlockerRectangle` is now confirmed
(`ARCH_22_15_NavmeshTabParamsShape.md` point 3) to deliberately mirror `MapArea`'s exact
`{originX, originZ, sizeX, sizeZ}` field shape specifically so this promotion is mechanical, not a
rewrite — the same bar this pack applies elsewhere (the Group/Bundle tree, §19.2; `HitTestManualInstances<GroupT>`,
§21.3) is met on the same terms: pure mechanics (hit-test math, resize-delta math, aspect-lock math)
with zero domain-field access beyond four named floats, now proven by a second real, independently-
designed consumer before generalization — not requested speculatively.

**Binding shape:**
- The template is accessor-callback-parameterized over the four floats (get/set `originX`/
  `originZ`/`sizeX`/`sizeZ`) and over the handle-radius/minimum-extent constants. Per
  `ARCH_22_15_NavmeshTabParamsShape.md` point 3 and Constitution §8, **each instantiation keeps its
  own named constant** for handle radius and minimum extent (never a shared literal), even where a
  value starts numerically equal to Areas' own.
- Genericity lives in the mechanism only, never in the data shape (§19.2's own rule, restated):
  `Params::MapArea` and `Params::NavmeshBlockerRectangle` remain two independently-written structs;
  only the drag-gesture algorithm becomes shared.
- **Ticket 230** (`NavmeshBlockerDragGesture_UI`, per the UI doc's own breakdown) is authorized to
  build the template AND both instantiations — Areas' own files are refactored to the new thin-shim
  shape as part of this same ticket, not left as dead duplicate code alongside the new template. This
  settles the UI doc's own flag-1/§6 ambiguity ("OR, if ARCH accepts the promotion recommendation...")
  in favor of the promotion branch explicitly; the "port a byte-identical ~140-line copy" fallback the
  UI doc also described is superseded by this ruling and must not be built instead.
