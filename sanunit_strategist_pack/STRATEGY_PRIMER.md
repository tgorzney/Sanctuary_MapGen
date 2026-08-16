# Strategy Primer — method for turning stats into advice

## The DPS / cost-efficiency method
1. `DPS = damage × muzzleSalvoSize / reloadTime` for each weapon; sum weapons
   on a unit for combined DPS, but keep AA and anti-surface weapons separate
   if a unit has both (they answer different questions).
2. Never fold in `DeathExplosion` — it's one-shot on death, not sustained output.
3. Cost efficiency = DPS ÷ alloy cost (or ÷ total cost if energy is the binding
   constraint that game). HP efficiency = HP ÷ alloy cost. A unit can be
   DPS-inefficient but HP-efficient (a tank) or the reverse (a glass-cannon
   raider) — state both when comparing two units, don't collapse to one number.
4. Range matters as much as DPS: a lower-DPS unit that outranges its target
   wins a straight fight by kiting. Always compare `rangeMax` when a matchup
   question is about direct combat, not just damage.
5. Time-to-kill a target = target HP ÷ attacker DPS; time-to-die = attacker HP
   ÷ target DPS. A simple 1v1 sim is just comparing these two numbers,
   adjusted for range (the longer-ranged unit gets free hits before the
   shorter-ranged one is in range at all — subtract that windup time).
6. For AoE weapons (`damageRadius > 0`), efficiency against groups scales with
   how many units the radius can plausibly hit — call this out qualitatively
   (e.g. "great vs a blob of T1 raiders, wasted vs a single sniper") rather
   than trying to compute a precise multi-target number without more info
   about formation spacing, which isn't in the unit data.

## Faction roster shape
All three factions (EDA, Chosen, Guard) mirror each other structurally: T1/T2/T3
tank, raider/fast unit, mobile artillery, MAA, engineer, land scout on the
land side; bomber, AA fighter, air scout, gunship, transport, drone carrier on
the air side; frigate (+Chosen alone gets a T3 battleship) on the naval side;
and point defence, AA, engineering station, factories (land/air/naval), tech
centres, alloy/energy production+storage, radar/sonar, shield on the structure
side. T4 diverges into faction-specific experimentals (see `UNITS_*.md` T4
entries). Because playability is so uneven (see `UNITS_STATUS.md`), the
*practical* roster available in a real game is much thinner than the template
count suggests — always check status before comparing "what EDA has vs Chosen"
since a template existing doesn't mean it's usable.

## Tag glossary (selected, strategy-relevant — see `common/systems/tags.lua`
for the full list; ask the live file if a tag here isn't enough context)
- `ANTI_SURFACE` / `ANTI_AIR` / `ANTI_NAVAL` / `ANTI_DRONE` — what a weapon
  targets, independent of what layer the unit itself occupies.
- `DIRECT_FIRE` vs `INDIRECT_FIRE` — direct needs line of sight and travels
  straight/low-arc; indirect (`HighArc` solver) arcs over obstacles, usually
  artillery, usually has a `rangeMin` (can't hit close targets).
- `ARTILLERY` / `SNIPER` — long range, low rate of fire, high alpha damage
  per shot; the strategic use case is poke-from-outside-response-range, not
  sustained DPS racing.
- `STEALTH_FIELD` — hides nearby units from enemy vision/radar (not the unit's
  own detectability necessarily — check the specific unit's `stealthRadius`).
- `SHIELD` — the unit/structure projects a bubble shield (see its `defence.shields`
  block for max HP, radius, regen) that absorbs damage to itself and often
  nearby units before their own HP is touched.
- `TRANSMITTER` — buffs nearby units via an adjacency-style aura (mechanic
  not represented in unit stat fields — treat as qualitative).
- `BUILDABLE_BY_T{1,2,3}_ENGINEER` vs `BUILDABLE_BY_T{1,2,3}_FACTORY` — engineers
  build structures/support; factories build mobile combat units. A unit/
  structure can be gated behind either or both.
- `CAPTURE` / `HARVEST` — order availability (capturing enemy structures,
  harvesting wreckage/resources), not combat stats.
- `TECH1`–`TECH5` — tier gates for what factories/tech centres can build; note
  the many tag/displayName tier mismatches in `DATA_QUALITY_NOTES.md`.

## Timing / economy reasoning framework
- **buildTime** scales with `buildPower` of the builder: actual seconds to
  complete = `economy.buildTime ÷ buildPower_of_builder`. A T1 engineer
  (buildPower 5) building something with `buildTime 100` takes 20s; a T3
  engineering station (buildPower 40) takes 2.5s for the same buildTime value.
  Always ask/confirm which builder before quoting a wall-clock time.
- **Economy timing**: alloy/energy extractors and generators list a flat
  production rate per second (e.g. T1 extractor 1 alloy/s, T3 extractor
  10 alloy/s) — compare against a unit's alloy cost to get "seconds of income
  this unit represents," a decent proxy for whether a purchase is affordable
  at a given economy size.
- **Upgrade chains**: most T1→T2→T3 economy/intel/factory structures have an
  explicit `upgradesTo` field pointing at the next tier's tpId — when advising
  a build order, follow this chain rather than assuming a fresh build.
- **Storage vs. production**: alloy/energy *storage* buildings raise the cap
  (buffer against stalls/spikes) but don't raise income — don't conflate the
  two when advising "how do I fix my economy."

## Cross-faction comparison caveat
Numerically the three factions' base T1 units are close cousins (near-
identical costs, similar HP/DPS bands) — differences show up more in tier-2/3+
kit shape (Guard's grenade bot/indirect-fire leaning, Chosen's hover-mobility
leaning, EDA's amphibious-engineer leaning) and in which units are actually
`true`/playable per faction, which varies more than the raw stat design does.
Lead a matchup answer with playability, not raw stats.
