# Shatter Feature — Dev Brief

**What we want:** many terrain-shatter zones per match, randomly placed and rotated, each firing when enough army weight sits on it. Terrain falls away leaving a hole that blocks pathing and building.

**What this brief is:** what already works, one test to run before anything else, three engine asks (each with a no-engine-change workaround), and two prerequisites that will bite on day one.

All line references are against `engine/LJ/lua`.

---

## 1. Run this test first — it may delete an engine ask

**U1: do navmap / grid modifiers honour their prefab instance's rotation?**

Navmap modifiers are prefab **child entities** resolved via `Engine.GetGlobalBone(rootGlobalID, entityName)` (`common/navmapModifiers.lua:12`), and `Engine.InstantiatePrefab` takes a full quaternion. But `NavmapModifierTemplate` is `{entityName, disabled, size: float2, layerIndex}` (`engineClasses.lua:224-228`) — a `float2` extent with no orientation field — and **nothing in the codebase has ever instantiated one at a non-identity rotation**. `playableAreaBarrier.lua:47` passes `EngineClasses.quaternion()`; the loader passes `GetIdentityQuaternion()`.

So nobody knows whether the modifier's footprint follows the instance transform or is an axis-aligned box at the instance position.

**The test, ~10 minutes:**

1. Instantiate one `PlayableAreaBarrier` prefab with a 45° quaternion and a `40 × 5` size.
2. `Engine.NavmapLineCast` across both diagonals of its bounding square.

| Result | Meaning |
|---|---|
| Blocked region is a **40×5 rotated slab** | Rotation works today. **Engine ask #3 disappears.** |
| Blocked region is a **40×5 axis box**, or its **31.8×31.8 AABB** | Rotation needs either tiling or an engine change. |

**Please run this before scoping anything else in section 3.**

---

## 2. What already works — don't rebuild it

| Capability | API | Notes |
|---|---|---|
| **Visual hole: create / destroy** | `Engine.AddHole` / `RemoveHole` | `client/entities/hole.lua`. **Client-side only** — see ask #2. |
| **Visual hole: scale** | `Engine.SetHoleSize` | runtime |
| **Visual hole: rotate** | `Engine.SetHoleRotation` | **radians**, `HoleTemplate.rotation`, `engineClasses.lua:358`. Works today. |
| **Visual hole: circle mode** | `Engine.SetHoleCircle` | see §6 |
| **Pathing block: scale** | `Engine.SetNavmapModifierSize(id, float2)` | `engineFunctions.lua:561` |
| **Pathing block: toggle** | `Engine.SetNavmapModifierEnabled(id, bool)` | `:555`. `common/playableAreaBarrier.lua` is the working recipe. |
| **Build block: scale** | `Engine.SetGlobalGridModifierSize(id, float2)` | `:1105` |
| **Build block: arbitrary cells** | `Engine.SetGlobalGridCellBaseFlags(gid, min, max, flags, on)` | `:1083`. See §3.3 — this one is better than it looks. |
| **Trigger volume: scale / offset** | `SetVolumeColliderSize:138`, `SetVolumeColliderOffset:125` | |
| **Spatial enter/exit events** | `Engine.GetVolumeCollisionEventsForWorld(worldID)` | `constructionManager.lua:5`. Engine does the broadphase. |
| **Resource spots in a zone** | `ResourceSpotUtils.resourceSpots` registry + `SetEnabled` | reuse `hostPlayableAreaManager.lua:24-29`'s pattern |

**Scale is already solved on every layer.** The only gap is rotation, and only on one layer.

---

## 3. The three engine asks

### 3.1 Ask #1 — a heightmap setter *(or confirm we don't need one)*

There is no way to modify terrain height from lua. `engineFunctions.lua` exposes readers only: `GetTerrainHeightmapSize:1310`, `SampleTerrainHeightFromCell:1331`, `SampleTerrainHeight:1337`, `SampleTerrainNormals:1346`, `SampleTerrainNormalFromCell:1355`. Grepping the FFI surface for `SetTerrain|SetHeightmap|ModifyTerrain` returns only `SetMovementTerrainAlignmentFactor`, a per-unit visual tilt.

