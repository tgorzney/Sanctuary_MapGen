# NavMesh Modifiers: making anything block movement, layer by layer

**What this is:** a per-map Lua technique for blocking unit pathing — for one mobility type or all of them — anywhere on the map, independent of terrain height or slope. Use it for a decorative prop that should be solid (a floating spire, an overhanging cliff), a body of water that should stop naval units without stopping land traffic, or any other footprint the terrain itself won't naturally block.

Everything below is taken from a live, working implementation — `Pandemonium Isthmus`'s air-blocker (all six mobility layers) and sea-blocker (Sea layer only), both confirmed in-game — and from engine source in `engine/LJ/lua/`. Where something is designed but not yet independently re-verified it is marked ⚠️.

---

## Contents

1. [Minimal working example](#1-minimal-working-example)
2. [What a Navmap Modifier actually is](#2-what-a-navmap-modifier-actually-is)
3. [All-layer blockers: reuse the engine's own barrier prefab](#3-all-layer-blockers-reuse-the-engines-own-barrier-prefab)
4. [Single/partial-layer blockers: you need your own prefab](#4-singlepartial-layer-blockers-you-need-your-own-prefab)
5. [Host vs client, and the state nuance that bites everyone once](#5-host-vs-client-and-the-state-nuance-that-bites-everyone-once)
6. [Timing: where this goes in your NewThread, and why](#6-timing-where-this-goes-in-your-newthread-and-why)
7. [Authoring the blocked area from a mask](#7-authoring-the-blocked-area-from-a-mask)
8. [Pixel-to-world coordinates](#8-pixel-to-world-coordinates)
9. [Worked example: a Sea-only blocker end to end](#9-worked-example-a-sea-only-blocker-end-to-end)
10. [Troubleshooting by symptom](#10-troubleshooting-by-symptom)
11. [Quick reference](#11-quick-reference)

---

## 1. Minimal working example

This drops into an existing map's `<MapName>_data.lua` (see `TUTORIAL_Scenarios.md` if you don't have the `NewThread`/`pcall` scaffolding yet — this technique lives inside the exact same file and the exact same single `NewThread`).

[CODE]
```lua
local NavmapModifiers = Import("common/navmapModifiers.lua")

-- One rectangle. Position is world x/z, centered. Size is world-space width/depth.
-- No rotation -- see §2.
local BLOCKER_RECTS = {
    { x = 1024, z = 1024, sizeX = 40, sizeZ = 40 },
}

local function SpawnAllLayerBlockers()
    for _, rect in ipairs(BLOCKER_RECTS) do
        local size = EngineClasses.float2(rect.sizeX, rect.sizeZ)
        local errorCode, id = Engine.InstantiatePrefab(
            _G.PlayableAreaBarrierPrefabID,
            EngineClasses.float3(rect.x, 0, rect.z),
            EngineClasses.float3(1, 1, 1),
            EngineClasses.quaternion()
        )

        if errorCode ~= EngineErrorCode.Success then
            Warn("SANGEN: blocker prefab instantiate failed, errorCode="..tostring(errorCode))
        elseif IsClient then
            Engine.SetLocalGridModifierSize(id, size)
            Engine.SetLocalGridModifierEnabled(id, true)
        else
            local modifierIDs = NavmapModifiers.GetNavmapModifierIDs(id, _G.PlayableAreaBarrierLayers)
            NavmapModifiers.SetNavmapModifiersSize(modifierIDs, size)
            NavmapModifiers.SetNavmapModifiersEnabled(modifierIDs, true)
            Engine.SetGlobalGridModifierSize(id, size)
            Engine.SetGlobalGridModifierEnabled(id, true)
        end
    end
end
```
[/CODE]

Call `SpawnAllLayerBlockers()` from your map's single `NewThread`, in the right place (§6). That's a complete, working, all-mobility-type blocker — air, land, sea, amphibious, hover, submarine, all at once, zero new prefabs, zero engine changes. Everything after this is either detail, or the harder single-layer case.

---

## 2. What a Navmap Modifier actually is

Not a Unity concept, not anything documented under that name anywhere obvious — the engine's native term is **Navmap Modifier**, and the real source is `common/navmapModifiers.lua` + `common/loading/navmapModifierLoader.lua` + `common/navigationLayers.lua`.

**Navigation layers are per-map, created at load.** `common/navigationLayers.lua`'s `CreateNavigationLayers()` bakes one native layer per mobility type from the terrain's own height/slope, at map load:

| Layer | Always created? | Height band | Max slope |
|---|---|---|---|
| `Land` | yes | sea level to +∞ | 30° |
| `Amphibious` | yes | -∞ to +∞ | 30° |
| `Hover` | yes | -∞ to +∞ | 30° above water, ∞ below |
| `Air` | yes | -∞ to +∞ | ∞ (unrestricted) |
| `Submarine` | only if `Engine.HasWater()` | -∞ to (waterLevel - 1.5) | ∞ |
| `Sea` | only if `Engine.HasWater()` | -∞ to waterLevel | ∞ |

Two things fall straight out of that table and matter a lot in practice:

- **`Air`'s band is `-∞..+∞`, slope `∞`.** Air is navigable *everywhere* on the map by default — terrain height and slope never restrict it. If you're trying to make air units respect a tall decorative feature, the terrain itself will never do that for you no matter how you sculpt the heightmap. You need an explicit modifier.
- **A waterless map has no `Sea`/`Submarine` layer at all.** Any code that tries to block one on such a map isn't wrong, it's just a no-op (see §4's note on this).

**A Navmap Modifier is one axis-aligned rectangle blocking one layer.** `NavmapModifierTemplate` (`host/generated/doc/engineClasses.lua`) has exactly three fields: `entityName`, `disabled`, `size` — a `float2`, world-space width/depth, centered on wherever the owning prefab *instance* is placed. **There is no rotation field anywhere in this template or its two real call sites.** A diagonal or rotated real-world feature has to be staircase-approximated with multiple axis-aligned boxes (§7 covers doing that well).

**The set of layers a prefab can block is fixed forever at prefab-*template*-creation time, not per-instance.** `common/loading/navmapModifierLoader.lua`'s `AddNavmapModifierTemplates(parentEntityName, layerNames, size, disabled, hierarchyEntities, navmapModifierTemplates)` appends one child entity **per layer name**, each carrying its own `NavmapModifierTemplate`. Call it with `{"Land", "Sea"}` when you build the prefab template, and every instance of that prefab has exactly those two modifier children — always. You can resize or enable/disable the ones that exist, per instance, but you cannot add a layer to an instance that wasn't in the original list.

**Two real prefabs already do this, and they're both worth knowing about before you write a third:**

- **Building structures**, via a unit template's `skirtSize` field (`common/loading/unitTemplateLoader.lua:498`). Blocks `Layers.GetStructureNavigationLayerNames()` — every layer **except `Air`** (`common/layers.lua`'s own comment: *"Structures do not block Air"*). Starts `disabled = true`, toggled on construction progress (`unitsBaseClass.lua`'s `SetNavmapModifiersEnabled`).
- **The engine's own map-edge barrier** (`common/loading/playableAreaBarrierLoader.lua`'s `CreatePlayableAreaBarrierPrefab`, invoked once per map by `common/systems/templateLoader.lua`, on **both host and client, before any per-map script runs**, storing the result in the globals `_G.PlayableAreaBarrierPrefabID` and `_G.PlayableAreaBarrierLayers`). Blocks `Layers.GetAllNavigationLayerNames()` — literally every layer the map has, **including `Air`**. This is what walls units out of the map beyond the playable area.

The rest of this tutorial is about which of those two patterns to follow.

---

## 3. All-layer blockers: reuse the engine's own barrier prefab

If you want to block **every** mobility layer at once, don't build a new prefab — reuse `_G.PlayableAreaBarrierPrefabID`. It already exists, on both host and client, for every map, before your `_data.lua` even runs. You're just instantiating more copies of it at your own positions, exactly the way `common/playableAreaBarrier.lua`'s own `CreateBarrier`/`SetBarrierSize`/`SetBarrierEnabled` already do for the four map-edge slabs:

[CODE]
```lua
local errorCode, id = Engine.InstantiatePrefab(
    _G.PlayableAreaBarrierPrefabID,
    position,                              -- EngineClasses.float3(x, 0, z) -- Y is unused, see below
    EngineClasses.float3(1, 1, 1),
    EngineClasses.quaternion()
)
```
[/CODE]

Then, on the host, resolve the modifier bones and turn them on:

[CODE]
```lua
local modifierIDs = NavmapModifiers.GetNavmapModifierIDs(id, _G.PlayableAreaBarrierLayers)
NavmapModifiers.SetNavmapModifiersSize(modifierIDs, EngineClasses.float2(sizeX, sizeZ))
NavmapModifiers.SetNavmapModifiersEnabled(modifierIDs, true)
```
[/CODE]

**Terrain height doesn't matter, and neither does the modifier's own Y.** `common/playableAreaBarrier.lua`'s `GetBarrierSlabs` always positions its barriers at `y = 0`, regardless of the actual terrain height under them, and it works everywhere on the map. A `NavmapModifierTemplate` blocks its footprint independent of local terrain height/slope — the height/slope bands in the table in §2 only gate the *terrain-driven bake* at map load, not an explicit modifier placed afterward. If you're blocking movement under something whose visual model doesn't match the terrain mesh (a floating rock, a prop with no matching heightmap bump), that mismatch is simply irrelevant to this mechanism. Use `y = 0` and move on.

**Why this specifically enables blocking `Air`, when nothing else on the map does.** Look back at the structure/`skirtSize` pattern in §2 — it explicitly excludes `Air`. If you reused a unit template's skirt mechanism, you could never block air traffic. `PlayableAreaBarrierLoader`'s `Layers.GetAllNavigationLayerNames()` call has no such exclusion. Reusing *this specific* prefab — not a unit's `skirtSize` — is what makes air blocking possible at all, with zero engine or `mapUtils.lua` changes.

---

## 4. Single/partial-layer blockers: you need your own prefab

Say you only want to block `Sea` — naval units shouldn't cross a certain strip, but land, air, and everything else should pass through freely. **You cannot get there by reusing `_G.PlayableAreaBarrierPrefabID` and only touching the `Sea` bone.**

Here's why. `PlayableAreaBarrierPrefabID`'s template was built once, back in `common/systems/templateLoader.lua`, with **all six** layers baked in:

[CODE]
```lua
-- CreatePlayableAreaBarrierPrefab, common/loading/playableAreaBarrierLoader.lua
NavmapModifierLoader.AddNavmapModifierTemplates(
    TemplateHeadEntityName,
    Layers.GetAllNavigationLayerNames(),   -- ALL SIX, forever, for every instance
    EngineClasses.float2(1, 1),            -- placeholder size
    false,                                 -- NOT disabled by default
    prefabTemplate.hierarchyTemplate.entities,
    prefabTemplate.globalTemplate.navmapModifierTemplates
)
```
[/CODE]

Every instance of this prefab carries six modifier children, one per layer, **whether you touch them or not**. If you instantiate it and call `NavmapModifiers.GetNavmapModifierIDs(id, {"Sea"})`, you get back only the `Sea` bone — but the other five (`Land`, `Air`, `Hover`, `Amphibious`, `Submarine`) still exist on that same instance, sitting at their template-creation defaults: **`disabled = false`** and **`size = (1, 1)`**. You'd silently drop a real, live, 1×1-world-unit blocker on every other mobility type at every single instance position. Small, easy to miss, and definitely a bug.

**The fix is a prefab of your own**, built the same way, but passing only the layer(s) you actually want:

[CODE]
```lua
local NavmapModifierLoader = Import("common/loading/navmapModifierLoader.lua")

-- Mirrors CreatePlayableAreaBarrierPrefab (common/loading/playableAreaBarrierLoader.lua)
-- exactly, except the layer list. Do NOT reuse PlayableAreaBarrierPrefabID for a
-- partial-layer blocker -- see the explanation above.
local function CreateSingleLayerBlockerPrefab(prefabName, layerNames)
    local prefabTemplate = EngineClasses.PrefabTemplate(prefabName)

    local headEntity = EngineClasses.EntityTemplate()
    headEntity.entityName = TemplateHeadEntityName
    headEntity.localTranslation = EngineClasses.float3(0, 0, 0)
    headEntity.localScale = 1.0
    headEntity.localRotation = EngineClasses.quaternion()
    headEntity.forceSimulated = false
    table.insert(prefabTemplate.hierarchyTemplate.entities, headEntity)

    NavmapModifierLoader.AddNavmapModifierTemplates(
        TemplateHeadEntityName,
        layerNames,                            -- e.g. { "Sea" } -- only what you asked for
        EngineClasses.float2(1, 1),
        false,
        prefabTemplate.hierarchyTemplate.entities,
        prefabTemplate.globalTemplate.navmapModifierTemplates
    )

    local errorCode, prefabID = Engine.CreatePrefab(prefabTemplate)
    if errorCode ~= EngineErrorCode.Success then
        Error("CreateSingleLayerBlockerPrefab: Failed to create prefab '"..prefabName.."'", 2)
    end
    return prefabID
end

local SeaBlockerPrefabID = CreateSingleLayerBlockerPrefab("SeaBlocker", { "Sea" })
```
[/CODE]

Then instantiate and configure it exactly like §3, just passing your own layer list to `GetNavmapModifierIDs` instead of `_G.PlayableAreaBarrierLayers`:

[CODE]
```lua
local modifierIDs = NavmapModifiers.GetNavmapModifierIDs(id, { "Sea" })
NavmapModifiers.SetNavmapModifiersSize(modifierIDs, size)
NavmapModifiers.SetNavmapModifiersEnabled(modifierIDs, true)
```
[/CODE]

**A prefab name (the string passed to `EngineClasses.PrefabTemplate(...)`) must be unique across everything registered this session** — don't reuse `"PlayableAreaBarrier"` or any other name already claimed by common engine code, and don't create the same custom prefab twice (call `CreateSingleLayerBlockerPrefab` once per distinct layer-set you need, store the resulting ID, reuse it across every instance of that blocker type).

**⚠️ Map-local vs. shared helper — a judgment call, not fixed here.** The function above works fine living directly in one map's own `_data.lua` (matching the "zero engine changes" philosophy the all-layer case already established) — that's the currently-proven, lowest-blast-radius option. If two or more maps end up needing the same partial-layer pattern, promoting this into a shared `common/loading/*.lua` helper is a reasonable next step, but it's an engine-file change and a bigger commitment than a per-map prototype. Don't reach for it until you actually have the second map that needs it.

**No layer on this map? No modifier, no crash.** `AddNavmapModifierTemplates` checks `Layers.NavigationLayers[layerName]` and, if it's `nil` (the layer was never created for this map — e.g. you asked for `"Sea"` on a waterless map), it logs a warning and skips that layer entirely rather than erroring. Harmless, but worth knowing if a blocker you authored appears to do nothing on a particular map: check whether that map actually has water first.

---

## 5. Host vs client, and the state nuance that bites everyone once

Both prefab patterns above split their per-instance setup by role:

[CODE]
```lua
if errorCode ~= EngineErrorCode.Success then
    Warn(...)
elseif IsClient then
    Engine.SetLocalGridModifierSize(id, size)
    Engine.SetLocalGridModifierEnabled(id, true)
else
    -- host: the calls that actually block simulated pathing
    local modifierIDs = NavmapModifiers.GetNavmapModifierIDs(id, layerNames)
    NavmapModifiers.SetNavmapModifiersSize(modifierIDs, size)
    NavmapModifiers.SetNavmapModifiersEnabled(modifierIDs, true)
    Engine.SetGlobalGridModifierSize(id, size)
    Engine.SetGlobalGridModifierEnabled(id, true)
end
```
[/CODE]

**The navmap modifier calls are host-only, on purpose.** The host is the authoritative simulation; pathing is decided there. The client never touches `NavmapModifiers.*` for this — it only sets its own **local building-placement grid** (`Engine.SetLocalGridModifierSize`/`Enabled`), a cosmetic/UI concern (stops the player from trying to place a building where a blocker sits), entirely separate from actual unit pathing. This exactly mirrors `common/playableAreaBarrier.lua`'s own `SetBarrierSize`/`SetBarrierEnabled`, which make the identical split.

**Here's the nuance that got mis-diagnosed twice in the same debugging session before being nailed down**, straight from `engine/LJ/script.lua`'s `init()`:

[CODE]
```lua
function init(libPath, isClient)
    if isClient then IsClient = true else IsHost = true end
```
[/CODE]

Two facts, both true, that look contradictory until you separate them:

- **Within one Lua state, `IsHost` and `IsClient` are mutually exclusive.** Exactly one gets set, ever, per state. If you're inside a function and `IsClient` is true, `IsHost` is not — full stop, for the lifetime of that state.
- **A solo or listen-server match runs the per-map chunk in TWO separate Lua states.** `InitLobby` calls `LoadMapData()` — which `Import()`s and executes your `_data.lua` — **twice**: once under `if IsHost` (`script.lua:156`), once in the `else` branch (`script.lua:189`). So your entire `_data.lua`, including the `if/elseif/else` block above, runs **once per state**, independently, each with its own `NewThread`, each closing over its own `IsHost`/`IsClient` values.

Put those together and the branching in §3/§4 is correct as written: when the host state executes it, `IsClient` is falsy there, so it takes the `else` branch and does the real work; when the client state executes it — *separately, later, in its own state* — `IsClient` is true there, so it takes the client branch. Both runs happen, each doing the right thing for its own role, on its own set of instantiated prefabs. There is no missing case and no double-instantiation bug hiding in that `elseif`.

The trap is inferring from "a solo match is both host and client" that the two flags must therefore both be true *simultaneously, in the same execution* — they're not; they're true in two different executions of the same code. Getting this backwards was the exact wrong turn taken (twice) trying to debug why a blocker "stopped working" in a solo test — the real cause both times was an unrelated ordering bug (§6), and the `IsHost`/`IsClient` branching itself was never broken.

---

## 6. Timing: where this goes in your `NewThread`, and why

This lives in the exact same file, and the exact same single `NewThread`, that `TUTORIAL_Scenarios.md` §8 already covers for the scenario system — read that section first if you haven't. Everything there about "only ONE `NewThread` per script," "errors inside it are swallowed with zero trace," and "ordering is load-bearing" applies here without modification, because it's the same thread.

Two additional ordering constraints specific to blocker prefabs, both learned from real regressions on `Pandemonium Isthmus`, both now load-bearing:

**1. Blocker instantiation must come *after* the host's `SetPlayableArea` calls, never before.** `SetPlayableArea` is called twice — a throwaway 1×1 nudge, then the real area (`TUTORIAL_Scenarios.md` §8, rule 3). A prefab instantiated while the playable area is a 1×1 rectangle is a plausible culling target. This was the original, correct order; a later edit moved blocker instantiation to the very top of the thread (to protect it from a different problem — see the next rule) and broke this constraint instead. Both constraints have to hold at once.

**2. Blocker instantiation should come early relative to anything not yet proven safe.** Errors inside the thread are swallowed silently (no exception; nothing after the throw point runs, with zero trace — F1 console and `game_logs/*.txt` both don't work in this build). If you bolt a diagnostic, an experiment, or any other not-yet-hardened code onto the same thread *before* the blocker call, and that code throws, your blocker silently never runs and looks identical to "the mechanism doesn't work" from every angle you can observe in-game.

**A third rule, learned after the first two, and more important than either:** don't rely on ordering alone to protect a blocker call from taking down something after it — **wrap the blocker call in its own `pcall`.** Ordering discipline (rule 2) reduces the *chance* something upstream fails before it; a `pcall` removes the *consequence* if anything, ever, throws inside the blocker call itself — including a bug in your own blocker code, not just something else in the thread. This is the difference between "probably fine" and "provably fine." Once every risky call in the thread is individually `pcall`-wrapped, exact ordering between them stops being fragile — the only genuine ordering constraint left is rule 1 (area before instantiation), since that one isn't about error propagation at all, it's about the area being wrong when the prefab is created.

The ordering that satisfies all three rules, confirmed live:

[CODE]
```lua
NewThread(function()
    if IsHost then
        -- 1. Playable area MUST be final before anything below instantiates prefabs.
        PlayableAreaManager.SetPlayableArea(Area.FromMapArea({ x = -1000, y = -1000, width = 1, height = 1 }))
        PlayableAreaManager.SetPlayableArea(Area.FromMapArea(chosenArea))
    end

    if IsHost and spawnsUnitsEnabled then
        local unitsOk, unitsErr = pcall(Scenario.SpawnMatchedScenarioUnits, chosenArea)
        if not unitsOk then
            Warn("SANGEN: scenario unit spawn failed, rest of map load unaffected: "..tostring(unitsErr))
        end
    end

    -- 3. Own pcall. LAST, on purpose -- not because order still matters once this is
    --    wrapped, but so nothing load-bearing can ever again depend on this call
    --    succeeding. Runs in BOTH states (§5).
    local blockerOk, blockerErr = pcall(SpawnAllLayerBlockers)
    if not blockerOk then
        Warn("SANGEN: SpawnAllLayerBlockers threw: "..tostring(blockerErr))
    end
    -- Additional blocker functions (SpawnSeaBlockerNavmapModifiers, etc.) each get their
    -- own pcall here too, for the same reason -- one blocker's bug should never be able
    -- to take another blocker, or anything else in this thread, down with it.
end)
```
[/CODE]

Comment the reasoning directly above the `NewThread` in your own file — all three rules, and why — so the next person (or the next session) adding something to this thread doesn't reintroduce a regression independently. That's exactly what happened here: two ordering fixes landed separately, each correct in isolation, each briefly breaking the other's constraint, before the actual root cause turned out to be a fourth, unrelated thing entirely (§6.1) that no amount of reordering could have fixed.

### 6.1 The ordering bugs were real, but they weren't the actual bug

⚠️ **A cautionary coda, added after the fact.** On `Pandemonium Isthmus`, the all-layer blocker appeared broken across several rounds of live debugging, and every fix described above was applied, correctly, in response to a real problem it solved. But the blocker kept failing anyway, because the actual root cause was unrelated to ordering: a `local NavmapModifiers = Import("common/navmapModifiers.lua")` line had been silently dropped by a later edit to the rectangle table (a scripted find-and-replace that overwrote a wider span of the file than intended, deleting an unrelated line that happened to sit in the overwritten region). Without it, `NavmapModifiers` resolved to a nil global, and every host-side call into it threw — invisibly, for the same reasons every other throw in this thread is invisible.

**The lesson, generalized:** when you regenerate a block of authored data (a rectangle table, a scenario list, anything) via a script rather than a manual edit, verify the edit's boundaries don't overlap a line of actual code sitting adjacent to the data — a diff review, or a tool that fails loudly on an exact-match mismatch rather than blindly overwriting a byte range, would have caught this immediately. An ordering bug and a dropped-import bug can produce the identical observable symptom (a silent throw inside this thread, nothing happens, no trace) — don't stop looking once you've found *a* plausible cause if you haven't independently confirmed it's *the* cause.

---

## 7. Authoring the blocked area from a mask

For anything beyond a single rectangle, the practical way to author the blocked footprint is a mask image, decomposed into a small set of axis-aligned rectangles. ⚠️ This is currently a manual, offline process — a script run by hand, not a SanGen feature — but it's the process that produced every real blocker shipped so far, and it's worth knowing even if you never touch the script yourself.

**1. Author the mask.** A grayscale or plain black/white image, same resolution as the map's `heightmapResolution` (2049×2049 on a 2048×2048 map), saved into the map's own `Textures/` folder. White = block this area. It doesn't need to be pure 0/255 — a softly anti-aliased brush stroke is fine.

**2. Threshold, then connected-component label.** Pick a cutoff around the middle of the mask's value range (roughly 50%), turn every pixel above it into a boolean "on," and run 8-connected labeling to find distinct blobs. Drop anything under ~20 pixels — that's antialiasing dust, not an intended feature. **Scan the whole mask, not just the region you're focused on** — a symmetric map's mask often has several mirrored feature-pairs scattered across it, and it's easy to only spot the one near the center.

**3. Decompose each blob into rectangles — exact first, then merge.** Two-phase, per blob:

- *Exact pass:* a greedy largest-rectangle-in-a-binary-matrix decomposition (repeatedly find the single largest all-true axis-aligned rectangle, mark it covered, remove it, repeat until nothing white remains). This guarantees **100% coverage of every white pixel by construction** — nothing is missed, ever, no matter how jagged the blob's edge is. It usually produces far more rectangles than you want (a jagged diagonal edge fragments into dozens of tiny slivers).
- *Merge pass:* repeatedly find the two rectangles whose combined bounding box would add the **fewest new black pixels** (compute this with a 2D integral/summed-area image over the true mask so you get an exact count, not an estimate), and merge them into that one bounding box — as long as the cost is under a threshold you pick. Merging can only ever grow coverage, so the zero-missed-white guarantee from the exact pass survives no matter how far you merge.

**The merge threshold is your one real dial**, trading rectangle count against how much extra black area gets swept in. Three data points below are one directly comparable sweep (same 32-component mask, ~74k white px total, each re-run from the same exact-decomposition baseline):

| Threshold (black px per merge) | Effect |
|---|---|
| 256 | 206 rectangles, 29.3% total overshoot |
| 512 | 139 rectangles, 41.5% overshoot — favors fewer rectangles |
| 1024 | 88 rectangles, 58.8% overshoot — already more black than white per box on average |

A separate, earlier pass at a tighter 16px cap — on a slightly different edit of the same mask (also 32 components, near-identical sizes) — produced 815 rectangles at 3.5%–15.3% overshoot **per individual component** (no combined aggregate was computed for that run, so don't treat that range as directly comparable to the single aggregate percentages above; it's cited here to show the low end of the tradeoff, not as a fourth row in the same sweep).

There's no universally correct value — it depends entirely on whether you (or whoever's reviewing the result) care more about a tight, accurate outline or a small, manageable rectangle count. Push it too far and you're effectively just taking each blob's bounding box, which for an irregular or diagonal shape can mean blocking a lot of terrain that was never meant to be blocked.

**4. Verify by rasterizing, never by summing areas.** Once you have your kept rectangles, build a boolean grid the size of the blob, paint every kept rectangle onto it, and diff that against the true mask directly:

- `missed = (true mask) AND NOT (rectangle union)` — this must be **exactly zero**. If it isn't, your merge or exact-decomposition logic has a bug; do not ship a blocker with gaps.
- `overshoot = (rectangle union) AND NOT (true mask)` — this is your real, honest black-pixel cost. **Do not estimate overshoot by summing each rectangle's own area and subtracting the blob's white-pixel count** — merged rectangles routinely overlap each other, and that naive estimate can be off by two orders of magnitude on a real jagged shape. Rasterize the union first, always.

**No rotation, ever (§2).** A diagonal blob comes out as an axis-aligned staircase, however you tune the merge threshold. That's the primitive's limitation, not the pipeline's — accept it, or manually author a tighter hand-picked set of boxes for a shape where the staircase looks bad.

---

## 8. Pixel-to-world coordinates

Once you have rectangles in mask **pixel** space (row/column ranges), converting to world space needs the map's own `width`/`length`/`heightmapResolution` (read these from the actual `.sanmap`, never hardcode — they differ per map) and one flip:

[CODE]
```
N = heightmapResolution        -- e.g. 2049
world.x = (N - 1) - pixel.col
world.z = pixel.row
```
[/CODE]

For a pixel rectangle spanning rows `[r0, r1]` and columns `[c0, c1]` (inclusive):

[CODE]
```
center.x = (N-1) - (c0 + c1) / 2
center.z = (r0 + r1) / 2
size.x   = c1 - c0 + 1
size.z   = r1 - r0 + 1
```
[/CODE]

**This is the same convention `SANMAP_FORMAT_SPEC.md` documents for sampling `heightmap.raw`** (`row = z; col = (N-1) - x`, used there in the opposite direction — world-to-pixel, for a height lookup), used here in reverse because a hand-authored mask in `Textures/` is the same kind of raster asset as `heightmap.raw`, not a `.sanmap` JSON entity-position field.

**⚠️ Don't confuse this with the format's other, separate flip.** `SANMAP_FORMAT_SPEC.md` also documents a *different* convention for entity positions stored in the `.sanmap`'s own JSON (`world.z = length - z - 1`), and explicitly flags that convention's exact axis as still unresolved on symmetric maps. These are two different conventions for two different kinds of coordinates (a raster pixel index vs. a JSON entity field) — don't use one where the other applies, and don't assume they're the same flip just because both involve a `z`-like axis.

**Sanity-check against something you can independently verify, if you can.** The convention above was cross-checked this session against 164 real prop-instance positions already baked into the map's `.sanmap` (props named after the visual feature the mask was drawn for) — 96% landed inside the mask under this convention, and a mirrored second candidate convention (equally consistent with the format spec's own uncertainty) gave the same final result only because the specific mask happened to have exact point symmetry. If your mask/map isn't symmetric and you have no independent landmark to check against, budget time to verify in-game before treating placement as final.

---

## 9. Worked example: a Sea-only blocker end to end

Goal: block naval pathing across a strip of a map, leaving every other mobility type free to cross it — e.g. a narrow, decorative channel that should read as "impassable to ships" without also stopping ground troops walking along its bank.

### Step 1 — author the mask

`Textures/heightmap_SeaBlocker.png`, same resolution as the map's `heightmapResolution`, white over the strip you want naval units excluded from.

### Step 2 — run the decomposition (§7), get world rectangles

Produces a list like:

[CODE]
```lua
local SEA_BLOCKER_RECTS = {
    { x = 1062, z = 1061, sizeX = 60, sizeZ = 27 },
    { x = 1083, z = 1072, sizeX = 65, sizeZ = 23 },
    -- ... one entry per merged rectangle
}
```
[/CODE]

### Step 3 — build the single-layer prefab (§4)

[CODE]
```lua
local NavmapModifierLoader = Import("common/loading/navmapModifierLoader.lua")
local NavmapModifiers = Import("common/navmapModifiers.lua")

local function CreateSingleLayerBlockerPrefab(prefabName, layerNames)
    local prefabTemplate = EngineClasses.PrefabTemplate(prefabName)

    local headEntity = EngineClasses.EntityTemplate()
    headEntity.entityName = TemplateHeadEntityName
    headEntity.localTranslation = EngineClasses.float3(0, 0, 0)
    headEntity.localScale = 1.0
    headEntity.localRotation = EngineClasses.quaternion()
    headEntity.forceSimulated = false
    table.insert(prefabTemplate.hierarchyTemplate.entities, headEntity)

    NavmapModifierLoader.AddNavmapModifierTemplates(
        TemplateHeadEntityName, layerNames, EngineClasses.float2(1, 1), false,
        prefabTemplate.hierarchyTemplate.entities,
        prefabTemplate.globalTemplate.navmapModifierTemplates
    )

    local errorCode, prefabID = Engine.CreatePrefab(prefabTemplate)
    if errorCode ~= EngineErrorCode.Success then
        Error("CreateSingleLayerBlockerPrefab: Failed to create prefab '"..prefabName.."'", 2)
    end
    return prefabID
end

local SeaBlockerPrefabID = CreateSingleLayerBlockerPrefab("SeaBlocker", { "Sea" })
```
[/CODE]

### Step 4 — the spawn function

[CODE]
```lua
local function SpawnSeaBlockerNavmapModifiers()
    for _, rect in ipairs(SEA_BLOCKER_RECTS) do
        local size = EngineClasses.float2(rect.sizeX, rect.sizeZ)
        local errorCode, id = Engine.InstantiatePrefab(
            SeaBlockerPrefabID,
            EngineClasses.float3(rect.x, 0, rect.z),
            EngineClasses.float3(1, 1, 1),
            EngineClasses.quaternion()
        )

        if errorCode ~= EngineErrorCode.Success then
            Warn("SANGEN: sea-blocker prefab instantiate failed, errorCode="..tostring(errorCode))
        elseif IsClient then
            Engine.SetLocalGridModifierSize(id, size)
            Engine.SetLocalGridModifierEnabled(id, true)
        else
            local modifierIDs = NavmapModifiers.GetNavmapModifierIDs(id, { "Sea" })
            NavmapModifiers.SetNavmapModifiersSize(modifierIDs, size)
            NavmapModifiers.SetNavmapModifiersEnabled(modifierIDs, true)
            Engine.SetGlobalGridModifierSize(id, size)
            Engine.SetGlobalGridModifierEnabled(id, true)
        end
    end
end
```
[/CODE]

### Step 5 — wire it into the shared thread

Following §6's rules — area final first, every blocker call in its own `pcall`:

[CODE]
```lua
NewThread(function()
    if IsHost then
        PlayableAreaManager.SetPlayableArea(Area.FromMapArea({ x = -1000, y = -1000, width = 1, height = 1 }))
        PlayableAreaManager.SetPlayableArea(Area.FromMapArea(chosenArea))
    end

    if IsHost and spawnsUnitsEnabled then
        ...
    end

    -- Each blocker gets its own pcall -- one blocker's bug can't take another down.
    local seaOk, seaErr = pcall(SpawnSeaBlockerNavmapModifiers)
    if not seaOk then
        Warn("SANGEN: SpawnSeaBlockerNavmapModifiers threw: "..tostring(seaErr))
    end
end)
```
[/CODE]

### Step 6 — test

Check, in order: no warnings about failed prefab instantiation; a naval unit ordered across the strip actually can't cross it; a ground unit walking the same geographic strip is unaffected; a unit of a layer the map doesn't have (e.g. `Submarine` on a shallow map) doesn't error, it just silently has nothing to block. If the map has no water at all, confirm the "layer not created for this map" warning appears exactly where expected and nothing else breaks.

---

## 10. Troubleshooting by symptom

⚠️ Same caveat as `TUTORIAL_Scenarios.md` §11: `Log()`/`Warn()` go to the F1 console, which doesn't function in this build, and `game_logs/*.txt` stays empty. The only reliably observable signal is behavior in-game — does the unit actually stop, does the building-placement grid actually refuse the spot.

### "Units still pass through where I blocked them"

1. **You reused `PlayableAreaBarrierPrefabID` for a partial-layer blocker.** Re-read §4 — the other layers' modifiers on that instance are still at their template defaults, but that's a *different* symptom (stray tiny blockers elsewhere), not this one. If you specifically wanted `Sea` blocked and it isn't, check you built and are instantiating a real `{"Sea"}`-only prefab, not accidentally the barrier prefab.
2. **The layer doesn't exist on this map.** `Sea`/`Submarine` only exist `if Engine.HasWater()`. Requesting them on a waterless map is a silent no-op (§4).
3. **Something upstream in the same `NewThread` threw before your blocker call ran, and it wasn't `pcall`-wrapped.** §6 — wrap the blocker call in its own `pcall`, and check for a `Warn` you may not have noticed (though see §6.1: on this build, don't count on ever seeing it).
4. **Your blocker call itself is throwing, silently, because of something as mundane as a missing `Import`.** §6.1 is the real story here: `NavmapModifiers.GetNavmapModifierIDs(...)` throws immediately if you forgot `local NavmapModifiers = Import("common/navmapModifiers.lua")` at the top of your file — the bare identifier resolves to a nil global, not an error at that line, so the throw happens one call later and looks unrelated. This exact bug produced "the blocker doesn't work" symptoms for an entire debugging session before being found. If a blocker that previously worked stops working after *any* edit to the surrounding code — especially a scripted find-and-replace regenerating a rectangle table — diff the whole function against its last-known-good version, not just the table you meant to change.
5. **You're testing on the wrong side.** The navmap modifier calls that actually affect pathing are host-only (§5). If you only ever validated `IsClient`'s branch (e.g. by checking the placement grid, which *is* client-visible), you haven't actually confirmed the host-side pathing block at all.
6. **The rectangle is in the wrong place.** Check the pixel→world conversion (§8) independently — plot the rectangle centers against a screenshot or the map's `preview.png`, or cross-check against any real entity positions you have for the feature you're blocking.
7. **You built the prefab template with the wrong layer name.** Layer names are case-sensitive strings (`"Sea"`, not `"sea"`) matching exactly what `CreateNavigationLayers()` registers.

### "A completely unrelated mobility type is blocked somewhere I didn't intend"

This is the stray-1×1-blocker bug from §4, almost always. You reused `PlayableAreaBarrierPrefabID` (or some other prefab with more layers baked in than you meant to use) for a partial-layer blocker, and the layers you didn't touch are still sitting there `disabled = false` at `size = (1, 1)`. Fix: build a purpose-specific prefab with only the layers you actually want (§4), don't partially use a broader one.

### "The blocker prefab silently didn't instantiate at all"

1. **Prefab name collision.** `EngineClasses.PrefabTemplate(name)`'s name must be unique. Reusing an existing name (including a common engine one) is asking for trouble.
2. **`Engine.CreatePrefab` failed and you didn't check its error code.** The worked example above logs an `Error` on failure — if you skipped that check, a failed prefab creation looks identical to everything downstream just quietly doing nothing.
3. **You built the prefab template inside the deferred `NewThread` instead of the synchronous body.** Prefab *template* creation (as opposed to *instantiating* it) doesn't need to be deferred and works fine done once, synchronously, near the top of your `_data.lua` — deferring it needlessly just adds a chance of it racing something else in the thread.

### "It worked in one test and not another, same map"

1. **You changed the mask and didn't regenerate the rectangles**, or regenerated them but didn't verify the missed-pixel count is still zero (§7).
2. **A composition-specific code path (scenario system, playable-area resize, etc.) throws for that specific lobby and takes the shared thread down with it before reaching the blocker call.** This is exactly the failure mode `TUTORIAL_Scenarios.md`'s troubleshooting section covers in depth — the fix is `pcall`-wrapping each risky call individually (§6) rather than relying on getting the order right, plus isolating which composition triggers it by testing each one directly rather than assuming.
3. **A regenerated rectangle table silently ate a line of real code next to it.** §6.1 — if a blocker worked once and then didn't after a scripted edit to its data table, this is the first thing to check, before re-deriving any theory about timing or roles.

---

## 11. Quick reference

**All-layer blocker (blocks every mobility type the map has, including Air), called via its own `pcall` from the shared `NewThread`:**

[CODE]
```lua
local errorCode, id = Engine.InstantiatePrefab(_G.PlayableAreaBarrierPrefabID, position, EngineClasses.float3(1,1,1), EngineClasses.quaternion())
-- host: NavmapModifiers.GetNavmapModifierIDs(id, _G.PlayableAreaBarrierLayers) -> SetSize/SetEnabled
-- client: Engine.SetLocalGridModifierSize/Enabled(id, ...)
-- caller: local ok, err = pcall(SpawnAllLayerBlockers); if not ok then Warn(...) end
```
[/CODE]

**Single/partial-layer blocker (needs its own prefab — never reuse the barrier prefab for this):**

[CODE]
```lua
local prefabID = CreateSingleLayerBlockerPrefab("YourBlockerName", { "LayerA", "LayerB" })
local errorCode, id = Engine.InstantiatePrefab(prefabID, position, EngineClasses.float3(1,1,1), EngineClasses.quaternion())
-- host: NavmapModifiers.GetNavmapModifierIDs(id, { "LayerA", "LayerB" }) -> SetSize/SetEnabled
```
[/CODE]

**Valid layer names:** `Land`, `Amphibious`, `Hover`, `Air` (always exist) — `Sea`, `Submarine` (only if `Engine.HasWater()`).

**Non-negotiables:**

- No rotation — `NavmapModifierTemplate.size` is a plain axis-aligned `float2`.
- A prefab's layer set is fixed forever at *template* creation; never add a layer to an existing instance.
- Navmap modifier calls are **host-only**; client sets only its own local placement grid.
- `local NavmapModifiers = Import("common/navmapModifiers.lua")` at the top of the file. Not optional, not implied, and its absence produces no error at the missing line — only a throw one call later, inside `NewThread`, where it's invisible. Check for this import by name after any edit that touches this function or the code around it.
- `IsHost`/`IsClient` are mutually exclusive **per Lua state**, but your whole `_data.lua` runs **once per state** in a solo/listen-server match — both states correctly reach their own branch independently. Don't "fix" the branching to handle both being true in one execution; that case doesn't happen.
- **Wrap every blocker-spawning call in its own `pcall`**, in the one shared `NewThread`, so its errors can never cascade into anything else in that thread (§6). Blocker instantiation still needs `SetPlayableArea` to be final first (§6, rule 1) — that one's about correctness, not error propagation, and a `pcall` doesn't fix it.
- When authoring from a mask: verify by rasterizing the rectangle union against the true mask and checking `missed == 0` — never estimate coverage by summing rectangle areas.
- Read the map's real `width`/`length`/`heightmapResolution` from its `.sanmap`; never hardcode them.
- After any scripted regeneration of a data table (rectangles, scenarios, anything), diff the whole function it lives in, not just the table — a wide find-and-replace can silently delete an adjacent line of real code (§6.1).

---

### Verification notes

Confirmed live, in-game, this session: the all-layer blocker pattern (§3), the ordering and `pcall` guidance in §6 (both ordering directions were broken once each, independently, and fixed; the `pcall`-per-call pattern is the final, more robust form), and the pixel→world convention in §8 (cross-checked against 164 real prop positions, 96% agreement, and confirmed correct in-game after applying it).

⚠️ **§6.1 is worth re-reading on its own.** The all-layer blocker on `Pandemonium Isthmus` went through several rounds of live debugging that each fixed a real ordering problem without fixing the actual failure, because the actual failure (a dropped `Import` line) was unrelated to ordering entirely and produced an identical-looking silent throw. Every ordering fix in §6 is genuine and worth keeping — they just weren't sufficient on their own, and no amount of further ordering analysis would have found the real bug. If a blocker you've built stops working and the timing all checks out, look at what else changed before you re-derive a new timing theory.

**Update: the single/partial-layer pattern is now also confirmed.** The `SeaBlocker` prefab built exactly as described in §4/§9 shipped on `Pandemonium Isthmus` and was confirmed working in-game by the human after this tutorial was first drafted — naval pathing blocked on the strips it covers, other mobility types unaffected. The stray-1×1-blocker failure mode it avoids (§4) was reasoned from reading the engine source directly, not observed as a live bug — that specific failure mode itself was never deliberately reproduced and confirmed broken, only the working alternative was shipped.

⚠️ Still not independently confirmed in-game as of this writing:

- **Promoting the single-layer helper to a shared `common/loading/*.lua` file** — noted as a reasonable future step in §4, not attempted. Both blockers on `Pandemonium Isthmus` remain map-local.
- **This technique's interaction with more than two or three simultaneous blocker types on one map** — `Pandemonium Isthmus` ships exactly two: one all-layer blocker and one single-layer (`Sea`) blocker, each with its own `pcall` in the shared thread. No map has yet combined more than two, or more than one distinct partial-layer prefab.
- **Deliberately reproducing the stray-1×1-blocker failure mode described in §4** to confirm it manifests exactly as reasoned (the reasoning is solid and source-verified, but nobody has intentionally triggered it and watched it happen).
