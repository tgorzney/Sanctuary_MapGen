# STEP244 — Add `MarkerTransform::linkIdentifier` + `TransformType` group aliases

**Layer:** PARAMS. **Domain:** `src/params/MarkerInstance_PARAMS.h`, `src/params/PropInstance_PARAMS.h`,
`src/params/DecalInstance_PARAMS.h` (or wherever `DecalInstanceGroup`/`DecalTransform` actually live —
confirm exact file by grep, don't assume the name). **Sequence:** foundational; STEP245-249 all depend
on this.

Ratifies `ARCH_19_33_LinkMembershipInstanceTierCorrection.md` (new field) and
`ARCH_21_09_LinkTierContractWidening.md` (the `TransformType` aliases). Corrects
`ARCH_19_29_LinkIdentifierBackReferences.md`'s retracted "neither field is added to `MarkerTransform`"
sentence — read `ARCH_19_33` in full before touching this ticket, it is the binding spec, this ticket
is only a restatement of its "New field — ratified" section plus `ARCH_21_09`'s alias addition.

## Session coordination

Check `ListAgents`/message peer sessions before touching EACH file above — this repo has multiple
concurrent Claude Code sessions active.

## Fix

1. `MarkerInstance_PARAMS.h` — `MarkerTransform` gains:
   ```cpp
   int linkIdentifier = -1;   // ARCH §19.33 — instance-tier Link membership; -1 = not Link-bound.
   ```
   Place near the existing `symmetryGroupIdentifier`/`instanceIdentifier` int fields (same "qualified
   noun, bare int, -1 sentinel" family, §1.9). Also add, per `ARCH_21_09`:
   ```cpp
   struct MarkerInstanceGroup { /* unchanged fields */ using TransformType = MarkerTransform; };
   ```
2. `PropInstance_PARAMS.h` — `PropInstanceGroup` gains `using TransformType = PropTransform;` (no
   `linkIdentifier` — Props have no Link concept, per `ARCH_21_09`'s `NoInstanceLink` design).
3. `DecalInstance_PARAMS.h` (confirm actual filename) — `DecalInstanceGroup` gains
   `using TransformType = DecalTransform;` (same reasoning as Props).

**Do not add `linkIdentifier` to `PropTransform`/`DecalTransform`** — Link membership stays
Markers-only, exactly as `ARCH_19_29`'s original (non-retracted) scoping already established.

## Verify

- Existing `MarkerInstance_PARAMS_Test.cpp`/`PropInstance_PARAMS_Test.cpp` (if present) stay green —
  this is a pure additive change, no existing field renamed or removed.
- New field defaults to `-1` on default-construction (covered by any existing default-value test for
  `MarkerTransform`, or add one if none exists).
- Full build succeeds — the three new `TransformType` aliases are unused until STEP249 wires them into
  `ManualInstanceHitTest_UI.h`/`ManualInstanceDelete_UI.h`, which is expected and not an error at this
  ticket's own completion.

## Out of scope

- IO round-trip for the new field (STEP245).
- Any resolver, UI, or consumer change (STEP246-249).
