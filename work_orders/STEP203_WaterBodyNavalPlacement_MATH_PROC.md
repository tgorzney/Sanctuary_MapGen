# STEP203 — `WaterBodyLabel_MATH` + `NavalFleetPlacement_PROC`: find water bodies, seat opposing player pairs in them

**Layers:** MATH (`src/math/`) then PROC (`src/proc/`). **Domain:** heightfield water-body
analysis and naval formation placement. **Sequence:** Map Scenario track, feeds the
`spawns`/unit-instruction data that **STEP70** (`ScenarioScript_DataLua_IO`) renders into
`<MapName>_Scenarios_Script.lua`.

> ⚠️ **This ticket produces no in-game result on its own.** Its output only reaches the game
> through the Map Scenario export pipeline — **STEP70/STEP71 are authored but NOT implemented**.
> Until they ship, the positions computed here can only be consumed by a test binary. That is a
> sequencing fact, not a reason to change this ticket; it is the reason the live
> `Pandemonium Isthmus_Scenarios_Script.lua` still carries hand-authored constants.

## 0. Why this exists

The live scenario script finds water in Lua with a hand-rolled spiral probe
(`FindNearbyWaterSpot`) around four hardcoded pond coordinates. That routine **cannot fail
loudly** — when no water is found inside its budget it returns the original, possibly-dry point,
so `CreateUnit` silently returns falsy and the ship never appears. Per the human: location-finding
belongs in SanGen, computed with the project's own DOD/SIMD math, not re-derived per map by hand.

## 1. Reuse, do not reinvent — the existing hardware-optimized primitive

`src/math/JumpFloodDistanceField_MATH.h` already provides
`SanmapGen::Math::ComputeJumpFloodDistanceField(heightField, width, height, minHeight, maxHeight,
gradientTolerance, maxDistance, outDistance)` — O(w·h·log max(w,h)), pointer-swap ping-pong, JFA+2
passes for exactness. It seeds every cell **outside** `[minHeight, maxHeight]`.

**Call it with `minHeight = -FLT_MAX`, `maxHeight = waterLevel`, `gradientTolerance` disabled.**
Every land cell becomes a seed, so `outDistance[i]` is, for each submerged cell, the **Euclidean
distance to the nearest shoreline**. That single existing call supplies:
- water-body interior depth (the anchor is the local maximum of this field), and
- the clearance guarantee that no ship in a formation is placed on or over the shore.

**Do not write a new distance transform, a new spiral search, or a per-cell `sqrt` loop.**

## 2. Unit A — `src/math/WaterBodyLabel_MATH.h`

Header-only, `namespace SanmapGen { namespace Math {`. Pure, stateless, raw arrays only — no
`FloatMask`, no PARAMS, no logging, no allocation inside the hot loop beyond the two scratch
buffers named below.

```cpp
struct WaterBodyRecord {
    int   labelIdentifier;      // 1-based; 0 in outLabel means "land"
    int   cellCount;            // submerged cells in this body
    float anchorX, anchorY;     // cell coords of the max-shore-distance cell (deepest interior)
    float anchorShoreDistance;  // outDistance at the anchor — the body's inscribed radius
    int   minX, minY, maxX, maxY;   // inclusive bounding box, for cheap rejection
};

// Labels 4-connected runs of cells whose height < waterLevel. Returns the number of bodies
// written to outBodies. outLabel must hold width*height ints.
inline int LabelWaterBodies(const float* heightField, const float* shoreDistance,
                            int width, int height, float waterLevel,
                            int minimumCellCount,
                            int* outLabel, std::vector<WaterBodyRecord>& outBodies);
```

**Algorithm — two row-major passes plus a union-find resolve.** This is the standard
connected-component-labelling shape and is the right one here: a single linear sweep over a
cache-coherent row-major array, which is what the DOD layout exists to make fast.

1. **Pass 1 (row-major, branch-light).** For each cell, `bool bSubmerged = heightField[i] < waterLevel;`
   Look only at the **west** and **north** neighbours (already labelled). No neighbour ⇒ mint a new
   provisional label. One neighbour ⇒ adopt it. Two different ⇒ adopt the smaller and union the pair.
2. **Resolve.** Path-compressed union-find over the provisional labels (`std::vector<int> parent`,
   `Find` with halving). Compact survivors into dense 1-based identifiers.
3. **Pass 2 (row-major).** Write the final label into `outLabel`, and accumulate per-body
   `cellCount`, bounding box, and `anchorShoreDistance = max(shoreDistance)` with its cell —
   accumulate into a `std::vector<WaterBodyRecord>` indexed by dense label, **not a map**.
4. **Reject** bodies with `cellCount < minimumCellCount` (caller passes the area needed to hold a
   formation): drop the record and zero their cells in `outLabel`, so a caller can trust
   `outLabel[i] != 0` to mean "usable water".
5. Sort `outBodies` by `cellCount` descending before returning, so index 0 is the largest body.

Determinism: labels derive from row-major scan order only — no floating-point comparison beyond
`< waterLevel`, no hashing, no parallel reduction. Identical input ⇒ identical output on any machine.