**Question before you build anything:** does the shatter prototype actually deform terrain, or is it a render effect over unchanged geometry? If it's the latter — which we assume — **this ask goes away entirely**, because pathing and building are blocked by modifiers and grid flags, not by geometry. Terrain height only matters if you want units to visibly fall or projectiles to pass through.

**Workaround if there's no setter:** none needed. Cosmetic hole + navmap modifier + grid flags is functionally complete.

### 3.2 Ask #2 — `Engine.AddHole` on the host binding *(probably not needed)*

`AddHole` / `RemoveHole` / `SetHoleSize` / `SetHoleRotation` / `SetHoleCircle` / `SetHoleEnabled` are all in `client/entities/hole.lua`. Grepping the **host** FFI surface for `hole` returns only `HoleTemplate` / `HolesTemplate` marshalling inside `CreatePrefab`. Holes attach to a `localId` and change **rendering only** — nothing in `Hole:*` touches navmap, grid, or collision.

**Workaround, and probably the right design anyway:** the host never needs to create a hole. Host decides, host applies the sim effects, host broadcasts:

```lua
SendToAllClients({ zone = i, x = x, z = z, size = s, rot = r }, "ShatterFired")
```

Each client calls `CreateHole(...)` locally. This is how the rest of the codebase already works — the client renders host-pushed state. **Only ask for a host binding if the sim genuinely needs hole geometry**, which we don't think it does.

### 3.3 Ask #3 — rotation on sim-side extents *(gated on U1)*

Neither `NavmapModifierTemplate` nor `GridModifierTemplate` carries an orientation. `GridModifierTemplate` size is `int2` (`engineClasses.lua:230-237`), which is structurally an AABB. And there is **no `SetVolumeColliderRotation`**.

**If U1 says rotation isn't honoured, the ask is:**

```
SetNavmapModifierRotation(globalID, radians)   -- + a rotation field on NavmapModifierTemplate
```

**Single axis (Y) only** — every one of these is an XZ-plane extent, so one float is sufficient. The work is turning an AABB into an OBB in whatever rasterizes the navmesh footprint, which must already walk cells. We'd expect that to be small; you'd know better.

**We do NOT need rotation setters on the grid modifier or the volume collider.** Both have free workarounds — §3.4 and §3.5.

**Workaround if the ask is declined:** tile the rotated shape with N axis-aligned navmap modifiers in a staircase. Costs N prefab instances per hole instead of 1. Bounded, shippable, ugly.

### 3.4 Build-block rotation — free, no engine change

`common/systems/templateLoader.lua:127` creates the grid with `Engine.CreateGlobalGrid(1)` — **cellSize 1, so grid coordinates equal world coordinates.** And `SetGlobalGridCellBaseFlags` takes a `(min, max)` range.

So rasterize whatever shape you like in lua and call it once per scanline row:

```lua
-- rotated rect, ellipse, irregular polygon -- doesn't matter, it's a scanline fill
for z = z0, z1 do
    local xa, xb = ShapeRowSpan(shape, z)
    if xa then
        Engine.SetGlobalGridCellBaseFlags(gid, int2(xa, z), int2(xb, z),
                                          PlacementLayer.InvalidTerrain, true)
    end
end
```

A 40×20 rotated hole is ~20–40 calls, **once, at fire time.** Arbitrary shape, no engine change.

> **Replication caveat:** the host runs `InitializePlacementGrid` at `script.lua:157` (before `RunMapSetup`); the client runs it at `:206` (**after**). Base flags and modifier flags are separate layers so it converges, but **shatter re-flagging must be replicated as an explicit message** — the client will not recompute it. Note `script.lua:118` already carries `--TODO: This might not be deterministic`, an admission that host and client may not agree on base flags at all.

### 3.5 Trigger-volume rotation — free, no engine change

Use **one axis-aligned collider** sized to the AABB of the rotated shape as the cheap broadphase gate, then do the exact rotated-rect / ellipse / polygon point test **in lua on enter**. The event already carries the unit, so it's two multiplies and a compare per boundary crossing. Exact result, engine still does all the spatial work.

