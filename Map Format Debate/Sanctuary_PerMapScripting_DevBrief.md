# Dev Brief: A Minimal Core Hook to Unlock Real Mod Support

**From:** Kiliax / SanGen map generator project
**Re:** One tiny, low-risk change unlocks a full mod-loading system we build and maintain.

**Timing:** raising this now due to the Aug 20 Kickstarter close. **Top pro:** likely the last easy merge window before MP opens and focus shifts to the ~2-year release cycle — after that, quick merges get much harder. **Top con:** MP opens to backers the same day, so any gameplay mod we ship before the integrity layer (§4) exists reaches real players sooner than ideal. Plan: merge the hook now (inert either way); hold the gameplay mod itself until §4 is addressed or we accept the interim risk.

---

## 1. The ask

Add one guarded line to `engine/LJ/lua/script.lua`, in `init()`, right after it requires the import system:

```lua
require("common/systems/import.lua")

-- NEW: hands off to our mod resolver, if/when it exists. No-ops until we ship
-- common/systems/modResolver.lua, so this is safe to merge today.
if Engine.FileExists(libPath.."common/systems/modResolver.lua") then
    Import("common/systems/modResolver.lua").ResolveMods()
end
```

Whole core-code ask: additive, gated on a file that doesn't exist yet, touches nothing else. Everything past it (resolver, config format, ordering, our first mod) is ours to build with no further core changes.

## 2. How we got here

Tracing the host boot (`script.lua → InitLobby() → LoadMapData() → gameUtils.CreateArmies() → hostMain.Start()`) to solve a unit-spawn problem, we confirmed `MapPopulate()`/`MapStart()` per-map hooks are dead code — nothing calls `Import()` on a per-map `_script.lua`.

We also found `common/systems/import.lua` already has a full mod resolution system (`replace/<path>`, `append/<path>`, checked against `Mods_Active`) — but the piece that enables a mod is an unimplemented stub:

```lua
--[[
function ResolveModOrder()
    ...
    Good luck to whoevers gonna write this i suppose!
end
--]]

Mods_Active = { "/", }  -- nothing else in the codebase ever appends to this
```

General modding (separate from the special-cased `AI/mods/` system) has no activation path today — built, but disconnected. Fixing that is a superset of the per-map-script fix: our unit-spawn problem becomes one mod on top of it, with zero further core changes after this.

**Note on the dev's own suggestion** ("you call CreateArmy in gameUtils.lua"): that pointed us to real, working spawn code — `SpawnGroup`/`SpawnGroupUnit`/`SpawnSubGroup`, which read army group data from a companion `<mapDataName>_data.lua` file. It's legitimate, not dead code. But nothing calls it automatically on a normal map load — it's wired up only in Survival mode's core script and a manual debug console command, neither of which is a per-map, automatic-on-load trigger we can reuse without inheriting Survival mode's other rules or requiring a human to type a debug command. Our hook creates that missing automatic trigger; our future mod would call the dev's own `SpawnGroup` functions from it. The two ideas combine, not compete.

## 3. What we'll build ourselves

- **`common/systems/modResolver.lua`** — populates `Mods_Active` in order before other `Import()` calls, implementing the ordering rules (`Requires`/`Load_Before`/`Load_After`, circular-dependency handling) already sketched in `import.lua`'s comments.
- **Enabled-mods config** — plain, inspectable file listing active mods (ID, version, path), kept simple so a later integrity layer (§4) can read it without us reworking the resolver.
- **Visual-only mods** — safe to ship immediately. Client/host split is already enforced (`IsClient`/`IsHost`, client never simulates — confirmed in `mapUtils.lua`'s `RunMapSetup(false)` client path), so client-presentation-only mods can't touch simulation state. No coordination needed.
- **Our first gameplay mod** — `append/script.lua`, wrapping `InitLobby()` to add the `MapPopulate()`/`MapStart()` call, as an opt-in mod instead of a core patch. Solves the original problem and proves the system end to end.

## 4. What we're NOT building, and why

Gameplay mods need to be provably identical across players — you're client-server with a player-hosted authoritative host. Verifying that (hashing active gameplay mods, comparing at lobby start, deciding what happens on mismatch) is your networking/trust call, not ours to invent.

This risk already exists today independent of us — a host can already hand-edit core Lua to cheat, which we've done locally throughout this project. And it's additive, not blocking: we ship the resolver, visual mods, and our gameplay-mod proof of concept now; verification can come later, reading the same mod metadata we're already producing, with no rework on our end.

## 5. Sequencing

1. You merge the one-line hook in §1 whenever convenient — inert until our file exists, no rush, no risk.
2. It ships in your next routine update.
3. We ship `modResolver.lua`, the config format, and the `append/script.lua` mod. Per-map scripting (and any future mod) works for real from here.
4. Whenever able, you add gameplay-mod integrity verification at the lobby layer — we're not blocked on this and it shouldn't require reworking what's shipped.

## 6. Concrete asks

1. Review and merge the one-line hook in §1.
2. Confirm `libPath.."common/systems/modResolver.lua"` is the right convention, or tell us your preferred location for mod files.
3. Rough timeline for §4 (integrity verification), so we can set expectations with our users about when gameplay mods are multiplayer-safe vs. single-player/local-testing-only.

## Appendix: file:line reference

| Fact | Location |
| --- | --- |
| Host boot order, `init()` / `InitLobby()` | `engine/LJ/lua/script.lua:7-208` |
| `Import()` implementation, environment-capture behavior | `engine/LJ/lua/common/systems/import.lua:122-226` |
| Mod resolution logic (`replace`/`append`) | `engine/LJ/lua/common/systems/import.lua:134-165` |
| Unimplemented mod-order resolver | `engine/LJ/lua/common/systems/import.lua:66-92` |
| `Mods_Active` hardcoded, never appended to elsewhere | `engine/LJ/lua/common/systems/import.lua:99-101` |
| Real spawn functions (`SpawnGroup`/`SpawnGroupUnit`/`SpawnSubGroup`), only called by Survival mode and debug tool | `engine/LJ/lua/common/gameUtils.lua`; callers: `host/survival/survival.lua:457`, `host/testUtils.lua` (`TestUtils.SpawnDebugUnitGroup`) |
| Special-cased AI-mod system (different mechanism) | `engine/LJ/lua/common/gameUtils.lua` (`aiSettings.modDirectory`), `AI/mods/AI-Sanctuary` etc. |
| Client never simulates (confirms visual-mod safety) | `engine/LJ/lua/script.lua:189-208` (`RunMapSetup(false)` on client path) |
| Dead `MapPopulate()`/`MapStart()` hooks (original problem) | `engine/LJ/lua/maps/defaultMap_script.lua`; working analog `maps/showcase_script.lua`, called via `host/testUtils.lua:544` |
