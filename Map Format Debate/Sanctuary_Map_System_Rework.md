# Sanctuary: Shattered Sun — Map Data & Gameplay Config Rework

**Scope:** authored (non-procedurally-generated) maps.
**Basis:** direct reading of `engine/LJ/lua` and the shipped `.sanmap` files. Every claim carries a `file:line`. Anything unverified is marked **UNVERIFIED** with what would settle it.

> ## Revision 2 — what an adversarial review changed
>
> Rev 1 was synthesised from three independent design passes, then attacked by a fourth. Six things changed. Read this before anything else.
>
> | | |
> |---|---|
> | **RETRACTED — §1.1** | "Every shipped map ignores its playable area" was **wrong**. All 12 official `.sanmap` files define `PlayableArea` correctly. The real bug is that sim, AI and survival use **three different lookup chains** and resolve three different rectangles on the same map. |
> | **RETRACTED — §1.9** | The O(N²) thread-dispatcher claim was **wrong**. `threads.lua` drains removals in descending index order — zero shifts in the scenario cited. The timer wheel must be re-justified on allocation grounds and **measured** first. |
> | **CORRECTED — §6.4** | `setfenv(chunk, {})` does **not** make a config pure. Three escapes: the string metatable, no instruction guard, and text mode being conditional on an unverified flag. Memory exhaustion, hang, and possibly RCE. |
> | **CORRECTED — §7.2** | The resolved config is **6.6–8.3 KB** at real scale, not 4 KB, and it is sent **per player in a loop with a deep copy each** — ~133 KB at 16 players. Broadcast it instead. |
> | **CORRECTED — §7.5** | Validation rule #4 as written **rejects ~72 of ~95 shipped maps** — every procgen map ships a `NEUTRAL_CIVILIAN` army with no spawn marker. Needs a civilian flag. Knock-on: observer demotion is already off by one on those maps. |
> | **CORRECTED — Tier 1** | Reordered. The RPC-hole fix is now #1. The `InitializePlayableArea` move is **not** one line — `RunMapSetup` is shared host/client code and moving it breaks the client. |
>
> **Added: §8.6**, a full feasibility pass on the shatter feature.
>
> `World_Domination` is an **unofficial, broken map** and has been struck as evidence throughout. No conclusion depended on it except the retracted §1.1 and the understated §7.2 size figure.
>
> **Not yet re-verified:** the appendix line numbers drift by a few lines in ~6 places (`mapUtils.lua:110-112` → `113-115`, `templateLoader.lua:412` → `413`, `army.lua:172` → `171`, `script.lua:172-181` → `174-182`). None change a conclusion, but a claim index whose value is precision should be regenerated.
>
> One further correction: the `Two_Step_Shuffle` chain bug is **not** a hard error. The bare `AlloyMarker` key does exist, so `ChainToPositions` resolves all three entries to the **same point** — a silent wrong result, which is worse, and which the proposed "chain integrity" validator would not catch. Add a duplicate-entry check.
>
> And the `functionWrappers.lua:246` "landmine" (OQ#2) is **not** one — the surrounding block is a uniform run of exact struct sizes including `PrefabTemplate(432)`, which is not a number a human types. Generator-emitted. Downgrade to a sanity check.

---

## 0. Executive summary

The system the dev wants to build **already exists in three-quarters-finished form**. The work is finishing and connecting what's there, not designing from scratch.

What exists and works:

- A **stacked lua config file** merged over the `.sanmap` — `common/mapUtils.lua:49-69`. Shipped maps use it.
- A **mod stack** with file-replace and file-append layering — `common/systems/import.lua:99-165`. Nothing populates it.
- A **runtime playable-area resize** — `hostPlayableAreaManager.lua:52-67`. Survival uses it.
- **Reclaim**, end to end. Order, UI, engineer harvest, income math.
- An **event bus pattern** with three live production sites — `HostGameTrigger`, used by the economy's energy triggers.
- A **host→client arbitrary-table channel** that lands before the client needs it — `SendToClient(data,"InitClient",…)`, `script.lua:172-181`.
- **Runtime army creation, diplomacy, and group spawning** — all functional.

What's missing:

1. A **per-map script hook**. `MapPopulate`/`MapStart` are empty stubs with zero call sites.
2. A **lobby→lua options channel**. `LobbyInformation` is `{playersInformation, mapPath}` and nothing else.
3. **Removal semantics** in the config merge. `table.merged` is purely additive.
4. **Anchor identity**. Markers have no id, no tag, no group, no symmetry.

**Only #2 requires an engine change.** It is one string field. Everything else is lua.

---

## 1. Findings that change the picture

These were discovered while verifying the design and each one is independently actionable.

### 1.1 Three subsystems disagree about which rectangle is the playable area

> **REV 2 — this section previously claimed every shipped map ignores its playable area. That was wrong.** All 12 official `.sanmap` files define `PlayableArea` correctly (The_Forge, Two_Step_Shuffle, Pandemonium Isthmus, and 9 procgen maps). `World_Domination`, which defines `"Playable"`, is an **unofficial and broken map** and is not evidence about shipped conventions. A naive "fallback chain" fix would be a no-op at best and a **stealth balance change** at worst.

The real bug is worse and different. Shipped maps carry **two conflicting playable-area definitions**, and three subsystems resolve them differently:

| Consumer | Lookup order |
|---|---|
| Sim | `mapUtils.lua:188` — `PlayableArea` only |
| AI | `AIFunctions.lua:4277-4303` — `PlayableArea` → `Area0` → `Playable` |
| Survival | `survival.lua:283` — **`Playable_Area` first**, then `PlayableArea` |

`The_Forge.sanmap` carries `PlayableArea = {x=512, y=512, w=1024, h=1024}`. `The_Forge_data.lua:756` carries `Playable_Area = {x=768, y=768, w=512, h=512}`. Because `table.merged` is additive, **both survive into `GameInfo.MapData.areas`**.

Result: **survival on The_Forge plays a 512² area centred at (768,768) while the sim and the AI use a 1024² area centred at (512,512).** Same map, same match, three different rectangles. Same divergence on Two_Step_Shuffle.

**Fix — three parts, none of them a 10-line patch:**
1. One `GetPlayableArea()` used by sim, AI **and** survival. The three-way disagreement is the live bug.
2. A validator rule that hard-fails any map defining more than one key matching `Playable.*`.
3. Decide explicitly whether `_data.lua` overrides the `.sanmap`. If yes, fixing The_Forge and Two_Step_Shuffle is a **balance change requiring playtesting**, not a bug fix.

### 1.2 The existing area-trigger code is non-functional, not merely dead

`triggers.lua:144`, `objectives.lua:315` and `hostFunctions.lua:99` all call `area:contains(...)`. **No such method exists anywhere in the tree.** `common/area.lua:78` defines `Area:Contains` — capital C — and it takes a `float2` (x = world x, y = world z). All three call sites pass a `float3` from `GetPosition()`, operating on a raw `MapArea` table from `GameUtils.GetArea` (`gameUtils.lua:111-118`) which has no metatable at all.

Casing bug *and* axis bug. `CreateSpecificUnitsAreaTrigger`, `TagsInAreaObjective` and `FilterEntitiesInArea` hard-error on first call.

**Do not wrap this code. Replace it.**

### 1.3 The engine already does spatial queries for us

`Engine.GetVolumeCollisionEventsForWorld(worldID)` reports **enter/exit deltas** to lua — used at `constructionManager.lua:5`, `intelManager.lua:10`, `targeterManager.lua:14`. And every unit already carries a `ConstructionCollider` in `CollisionWorld.Construction` on `CollisionLayer.Units` (`unitTemplateLoader.lua:392-400`).

So an area trigger is **a box collider**, not a polling loop. Engine does the broadphase. Lua cost is O(boundary crossings), not O(triggers × units). No coroutine, no distance math, no per-tick cost.

Collider size/offset are settable per instance (`Engine.SetVolumeColliderSize`/`SetVolumeColliderOffset`), and a prefab can be registered after `LoadAllTemplates` — `RunMapSetup` proves it at `mapUtils.lua:92`. So: one generic `triggerVolume` prefab, N instances, resized per trigger.

### 1.4 The player-count blocker is one line

`CreateArmies` (`gameUtils.lua:209`) receives nothing, then calls `Engine.GetLobbyInformation()` **itself** at line 221 — reaching around its own caller, which already has the data. That is the entire reason lua cannot influence slot assignment.

```lua
-- script.lua:160
Import("common/gameUtils.lua").CreateArmies(lobbyData)   -- pass it in
-- gameUtils.lua:221
local playerInfo = lobbyData.playersInformation          -- was Engine.GetLobbyInformation()
```

### 1.5 Symmetry, numerically pinned

On `~TEAM-1v1_Tropical_256_47940.sanmap` (W=256, L=256), every `Mex N` / `Mex N sym 0` pair and both `Spawn` transforms satisfy **exactly**:

```
x + x′ = 256 = W
z + z′ = 254 = L − 2
```

9/9 pairs, exact. The generator uses 180° rotation about `(W/2, (L−2)/2)`.

`MirrorPostion` (`mapUtils.lua:233`) computes `(W−x, y, L−z)` — **wrong by 2 in z on every map**. A 2-unit error produces "no partner found" for every anchor, which is exactly the fairness bug the feature exists to prevent.

Its only caller is a debug JSON dumper (`testUtils.lua:255`). Delete it or fix it.

**UNVERIFIED** that `L−2` generalises (n=1 map, 9/9 pairs exact). The design does not depend on it — the orbit fitter searches for the centre rather than assuming one.

### 1.6 The map that is literally the worked example

`World_Domination.sanmap`: **8 armies, 96 alloy anchors**, area key `"Playable"` → not found by the loader.
`Pandemonium Isthmus.sanmap`: **2048², 16 armies, 282 alloy anchors**, `PlayableArea` correct.

282 anchors is the scale target for the click-toggle UI. A linear scan over 282 floats is ~0.02 ms — no spatial index needed.

### 1.7 Reclaim: three separate problems, currently conflated

- The `/2` at `templateLoader.lua:412` is a **refund fraction** (how much of a unit's cost survives as a wreck — a design constant), not a **reclaim percentage** (a lobby option). Conflating them is probably why you were told reclaim doesn't exist.
- **Wrecks auto-delete after 180 s**, unconditionally (`wreckageClass.lua:53-56`). This silently caps the value of any reclaim-% option.
- `income = {}` is **allocated every tick per actively-reclaimed wreck** (`wreckageClass.lua:88`) — a bigger cost than anything a multiplier adds.
- Reclaiming a **live unit pays nothing and kills nothing**. `Reclaimable:ReclaimProcessThread` (`reclaimable.lua:61-86`) integrates progress, emits no income, creates no wreck, and `OnReclaimProcessEnded` is an empty stub. Unfinished — the two-phase intent is in the comments at `unitsBaseClass.lua:2893-2898`.

### 1.8 Two remote-code-execution holes

```lua
-- hostListeners.lua:12-14
function functions.TestUtilities(data)
    Import("host/testUtils.lua")[data.fn](unpack(data.args or {}))
end
```

`Import` returns a table whose metatable is `{__index = _G}` (`import.lua:107-109`). Any `data.fn` not in `testUtils` **falls through to the real global table**. Any client can invoke `CreateUnit`, `CreateArmy`, `Import`, `SetPlayableArea`, `error` with arbitrary wire-supplied arguments, no `pcall`.

Same bug one hop away: `functions.SimpleEvent` → `CallEvent` → `Import("host/simpleEvents.lua")[name]()` (`hostSimpleEvent.lua:3-6`). Zero-arg only, but `_G` has plenty of zero-arg functions with side effects.

Not a user goal, but it is a one-packet remote crash of every player in the match. `rawget` + a whitelist + a debug-build gate. ~12 lines.

### 1.9 ~~The thread dispatcher is O(N²) for phase-aligned pollers~~ — RETRACTED

> **REV 2 — this claim was wrong.** `threads.lua:241-268` collects departing threads into `threadsToRemove` and drains them in **descending index order**, with a comment saying exactly why (*"Remove in reverse order to avoid index shifting"*). In the scenario originally cited — N phase-aligned pollers all rescheduling on the same tick — descending removal always removes the current last element: **zero shifts, O(N) total.** The claimed 33k moves/s is 0.

A smaller real cost remains in the *mixed* case: if only some threads leave, removing index *k* still shifts the `#threads − k` survivors. Worst case ~N²/4 for "half the bucket leaves, evenly interleaved" — which is the opposite of phase-aligned polling.

**Consequence for the plan:** the timer wheel (§8.2b) may still be worth building on **allocation** grounds — a LuaJIT coroutine has a ~900 B minimum stack, plus two tables per timer — but the O(N²) justification is void. **Measure before writing 400 lines.**

Minor: `threadsToRemoveCache` is a module-level singleton reused across `ProcessThreads` calls, drained by `threadsToRemove[i] = nil`. Correct as written, but any early return or error between collection and drain leaves stale indices for the next tick. Worth an assert.

---

## 2. Architecture

### 2.1 The model in one picture

```
                      ┌─────────────────────────────────────┐
   AUTHORED           │  .sanmap  — terrain, art, the FULL  │
   (never patched)    │           superset of anchors       │
                      │  anchors.json — ids, orbits, tags   │
                      │  map.json  — manifest: the options  │
                      └─────────────────────────────────────┘
                                       │
   LAYERED            ┌────────────────┴────────────────────┐
   (composable)       │  base.cfg → profile.cfg → pack.cfg  │
                      │           → lobby choices           │
                      └────────────────┬────────────────────┘
                                       │
   RESOLVE            ┌────────────────┴────────────────────┐
   (pure, host-only)  │  fold layers → ResolvedConfig (4KB) │
                      │  flat SoA + bitmasks, no allocation │
                      └────────────────┬────────────────────┘
                            ┌──────────┴──────────┐
                            ▼                     ▼
                    host applies          replicated verbatim
                                            to every client
```

### 2.2 The five principles

**1. The `.sanmap` keeps the full superset. Configs select, they never invent.**
Terrain and the anchor superset are authored once. A config turns things on and off and moves scalars. It cannot add an anchor, a marker category, or a prop blueprint.

**2. Config is data. Scripts are code. They are different files.**
A config is a pure lua table loaded with `setfenv(chunk, {})` — an empty environment, no metatable. Any global reference yields nil; any call errors at load, offline, deterministically. Side effects are impossible, not discouraged.

**3. The host resolves. The client obeys.**
This deletes the entire class of "two peers computed different configs" bugs rather than trying to detect it. See §5.2 — this was the one real disagreement between design passes, and the reasoning is set out there.

**4. Prefab creation is driven by the manifest, never by config.**
This makes the prefab-ID parity hazard *structurally impossible* rather than a rule people have to remember. See §5.3.

**5. Deletion is designed out, not solved.**
`table.merged` can't delete because it merges an open-world tree — absence is indistinguishable from "not mentioned". Replace it with a **closed set of typed channels over a fixed domain** and the problem evaporates: the anchor set is fixed by `anchors.json` and immutable, so a config only ever writes a per-anchor boolean. "Remove anchor 37" becomes "set `anchors[37] = false`". There is nothing to delete because nothing is ever created.

---

## 3. The map package

### 3.1 Layout

```
Sanctuary_Data/Maps/World_Domination/
├── map.json                  MANIFEST. Options, profiles, prefab union, event
│                             declarations. The ONLY file a map-list scan reads.
├── World_Domination.sanmap   Unchanged, fileVersion 3. Editor-owned. Never patched.
├── anchors.json              Stable ids, symmetry orbits, tags, positions, categories.
│                             Converter-maintained. NOT patchable by configs.
├── preview.png
├── preview_meta.json         px→world transform, so the lobby can hit-test a click.
├── Textures/                 Unchanged.
├── configs/
│   ├── base.cfg.lua          Author's always-on layer.
│   ├── profile_2p.cfg.lua    Player-count profiles.
│   ├── profile_4p.cfg.lua
│   ├── profile_8p.cfg.lua
│   └── events_default.cfg.lua
├── scripts/
│   ├── map.lua               Optional per-map script. Fills the MapPopulate void.
│   └── events/raiders.lua    One file per scripted event.
└── loc/en.json               id → display string.
```

**Why the data tree, not `LJ/lua/maps/`:** the lobby has no `Import`, no `libPath`, no sandbox. If the manifest lives in the lua tree, the lobby needs a second search path *and* a lua reader. Also, a map should be one shippable unit — zip the folder and you have the map. Today you need two directories from two trees plus the knowledge that the pairing key is the sanmap *filename*, not `data.name` (`mapUtils.lua:26`).

The current split is explicitly an artifact — `mapUtils.lua:44` says *"Lua file with data as workaround while we dont have map editor support."*

**UNVERIFIED:** that lua can read sibling files of `mapPath` via `Engine.GetFileContent`. `LoadMapData` already does `Engine.GetFileContent(mapPath)` with the lobby-supplied path (`mapUtils.lua:13`), so string-manipulating it to reach `map.json` *should* work. Settle with one call to `Engine.FileExists(mapPath:gsub("[^/\\]+%.sanmap$", "map.json"))`. If it fails, `map.json` moves next to `_data.lua` and the design survives — the lobby story just gets worse.

### 3.2 Third-party config packs

A pack **never** writes into the map's folder.

```
Sanctuary_Data/MapConfigs/lowspec_rebalance/
├── pack.json          identity + which maps it targets + options it ADDS
├── configs/wd_lowprops.cfg.lua
├── scripts/events/meteor.lua
└── loc/en.json
```

```jsonc
{
  "packVersion": 1,
  "id": "lowspec_rebalance",        // globally unique; also the option namespace prefix
  "priority": 100,                  // layering rank; ties broken by id (stable)
  "targets": [
    { "map": "World_Domination",    // map.json id, NOT the folder name
      "mapVersion": ">=1 <2",
      "anchorsEpoch": 3,
      "configs": ["configs/wd_lowprops.cfg.lua"],
      "options": [ /* extra options this pack contributes */ ],
      "events":  [ { "id": "lowspec.meteor", "script": "scripts/events/meteor.lua",
                     "default": false } ] }
  ]
}
```

**Reconciling with `Mods_Active`.** Config *data* is not `Import`ed — it's discovered by directory scan and loaded into an empty environment. Config pack *scripts* are `Import`ed, so the loader pushes roots onto `Mods_Active` in config-layering order:

```lua
Mods_Active = {
    "/",                                              -- base game (import.lua:99)
    "Sanctuary_Data/Maps/World_Domination/",           -- map package
    "Sanctuary_Data/MapConfigs/lowspec_rebalance/",    -- packs, ascending priority
}
```

That makes `Import`'s reverse-scan replace precedence and config precedence the same order — one mental model instead of two.

**Rule: config packs must not ship an `append/` directory.** The validator hard-fails on one. Text-appending onto a chunk reintroduces exactly the additive-only, order-sensitive, un-diffable behaviour this design exists to kill.

### 3.3 The manifest

Read by the lobby *before* the map loads. Budget: **≤6 KB, single parse, zero dependent file reads.**

```jsonc
{
  "manifestVersion": 1,
  "id": "World_Domination",         // stable join key. Configs target THIS.
  "name": "World Domination",       // display; may contain spaces
  "version": "1.0.0",               // semver. Packs range-match on it.
  "sanmap": "World_Domination.sanmap",   // explicit — kills the implicit filename pairing
  "anchors": "anchors.json",
  "size": [2048, 2048],             // duplicated so the lobby never opens the .sanmap

  "requires": { "engine": ">=0.4.0", "anchorsEpoch": 3 },

  // Replaces "names carry hardcoded meaning" folklore with a checkable declaration.
  "contracts": ["core.spawns", "survival.v1"],
  "spawnKeyIsArmyKey": true,        // the Spawn/<ArmyName> coupling, gameUtils.lua:359

  // Fixes the §1.1 footgun by declaration instead of renaming 28 maps.
  "playableAreaKey": "Playable",
  "areas": { "full": "Playable", "south_basin": "South_Basin", "north_ridge": "North_Ridge" },

  "profiles": [
    { "id":"p8", "players":8, "default":true, "config":"configs/profile_8p.cfg.lua",
      "defaults": { "map.area":"full" } },
    { "id":"p4", "players":4, "config":"configs/profile_4p.cfg.lua",
      "defaults": { "map.area":"south_basin" } },
    { "id":"p2", "players":2, "config":"configs/profile_2p.cfg.lua",
      "defaults": { "map.area":"north_ridge" } }
  ],
  "profileSelector": "map.players",

  "options": [ /* §4 */ ],
  "configs": ["configs/base.cfg.lua", "configs/events_default.cfg.lua"],
  "script":  "scripts/map.lua",
  "events":  [ /* §6.3 */ ],

  // THE parity guarantee — §5.3. Every blueprint any config may ever instantiate.
  "prefabUnion": {
    "resourceSpots": ["alloys"],    // replaces markers["Alloys"].resource gating
    "props": [],                    // sorted, deduped
    "decals": []
  }
}
```

**Lobby scan cost.** N maps → N × `map.json` (~3 KB each). 300 maps ≈ 900 KB, one JSON parse each. **Never open a `.sanmap` in a scan** — `Two_Step_Shuffle.sanmap` alone is 9.3 MB. `anchors.json` (~12 KB for Pandemonium's 282 anchors) is read only on selection.

---

## 4. The option model

### 4.1 Types

| type | value | notes |
|---|---|---|
| `bool` | true/false | |
| `enum` | one of `values[]` | values carry `label` + optional `requires` |
| `int` | integer in `[min,max]` step `step` | clamped at load, never rejected |
| `percent` | integer × `scale` | the reclaim case |
| `multiselect` | set over `domain[]` | encoded as a bitmask |
| `anchorset` | set over anchors carrying `tag` | **the per-anchor toggle**, §5 |
| `profile` | one of `profiles[].id` | selects a config *and* rewrites defaults |

Each option declares which **channel** and **key** it writes, and a **combine rule** (`override` / `mul` / `min` / `max` / `and`).

### 4.2 Dependencies are data predicates, not expressions

No mini-language to parse, sandbox, or get wrong:

```jsonc
"visibleIf": { "all": [ { "opt":"map.players", "eq":"p4" } ] },
"enabledIf": { "any": [ { "opt":"map.area", "in":["south_basin","north_ridge"] } ] },
"requires":  { "map.area": "full" }        // sugar for a single-term `all`
```

Grammar is exactly `{all|any|not: [term]}` where `term = {opt, eq|ne|in|gte|lte, value}`. Depth-limited to 3. Evaluable in C# in the lobby and in lua from the same data. Cycles are a validator hard-fail.

### 4.3 Profiles: overridable defaults

The lobby keeps, per option, `{value, explicit}`. `explicit` flips true the moment the host touches that control and **never flips back** for the session.

```lua
local function ApplyProfile(state, profile)
    for optId, value in pairs(profile.defaults) do
        local slot = state[optId]
        if slot and not slot.explicit then
            slot.value = value           -- profile retargets it
        end
        -- else: host owns it. Profile loses. UI shows a "modified" dot.
    end
end
```

Why this matters: the host will change player count *after* tweaking options — someone joins late. Without `explicit`, a late join silently reverts the host's reclaim setting.

For `anchorset` specifically: if the host has toggled anchors and then switches to a profile with a *different domain*, the explicit toggles are **intersected**, not discarded (`enabled' = enabled ∩ profileDomain`), with out-of-domain toggles kept in a shadow set and restored on switching back. Nothing the host did is thrown away.

### 4.4 The worked example, end to end

```lua
-- 1. Map selected. Lobby parses map.json (3 KB), seeds from defaults.
state = {
  ["map.players"]      = { value="p8",       explicit=false },
  ["map.area"]         = { value="full",     explicit=false },
  ["map.alloys"]       = { value="all",      explicit=false },
  ["map.reclaim"]      = { value=100,        explicit=false },
  ["map.events"]       = { value={["wd.supplydrop"]=true}, explicit=false },
}

-- 2. Host picks 4 players. Profile p4 applies; map.area was untouched → retargeted
--    to south_basin. anchors.json loads now (12 KB) for the preview.
state["map.players"] = { value="p4", explicit=true }

-- 3. Host clicks two alloy points on the preview:
--      px → world (preview_meta.json)
--      world → nearest anchor within 12 units (anchors.json x[]/z[])
--      anchor → orbit; orbitLocked so the WHOLE orbit flips
--    Clicking anchor 37 flips orbit 12 = {37, 71}, a rot180 pair.
state["map.alloys"] = { value={ orbitsOff={12,19} }, explicit=true }
--    Constraint: minEnabledOrbits=4. 48−2=46. OK.

-- 4. Host enables 2 of 5 events. wd.storm greys out (requires map.area=="full",
--    which the profile moved). wd.blackout greys out (conflicts with wd.raiders).
state["map.events"] = { value={["wd.raiders"]=true,["wd.supplydrop"]=true}, explicit=true }

-- 5. Reclaim 50%.
state["map.reclaim"] = { value=50, explicit=true }
```

---

## 5. Anchors, identity and symmetry

### 5.1 Three keys, each with one job

| key | role | stability |
|---|---|---|
| `name` | join key into `.sanmap` `markers.<CAT>.transforms.<name>` | editor-owned, can change on re-export |
| `id` (int) | **the only thing configs ever reference** | assigned once, never reused, never renumbered |
| `posKey` (quantized x,z) | reconciliation key when `name` changes | changes when the author moves an anchor |

Configs reference `id` and nothing else. That is what makes an editor rename a non-event for every third-party pack in existence.

Why not name-as-id: procgen names are `"Mex 4 sym 0"`; hand-authored ones are `AlloyMarker_47` with a bare `AlloyMarker` at index 0 (verified in both `World_Domination` and `Pandemonium Isthmus`). Not semantic, and the editor renumbers on delete.

### 5.2 `anchors.json`, column-oriented

```jsonc
{
  "anchorsVersion": 1,
  "epoch": 3,                      // bumped ONLY when ids are added or tombstoned
  "nextId": 97,                    // monotonic; never decreases

  // The category list. Configs cannot touch it — it is not a config channel.
  // This is the prefab-parity anchor point (§5.4).
  "categories": ["Alloys", "Spawn"],

  "count": 104,
  "id":    [1, 2, 3, /*…*/],       // parallel arrays; index i = runtime dense slot
  "name":  ["AlloyMarker","AlloyMarker_1", /*…*/],
  "cat":   [0, 0, 0, /*…*/],       // index into categories[]
  "x":     [512.5, 601.5, /*…*/],
  "z":     [1024.5, 998.5, /*…*/],
  "orbit": [1, 2, 2, /*…*/],       // 0 = singleton

  "orbitKind":   "rot180",
  "orbitCenter": [1024.0, 1023.0],  // FITTED, not assumed (see §1.5)
  "orbits": [
    { "o":1, "members":[1],   "kind":"none" },
    { "o":2, "members":[2,3], "kind":"rot180" },
    { "o":49,"members":[96,97,98,99,100,101,102,103], "kind":"rot90x2" }
  ],

  "tags": {
    "alloy_optional": [2,3,4,/*…*/95],   // host-toggleable
    "alloy_core":     [1],               // always on, not in any option domain
    "expansion":      [40,41,42,43]
  },

  "tombstones": [17, 18]           // ids that existed and no longer do. NEVER reused.
}
```

### 5.3 Orbits: the exporter proposes, a human accepts

Load-time derivation is off the table — the only derivation function in the codebase is wrong by 2 units (§1.5), and a silent "no partner found" for every anchor is exactly the fairness bug the feature exists to prevent. Hand-authored maps encode no symmetry at all.

So the converter fits a transform offline and the result is **frozen in `anchors.json`**:

```lua
---@return {kind, center, orbits, unmatched}
local function ProposeOrbits(anchors, w, l)
    -- 1. Fit the centre from SPAWNS first — few (2..16) and fairness-critical,
    --    so a bad fit is visible immediately. Candidates: rot180, mirrorX,
    --    mirrorZ, rot90. Centre is GRID-SEARCHED over
    --    {w/2,(w-1)/2,(w-2)/2} x {l/2,(l-1)/2,(l-2)/2} plus a least-squares
    --    refine — do NOT hardcode (w/2, (l-2)/2) even though that is what the
    --    shipped procgen maps land on.
    -- 2. Greedy nearest-partner match, epsilon 0.75 world units. The verified
    --    pairs are EXACT, so 0.75 is slack for hand-nudged maps, not sloppy ones.
    -- 3. Cross-category matches are rejected: an orbit is homogeneous.
    -- 4. Return unmatched ids. The exporter REFUSES to auto-accept if any remain;
    --    the author fixes the map or marks them kind="none" by hand.
end
```

Two properties: an orbit is **homogeneous in category**, and orbit membership is a **partition** — every anchor is in exactly one orbit, singletons included. That makes "toggle an orbit" total, and makes fairness checking a set-cardinality test rather than a geometric one.

### 5.4 Surviving a re-export

The sidecar is **reconciled**, not regenerated:

```
PASS 1 — name match, same category      → keep id, update position   ("author moved a mex")
PASS 2 — position match (±0.5)          → keep id, update name       ("editor renumbered")
PASS 3 — leftovers in sanmap            → NEW: id = nextId++, epoch bump
PASS 4 — leftovers in sidecar           → REMOVED: tombstone, epoch bump
```

**Passes 1 and 2 never bump the epoch.** Renames and moves — the two things an editor round-trip actually causes — are invisible to every third-party config.

| author change | epoch | effect on a pack pinned to epoch 3 |
|---|---|---|
| rename anchor | 3 | none |
| move anchor | 3 | none — same id, new position |
| **add** anchors | 4 | pack still loads; new anchors take the map default; lobby shows an info badge |
| **remove** anchors | 4 | pack still loads; tombstoned refs dropped with a `Warn` |
| change option schema | `version` **major** | pack rejected — the only hard break |

The rule behind the table: **id reuse is the only thing that can silently corrupt a config**, so ids are never reused. Everything else degrades loudly but keeps working — which is what a modding ecosystem needs.

### 5.5 Lobby click → orbit

```
click px → preview_meta.json (origin, scale, flipZ) → world (x,z)
         → linear scan anchors.json x[]/z[], reject if > 12 world units
         → i → orbit[i] → if orbitLocked, flip every member
```

282 floats worst case ≈ 0.02 ms. No spatial index. The UI renders one dot per anchor coloured by state and highlights the **whole orbit on hover** — that's how the host learns symmetry is being respected: hover one point, four light up.

### 5.6 Does the `.sanmap` need a format bump?

**Not for v1.** The sidecar carries everything and `fileVersion 3` stays untouched, which matters because the C# editor owns that file.

**Recommend v4 later**, folding `anchorId` into the transform. Safe on both axes that matter: adding a key to a transform object is invisible to `table.merged` and to `GetMarker`, and transform *contents* are not prefab-forming — only the *category set* is. **UNVERIFIED:** whether the editor can persist an extra transform field and maintain an id allocator. Ask the C# editor owner.

---

## 6. The config language

### 6.1 Why not the two obvious designs

**Ordered patch-op list** (`{op="disable", target="anchor", id=37}, …`): composes by concatenation, which is *not commutative* — two packs in different orders give different games, and `Mods_Active`'s order is decided by an unwritten resolver (`import.lua:62-92`). Cannot be validated in isolation: op *n*'s legality depends on ops 1..*n*−1. Diffs are useless — inserting one op shifts nothing textually but changes everything semantically.

**Declarative add-set + remove-set per channel**: composes, but it's a semilattice with a tie-break problem. What does `A.enabled ∩ B.disabled` mean? Every channel needs its own hand-written answer that authors must memorise.

### 6.2 Chosen: closed-schema typed channels over a fixed domain

Each channel is a partial function `key → value` over a **finite, pre-declared domain**. A config is a set of such partial functions. Composition is a fold with a per-field combine rule declared in the manifest — commutative where the rule is (`mul`, `and`, `min`, `max`), order-defined-and-logged where it isn't (`override`). Fully validatable in isolation, because domains come from the map, not from other configs.

```lua
-- configs/profile_4p.cfg.lua
-- Loaded with an EMPTY environment. It can only build tables and return one.
return {
    schema  = 1,
    id      = "wd.profile.4p",
    targets = { map="World_Domination", mapVersion=">=1 <2", anchorsEpoch=3 },

    -- CHANNEL: anchors — domain = anchors.json ids. Boolean.
    -- Absent id = "not mentioned" = inherit from below. NOT "disabled".
    anchors = {
        byOrbit = { [25]=false, [26]=false, [27]=false, [49]=false },
        byId    = { [96]=true, [97]=true, [100]=true, [101]=true },
    },

    -- CHANNEL: areas — domain = map.json.areas logical names.
    areas = { playable = "south_basin" },

    -- CHANNEL: scalars — domain = manifest-declared scalar keys.
    scalars = { ["alloys.yieldScale"] = 1.0 },

    -- CHANNEL: props — domain = prefabUnion.props. false = do not instantiate.
    -- The PREFAB is still created either way (§7.3).
    props = {},

    -- CHANNEL: armies — domain = MapData.armies keys.
    armies = {
        Army_5 = { enabled=false }, Army_6 = { enabled=false },
        Army_7 = { enabled=false }, Army_8 = { enabled=false },
    },

    -- CHANNEL: events — domain = declared event ids.
    events = {},
}
```

> **`armies.<X>.enabled = false` rather than deleting the key is the single most important line in this file.** Army index comes from the alphabetical sort position of the key (`gameUtils.lua:213-219`). Removing keys renumbers every later army and `Spawn/<ArmyName>` (`gameUtils.lua:359`) silently points at the wrong start position. Marking-not-removing pins indices across every possible config combination.

### 6.3 Layer order and conflict resolution

```
0  .sanmap
1  LJ/lua/maps/<N>/<N>_data.lua      legacy overlay, table.merged as today — additive-only
                                     is fine HERE because nothing is beneath it
2  map.json option defaults
3  configs/base.cfg.lua, then the rest of map.json.configs
4  the selected profile's config
5  third-party packs, ascending priority, ties broken by pack id
6  host lobby choices                ALWAYS WINS — otherwise the UI is lying to the host
```

**Contested-field reporting is mandatory.** Any field written by ≥2 layers with differing values is recorded:

```lua
resolved.contested["scalars.game.reclaimYieldScale"] = {
    { layer="pack:lowspec_rebalance", value=0.8 },
    { layer="lobby",                  value=0.5 },
}
```

The lobby renders these as an expandable "N settings changed by mods" panel. Silent override by a pack is the failure mode that destroys trust in a config system; making it visible costs one table.

### 6.4 The loader — how "pure data" is enforced

```lua
--- NOT Import: Import sets __index = _G (import.lua:107-109) and would hand
--- a config file the full Engine table.
local function LoadConfigChunk(path)
    local src = Engine.GetFileContent(path)
    if not src then return nil, "missing: " .. path end

    local chunk, err = load(src, "@" .. path)     -- bootstrap already forces text mode
    if not chunk then return nil, err end

    -- The whole enforcement mechanism, one line: an empty environment with NO
    -- metatable. Any global reference yields nil; any call on it errors at load,
    -- offline, deterministically. No io, no Engine, no require, no math —
    -- a config cannot compute, only declare.
    setfenv(chunk, {})

    local ok, result = pcall(chunk)
    if not ok then return nil, result end
    if type(result) ~= "table" then return nil, "config must return a table" end
    return result
end
```

The offline validator runs the *same function* under stock luajit, so author-time and load-time agree by construction.

> **REV 2 — the original text claimed "side effects are impossible, not discouraged" and "a config cannot compute, only declare". Both are false.** An empty environment is necessary but **not sufficient**:
>
> **(a) The string metatable is untouched.** `setfenv` replaces the *global* environment; it does not remove `getmetatable("").__index = string`, which is independent of the chunk's env and which `bootstrap.lua` never alters. So a config can still do `return { name = ("A"):rep(2^30) }` — a 1 GB allocation before `pcall` returns — and has full access to `string.rep/format/gsub/byte`. *(Same escape applies to `ParseTagsFromString`, whose env is `{Tags = Tags}`.)*
>
> **(b) No instruction-count guard.** `while true do end` in a downloaded config hangs the process on load. The codebase already has this discipline elsewhere (`threads.lua:239`, `maxIterations = 100`); the config loader would be the only place executing untrusted code without one. Fix: run the chunk on a coroutine with `debug.sethook(co, fn, "", 1e7)` — `debug.sethook` is whitelisted (`bootstrap.lua:80`).
>
> **(c) Text mode is only forced when `enableSandbox` is true.** `bootstrap.lua:105-107` wraps `load` to force `'t'` **inside `if enableSandbox then`**; the `else` branch at `:113` restores raw `load`. So "bootstrap already forces text mode" holds only under a condition Open Question #1 says is **unknown**. If it's false, a downloaded config pack can ship LuaJIT bytecode, which trivially escapes `setfenv` and gives arbitrary memory access. **Fix: pass the mode explicitly — `load(src, "@"..path, "t")` — rather than depending on a global that may not be set.** One argument closes it.
>
> **(d) Nested-table depth.** A 200k-deep table blows the C stack on any recursive traversal (`table.merged`, `table.deepCopy`, `json.encode`). `pcall` usually catches LuaJIT's stack overflow, so this is PLAUSIBLE rather than confirmed — but a depth counter in the validator is cheap enough to add regardless.
>
> **Net DoS surface for a downloaded config: memory exhaustion (a), hang (b), and — conditional on OQ#1 — full RCE (c).** All three are cheap to close. None of them were in rev 1.

The determinism property does hold: no `math.random` is reachable, which matters because **`math.randomseed` is never called anywhere in the tree** (§9.3).

---

## 7. Load order, parity, and the resolve phase

### 7.1 The new host timeline

Changes marked **[NEW]** / **[MOVED]** / **[EDIT]**.

| # | Step | Change |
|---|---|---|
| 1 | `bootstrap.lua SecureInit` | — |
| 2 | `script.lua init()` | — |
| 3 | `TemplateLoader.LoadAllTemplates()` `script.lua:91` | — (do **not** make this option-dependent; see §8.1) |
| 4 | `hostMain.Init()` | — |
| **4a** | **[NEW]** `Options.Ingest(lobbyData.options)` → validate/clamp → frozen `GameInfo.Options` | in `InitLobby`, `script.lua:150` |
| 5 | `InitLobby(lobbyData, argv)` | **[EDIT]** body gains 4a / 6b / 6c |
| 6 | `LoadMapData(mapPath)` `mapUtils.lua:10` | — |
| 6a | `_data.lua` merge `mapUtils.lua:49-69` | kept for compat (§10) |
| **6b** | **[NEW]** `Manifest.Load(mapName)` — pure file read | between 6a and 6c |
| **6c** | **[NEW] RESOLVE** — `resolved = Resolve(raw, manifest, options)`, pure, no world access | **between `script.lua:156` and `:157`** |
| **6d** | **[MOVED]** `InitializePlayableArea()` — **out of `RunMapSetup`** | before `CreateArmies` |
| 7 | `InitializePlacementGrid` `script.lua:157` | now runs on the *resolved* map |
| 8 | `CreateArmies(lobbyData)` | **[EDIT]** takes the caller's lobbyData (§1.4); honours `ActiveSlots` |
| 9 | `SpawnInitialUnits()` | **[EDIT]** clamp to the resolved playable area, not the heightmap |
| 10 | observer demotion + `SendToClient(…,"InitClient",…)` | **[EDIT]** payload gains `resolvedConfig` + `resolvedHash` |
| **10a** | **[NEW]** load `scripts/map.lua`; fire `OnResolved` (pure) | after 6c |
| 11 | `hostMain.Start()` | — |
| 12 | `RunMapSetup(true)` | **[EDIT]** prefab creation hoisted to the manifest (§7.3) |
| **12a** | **[NEW]** `Script.OnWorldReady()` — first hook allowed to touch entities | replaces the hardcoded survival call |
| 13 | `TryStartSurvival()` `hostMain.lua:69` | **[EDIT]** becomes mode dispatch (§8.5) |

**Client:** `InitClient` arrives with the resolved config already in it → `LoadMapData` → `RunMapSetup(false)`. **The client never resolves.**

**Why resolve sits at `script.lua:156↔157`:** `InitializePlacementGrid` and `CreateArmies` both consume `GameInfo.MapData` — armies from `MapData.armies` keys — and the whole point of the player-count profile is that it rewrites armies and the playable area. Resolve must precede both, and must follow `LoadMapData` because that's where `MapData` first exists. There is exactly one slot.

**Why `InitializePlayableArea` moves:** units spawn at step 9, the area is set at step 12 today. That inversion is already a live bug. Dependency-checked: `InitializePlayableArea` → `CreatePlayableAreaBarriers` → `_G.PlayableAreaBarrierPrefabID`, assigned at `templateLoader.lua:180` inside `LoadAllTemplates` (step 2). And `CheckResourceSpots` is only called from `SetPlayableArea`, not from `InitializePlayableArea`. **The move is safe.**

### 7.2 Host resolves, client obeys — the one design disagreement

Two of the three design passes disagreed here. The resolution:

**Against both-sides-resolve:** the resolver being pure means "identical inputs → identical output", but the failure mode isn't the resolver — it's **input divergence**. A client with a different `.sanmap`, a different `_data.lua`, a different manifest, or a different pack set resolves differently and correctly. And `Import`'s mod stack (`import.lua:140-148`) makes exactly that possible with no detection whatsoever.

**The cost objection doesn't hold.** What gets replicated is not the map — it's the *resolved config*. The client still parses the `.sanmap` itself, exactly as today.

> **REV 2 — the size and cost figures were wrong.** The 36-byte bitmask claim is true *in memory*, but on the wire the dominant term is the three float arrays `anchorX/anchorY/anchorZ`, which grow **linearly with anchor count**. Minified-JSON encoding the §9.2 shape:
>
> | map | anchors | resolved JSON |
> |---|---|---|
> | World_Domination *(invalid map)* | 104 | 3.3 KB ← the original "~4 KB" |
> | ~FFA-16P_Desert_2048 | 240 | **6.6 KB** |
> | Pandemonium Isthmus | 298 | **8.3 KB** |
>
> **And "zero cost" is wrong by a factor of N players.** `script.lua:168-184` sends `InitClient` **inside a per-player loop**, each iteration doing a full `table.deepCopy(lobbyData)` (`:174`) and a full `json.encode` (`commands.lua:1044`). At 16 players × 8.3 KB that is **~133 KB encoded and transmitted 16 times at match start**, plus 16 deep copies.
>
> **Fix:** `SendToAllClients` (`networking.lua:18`) exists and is not used here — the per-player copy exists only to set `focus` and demote observers. **Broadcast the resolved config once under its own message name** and keep the tiny per-player payload as-is.
>
> **Packet size limit: UNVERIFIED (U4).** `Engine.CreateCustomCommandSingle` marshals via a 16-byte descriptor (ptr+len), not a fixed buffer, so there is no lua-side cap. Whether the transport fragments or truncates is engine-side.
>
> **Late join / reconnect: does not exist.** Grepping the whole tree for `reconnect|rejoin|late.join|OnPlayerJoin|PlayerConnected` returns nothing, and `InitClient` is sent once inside `if _G.Tick == 0`. So the question is moot today — but this design adds a second thing that must enter any future resync snapshot.

**And the timing already works, unchanged.** The host sends `InitClient` at `script.lua:172-181`, before its own `RunMapSetup(true)` at `hostMain.lua:67`. The client receives it, then calls `LoadMapData` at `script.lua:189` and `RunMapSetup(false)` at `script.lua:205`. The payload is `json.encode`'d (`commands.lua:1044`), so an arbitrary table rides along at zero cost. **The resolved config is in hand before either side creates a prefab, with no engine change.**

Ship a hash alongside it (FNV-1a over the canonical serialisation) purely as belt-and-braces: the client recomputes from the received struct and hard-fails with "config transfer failed" instead of a mystery desync 40 seconds in.

### 7.3 Making the prefab hazard structurally impossible

Today prefab creation is **gated on map content**:

```lua
-- mapUtils.lua:110-112 — a data-dependent prefab decision
local alloySpotMarker = GameInfo.MapData.markers["Alloys"]
if alloySpotMarker and alloySpotMarker.resource then
    ResourceSpotLoader.CreateResourceSpotPrefab("alloys")
```

`resourceSpotTemplateLoader.lua:10-13` says in so many words that this must not be: *"Must be called at the same point in the setup sequence on both the host and the client, as prefab IDs are assigned by creation order."* The host even creates decal prefabs it never uses purely to keep the counter aligned (`mapUtils.lua:161-163`).

The same hazard applies to props: `mapUtils.lua:74-92` sorts by `blueprintPath` and calls `CreatePropPrefab` per entry unconditionally — so "choose which props spawn" is a prefab-ID hazard too, not just alloys.

**Fix — prefab creation reads the manifest, which no config can touch:**

```lua
for _, resType in ipairs(Manifest.prefabUnion.resourceSpots) do   -- pre-sorted at export
    ResourceSpotLoader.CreateResourceSpotPrefab(resType)          -- unconditional
end
for _, path in ipairs(Manifest.prefabUnion.props) do              -- pre-sorted at export
    PropLoader.CreatePropPrefab(LoadPropTemplate(path))
end

-- Instantiation, and ONLY instantiation, consults the resolved config:
if shouldInstantiateEntities then
    for slot = 1, resolved.anchorCount do
        if resolved.catIsResource[resolved.anchorCat[slot]] and AnchorEnabled(resolved, slot) then
            _G.CreateResourceSpot("alloys", AnchorPos(resolved, slot), scale, rotation)
        end
    end
end
```

A category with zero enabled transforms instantiates nothing. `CreateResourceSpotPrefab` already memoises per type. Prefab creation becomes a function of the manifest — **and the resolver is then structurally incapable of shifting a prefab ID.**

Three reinforcing mechanisms, none of which anyone has to remember:

1. **Categories are not addressable.** The channel set is closed — `anchors, areas, scalars, props, armies, events` — and none of them can name a marker category. So `gameUtils.CreateMarker`'s create-on-demand behaviour is unreachable from config data.
2. **Prefab creation is config-independent** (above).
3. **The client doesn't resolve**, so even a resolver bug can only be wrong identically on both sides — a gameplay bug, not a crash.

Incidental win: instantiation now iterates a **dense array in index order** instead of `pairs(alloySpotMarker.transforms)` (`mapUtils.lua:117`), which is hash-order and therefore nondeterministic. Harmless today because only one prefab is created for all spots — a trap waiting for the first per-anchor prefab variant.

### 7.4 The one engine change

`LobbyInformation` is `{playersInformation, mapPath}` (`engineClasses.lua:86-88`). Add one field:

```c
struct LobbyInformation {
    PlayerInformationArray playersInformation;
    UnsafeString mapPath;
    UnsafeString gameOptions;   // NEW: opaque JSON blob
};
```

One string, opaque to the engine. The engine carries bytes from lobby UI to `Engine.GetLobbyInformation()` and has zero knowledge of option semantics — so the struct never changes again as the option schema evolves.

> **Landmine:** `functionWrappers.lua:246` does `ffi.cast(ffiTypes.LobbyInformation, AllocatePersistentMemory(32))` — a hardcoded 32-byte buffer sized to the current struct. Adding an `UnsafeString` makes it 48. **If that `32` is hand-written rather than emitted by the generator, adding a field silently corrupts memory.** Verify before landing. Five minutes.

**Client side: zero engine change.** The client never calls `GetLobbyInformation()` — it gets everything through `InitClient` (§7.2).

**Interim, zero engine change:** `Engine.GetCommandLineOverrides()` is already passed to `InitLobby` at `hostMain.lua:66` and parsed by `ProcessCmdLineString`. Survival already ships options this way. That gives the full end-to-end pipeline — options → host resolve → client — testable from a shortcut, with only the lobby UI missing.

Two real bugs in that parser to fix first if you use it:
- `string.gmatch(argString, "([^%s-]+)")` (`commandLineArguments.lua:24`) splits on **every** `-`, not just leading ones. `-config=small-4p` parses as two tokens.
- `arguments[arg] = value` converts `"false"` to boolean `false`, and `HasCommandLineArgument` then returns `false` for it (`:47-51`) — an explicitly-disabled boolean is indistinguishable from an absent one. Use `arguments[arg] ~= nil`.

### 7.5 Validation

**Author time (CLI validator + editor export gate) — hard-fail on everything:**

1. **Schema** — every file parses; versions known; no unknown channel keys (unknown key = typo = otherwise a silent no-op); no `append/` dir in a pack.
2. **Reference closure** — every option key exists in its channel's domain; every event script exists; every area name resolves; every army key exists; every prop path is in `prefabUnion`; every anchor id exists and isn't tombstoned; no dependency cycles.
3. **Orbit completeness (fairness)** — for every `orbitLocked` option and every config: the enabled set must be a **union of complete orbits**.
   ```lua
   for _, orbit in ipairs(anchors.orbits) do
       local on, off = 0, 0
       for _, id in ipairs(orbit.members) do
           if resolved.anchors[id] then on = on + 1 else off = off + 1 end
       end
       if on > 0 and off > 0 then
           Fail(("orbit %d split: %d on, %d off — asymmetric for this config"):format(orbit.o, on, off))
       end
   end
   ```
4. **Spawn count vs profiles** — for `players = N`: exactly N Spawn anchors enabled, exactly N **non-civilian** armies enabled, the enabled spawn set orbit-closed (a 4p profile on an 8-fold ring must take a symmetric 4-subset, not any 4), and every enabled spawn's name equals its army key.

   > **REV 2 — as originally written ("exactly N armies enabled") this rule rejects ~72 of the ~95 shipped map folders.** Every procgen map ships an extra army with no spawn marker:
   >
   > | map | armies | Spawn markers |
   > |---|---|---|
   > | ~TEAM-1v1_Desert_512 | ARMY_1, ARMY_2, **NEUTRAL_CIVILIAN** | 2 |
   > | ~TEAM-3v3v3_Tropical_1024 | ARMY_1..9, **NEUTRAL_CIVILIAN** | 9 |
   > | ~TEAM-8v8_Desert_2048 | ARMY_1..16, **NEUTRAL_CIVILIAN** | 16 |
   >
   > Armies ≠ spawns on the majority of the library, and `spawnKeyIsArmyKey` is false for all of them. **The manifest needs a first-class `civilian`/`neutral` army flag**, excluded from spawn-count and key-coupling checks.
   >
   > Two knock-on consequences rev 1 missed: (i) `NEUTRAL_CIVILIAN` sorts *after* `ARMY_9`, so `gameUtils.lua:213-219` gives it the last index and `script.lua:163-166`'s `mappedArmyCount` counts it — **the observer-demotion threshold is already off by one on every procgen map**, a pre-existing instance of the exact bug §8.5's `ActiveSlots` proposal warns about; (ii) §8.3 proposes creating a neutral army at runtime via `CreateArmy(…, civilian=true)` when the maps that matter **already ship one in map data** — the helper should *adopt* `NEUTRAL_CIVILIAN` when present rather than creating a second.
5. **Contract checks** — `survival.v1` requires `Spawn_Marker_1..N` contiguous, `Target_Marker_<N>` for each, `Enemy_Target_Marker`, the `Enemy_Intel` group. `core.spawns` requires ≥2 spawns and the name/army coupling.
6. **Chain integrity** — catches the shipped latent bug: `Two_Step_Shuffle.sanmap`'s `FirstChain` names `"AlloyMarker"` three times but the map's alloys are `AlloyMarker_0..51`, so `ChainToPositions` would hard-error the moment anything called it.
7. **Prefab union closure** — every blueprint reachable from any config or event script is listed.

**Load time — split by consequence class:**

*Hard-fail (abort with a readable message):* manifest/anchors missing or unparseable; version from the future; pack targeting excludes this map; a pack with a non-empty `prefabUnion` present on host and absent on a client; a blueprint that fails to load; spawn count ≠ profile player count.

*Validate → repair → log:* unknown or tombstoned anchor id → drop the entry; unknown event id → drop; scalar out of range → clamp and log; **split orbit → enable the whole orbit** (fail toward *more* symmetry, never less); unknown area name → fall back through the key chain and log — which finally makes §1.1's silent failure loud.

**The justification for the split:** hard-fail iff the fault can produce a **desync or an unfair match**; degrade otherwise. A desync is unrecoverable and blames the wrong person. An event that quietly didn't fire is a bug report. A four-player game where one side has three fewer mexes is worse than no game at all — so orbit and spawn faults are in the hard tier or the fail-toward-symmetry repair.

---

## 8. Lifecycle, events, and the features

### 8.1 Four phases with hard capability boundaries

| Phase | Fires | May touch | May **not** touch |
|---|---|---|---|
| **CONFIGURE** | after `LoadMapData` | its manifest table; resolved option values | `Engine.*`, `GameInfo`, `Armies`, `_G` writes, `NewThread` |
| **POPULATE** | between resolve and `CreateArmies` | `CreateArea/Marker/Group`, area profile selection, active-slot set, `__Templates` mutation | unit spawning (armies don't exist yet) |
| **BUILD** | immediately after `RunMapSetup(true)` | `CreateUnit`, `SpawnGroup`, trigger instantiation, colliders | treating options as mutable |
| **RUNTIME** | tick ≥ 1 | everything | — |

CONFIGURE is *pure* — a function from option values to a resolved config, with no world access. That's what makes it safe to run before the map exists, safe on the client for preview, and safe in a lobby process with no simulation at all. The lobby needs to render "4 players → this rectangle, these alloy spots" **before the match starts**, and it can only do that if CONFIGURE touches nothing.

POPULATE and BUILD are split because of the step-9/step-12 inversion: anything that decides *where the world is* must run before armies are created; anything that puts units in it must run after.

### 8.2 The event system

**Event sources already emitted — one-line publish insertion each, no engine change:**

| Event | Site |
|---|---|
| `UnitDestroyed` / `UnitDeleted` / `UnitDamaged` | `unitsBaseClass.lua:800 / :929 / :1034` |
| `UnitUpgraded` | `unitsBaseClass.lua:2835` |
| `PropDestroyed` / `PropDeleted` / `PropDamaged` | `propBaseClass.lua:35 / :56 / :120` |
| `PlatoonDestroyed` / `PlatoonDisband` | `platoon.lua:193 / :210` |
| `OrderTargetSet` | `hostGameTriggers.lua:3` |
| `ArmyEnergyDown` / `ArmyEnergyUp` | `economy.lua:177 / :179` |
| `ArmyStatChanged` | `army.lua:172` (`SetUnitStat` — the single mutation point) |
| `ArmyDefeated` | `winCondition.lua:19` |
| `PlayableAreaChanged` | `hostPlayableAreaManager.lua:52` |
| `VolumeEnter` / `VolumeExit` | `constructionManager.lua:5` event stream |

**Must be added in lua (one line each):** `UnitCreated`, `UnitCompleted`, `UnitCaptured`, `ConstructionFinished`.

**Must be added in the engine: nothing.** The trigger system needs zero engine changes.

**Three data structures, all O(1) per tick when idle:**

**(a) Event bus** — event ids are small dense integers assigned at CONFIGURE, so subscriber lists are arrays, not hashes.

```lua
-- lists[eventId] = { n, holes, [1..n] = sub }
function Publish(eventId, ev)
    local l = lists[eventId]
    if not l then return end            -- no subscribers: 1 index + 1 test, ~2-3 ns
    for i = 1, l.n do                   -- numeric for, no pairs, no alloc
        local s = l[i]
        if s.alive then s.fn(s.ctx, ev) end
    end
    if l.holes ~= 0 then Compact(l) end -- amortised, only after a disarm
end
```

`ev` is a scratch table owned by the source and reused — no allocation on the publish path.

> Contrast with `HostGameTrigger:Invoke` (`hostGameTrigger.lua:19-23`), which uses `pairs`, and `RemoveHandler` (`:11-17`), which nils array slots — so once any handler is removed the array acquires holes and `pairs` falls back to hash-part iteration. With radar units subscribing/unsubscribing constantly on `EnergyUpTrigger` this is fragile. **The replacement must never iterate with `pairs`, and must compact rather than nil in place.**

**(b) Timer wheel** — replaces `CreateTimerTrigger`, which allocates a coroutine (LuaJIT min stack ≈ 900 B) plus two tables per timer.

```lua
local WHEEL = 1024                     -- power of two; 1024 ticks = 102.4 s at TickRate 10
local slotCount = {}
local tDeadline, tFn, tCtx, tPeriod = {}, {}, {}, {}   -- parallel arrays, zero per-timer alloc
function Tick(now)
    local slot = bit.band(now, WHEEL - 1)
    if slotCount[slot] == 0 then return end            -- the common case
    ...
end
```

Idle cost: one `band`, one index, one compare — **~5 ns/tick**, independent of timer count. Swap-with-last removal is O(1), which sidesteps §1.9 entirely.

**(c) Area triggers = colliders.** See §1.3. One generic `triggerVolume` prefab, N instances resized per trigger, registered in `CollisionWorld.Construction` with `collisionMask = CollisionLayer.Units`. `constructionManager.lua:11` already does `if collider then` before dispatching, so trigger colliders sharing that world are skipped harmlessly by the existing loop. Start with the shared world; only split into a dedicated `CollisionWorld.Trigger` if profiling shows contention.

**(d) Army-stat triggers.** The existing one polls every 3 s and re-sums the entire tag set (for `Tags.ALL_UNITS`, every template in the game). Replace with incremental: `Army:SetUnitStat` (`army.lua:172`) is the single mutation point. Publish there; watchers keyed by `(armyId, statName, tpId)` re-evaluate only the affected counter. **Per-tick cost: zero.** ~10 lines, strictly better.

**(e) Distance watchers** — the only thing that genuinely needs sampling, and only unit-to-unit (unit-to-point collapses into a sphere trigger volume). Striped scheduler, fixed 32 position reads/tick regardless of watcher count; latency `ceil(#W / 32)` ticks — 256 watchers → 0.8 s worst case.

### 8.3 Declaring an event so a lobby option can toggle it

```lua
-- map.json events + a pure-data manifest block
events = {
  { id      = "north_pass_ambush",
    enabled = { option = "north_pass_ambush" },        -- <- the gate
    once    = true,
    on      = { type="AreaEnter", area="NorthPass",
                filter={ armies="players", tags="Tags.MOBILE - Tags.SCOUT", minCount=3 } },
    run     = { action="SpawnAmbush", args={ group="North_Raiders", army="Neutral_Raiders" } } },
}
```

**The gate is evaluated once, at POPULATE.** A disabled event is *never instantiated* — no collider, no subscription, no timer entry. It is not "armed but skipped"; it costs literally nothing at runtime. That is the whole point: toggling is a construction-time decision expressed as data, so the runtime path has no per-event `if enabled` branch at all.

`tags="Tags.MOBILE - Tags.SCOUT"` is safe as a string because `ParseTagsFromString` (`tags.lua:198-213`) already evaluates tag expressions in a restricted environment containing only `Tags`, and caches the result. Reuse it verbatim.

The action file is the only lua that ever executes, and only at RUNTIME:

```lua
-- scripts/events/raiders.lua
function Actions.SpawnAmbush(ctx, args)
    local armyId = ctx.armies[args.army]        -- resolved at POPULATE, not looked up now
    local units  = ctx.SpawnGroup(armyId, args.group)

    -- Orders issued on the same tick as CreateUnit are silently dropped.
    -- Both real call sites do this: survival.lua:530-531, showcase_script.lua:114-115.
    -- ctx.After defers onto the timer wheel; it does NOT create a coroutine.
    ctx.After(1, function()
        ctx.IssueOrder(OrderTasks.ATTACK_MOVE, units, ctx.markers.North_Pass_Target)
    end)
end
```

**Runtime cost of this event: zero** until a unit crosses the boundary. The engine broadphase was already running. On crossing, the engine emits one `VolumeEnter` in the existing event stream; the trigger manager resolves the collider to its unit via `Engine.GetGlobalEntityRoot` (the pattern at `constructionDetector.lua:24`), checks `unit:HasTags` (a single table index), counts, fires, auto-disarms, and deletes its collider.

**The neutral army:** `civilian = true` in `_G.CreateArmy` gives exactly the semantics wanted — civilians are forced neutral at setup (`gameUtils.lua:326-329`) and `Army:IsAlive()` returns false (`army.lua:212`), so they can never block a win condition. Survival's `GetOrCreateEnemyArmy` (`survival.lua:312-355`) is the working recipe for a hostile one and should become the shared helper.

### 8.4 Reclaim × N%

Four options were evaluated. **Chosen: an economy-level scalar** (`economy.lua:83-84`).

**Why, not just what:**

1. **It is the only chokepoint provably exclusive to reclaim.** Every producer of `harvest`-category income was traced: `HostWreckage.harvestIncomeEntity` (the payout) ✔; the `harvest` entity on `Tags.HARVEST` units — constructed `income=nil, enabled=false` and **nothing anywhere populates it**, inert; extractors and generators use `generation`/`production`, summed separately at `economy.lua:84`. So scaling `resoucesEntitiesTotals.harvest` scales reclaim **and nothing else** — a stronger guarantee than patching `wreckageClass`, because it holds for any future reclaimable regardless of class.
2. **Cost is O(resources × armies), not O(wrecks).** `economy.lua:82-87` already loops once per army per tick. 2 resources × ≤64 armies = **≤128 muls/tick**. Below noise.
3. **Zero client-parity risk.** The client renders host-pushed totals (`SendUpdateEconomyTotalsCommand`). The one client value derived from a template is `harvestTime` for the progress ETA — untouched, so the progress bar stays truthful while the payout scales.
4. **No ordering problem.** `Economy` objects are created in `Army:__init` at step 8, long after the map and options are known. This dodges the `LoadAllTemplates`-runs-before-`mapPath` problem entirely.
5. **Hot-swappable.** A mode can do "reclaim doubles after 10 minutes" with one assignment.

```lua
-- economy.lua, Economy:__init
self.harvestMultiplier = ReclaimMultiplierDefault or 1.0

-- economy.lua, replacing Economy:Update lines 82-87
for resName, res in next, self.resources do
    local harvest = self.resoucesEntitiesTotals.harvest[resName] * self.harvestMultiplier
    res.harvest = harvest
    res.income  = self.resoucesEntitiesTotals.generation[resName] + harvest
    res.request = self.resoucesEntitiesTotals.consumption[resName]
    res.outcome = res.request
end

-- new module function, called from POPULATE/BUILD
function SetReclaimPercent(pct)
    local m = pct * 0.01
    _G.ReclaimMultiplierDefault = m           -- picked up by armies created LATER,
                                              -- e.g. survival's GetOrCreateEnemyArmy
    for _, eco in pairs(Economies) do eco.harvestMultiplier = m end
end
```

**Two lines of real change plus a setter.** Seeding the default matters — survival creates armies after setup, and without it those armies silently run at 100%.

Four things to do alongside it:

- **Separate the two knobs.** Hoist `templateLoader.lua:412`'s `/2` to a named `WreckRefundFraction = 0.5`. It is a *refund fraction*, not a *reclaim percentage*. Costs nothing, makes the distinction visible.
- **Fix energy reclaim** (currently hardwired to 0): three lines in `GenerateUnitWreckage`. Note this is a balance change, not a bug fix — but it composes correctly, since the same multiplier scales both resources.
- **Expose or hoist the 180 s wreck lifetime** (`wreckageClass.lua:53-56`), or the option is much less interesting.
- **Guard `harvestTime <= 0`** — `wreckageClass.lua:90` divides by it, and a hand-authored prop could set it to 0 → infinite income.

**Finish live-unit reclaim as part of this.** In `HostUnit:OnReclaimProcessEnded`: if complete, `CreateWreckageFromUnit(self)` then `self:Delete()`. That routes **all** live-unit reclaim value through the wreck, so it inherits the reclaim % with zero extra work — a good argument for finishing it now rather than after.

### 8.5 Playable area, player count, props, units, modes

**Playable area.** Fix the key chain first (§1.1) — 10 lines, and every shipped map immediately gets the area its author intended. Then:

- **Dash clamp** (`unitsBaseClass.lua:1460-1472`): delete 12 lines, add one. `GetNearestPointInPlayableArea` (`hostPlayableAreaManager.lua:104-111`) already returns a `float3` preserving `.y` and already returns the input unchanged when uninitialised — exactly the current semantics.
- **AI** (`AIFunctions.GetPlayableArea:4277-4305`): delegate to the manager, and **drop `PlayableAreaCacheDuration` to 0, invalidating on `PlayableAreaChanged`**. A 10-second stale window immediately after a resize is precisely when the AI walks units into a barrier. (This now allocates a table per call where it returned a reference; both real consumers copy scalars out immediately — **verify no other caller mutates it** before landing.)
- **Initial spawn**: fixed by the `InitializePlayableArea` move (§7.1), then clamp to the area rather than the heightmap.
- **Spawn markers outside the selected area**: hard error in the linter and in CI so it never ships; at runtime `Warn` and fall back to the full-map profile. **Never silently clamp** — a commander in a plausible-looking wrong place is worse than an obvious failure.

**Player count.** §1.4's one-line fix, plus an `ActiveSlots` set chosen at POPULATE. `nil` means "all slots", so behaviour is byte-identical until a profile opts in. Two consequences that must be handled or it's a regression: `mappedArmyCount` (`script.lua:163-166`) must count *active* slots, not map slots, or a legitimate player is demoted to observer; and `CreateArmy`'s auto-id path is already broken (`hostFunctions.lua:22` does `table.getn` on a hash table → always 0, worked around by hand in `survival.lua:327-332`) and shrinking slot counts drives more code down it.

**Props.** Two separate problems:

*Selection* — ~6 lines, zero runtime cost: merge `props` in `LoadMapData` (it currently isn't), and filter on an authored `propData.set` in the `RunMapSetup` loop against the resolved enabled sets.

*Props aren't objects* — `mapUtils.lua:98` instantiates a bare prefab. No `HostProp`, so no tags, no health, no reclaim, no callbacks. **Recommendation: opt-in promotion, not wholesale.** Budget (**rough-estimate, must be measured**): a `HostProp` is ~4 tables at ~400–600 B → at 60 000 map props, **~24–36 MB of lua heap and ~240 000 GC-traced objects**, which on a 10 Hz sim shows up as periodic hitching. Keep the bulk as bare prefabs; let the manifest mark specific prop *sets* as `interactive`. A few hundred reclaimable rocks costs ~0.2 MB. The measurement to settle it is cheap: instantiate N dummy `HostProp`s, read `collectgarbage("count")` and frame time at N = 1k / 10k / 60k.

*(Prerequisite for promotion: map prop templates are loaded ad hoc at `mapUtils.lua:82-92` and **never registered in `__Templates`**, so `GetClass`/`GetTemplate` would both fail. ~15 more lines.)*

**Units.** The machinery all works and nothing calls it. Declarative binding, resolved at BUILD:

```lua
spawns = {
  { slot="Player_1", group="EDA_Commander_Group", anchor="Spawn/Player_1", faction="auto" },
  { slot="Player_1", group="Starter_Engineers",   anchor="Spawn/Player_1",
    enabled={ option="starting_engineers" } },
}
```

`faction="auto"` runs each tpId through `GameUtils.FactionConvert` (`gameUtils.lua:429-440`) so one authored group serves all three factions — that function exists and is currently unused for this.

**Fix the discarded rotation** (`gameUtils.lua:386`, `-- TODO: rotation`). The map data carries a full quaternion and `CreateUnit` already takes `orientation`. The gotcha: `EngineClasses.quaternion` is `{value = float4}`, **not** `x,y,z,w` — `mapUtils.lua:97` already shows the correct wrapping idiom.

**Modes.** Turn `hostMain.lua:69` into one dispatched call against a registry. `Army:ComputeWinCondition` becomes a one-line delegate with its **current body moved verbatim into `modes.annihilation.EvaluateArmy`**, so default behaviour is preserved exactly. ~20 lines. One thing must generalise alongside: today evaluation only happens when an army dies; "survive 20 minutes" or "hold 3 points" needs a periodic poke — that goes on the **timer wheel**, not a new polling coroutine.

### 8.6 The shatter feature — feasibility against this architecture

**Requirement:** terrain falls away leaving a hole; scriptable size and location; **many zones, randomly placed and rotated per match**; fires when enough weight (`UnitCount × TotalUnitsMass`) is inside a trigger location.

#### Verdict per sub-requirement

| Sub-requirement | Verdict |
|---|---|
| Script it as an event, control size and location | **Supported** — this is exactly the event system. Needs one addition (below). |
| Many zones, **randomly placed** per match | **Supported.** Determinism is a non-issue — see below. |
| **Randomly rotated**, cosmetically | **Supported today, no work.** `HoleTemplate.rotation` is in radians (`engineClasses.lua:358`); `Hole:SetRotation` exists (`client/entities/hole.lua:78`). |
| **Randomly rotated**, *functionally* (pathing, build-blocking, the trigger) | **Blocked on an engine question.** See U1. |
| Weight-threshold trigger | **Not supported as specified.** No mass field exists; the accumulator needs a design the doc didn't have. |
| The hole as a *simulation* event | **Not supported.** Three engine asks. |

#### Random placement does NOT conflict with §9.3

§9.3 bans `math.random` **in the resolver**, which is the CONFIGURE phase. The shatter roll belongs in **POPULATE**, which §8.1 already permits to touch world state. Stated explicitly so nobody reads the ban as global:

> **POPULATE is the only phase permitted to consume `ctx.Random()`.**

And §7.2's "host resolves, client obeys" makes determinism moot here entirely. Host and client are separate processes with separate LuaJIT states and already consume `math.random` independently today — client scorch decals (`client/units/unitsClasses/unitsBaseClass.lua:291,341,343`), host weapon spread (`weaponsBaseClass.lua:398-399`), survival spawn scatter (`survival.lua:167-169`). Nothing reconciles them and nothing needs to.

#### The mechanism — and why it does NOT need the resolved config

| when | who | what |
|---|---|---|
| POPULATE | host | `rng = SeededRng(hash(mapId, optionVector, matchId))`; roll N zones `{x,z,radius,rot}`; reject overlaps with spawn anchors |
| BUILD | host | instantiate N trigger volumes from the **one** `triggerVolume` prefab |
| BUILD | host → all | `SendToAllClients(zones, "ShatterZones")` — once, ~30 B/zone |
| RUNTIME, on fire | host | apply grid flags, navmap modifier, destroy units; `SendToAllClients({zone=i}, "ShatterFired")` |
| RUNTIME, on receive | client | `CreateHole(...)` — the cosmetic, which **is** rotatable |

**The key structural fact, which follows from §7.3's own rule:** prefab **creation** order must match across host and client; prefab **instantiation** has no such constraint (`mapUtils.lua:98` instantiates props host-only and nothing breaks). So create the shatter prefab unconditionally from `prefabUnion` on both sides, and instantiate N times on the host with random transforms. **The client needs nothing at load.** The resolved config does not grow, and `InitClient` does not need to carry zones.

#### What the engine cannot do today

- **No heightmap setter exists.** `engineFunctions.lua` exposes only readers — `GetTerrainHeightmapSize:1310`, `SampleTerrainHeight:1337`, `SampleTerrainNormals:1346`. Grepping the FFI surface for `SetTerrain|SetHeightmap|ModifyTerrain` returns only `SetMovementTerrainAlignmentFactor` (a per-unit visual tilt). **Engine ask #1** — or accept that terrain never deforms and the hole is a render trick over unchanged geometry, which is presumably what the prototype is.
- **`Engine.AddHole` is client-only.** `client/entities/hole.lua` wraps `AddHole`/`RemoveHole`/`SetHoleSize`/`SetHoleRotation`/`SetHoleCircle`. Grepping the *host* binding returns only `HoleTemplate` marshalling inside `CreatePrefab`. Holes attach to a `localId` and change rendering only — nothing touches navmap, grid, or collision. **Engine ask #2**, or accept client-only visuals driven by a replicated message.
- **No rotation setter on any simulation-side extent.** `NavmapModifierTemplate` is `{entityName, disabled, size: float2, layerIndex}`; `GridModifierTemplate` size is `int2` (structurally AABB); there is `SetVolumeColliderSize` and `SetVolumeColliderOffset` but **no `SetVolumeColliderRotation`**. Nothing in the codebase instantiates a modifier with a non-identity rotation — `playableAreaBarrier.lua:47` passes `EngineClasses.quaternion()`.

#### What the architecture already handles, and one thing the doc missed

- **Pathability:** navmap modifiers, runtime enable/resize. `playableAreaBarrier.lua` is the working recipe. Axis-aligned.
- **Build grid — better than expected.** `templateLoader.lua:127` creates the grid with `Engine.CreateGlobalGrid(1)` — **cellSize 1, so grid coords equal world coords.** Invalidating a shatter footprint is one call:
  ```lua
  Engine.SetGlobalGridCellBaseFlags(gid, int2(x0,z0), int2(x1,z1), PlacementLayer.InvalidTerrain, true)
  ```
  Caveat: the host runs `InitializePlacementGrid` at `script.lua:157` (before `RunMapSetup`), the client at `:206` (**after**). Base flags and modifier flags are separate layers so it converges, but shatter re-flagging must be **replicated as an explicit message** — the client will not recompute it.
- **Resource spots:** handled. `ResourceSpotUtils.resourceSpots` is a live registry; reuse `CheckResourceSpots`'s `SetEnabled` pattern.
- **Units standing on it:** no mechanic exists. Must be scripted destruction. `DestroyType.Dissolve` is the closest existing visual.
- **Props inside it — impossible today.** `mapUtils.lua:95-99` declares `instantitatedPropID` **inside the loop and discards it**. There is no registry of map-prop GlobalIDs anywhere in the tree, so you cannot enumerate, query, or delete props in a zone. **Hard prerequisite.**
- **AI path map — stale forever.** `AIMarkerGenerator.BuildTerrainPathMap()` builds `PathMap[x][z]` per cell (~4.2M lua tables on a 2048² area), once, with **no invalidation entry point**. After a shatter the AI believes the hole is walkable land, permanently. This is a second, much larger cache than the `PlayableAreaCacheDuration` one §8.5 addresses.

#### There is no mass field

`mass` appears in exactly three places: `templateExplainations.lua:421`, inside a block headed *"Old format, still have some leftover stuff"* — *"Only used in unit collisions for now. When set to 0, it uses footprint properties to calculate mass"* — and twice in `UnitBlueprintValidator.lua` as a non-required field. **Zero real templates define it.** And `mass` lives under `movement`, which structures do not have at all — so even if populated, every structure would weigh nothing, which is precisely the thing a player parks in a trigger zone.

| candidate | on every unit? | meaning |
|---|---|---|
| `economy.cost.alloys` | yes | "investment weight". Commander = 100 000 → one commander outweighs ~3 000 T1 tanks. Needs a cap or a curve. |
| **`footprint.x * footprint.y`** | **yes, incl. structures** | "ground covered". Commander:T1 ≈ 5.7:1. **This is the engine's own mass fallback when `mass == 0`.** |
| `collisionInfo.collisionSize` | yes | hitbox volume, hand-tuned for shooting |

**Recommend `footprint.x * footprint.y`** — defined on 100% of templates, physically sensible curve, and it is literally what the engine substitutes for mass.

Also: **`UnitCount × TotalUnitsMass` is dimensionally `count² · mass`** — it double-counts, since a total already sums over the units. Pick either "total footprint area" or "count above a size threshold" and name it in the UI. Do not ship the formula as written.

#### The weight accumulator

Enter/exit alone cannot stay drift-free, and the codebase contains the proof. `constructionDetector.lua:29-32`:

```lua
function ConstructionDetector:OnColliderExit(volumeCollisionEvent)
    if not Engine.IsValidGlobalID(volumeCollisionEvent.colliderGlobalID) then
        -- We can receive OnColliderExit when the entity gets destroyed
        return
```

Good news: the engine **does** emit an exit when an entity dies inside a volume. Bad news: the id is already invalid, so you cannot resolve it back to a unit to subtract its weight — and the shipped code's `return` leaves a **live membership leak in production today**, on exactly the pattern this design builds on.

**The fix that makes it exact:** never resolve the root at exit time. Key by collider id.

```lua
-- on enter
inside[colliderGlobalID.index] = { root = rootID, w = weight }
total = total + weight
-- on exit — no engine call, works fine on an already-destroyed entity
local e = inside[idx]
if e then total = total - e.w; inside[idx] = nil end
```

Remaining drift sources, each needing a hooked event or an answered question: unit **built inside** (U2), unit **captured** (no event today), unit **upgraded** (`unitsBaseClass.lua:2835`), unit **transported** (cargo weight vanishes), volume **resized** (U3), and **the shatter itself** deleting units in the zone (case 2 ×N — the trigger latches on and re-fires).

**And the rising-edge trick that makes a periodic sweep unnecessary:** you only care about the moment `total ≥ T`. Run the accumulator as the cheap continuous gate; when it first crosses `T`, **verify once** with an exact sweep before firing. The sweep is a linear pass over armies' unit lists with an AABB test — at 500 units × 8 armies that's ~4 000 `GetPosition` calls, **sub-millisecond, once per candidate firing**. Disagreement resets the accumulator and logs, giving free drift telemetry.

#### Required addition to the config language: a `spatial` channel

§6.2's closed channel set (`anchors, areas, scalars, props, armies, events`) **cannot express a shatter zone** — it is a new spatial object with `(x, z, radius, rotation)` that does not exist in the `.sanmap`, and §2.2 principle 1 forbids configs from inventing one. The only escapes are pre-authoring dummy anchors (which makes placement *selected*, not random, and anchors carry no rotation) or putting it in an event script (arbitrary lua).

That exposes the design's real shape, and it should be stated plainly rather than discovered: **the closed channel language is not the modding surface — the event scripts are.** Every interesting third-party feature will be an event script, and event scripts have none of the properties §6.1 argues for. The config language governs the *boring* half well.

**Add a `spatial` channel** with a closed *shape* domain (`rect|circle` plus `x, z, w, h, rot`). The shape kinds stay closed even though the coordinates are free, so the closed-domain property survives — and it is the difference between "packs can retune the map" and "packs can build a new game mode".

---

## 9. Performance

### 9.1 Three phases

| phase | work | budget |
|---|---|---|
| **Author time** (converter/validator/editor) | orbit fitting, id allocation, reconciliation, prefab-union computation, closure + fairness checks | seconds, on a dev machine |
| **Load time** (host, once) | parse manifest + anchors + K configs, fold, flatten to SoA, hash, serialise ~4 KB | **< 5 ms** for 282 anchors × 6 layers |
| **Runtime** | bitmask tests, array indexing | zero allocation, zero hashing |

Resolve runs inside a block that already does a full JSON decode of a multi-megabyte map, a `table.deepCopy` of five sections, and an O(mapX × mapY) placement-grid sweep calling `Engine.SampleTerrainNormals` per cell — ~2M FFI calls on a 1024² map. **Resolve is noise against that.** The only ways to blow the budget are deep-copying per layer, parsing string paths per op, or `pairs()` over the full marker set per op. The design rules out all three.

Nothing symmetry-related runs at load or runtime. Orbits are read from disk as integers.

### 9.2 The resolved runtime structure

Structure-of-arrays, not the nested `markers[cat].transforms[name]` dicts used today.

```lua
ResolvedConfig = {
    hash = 0x8f2a91c4,

    -- categories: FIXED, config-independent. Order == prefab creation order.
    categories    = { "Alloys", "Spawn" },
    catIsResource = { true, false },

    -- anchors, SoA, sorted by (cat, id) so a category is a contiguous range
    anchorCount = 104,
    catFirst    = { 1, 97 },        -- iterate one category with a plain numeric for
    catLast     = { 96, 104 },
    anchorId    = { 1, 2, 3, --[[…]] },
    anchorX     = { 512.5, 601.5, --[[…]] },
    anchorY     = { 27.4, 31.2, --[[…]] },   -- terrain-sampled ONCE at load
    anchorZ     = { 1024.5, 998.5, --[[…]] },
    anchorOrbit = { 1, 2, 2, --[[…]] },

    -- enable state: bitmask, 32 slots/word. 282 anchors -> 9 words = 36 bytes.
    anchorEnabled = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFF3FFFF, 0x000000FF },

    eventBits = 0x00000005,
    eventBit  = { ["wd.raiders"]=0, ["wd.supplydrop"]=2, --[[…]] },

    scalars = { ["game.reclaimYieldScale"] = 0.4 },
    playableArea = { x=512.0, y=1024.0, width=1024.0, height=512.0 },

    -- armies: parallel arrays in the SAME sorted order gameUtils uses
    armyName    = { "Army_1", "Army_2", --[[…]] },
    armyEnabled = { true, true, true, true, false, false, false, false },
    armyGroup   = { "Player_EDA_Initial_Units", nil, --[[…]] },
}

--- Hot path. No allocation, no hashing, no string concat.
local function AnchorEnabled(cfg, slot)
    local i = slot - 1
    return bit.band(cfg.anchorEnabled[bit.rshift(i,5)+1], bit.lshift(1, bit.band(i,31))) ~= 0
end
```

Wins over the current shape: the per-marker terrain sample (`mapUtils.lua:130-133`) happens once into `anchorY`; iteration is a numeric `for` over a contiguous range instead of `pairs` over a hash (which also makes order deterministic); the enable state for a 282-anchor map is **36 bytes**; "how many alloys are live" is a popcount, not a predicated loop.

`GameInfo.MapData.markers` stays populated for backwards compatibility with `GetMarker`/`MarkerToPosition` and everything in `AI/` and `survival.lua`. **The SoA is an addition, not a replacement** — no flag day.

### 9.3 Determinism

`math.randomseed` is **never called anywhere in the tree**. LuaJIT does not auto-seed, so `math.random` starts from a fixed internal state and the sequence is identical in every process. **The game is deterministic by accident, not by design.**

Two ways this bites:

1. **The sequence is position-dependent, not value-dependent.** Host and client execute different code paths. Any `math.random` consumed on one side and not the other permanently desynchronises the streams. So it's only safe where the result is used on exactly one side.
2. **It looks fixed until someone "fixes" it.** The first person who adds `math.randomseed(os.time())` for map variety — and `os.time` **is** in the sandbox whitelist — converts every existing accidental-determinism into a live desync, in code they did not touch.

Mitigations:

- **Ban `math.random` in the resolver outright.** It's a pure function of `(rawMapData, manifest, options)` and must not consume global RNG state. Trivially enforced by shadowing it to `error()` in the resolve environment.
- Where a config genuinely wants randomness, derive it from a local PRNG seeded from `hash(mapName, optionVector)` — reproducible and independent of call order.
- Give map code a seeded `ctx.Random()`, never raw `math.random`. Seed from a replicated match id.
- Add `math.randomseed` to the sandbox denylist, or at minimum `Warn` on call.

**Settle it first:** log `math.random()` at the top of `script.init()` on host and client, compare across two launches. Five minutes, and it also tells you whether `enableSandbox` is on.

**Existing determinism discipline to preserve:** the deliberate sorts at `mapUtils.lua:75-79` and `:148-154` (props/decals by `blueprintPath`) and `gameUtils.lua:215` (army names) are **load-bearing** and must survive any refactor. `table.save` sorting keys (`table.lua:236`) is why regenerated `_data.lua` files are stable across runs.

---

## 10. Migration

### 10.1 What loads unchanged

Everything, if three invariants hold:

1. `Manifest.Load` returns a **default manifest** when no `map.json` exists — `resourceCategories = {"alloys"}`, no options, no layers.
2. `Resolve` with an empty option vector and no layers is the **identity function**.
3. The `_data.lua` merge stays exactly where it is, running *before* resolve.

Then a map with no manifest produces byte-identical `MapData` to today.

One behavioural change to check first: the marker-category hoist means a map where `markers.Alloys` exists but `.resource` is falsy previously created no alloy prefab and now creates one (unused), shifting decal prefab IDs by one — **identically on both sides**, since the manifest is the same. Check the three base maps for a falsy `resource` before landing; if any has one, derive `resourceCategories` from the raw map data on both sides instead of hardcoding, which is still parity-safe.

### 10.2 The converter, per map

| step | action |
|---|---|
| 1 | Read `<Name>.sanmap`; read `<Name>_data.lua` under `setfenv(chunk,{})` (it's data-shaped, being machine-generated by `table.save(…, "MapData = ")` at `testUtils.lua:354`); apply `table.merged` exactly as `LoadMapData` does, so the converter sees precisely what the game sees. |
| 2 | Emit `anchors.json`: allocate ids in `(category, sorted name)` order, run `ProposeOrbits`, write columns. **Human review gate on the orbit report** — the one non-automatable step, one screen per map. |
| 3 | Emit `map.json`: `id` = folder name; `playableAreaKey` = whichever of `PlayableArea`/`Playable_Area`/`Playable` the map actually has; `prefabUnion.props` = sorted distinct `blueprintPath` from the sanmap. |
| 4 | Propose `profiles` from spawn-orbit structure. A map with a single 8-member spawn orbit gets `p8` and `p4`; `p2` requires a symmetric 2-subset to exist. |
| 5 | `Textures/`, `preview.png` untouched. Generate `preview_meta.json`. |
| 6 | Delete `<Name>_data_debug.lua` (no reader). Delete `<Name>_info.lua` after harvesting `description`/`version`. |
| 7 | **Leave `<Name>_data.lua` in place, unchanged.** It becomes layer 1. Zero risk, zero rewrite, keeps working for anyone mid-edit. Its `groups` are still the only source of unit groups. |

Special cases: flatten the nested `Maps\World_Domination\World_Domination\` folder — `map.json.id` pins identity so the path change breaks nothing. Display-name/folder-name divergence becomes explicit as `name` vs `id`.

### 10.3 Survival: 18 folders → 1

Everything survival needs is already parameterised. `StartSurvival(difficulty, enemyFactions)` takes exactly the two arguments a config would supply. **The only reason 18 duplicate folders exist is `ReadMapNameDifficulty` doing `string.find` on the folder name — because there is nowhere else to put a difficulty number.**

With an options channel that mechanism disappears:

```lua
mode = "survival",
modeOptions = { difficulty = 5, factions = { "EDA", "Chosen" } },
```

Delete `ReadMapNameDifficulty` and the keyword table, delete 17 map folders. **This is a deletion, and it is the cheapest large win in this document.** It also resolves the 6-vs-8 discrepancy — the tables carry 8 difficulties but only 6 folders ship; all eight become selectable.

Keep the folder-name sniff for one release as a fallback so external/older packs keep working. Diff the three `_data.lua` sets before deleting; if a variant has diverged, that difference becomes a config layer rather than a lost edit.

Two things survival must stop doing before it composes with anything:

- It forces mutual `SetEnemy` across all armies and replaces the playable area. Both are destructive global mutations. Move them behind explicit declarations (`diplomacy = "ffa_vs_wave"`, `playableAreaProfile = "…"`) so two modes can't silently fight over global state.
- It hardcodes map-specific names as module constants: `"Enemy_Intel"`, the `"l1701"` scout pattern, `Spawn_Marker_N`. Those belong in the manifest.

The genuinely good part — its **map↔script contract via named markers and groups** — is preserved verbatim and formalised as `contracts: ["survival.v1"]`, so the validator enforces at author time what survival currently discovers by erroring at runtime.

---

## 11. Implementation plan

### Tier 1 — ~60 lines, no engine change, every item fixes a live bug

Reordered in rev 2. Item 1 was retracted; items 5 and 8 were undercounted.

| # | Item | Files | Why first |
|---|---|---|---|
| 1 | **Close both `__index=_G` RPC holes** | `hostListeners.lua:12-14`, `hostSimpleEvent.lua:3-6` | One-packet remote crash of every player in the match. `rawget` + whitelist + debug gate. ~12 lines. **Ship before anything else in this document.** |
| 2 | **Marker-category hoist** — unconditional loop over a declared list | `mapUtils.lua:113-115` | The one change strictly required before anyone can safely touch map data. ~15 lines. |
| 3 | **Parity digest** in the `InitClient` payload, **broadcast not per-player** | `mapUtils.lua`, `script.lua:174-184` | Converts the worst class of bug in this codebase from undebuggable to a one-line log. ~60 lines. |
| 4 | **Reclaim % via `Economy.harvestMultiplier`** | `economy.lua:22, :82-87` + setter | Delivers goal 4 outright the moment any option channel exists. 2 lines + setter. |
| 5 | **`CreateArmies(lobbyData)` + `ActiveSlots`** | `gameUtils.lua:209`, `script.lua:160-166` | Unblocks player count with no engine change. Behaviour-identical until a profile opts in. Also fix `CreateArmy`'s `table.getn` bug **and** the pre-existing `NEUTRAL_CIVILIAN` off-by-one in `mappedArmyCount`. ~20 lines. |
| 6 | **Unify the playable-area lookup** across sim / AI / survival | `mapUtils.lua:188`, `AIFunctions.lua:4277`, `survival.lua:283` | Three subsystems currently resolve three different rectangles on the same map (§1.1). **Not** a fallback chain — one function, plus a validator rule. |
| 7 | **Rotation in group spawning** | `gameUtils.lua:386` **and `:453`** | Two sites, not one. Data is already in the map; `CreateUnit` already takes `orientation`. |
| 8 | **Assert `Tick == 0`** in `CreatePropPrefab` / `CreateResourceSpotPrefab` | `common/loading/*` | Closes the one residual prefab-parity hole an event script could reach at runtime. 2 lines. |

> **REV 2 — item "move `InitializePlayableArea` before `CreateArmies`" is removed from Tier 1.** The dependency check was correct (`PlayableAreaBarrierPrefabID` exists from `templateLoader.lua:180`; `CheckResourceSpots` is only called from `SetPlayableArea`) — but `mapUtils.lua:181` is the **only** call site, and `RunMapSetup` is **shared host/client code**. Moving that line into the host's `InitLobby` branch means **the client never initialises its playable area at all**: `playableArea` stays nil, no barriers render, every `IsInPlayableArea` answers wrong. The correct change is a move *plus* a new client call site *plus* an ordering decision against `script.lua:206` (the client runs `InitializePlacementGrid` **after** `RunMapSetup`, the opposite of the host). **~10 lines and two ordering decisions, not one line.** It belongs in Tier 3 with the resolve seam.

### Tier 2 — the single engine dependency

| # | Item | Why |
|---|---|---|
| 9 | **One `string gameOptions` field on `LobbyInformation`** | The gate on every user-facing goal. Everything in Tier 1 works without it; nothing ships without it. **Request immediately so it lands in parallel.** Check the 32-byte buffer at `functionWrappers.lua:246` first. |

### Tier 3 — the rework proper

| # | Item | Notes |
|---|---|---|
| 10 | **Resolve seam** — identity-when-empty, at `script.lua:156↔157` | ~70 lines. Everything later hangs off it. |
| 11 | **Manifest loader + channel applier** | ~150 lines. |
| 12 | **Anchor SoA + bitmask + symmetry orbits**, with a `transforms` view shim | ~120 lines. Port `RunMapSetup` to indexed iteration. |
| 13 | **Event bus + timer wheel + collider area triggers** | ~400 lines, 3 new files, ~10 one-line publish insertions. Zero engine change. **Do not wrap `triggers.lua` — replace it** (§1.2). |
| 14 | **Prop set filtering** | ~6 lines, zero runtime cost. |
| 15 | **Mode registry + survival as config; delete 17 folders** | Mostly deletion. High morale-per-line. |
| 16 | **AI playable-area integration + event-driven cache invalidation** | Only matters once 1 and 5 land; then it matters a lot. |
| 17 | **Finish live-unit reclaim** | Makes reclaim % apply uniformly. As much a balance decision as a code one. |
| 18 | **Interactive `HostProp` promotion** | **Defer.** Take the measurement first; opt-in covers the realistic cases at ~0.2 MB. |

**Bottom line:** items 1–8 are roughly sixty lines, need no engine change, and each fixes something broken right now. Item 9 is the only engine ask. Items 10–15 are the rework, worth starting once 9 has a delivery date — without an options channel, a lobby-toggleable event system has nothing to read.

---

## 12. Open questions

Each is cheap to settle and each changes a decision.

1. **Is `SecureInit` called with `enableSandbox = true` in shipping builds?** The only caller is native. Determines the entire trust model. *Settle: the engine/C# call site, or the `bootstrap.lua` copy in StreamingAssets referenced at `bootstrap.lua:7`.*
2. **Is the `32` at `functionWrappers.lua:246` generator-emitted or hand-written?** If hand-written, adding a `LobbyInformation` field silently corrupts memory. *Settle: read the generator.*
3. **Can lua reach sibling files of `mapPath` via `Engine.GetFileContent`?** Determines whether `map.json` can live beside the `.sanmap`. *Settle: one call to `Engine.FileExists(mapPath:gsub("[^/\\]+%.sanmap$", "map.json"))`.*
4. **Is `Engine.GetCommandLineOverrides()` callable during `init()`?** Determines whether template-time options are possible at all. *Settle: call it at `script.lua:90` and log.*
5. **Can the map editor persist an extra `anchorId` field on a transform, and maintain an id allocator?** If yes, `.sanmap` v4 and the whole reconciliation machinery both simplify. *Ask the C# editor owner.*
6. **Does `Engine.GetFileContent` normalise `..` in the `require` loader path?** `script.lua:139-148` does no normalisation. *Settle: call with a `..` path and read the log.*
7. **Why exactly are same-tick orders dropped after `CreateUnit`?** Worked around in three places by comment-and-`WaitTicks(1)`. Likely `orderManager.Update()` at `hostMain.lua:104` running after `CreateUnit`'s `HostSend` but before movement state is live. *Settle: instrument `HostIssueOrder`.*
8. **What is the real per-instance cost of a `HostProp` at 10k / 60k?** Gates the interactive-prop decision. *Settle: instantiate N dummies, read `collectgarbage("count")` and frame time.*
9. **Does anything between host steps 7 and 11 create an engine prefab?** If so, the host/client prefab-ID counters diverge before `RunMapSetup`. *Settle: read `_G.CreateUnit` and `Army:__init` for `Engine.CreatePrefab` calls.*
10. **Does `z + z′ = L − 2` generalise?** **Answered in rev 2: no.** Modal `x+x′`/`z+z′` across the library gives `W, L−2` on five maps but `W+1, L−3` on ~FFA-16P_Desert_2048, `W−3, L−5` on ~TEAM-8v8_Desert_2048, and `W, L` on Pandemonium — **none of the last three are inside §5.3's candidate grid `{w,w−1,w−2}/2 × {l,l−1,l−2}/2`.** Worse, ~14 shipped maps are **3-fold** (`~FFA-3P/6P/12P`, `~TEAM-2v2v2/3v3v3/4v4v4`), plus 5-fold and 7-fold maps, and the candidate list `rot180/mirrorX/mirrorZ/rot90` cannot match any of them. Combined with "the exporter REFUSES to auto-accept if any unmatched remain", **conversion blocks on roughly a quarter of the library.** And even Pandemonium is mixed — 209/282 alloy pairs at `x+x′=2048` but 83 at `2044`.
    **Fix §5.3:** replace the hardcoded candidate list with a general orbit fitter — cluster the centres implied by every pair, take the mode, verify closure — supporting `rotN` for N ∈ {2..8}; and make unmatched anchors a **warning that marks them `kind="none"`** rather than a refusal, since a singleton is already a legal partition member by §5.3's own rules.

### Opened by the adversarial review

| # | Question | Settle it by |
|---|---|---|
| U1 | **Do navmap/grid modifiers honour the prefab instance's rotation?** Decides whether rotated shatter zones are possible at all. | Instantiate one `PlayableAreaBarrier` at 45° with a 40×5 size; `Engine.NavmapLineCast` both diagonals. Ten minutes. |
| U2 | Does a unit **spawned inside** an existing volume generate a `VolumeEnter`, or only a boundary crossing? | `CreateUnit` inside a live trigger volume, log `GetVolumeCollisionEventsForWorld`. |
| U3 | Does `SetVolumeColliderSize` re-broadphase and emit deltas for newly-enclosed units? | Resize a volume over stationary units, log the event stream. |
| U4 | Is there a transport/packet cap on `Engine.CreateCustomCommandSingle`? The lua side has none. | Send a 64 KB `InitClient` on a 2-player LAN game. |
| U5 | **Is `SecureInit` called with `enableSandbox = true`?** Now load-bearing — decides whether §6.4(c) is a DoS or an RCE. | Log `type(io)` at `script.init()`. |
| U7 | Does a deeply nested config table blow the C stack past `pcall`'s reach? | 200k-deep table through `table.deepCopy` under `pcall`. |

### §5.4 anchor reconciliation — the silent-corruption case, found

**Swap two anchors' positions but keep their names.** Pass 1 matches both by name and updates positions — **ids follow the names, not the places.** A config that disabled "the northern mex" now disables the southern one, at **epoch 3, with no warning**, and every pinned pack silently plays a different map.

**Delete one and add another nearby** in the same edit: pass 1 misses (name gone), pass 2 hits if within 0.5 — the new anchor **inherits the deleted one's id**, again silently.

So §5.4's stated rule ("id reuse is the only thing that can silently corrupt a config") is incomplete: **id retention across a semantic move corrupts identically.**

**Fix:** require passes 1 and 2 to **agree** before keeping an id. If name-match and position-match disagree about which sidecar entry a transform corresponds to → tombstone, new id, epoch bump. Both cases become loud, at the cost of one extra epoch bump in a genuinely ambiguous edit — the correct trade.

### What survived the attack unchanged

Worth knowing, because these are the load-bearing parts: **§1.2** (the `area:contains` bug — confirmed at all three sites, worse than described); **§7.2's ordering claim** (traced end to end, holds exactly — only the size and cost figures were wrong); **§7.3's prefab-parity fix** (no counterexample constructible within the design's own rules — and the two hazards specifically probed, pack-contributed unit types and generated wreckage prefabs, turn out to be **already closed by construction**, since both are created unconditionally in `LoadAllTemplates` before any map data is read); **§1.4's one-line player-count fix**; **§1.8's RPC holes**; **§8.2's entire event-source inventory**; **§8.4's reclaim chokepoint** and all four sub-recommendations; and **§9.3's "deterministic by accident"** (zero `randomseed` hits tree-wide).

---

## Appendix — verified claim index

Everything asserted above, by file.

| Claim | Location |
|---|---|
| lua merge over the sanmap, areas/chains/markers/groups only | `common/mapUtils.lua:49-69` |
| `table.merged` is deep, t2-wins, non-mutating, purely additive | `common/utilities/table.lua:31-58` |
| Prefab-ID parity requirement, stated in-source | `common/loading/resourceSpotTemplateLoader.lua:10-13` |
| Host creates unused decal prefabs to keep the counter aligned | `common/mapUtils.lua:161-163` |
| Alloy prefab creation gated on map content | `common/mapUtils.lua:110-112` |
| Prop prefabs created unconditionally, sorted by path | `common/mapUtils.lua:74-92` |
| Playable-area key hardcoded to `"PlayableArea"` | `common/mapUtils.lua:188` |
| Shipped maps define `Playable_Area` / `Playable` | `The_Forge_data.lua:756`, `Two_Step_Shuffle_data.lua:696`, `World_Domination.sanmap` |
| `area:contains` does not exist; `Area:Contains` takes float2 | `triggers.lua:144`, `objectives.lua:315`, `hostFunctions.lua:99` vs `common/area.lua:78` |
| Engine reports volume enter/exit deltas to lua | `constructionManager.lua:5`, `intelManager.lua:10`, `targeterManager.lua:14` |
| Every unit carries a ConstructionCollider | `unitTemplateLoader.lua:392-400` |
| A prefab can be registered after LoadAllTemplates | `common/mapUtils.lua:92` |
| `CreateArmies` re-reads lobby info itself | `common/gameUtils.lua:221` |
| Slot count from sorted `MapData.armies` keys | `common/gameUtils.lua:213-219` |
| `Spawn/<ArmyName>` coupling | `common/gameUtils.lua:359` |
| `CreateArmy` auto-id uses `table.getn` on a hash table | `host/hostFunctions.lua:22` |
| `MirrorPostion` computes `(W−x, L−z)`; one caller, a debug dumper | `common/mapUtils.lua:233`, `host/testUtils.lua:255` |
| Reclaim value = `floor(cost.alloys / 2)` at template load | `common/systems/templateLoader.lua:412` |
| Reclaim payout formula | `host/props/propsClasses/wreckageClass.lua:89-93` |
| `income = {}` allocated per tick per reclaimed wreck | `wreckageClass.lua:88` |
| Wrecks auto-delete after 180 s | `wreckageClass.lua:53-56` |
| Live-unit reclaim pays nothing, creates no wreck | `reclaimable.lua:61-86` |
| Harvest bypasses `SetUserMultiplier` | `resourceEntity.lua:265`, `economy.lua:83-84` |
| `SetPlayableArea` is a real runtime resize | `hostPlayableAreaManager.lua:52-67` |
| Dash clamp reads raw map data | `unitsBaseClass.lua:1460-1472` |
| AI scans for literal area keys, caches 10 s | `AIFunctions.lua:4277-4325` |
| `MapPopulate`/`MapStart` defined, zero call sites | `maps/defaultMap_script.lua:3,7` |
| `LobbyInformation` = `{playersInformation, mapPath}` | `engineClasses.lua:86-88` |
| Hardcoded 32-byte LobbyInformation buffer | `functionWrappers.lua:246` |
| `InitClient` payload is `json.encode`'d, arbitrary | `commands.lua:1044`, `script.lua:172-181` |
| `Mods_Active` ships as `{"/"}`, nothing appends | `common/systems/import.lua:99-101` |
| Import replace + append scan | `common/systems/import.lua:140-165` |
| `Import` env metatable is `{__index=_G}` | `common/systems/import.lua:107-109` |
| RPC fallthrough to `_G` | `hostListeners.lua:12-14`, `hostSimpleEvent.lua:3-6` |
| Thread removal is `table.remove` in-bucket | `common/systems/threads.lua:266` |
| `HostGameTrigger` live sites (economy energy) | `economy.lua:24-25, :177, :179`, `unitsBaseClass.lua:354-355` |
| `Army:SetUnitStat` is the single stat mutation point | `host/systems/army.lua:172` |
| `ParseTagsFromString` evaluates in a restricted env, cached | `common/systems/tags.lua:198-213` |
| Civilian armies forced neutral, `IsAlive()` false | `gameUtils.lua:326-329`, `army.lua:212` |
| Survival difficulty from folder-name `string.find` | `host/survival/survival.lua:754-786` |
| Same-tick orders after `CreateUnit` are dropped | `survival.lua:530-531`, `showcase_script.lua:114-115` |
| `SpawnSubGroup` discards rotation | `common/gameUtils.lua:386` |
| Map props instantiated as bare prefabs | `common/mapUtils.lua:98` |
| `math.randomseed` never called | grep, whole tree |
| Sandbox whitelist contents | `bootstrap.lua:9-118` |
| Deliberate determinism sorts | `mapUtils.lua:75-79, :148-154`, `gameUtils.lua:215`, `table.lua:236` |
| Symmetry: x+x′=W, z+z′=L−2, 9/9 exact | `~TEAM-1v1_Tropical_256_47940.sanmap` |
| World_Domination: 8 armies, 96 alloys, key `"Playable"` | `World_Domination.sanmap` |
| Pandemonium Isthmus: 2048², 16 armies, 282 alloys | `Pandemonium Isthmus.sanmap` |
