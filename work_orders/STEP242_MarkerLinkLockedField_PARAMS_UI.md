# STEP242 — Add `bLocked` to the Link master/slave set

**Layer:** PARAMS + UI. **Domain:** `src/params/MarkerLink_PARAMS.h`,
`src/ui/MarkersTab_ManualLayerHelpers_UI.h`, `src/ui/MarkersTab_LinksHeaderExtras_UI.cpp`, and
wherever STEP241 put the Lock toggle's disable-while-linked treatment (likely
`MarkersTab_ManualLayerRowBody_UI.cpp`, confirm by reading STEP241's actual landed diff first).
**Sequence:** depends on STEP241 landing first — same files, must not run concurrently with it.

Ratifies the second `ARCH_19_31_PropagatedPropertyMechanisms.md` amendment (bLocked as field #7).

## Session coordination

Check `ListAgents`/message peer sessions before touching EACH file. Confirm STEP241 has actually
landed (read the current state of `MarkerLink_PARAMS.h` and `MarkersTab_ManualLayerHelpers_UI.h`
first) before starting — do not assume this ticket's file list is accurate if STEP241 changed the
shape of things.

## Fix

1. `MarkerLink_PARAMS.h` — add `bool bLocked = false;` alongside the six fields STEP241 added.
2. `MarkersTab_ManualLayerHelpers_UI.h` — add `EffectiveManualMarkerLayerLocked(...)`, same
   read-and-resolve shape as the other six `Effective*` resolvers STEP241 built.
3. Gate the existing Lock toggle control the same disabled-while-linked way STEP241 already applied
   to color/hidden/iconScale/gridSnap/symmetry.
4. `MarkersTab_LinksHeaderExtras_UI.cpp` — add a Link-side Lock toggle control, mirroring the other
   five Link-side controls STEP241 added.
5. **No `MarkerLayerBundle`-tier field** — confirmed no lock-like field exists there and none is
   proposed. Do not add one.

## Verify

- Toggling a Link's `bLocked` updates every bound Layer's resolved lock state; each Layer's own Lock
  toggle is disabled while `linkIdentifier >= 0`.
- Un-linking restores independent Lock editing at the last-resolved value.
- Extend whichever test file STEP241 used for the other six fields to cover `bLocked` the same way.
- Full `MarkersTab_UI_Test` suite stays green.

## Out of scope

- Bundle-tier locking (not proposed, would need its own field + ARCH ruling).
