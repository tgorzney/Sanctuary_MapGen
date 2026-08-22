# STEP75 — Mirror odd armies onto even armies (180° about map center)

**Layer:** UI (+ a small PIPELINE passthrough addition). **Domain:** Armies tab.
**Consulted:** SanGen Format Expert, SanGen UI Expert (this session). **Coordinated with:**
`map-generator-13` (owns the related `STEP68` orbit-position wrapper — confirmed no conflict,
this ticket does not modify their work).

## Problem
Human's ask: Army1/3/5 (odd, by position in `recipe.armies`) get duplicated onto Army2/4/6
(even), renamed, every duplicated unit rotated 180° about map center (position + orientation).
One-time duplicate action, not a persistent link.

## Ruling (UI Expert + Format Expert consults)
1. **Per-army button, not global.** In `DrawArmySettings` (`src/ui/ArmiesTab_UI.cpp`), visible
   only when the selected army is even-0-indexed (odd-numbered: Army1/3/5) **and** a successor
   row exists in `recipe.armies`.
2. **Pair generically by consecutive position** (`sourceIndex`, `sourceIndex + 1`), not hardcoded
   to 3 pairs — matches the army stack's existing arbitrary add/remove/reorder via `DraggableList`.
3. **Gate behind `ConfirmDialog_UI`** — this overwrites the target army's existing hand-placed
   `groups` tree, a destructive operation needing confirmation. Body text names both armies and
   warns the target's current units will be replaced.
