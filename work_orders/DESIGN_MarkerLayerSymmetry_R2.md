# Design Output — Markers Tab UI + Layer-Scoped Symmetry, Round 2

Continuation of `DESIGN_MarkerLayerSymmetry_R1.md` (read that first — this supersedes its §2/§3/§4
items 2 and 6 where noted, carries the rest forward unchanged). Design phase only, no code. Still
no coder-dispatchable ticket — pending ARCH (+ Format/Generator/IO Architecture) ratification.

## 1. The any-member-drag identity gap — resolved

Key fact: `BuildSymmetryOrbit` always writes the seed point to output slot 0 before running any
transform. So: whichever member is dragged always maps to slot 0 of its own recompute, and calling
the function with a member's **pre-drag** position reproduces every other member's pre-drag
position exactly (same physical point set, same existing `duplicateEpsilon`, no new tolerance
needed).

**Mechanism** (reuses the ARCH-ratified C2 gesture-cache pattern for identity, not just vertex bytes):
- **Gesture start** (mouse-down on any member M): run the orbit once from M's pre-drag position,
  match its output 1:1 against the existing siblings' pre-drag positions by exact-value equality —
  safe because both sides are the *same, unmoved* point set (the case the Generator Expert flagged
  as unreliable was matching an old cloud against a *rotated* new one; that doesn't apply here).
  Cache the `{slot → MarkerTransform}` table for the gesture.
- **Live drag**: re-run the orbit from M's current position every frame, write results straight
  into the matched existing instances — alias/army-assignment never touched, only position.
- **Cardinality change mid-drag**: never mutate live. Extra points during the drag are a
  screen-space-only ghost preview (zero PARAMS write); a live count below the original is a
  soft-hide (ephemeral flag, nothing erased). Structural changes (real create/delete) commit only
  at mouse-up, reusing R1's existing materialize/cascade-delete machinery.
- **Spawn-specific**: a Spawn-category group additionally refuses to commit a cardinality change on
  release — snaps back to the last position where the count still matched. Ordinary repositioning
  (no cardinality change) is exactly as unrestricted as any other marker type; only the resize-via-
  drag path is blocked for Spawn. Resizing a Spawn group stays possible via the layer's symmetry
  setting or the explicit roster Add/Delete — just not an accidental drag.

**PARAMS consequence — shrinks R1's ask**: `bSymmetryAnchor` is dropped (no member is privileged).
`symmetryOrbitIndex` is dropped too — the correspondence table above is fully re-derivable on
demand, never needs persisting. Only **`symmetryGroupId`** remains as new `MarkerTransform` state.

## 2. Spawn/Army shrink — orphan, not auto-delete

When a Spawn group shrinks (via the layer-mask path — drag-triggered shrink is refused per §1),
the Army that loses its Spawn marker is **orphaned, never auto-deleted**. Reasoning: an `Army`
carries a full authored `UnitGroup` tree plus alloys/energy/faction/color — auto-deleting it from a
marker-tab side effect would silently discard real content. STEP49 already treats a missing Spawn
group as a tolerated soft-degrade ("that army gets no commander"), and `ArmiesTab_UI` already lets
a designer delete a whole Army with no confirmation dialog — orphaning here is lower-stakes than
that already-unconfirmed action, so no new confirmation dialog either.

## 3. Sanmap Spawns vs. Scenario Spawns — boundary confirmed, no rework risk

This round's Spawn logic keys off `MarkerInstanceGroup::name == "Spawn"` (a plain string check,
same as STEP49 already uses) — recommend one named UI-layer constant for this instead of repeating
the literal, so extending the set later is a one-line change. Scenario Spawns remain architecturally
separate (a different file SanGen doesn't IO yet) and never merge with `MarkerTransform`. The
visual-distinctness the human wants for Scenario Spawns, whenever that lands, is already free once
`MarkerInstanceLayer`'s `color`/`iconScale` ship (R1 §1) — no rework needed from this round.

## 4. Revises R1 §2/§3
- No `bSymmetryAnchor` — every member is a full member from creation.
- **Any group member is draggable**, including on the canvas. R1's "siblings not independently
  draggable" (tab sliders disabled, canvas drag refused) is retracted — every roster row's position
  fields stay live, going through the same §1 mechanism as a canvas drag.
- "Break Symmetry Link" and cascade-delete-on-any-member-delete carry over unchanged, restated as
  "any member" rather than "the anchor."

## 5. Flagged for ARCH etc. — revises R1 §4 items 2 and 6, carries the rest forward unchanged
2. New `MarkerTransform` field: **`symmetryGroupId` only** (not 3 fields as R1 proposed).
6. New merged field on `markers[type].transforms[name]`: **`symmetryGroupId` only** + R1's
   already-carried `layerIndex`.
9. NEW — Spawn/Army shrink ruling (§2): orphan, never auto-delete. Route to Format Expert re:
   interaction with STEP49's still-unbuilt export-time "warn on missing Spawn group" idea.
10. NEW — house-keeping only, not a PARAMS ask: one named constant for "Army-keyed marker group
    name(s)," not a repeated string literal.

## Who else this touches (revises R1)
- Generator Expert: as R1, plus confirm the gesture-start matching helper can live wherever
  `BuildSymmetryOrbit` ends up after R1's MATH-relocation question is settled.
- Format Expert: R1 items 4-6 (smaller now), plus §2's orphan ruling.
- IO Architecture Expert: unchanged from R1 — smaller migration surface since 2 of 3 proposed
  fields were retracted before ever shipping.

No coder-dispatchable ticket this round.
