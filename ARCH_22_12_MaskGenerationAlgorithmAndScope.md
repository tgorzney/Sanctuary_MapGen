[← ARCH index](ARCH.md) · [§22 ARCH_22_NavmapModifierBlockers](ARCH_22_NavmapModifierBlockers.md) · SanGen ARCH §22.12. **Only the ARCH Expert writes this file.**

### 22.12 Mask-generation algorithm, accuracy class, and v1 layer scope — Sea + Submarine only

Ratifies `work_orders/DESIGN_NavmeshBlockerMaskGeneration_R1.md` (Generator Expert) §3-§4 as
designed, with the scope question it raised now closed.

**1. The algorithm confirmed as designed.** Per-instance plane derivation via the general
inverse-transpose-under-scale form (never the naive rotate-only shortcut — see
`ARCH_22_14_GeometryMathAndDispatch.md` point 2), the four-case per-triangle clip table, the
layer-specific **exact-slice** height rule (not "everything below H, unioned" — the porthole
counter-example in that doc's §3.3 is correct and load-bearing), world→pixel rasterization, and
cross-instance OR-union feeding `NAVMAP_MODIFIER_BLOCKER_SPEC.md` §7's unmodified decomposition —
all confirmed, no correction.

**§3.4's retraction is accepted as correct and closed; no further ARCH action needed on it.** The
design's own direct read of `MapExporter_DocumentAssembly_IO.cpp` confirming `worldUnitsPerCell`
never enters the exported map's declared `width`/`length` is accepted ground truth; §8's original
pixel↔world formula (`NAVMAP_MODIFIER_BLOCKER_SPEC.md` §8) needed no correction and none is made
here.

**2. Accuracy-class assignment confirmed:** plane-classify/clip/rasterize/exact-decompose = **Exact**
class (Constitution §4 — a missed pixel is a real pathing hole); the agglomerative merge pass =
**Accurate** class (a stated, human-tunable overshoot tolerance). No accuracy-class tension.

**3. v1 layer scope RULED: Sea + Submarine only** (that doc's Q1 → option (a), confirmed, not merely
accepted as a recommendation). Land, Amphibious, Hover, and Air are explicitly OUT OF SCOPE for this
water-plane-slice technique — per the now-ratified six-layer height-band table
(`NAVMAP_MODIFIER_BLOCKER_SPEC.md` §2, folded in by this ratification from
`forum_posts/TUTORIAL_NavMeshBlockers.md`, confirmed against the same engine-source ground truth
`NAVMOD`/`NAVLAYERS` already establish), those four layers' bands are either `-∞..+∞` or otherwise
water-independent — the water-plane technique is structurally the wrong tool for them, not merely
an unfinished extension of the right one. Forcing them into this technique now would produce wrong
output or silently degrade into a different technique anyway.

**A future, structurally different "full mesh silhouette" technique is recorded, not designed, as
the correct eventual tool** for Amphibious/Hover/Air (and an optional dry-land Land refinement) —
project the instance's full mesh/AABB to XZ directly, no plane math, reusing this design's
§3.4/§3.5/`NAVMAP_MODIFIER_BLOCKER_SPEC.md` §7 rasterize/decompose machinery unchanged. This mirrors
§22.9's own "recorded, not scheduled" posture exactly. A future ticket toward it needs its own
separately-scoped design consult before any coder builds toward it — the same discipline this
ratification itself was gated behind.

**4. A separate, unrelated defect, recorded but not authorized for work here.** While verifying
§3.4, the Generator doc surfaced that exported entity positions ARE scaled by `worldUnitsPerCell`
(`Placement_Emit_PROC.cpp`) while the exported map's own declared size is not — a probable general
map-export correctness bug, unrelated to navmesh blockers. Recorded as a flagged, unscheduled
finding; a future investigation ticket, not authorized, designed, or scoped by this ruling.

**5. Storage-shape question** (one tagged vector vs. six parallel per-`NavLayerKind` vectors, raised
identically in this doc and in `DESIGN_NavmeshTab_UI_R1.md`) is ruled once, at
`ARCH_22_15_NavmeshTabParamsShape.md` point 2 — not duplicated here; cross-reference only.
