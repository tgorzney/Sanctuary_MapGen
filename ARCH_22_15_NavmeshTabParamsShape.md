[← ARCH index](ARCH.md) · [§22 ARCH_22_NavmapModifierBlockers](ARCH_22_NavmapModifierBlockers.md) · SanGen ARCH §22.15. **Only the ARCH Expert writes this file.**

### 22.15 Navmesh PARAMS shape and Tab structure — closed `NavLayerKind` enum, tagged-vector storage, origin+extent coordinates

Rules on the structural findings and open questions in `work_orders/DESIGN_NavmeshTab_UI_R1.md`
(UI Expert) §4-§10 the human specifically asked ARCH to weigh in on.

**1. `Params::NavLayerKind` as a closed C++ enum vs. `ARCH_19_14_TypeSectionUiDerived.md`'s open-
string `markerTypeName` precedent — RULED correctly reasoned, not accidental drift.**
`markerTypeName`'s open-string shape exists specifically because a designer can invent any Bundle
"type" freely (§19.3/§19.14). A nav layer is the structural opposite — exactly six engine-fixed
values (now ratified ground truth, `NAVMAP_MODIFIER_BLOCKER_SPEC.md` §2, folded in by
`ARCH_22_12_MaskGenerationAlgorithmAndScope.md`), two conditionally absent, never designer-
extensible. `enum class Params::NavLayerKind { Land, Amphibious, Hover, Air, Sea, Submarine }` is
confirmed the correct shape. §19.14's own ruling stands unmodified and ungeneralized beyond its own
open-string domain — this is not a second precedent diluting it, it is the correct application of
the closed-vs-open distinction §19.14 already implies (compare `Params::SymmetryAxis`, itself a
closed bit-set enum, never an open string).

**2. Storage shape RULED, resolving the identical open question raised independently by both the
Generator and UI docs: the filtered-copy tagged-vector convention.** One shared
`recipe.navmeshBlockerLayers` / `navmeshBlockerLayerBundles` / `navmeshBlockerRectangles`, each entry
carrying its own `navLayerKind` tag, with the Tab building a per-`NavLayerKind` filtered view exactly
as `MarkerLayerBundle` already does (`ARCH_19_15_TypeSectionTreeComposition.md`) — **not** six
parallel per-`NavLayerKind` vectors. Both docs independently recommended this default for the same
reason (matches the proven Markers/Props/Decals precedent); ARCH formally closes it here rather than
leaving it "PARAMS/Format Expert's call" twice over.

**3. Coordinate-convention finding confirmed binding, not merely recommended:**
`Params::NavmeshBlockerRectangle` uses `MapArea`'s origin+extent shape (`{originX, originZ, sizeX,
sizeZ}`), never the engine's center-anchored shape — the origin→center conversion happens once, at
the Lua-export boundary, never mid-pipeline. This is not a UI-taste preference: it is the load-
bearing precondition for `ARCH_22_16_RectangleDragGesturePromotion.md`'s promotion ruling (a
center-anchored struct would require re-deriving the resize math, not merely renaming fields).

**4. §5.3's symmetry-drop confirmed correct, for the stated geometric reason, not merely "matches
Areas."** No `SymmetrySetting` field on `NavmeshBlockerLayer` — quarter-turn symmetry on an
axis-aligned rectangle requires an extent-swap this codebase's existing orbit machinery was never
built to do, and `Radial`/arbitrary-N-fold symmetry produces genuinely non-axis-aligned geometry
`NavmapModifierTemplate` cannot represent at all. This finding also applies to `Params::MapArea`
(already, independently, carrying no symmetry field) — recorded here as the pointer; no standing
note is added to `ARCH_13_RadialSymmetry.md`/`ARCH_16` by this ruling, since no ticket currently
proposes rectangle symmetry for either domain. A future ticket that does must re-derive this from
source; this paragraph is the shortcut.

**5. Q2 (Bundle Move/Rotate) confirmed: Move only, ported verbatim; Rotate is dropped entirely, not
merely disabled in UI** — same reasoning as point 4 (a rotated axis-aligned rectangle is not
representable at all).

**6. §10's compositing-model call confirmed correct in direction; not fully specified here.** A
navmesh blocker rectangle is filled/bordered/handle-editable geometry — the same rendering shape as
`Params::MapArea`, not a point-entity icon — so it belongs with `PreviewFieldLayer`/
`PreviewLayerKind` (mirroring `ARCH_14_17_MapAreaFieldLayer.md`'s `MapAreas` model), not the
overlay-icon-stack ARCH §14's six-domain list otherwise governs. This ratification confirms the
**direction** of that call (composited field layer, not overlay icon) as correct and binding on the
eventual design; the exact seam — new `PreviewLayerKind` value(s), binding indices, GPU record
layout, defaults — is deliberately **not** specified here. It needs its own `§14.17`-shaped ruling
once UI ticket 231 is scoped in the same detail §14.17's own human-approved design required, not
invented ahead of that scoping.

**7. Everything else in the UI doc's §4 proposed struct sketch** (exact field names, exact wire
keys beyond points 1-3 above) remains the Format/PARAMS Expert's call, unratified here — this
section rules only the structurally load-bearing findings (points 1, 3) plus the confirmations
(points 2, 4, 5, 6) explicitly asked for.