Companion `src/math/WaterBodyLabel_MATH_Test.cpp` (the project's usual bare-`main` test binary):
- a single 8×8 pond ⇒ 1 body, correct `cellCount`, anchor at the centre;
- two ponds separated by one land column ⇒ 2 bodies, **never merged** (the west/north-only
  neighbour rule is the thing being tested);
- a U-shaped body that forces a union on pass 1 ⇒ 1 body, not 2;
- an all-land field ⇒ 0 bodies;
- `minimumCellCount` above the smaller pond's size ⇒ that pond dropped **and its `outLabel` cells zeroed**.

## 3. Unit B — `src/proc/NavalFleetPlacement_PROC.h` / `.cpp`

```cpp
namespace SanmapGen { namespace Proc {

struct NavalFleetPlacementInput {
    const float* heightField;
    const float* shoreDistance;      // from ComputeJumpFloodDistanceField, §1
    const int*   waterBodyLabel;     // from LabelWaterBodies
    const std::vector<Math::WaterBodyRecord>* waterBodies;
    int   width, height;
    float waterLevel;
    int   armyCount;                 // armies needing a fleet
    int   shipsPerArmy;
    float shipFootprintRadius;       // half the longer footprint axis + gap
    float minimumPairSeparation;     // world units between fleets of DIFFERENT bodies
};

struct NavalShipPlacement { int armyIndex; float x, y, z; };

struct NavalFleetPlacementResult {
    bool bSucceeded = false;
    std::vector<NavalShipPlacement> ships;
    int  unplacedShipCount = 0;      // NEVER silently zero — see §4
    std::vector<int> armiesWithoutWater;
    std::vector<int> pairsBelowMinimumSeparation;   // pair index, when no eligible body remained
};

NavalFleetPlacementResult ComputeNavalFleetPlacement(const NavalFleetPlacementInput& input);
}}
```

**Policy, per the human, 2026-08-28 — do not elaborate beyond this:**
- **Two armies per water body**, seated as an **opposing pair on the same side** of that body.
- Pair armies in input order: `(0,1)`, `(2,3)`, … Each pair takes the next-largest body from
  `waterBodies` (already sorted descending) that can hold both formations.
- Split the pair across the body's **longest bounding-box axis**: army A's formation centred at
  25 % along that axis, army B's at 75 %, both snapped to the nearest cell with
  `shoreDistance >= formationInscribedRadius`.
- A formation is a square grid, `ceil(sqrt(shipsPerArmy))` columns, spacing
  `2 * shipFootprintRadius`. `formationInscribedRadius` = half the grid diagonal.
- If two pairs land in bodies closer than `minimumPairSeparation`, move the later pair to the next
  eligible body. If none exists, place it anyway and **record it in `pairsBelowMinimumSeparation`** —
  do not silently accept it.
- `y` for every ship is `waterLevel` (naval hulls float at the surface), not the sampled terrain height.

**Every ship position is re-validated before it is emitted:** `waterBodyLabel[cell] != 0` **and**
`shoreDistance[cell] >= shipFootprintRadius`. A point failing either is **not** nudged by a spiral
probe — it is dropped and counted in `unplacedShipCount`.

## 4. The rule this ticket exists to enforce

**Never substitute a silently-wrong position for a failure.** The Lua spiral probe's
`return idealX, idealZ` fallback is exactly the defect being replaced. A position that cannot be
validated must surface as `unplacedShipCount` / `armiesWithoutWater` /
`pairsBelowMinimumSeparation` / `bSucceeded = false` and be logged by the caller — never as a
coordinate the caller believes is good.

## 5. Constants the caller supplies (measured, not guessed)

From the live `.santp` templates, read 2026-08-28 by the Unit/Strategist Expert:

| Value | `ucn3001` Chosen T3 Battleship |
|---|---|
| Max weapon range | **140** world units (3 turrets, all 140) |
| Vision radius | 20 (⇒ **gun range, not vision, is the binding constraint**) |
| Footprint (x, y) | 2.5 × 9.5 — `skirtSize` is **absent** on this template |
| Movement speed | 3.2 |

- `shipFootprintRadius` = `9.5 / 2` + `1.5` gap = **6.25**.
- `minimumPairSeparation` = **200** — the 140 range floor plus margin for formation extent on both
  sides (`separation >= 140 + 2 * formationInscribedRadius`).
- 140 is the **highest range of any naval hull in the game** (EDA `uen3001` and Guard `ugn3001`
  battleships are 100; T2 cruisers 55), so 200 is a true worst case for naval-vs-naval.

⚠️ Two caveats recorded, neither blocking: `ucn3001` is `false / OK_PENDING_APPROVAL` in
`common/units/availableUnits.lua` — but that table is read only by AI code and the build menu, **not
by `CreateUnit`**, so a scripted spawn is not gated by it. And its Turret03 carries
`layerWeaponLimits = {"Land"}` while the hull is `WaterSurface`, a known data bug that likely costs
about a third of nominal DPS; it does not change the 140 figure.

## 6. Out of scope

Air/land placement, formation facing/rotation, the `.sanmap` schema, the Lua rendering (STEP70),
and any change to the live `Pandemonium Isthmus_Scenarios_Script.lua`.