---

## 4. Two prerequisites that will bite immediately

### 4.1 Map-prop GlobalIDs are thrown away

`common/mapUtils.lua:95-99`:

```lua
for _, transformData in pairs(propData.transforms) do
    local instantitatedPropID = {}          -- declared INSIDE the loop
    Engine.InstantiatePrefab(propPrefabID, transformData.position,
                             transformData.scale, rotation, instantitatedPropID)
end                                          -- ...and discarded
```

There is **no registry of instantiated map-prop GlobalIDs anywhere in the tree**. You cannot enumerate, query, or delete the props inside a shatter zone. On a map like Two_Step_Shuffle that's ~63k prop instances that will keep standing over a hole.

**Fix:** collect the ids into a spatial bucket keyed by grid cell at instantiation. Cheap, and it's a prerequisite for anything that needs to touch map props — not just shatter.

### 4.2 The AI's path map is never invalidated

`AI/AIMarkerGenerator.lua:151-202` `BuildTerrainPathMap()` builds `PathMap[x][z]` per cell over the playable area from `SampleTerrainHeight` and `IsPathable`. On a 2048² area that's ~4.2M lua tables, built once, spread over frames. **There is no invalidation entry point** — `ClearMemoryMarkerGenerator` nils the whole map and nothing rebuilds it.

After a shatter, the AI believes the hole is walkable land, permanently, and will path units into it forever.

This is separate from and much larger than the `PlayableAreaCacheDuration = 10` cache in `AIFunctions.lua:4317`. Note the AI is currently gated off at `AIInit.lua:207` (`if 1 == 1 then return end`), so this is a **time bomb** rather than a live bug — it detonates on whoever deletes that line, not on whoever writes the shatter.

**Fix:** an incremental `InvalidatePathMapRegion(min, max)` that re-derives only the affected cells, called from the same place that applies the grid flags.

---

## 5. The trigger — settled decisions

### 5.1 Weight formula

**`weight = unitCount × Σ(economy.cost.alloys)`, commanders excluded.**

Rationale: there is **no usable mass field** in the game. `mass` appears only at `templateExplainations.lua:421` inside a block headed *"Old format, still have some leftover stuff"*, and twice in `UnitBlueprintValidator.lua` as a non-required field. **Zero shipped templates define it**, and it lives under `movement`, which structures don't have at all.

`economy.cost.alloys` is present on 100% of templates including structures, and is already the reclaim basis (`templateLoader.lua:413`).

Excluding commanders is necessary because of the range — a commander is 100,000 alloys, i.e. **3,333 T1 tanks**:

| scenario | weight |
|---|---|
| 20 T1 tanks | 12,000 |
| 30 T3 | 288,000 |
| 100 T1 tanks | 300,000 |
| ~~commander + 10 tanks~~ | ~~1,103,300~~ ← why commanders are excluded |

Detect with `unit:HasTags(Tags.COMMAND)` — a single table index (`unitsBaseClass.lua:943`). Equivalently, tpId char 4 == `'0'`.

The `× unitCount` term is deliberate: it makes numbers matter, so a large cheap army outweighs a small expensive one. Without it a plain sum leaves high-tech blobs dominating.

### 5.2 The accumulator — key by collider id, not by unit

Enter/exit alone drifts, and the codebase already proves it. `host/managers/colliders/constructionDetector.lua:29-32`:

```lua
function ConstructionDetector:OnColliderExit(volumeCollisionEvent)
    if not Engine.IsValidGlobalID(volumeCollisionEvent.colliderGlobalID) then
        -- We can receive OnColliderExit when the entity gets destroyed
        return
```

The engine **does** emit an exit when an entity dies inside a volume — but the id is already invalid, so you can't resolve it back to a unit to subtract its weight. The shipped code's `return` leaves a stale entry: **a live membership leak in production today**, on exactly the pattern we'd be building on.

**Don't resolve the root at exit time. Key by collider id:**

