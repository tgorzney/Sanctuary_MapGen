[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.10. **Only the ARCH Expert writes this file.**

### 19.10 v1 scoping confirmed: tab-driven Move/Rotate only, no new canvas gesture
**Ratified as designed, no correction.** Move = X/Z offset fields + "Apply Move"; Rotate = degrees
field + "Apply Rotation" — both operate on the Bundle's resolved Manual membership (§19.9),
computed once at Apply-time, both sharing the MATH-layer rotate function (§19.8) Assembly's own
design already needs.

**Why this is the right sequencing call, confirmed.** Assembly's own canvas multi-select machinery
(`CrossLayerSelectionState`, ctrl-click/shift-drag/marquee, `DESIGN_Assembly_R1.md` §2) is itself
unbuilt and unratified as of this session. Building a second, independently-designed canvas
gesture layer for Bundle-drag before Assembly's ships would fork two similar-but-different
interaction models inside `MapCanvas_Draw_UI.cpp` — a real, avoidable design smell, and directly
against the delivery-scoping goal of keeping the first Group/Bundle ticket narrow. Canvas
live-drag-of-a-Bundle stays explicitly deferred, ideally unified with Assembly's own canvas
selection once that ships as one shared interaction model, not designed independently for Bundle
first.

**STEP94 interaction — confirmed, mirrors Assembly's own deferred hazard, not re-derived.** Since
v1 has no live canvas Bundle-drag, the only interaction surface is whether "Apply Move/Rotate" on
a Bundle silently also moves a member's symmetric sibling outside the Bundle. **Ruled: no,
deferred, same as Assembly's own `DESIGN_Assembly_R1.md` §4 hazard note.** v1 applies the flat
rigid delta to exactly the resolved member set and nothing else; a sibling outside the Bundle is
left untouched, no composition attempted. The eventual design, if/when built, should be the same
precedence rule `DESIGN_Assembly_R1.md` §4 already proposed (co-selected symmetric pairs suppress
follow; non-co-selected members keep independent orbit-follow) — the Generator Expert resolves
this once for both Assembly and Bundle when that round happens, not twice for the structurally
identical question.