4. **New pure helper in `ArmiesTab_UI.h`**, same style as `DropUnitRulesForRemovedArmy`:
   ```cpp
   void MirrorArmyGroupsOntoNextArmy(std::vector<Params::Army>& armies, int sourceIndex,
                                     const Params::Geometry& geometry);
   ```
   Recurses `UnitGroup.groups` and `UnitGroup.units` (both must be walked — a group can have
   nested child groups).

   **⚠️ Naming law (confirmed by the human, recorded in the Format Expert's charter): army names
   are load-bearing.** The engine assigns lobby slots by sorting army names ALPHABETICALLY
   (`gameUtils.lua`'s `CreateArmies()`), so an unpadded name (`ARMY_1`/`ARMY_2`/`ARMY_10`) sorts
   wrong from the 10th army onward — silently wrong slot assignment, no error. The required
   format is `ARMY_XX`, zero-padded two digits. **Set the target army's name to the correctly
   zero-padded form matching its own position in `recipe.armies`** (`ARMY_02`, `ARMY_04`, etc.) —
   do NOT copy the source army's name verbatim (it names a different slot) and do NOT use
   `NextArmyName`/`Params::Army armyCount)` (that seeds brand-new rows from a count string like
   `"Army1"`, not the required `ARMY_XX` format — see the pre-existing gap noted below). Feed
   `bArmiesMoved`-style true so any existing uniqueness repair still runs, but the padded format
   is this ticket's own responsibility, not something to delegate to a helper that doesn't
   currently produce it.

   **Pre-existing gap, OUT OF SCOPE for this ticket, flagged not fixed here:** `NextArmyName`
   (`ArmiesTab_UI.h:77`, `NextUniqueLabel("Army", armyCount)`) already produces `"Army1"`/`"Army2"`
   style names for every newly-added army today — not `ARMY_XX`. This is a real, broader,
   pre-existing correctness bug affecting the whole Armies tab (not just this ticket's mirror
   action), silently wrong from the 10th army onward. This ticket does not fix `NextArmyName`
   itself — only ensures the army IT creates is correctly formatted. Report this gap to the human
   as a separate ticket candidate; do not silently absorb the larger fix into this one.
5. **No preview dirtying.** Zero PIPELINE code reads `Army.groups` today (confirmed by grep) —
   matches the existing posture for Army's other fields (`ArmiesTab_UI.h` SCOPE NOTE 1). Do not
   call `previewDriver->NotifyParametersChanged()`.

## The rotation math — position via STEP68's wrapper, orientation via a new small sibling
**Position**: once `STEP68` (`src/pipeline/SymmetryOrbitQuery_PIPELINE.h`,
`Pipeline::BuildWorldSymmetryOrbit`) lands, call it with `symmetryMask = Params::SymmetryAxis::
RotateHalfTurn`, the source `UnitTransform`'s world `(positionX, positionZ)` — it returns the
180°-about-center mirrored world position, already handling the cell/world unit conversion
correctly via `geometry.worldUnitsPerCell`. **Do not** re-derive map center independently or
duplicate this math — this was the exact open ARCH question from this ticket's earlier design
pass (`mapSize/2` vs `mapSize*worldUnitsPerCell/2`), and `STEP68`'s wrapper already resolves it
correctly by construction. `positionY` (elevation) is untouched by a yaw rotation.

**Orientation**: `map-generator-13` (STEP68's owner) flagged, correctly, that `STEP68`'s orbit
math was designed for stochastic scatter re-sampling and its `yawScale`/`yawOffsetRadians` fields
are unverified for the general mirror-axis case on an arbitrary 3D quaternion — reflections and
rotations are not the same class of operation for a full quaternion. **This ticket sidesteps that
open question entirely**: `RotateHalfTurn` is a pure rotation (not a reflection), so composing an
existing quaternion with a 180° yaw is well-defined and simple — the literal quaternion
`(x=0, y=1, z=0, w=0)` (sin(90°)=1, cos(90°)=0 — hardcode this, do not compute via trig, to avoid
float error), composed via Hamilton product. **Do not extend `STEP68`'s shared struct** — add a
new, separate, small PIPELINE function instead:
```cpp
// A new sibling in SymmetryOrbitQuery_PIPELINE.h (once that file exists) or its own small file —
// confirm placement with whichever session owns that file at implementation time.
// Composes `sourceRotation` with a 180-degree yaw about the vertical (Y) axis. ONLY valid for
// RotateHalfTurn (pure rotation) — do NOT generalize to mirror axes (MirrorAcrossX/Z) without a
// separate, verified design; see map-generator-13's flagged correctness concern.
void ApplyHalfTurnYaw(float sourceRotationX, float sourceRotationY, float sourceRotationZ,
                      float sourceRotationW, float& outRotationX, float& outRotationY,
                      float& outRotationZ, float& outRotationW);
```
Reuses `Placement_Transform_PROC.h`'s `QuaternionMultiply` (Hamilton product) as its
implementation — this new function is PIPELINE-layer specifically so `ArmiesTab_UI.h` (UI layer)
can legally call it (`ARCH_03_ModuleBoundaries.md` §3.1: `UI → PIPELINE → PROC` is legal, `UI → PROC` directly is
not — the exact same layering rule `STEP68` exists to satisfy for the position case).

**⚠️ Dependency**: this ticket cannot be implemented until `STEP68` actually exists on disk
(confirmed with `map-generator-13`: drafted, not yet dispatched to a coder as of this writing).
Either wait for `STEP68` to land, or coordinate to land both in the same coder pass.

## Files touched
- `src/ui/ArmiesTab_UI.h` — `MirrorArmyGroupsOntoNextArmy` helper, new `ConfirmDialogState` field
  on `ArmiesTabState`
- `src/ui/ArmiesTab_UI.cpp` — the per-army button, confirm-dialog wiring
- `src/pipeline/SymmetryOrbitQuery_PIPELINE.h`/`.cpp` (STEP68's file) — `ApplyHalfTurnYaw` added
  as a sibling function, OR a new small file if STEP68's owner prefers not to share the file

## Verify
Full solo rebuild + `ctest -C Debug`, full suite green. New test:
`MirrorArmyGroupsOntoNextArmy` on a synthetic `Params::Army` with a nested `UnitGroup` tree —
confirm every leaf `UnitTransform` position is correctly mirrored (`newX = 2*centerX - X` in
world units, matching `BuildWorldSymmetryOrbit`'s own convention) and every rotation is correctly
composed (verify against a hand-computed expected quaternion for at least one non-identity source
rotation, not just the identity case). Confirm nested child groups (not just top-level `units`)
are recursed correctly.