```lua
-- on enter
inside[colliderGlobalID.index] = { root = rootID, w = weight }
total = total + weight

-- on exit -- no engine call, works fine on an already-destroyed entity
local e = inside[idx]
if e then total = total - e.w; inside[idx] = nil end
```

This also fixes the shipped `constructionDetector` leak.

### 5.3 Rising-edge verification instead of a periodic sweep

Remaining drift sources: unit built inside (U2), captured (no event today), upgraded (`unitsBaseClass.lua:2835`), transported (cargo weight vanishes), volume resized (U3), and the shatter itself deleting units in the zone.

You don't need a periodic reconciliation. You only care about the moment `total ≥ T`:

1. Run the accumulator continuously as the **cheap gate**.
2. When it first crosses `T`, **verify once** with an exact sweep before firing — a linear pass over armies' unit lists with a point-in-shape test.
3. If the sweep disagrees, reset the accumulator to the swept value and log.

At ~500 units × 8 armies that's ~4 000 `GetPosition` calls — **sub-millisecond, once per candidate firing, not per tick.** And the disagreement log is free drift telemetry.

### 5.4 Two behaviours to run past U2 / U3

- **U2:** does a unit **spawned inside** an existing volume generate a `VolumeEnter`, or only a boundary crossing? *Test: `CreateUnit` inside a live trigger volume, log `GetVolumeCollisionEventsForWorld`.*
- **U3:** does `SetVolumeColliderSize` re-broadphase and emit deltas for newly-enclosed units? *Test: resize a volume over stationary units, log the event stream.*

---

## 6. Open question back to you: what shape is the prototype?

`Engine.SetHoleCircle` exists. If shatter zones are **circles or ellipses**, sim-side rotation is meaningless — you'd only rotate the visual texture, which already works, and **engine ask #3 disappears regardless of the U1 result.**

If it's a **rotated rectangle**, ask #3 stands (pending U1).

If it's an **irregular polygon**, rotation is probably the wrong parameter entirely — vary the shape from a seed instead, and the scanline approach in §3.4 handles the build grid for free.

**Please tell us what the prototype actually produces.** It changes whether we ask you for anything at all.

---

## 7. Random placement — no engine involvement

For completeness, so nobody worries about determinism: the host rolls zone placement from a seeded PRNG, instantiates the trigger volumes, and broadcasts. The client never rolls.

| when | who | what |
|---|---|---|
| map populate | host | `rng = SeededRng(hash(mapId, options, matchId))`; roll N zones; reject overlaps with spawn markers |
| build | host | instantiate N trigger volumes from **one** shared `triggerVolume` prefab |
| build | host → all | `SendToAllClients(zones, "ShatterZones")`, ~30 B/zone |
| on fire | host | grid flags, navmap modifier, destroy units; `SendToAllClients({zone=i}, "ShatterFired")` |
| on receive | client | `CreateHole(...)` — the cosmetic |

**The parity rule this respects:** prefab **creation** order must match host and client (`resourceSpotTemplateLoader.lua:10-13`). Prefab **instantiation** has no such constraint — `mapUtils.lua:98` already instantiates props host-only and nothing breaks. So create the shatter prefab unconditionally on both sides, instantiate N times on the host with random transforms.

---

## Summary — what we're asking for

| # | Ask | Blocked on | Workaround if declined |
|---|---|---|---|
| **0** | **Run the U1 rotation test** | — | *(this is the request)* |
| 1 | Heightmap setter | Does the prototype deform terrain? | Not needed if it's a render effect |
| 2 | Host-side `AddHole` | Does the sim need hole geometry? | Broadcast + client-side `CreateHole` |
| 3 | `SetNavmapModifierRotation(id, radians)` | U1 result **and** hole shape (§6) | Tile with N axis-aligned modifiers |
| 4 | *(us, not you)* prop GlobalID registry | — | — |
| 5 | *(us, not you)* AI path-map invalidation | — | — |

**Realistic outcome:** if U1 passes, or if the holes are circles, **we need nothing from the engine team.** Everything else is lua.
