# Data Quality Notes — read before treating an oddity as a real balance choice

Found while reading all ~283 `.santp` files (2026-08). These are quirks in the
source data itself, not features of this pack. When the user asks "why does X
do this," check here first.

## Mislabeled `unitTypeName` (copy-paste leftovers)
Several air/naval factories carry their **land** factory's internal type name
even though tags/class/displayName correctly identify them as air or naval:
- Chosen: ucs1512, ucs1513, ucs2512, ucs3512 (air factories say `...LandFactory`)
- EDA: ues2512, ues3512 (air factories say `EDAT2/T3LandFactory`)
- Guard: ugs2512, ugs3512 (air factories say `GuardT2/T3LandFactory`)
- Tech centres: ucs1801/1802, ucs2801/2802/2803, ues1801/1802, ues2801/2802,
  ugs1801/1802/1803, ugs2801/2802/2803 mostly say `...LandTechCentre` or
  `...T1/T2LandTechCentre` regardless of actual domain (land/air/naval) or
  displayed tier.
- **ugl2806** ("Relay", Guard Mobile Transmitter) is the worst case: its
  `unitTypeName` reads `ChosenT2MobileShieldBooster` and it carries a
  **CHOSEN** tag instead of GUARD — a straight copy from the Chosen faction
  file. Don't treat this unit as cross-faction; it's a Guard unit with a bug.

## Tier tag vs. displayName mismatches
Nearly every "Tier 2" and "Tier 3" Tech Centre across all three factions has
its `tags` TECH level one lower than its displayName says (e.g. ucs1801
displays "Tier 2: Land Tech Centre" but tags TECH1; ucs2801 displays "Tier 3"
but tags TECH2). This pattern repeats identically in EDA (ues18xx/28xx) and
Guard (ugs18xx/28xx). Likely a batch-generation artifact, not intentional
balance. **uga3011** is a standalone case: tpId slot suggests low tier but its
cost/HP/DPS (900/18000, 6000 HP, 250 DPS) matches T3 heavy-gunship siblings
while its tag says TECH1 — treat it as unfinished, not a real T1 unit.

## Confirmed: no T5-tier unit exists anywhere in this build, any faction (verified 2026-08-17)
A live game-testing session exhaustively checked for a `T5`/`5xxx` tpId and
found zero, anywhere:
- `common/units/availableUnits.lua` — the authoritative playability list.
- `common/units/unitsTemplates/` (loose per-tpId folders) — every
  faction/domain (Chosen `ucl`/`uca`, Guard `ugl`/`uga`, EDA `uel`/`uea`, plus
  structures `ucs`/`ugs`/`ues`) tops out at a `4xxx` tpId. No `5xxx` directory
  exists for any faction.
- `Gamedata/UnitsTemplates.sanpack` — confirmed genuinely empty (0
  central-directory entries), not a hidden source of higher-tier templates.
- `Gamedata/Gameplay.sanpack` (592 entries, fully inspected) — zero `t5`
  string matches; strategic-icon tiers only go up to `_t3_`.
- Not checked: `Gamedata/Units.sanpack` (1.35GB) — likely models/animations
  rather than Lua templates given the dedicated (empty) template pack above,
  so very unlikely to change the conclusion, but it's the one gap short of
  100% certainty.

**Highest validated tier is T4, every faction.** This also resolves the
"Tier 5" displayNames noted elsewhere in this pack: **ucl4005** (ChosenT4BotMega,
see `UNITS_CHOSEN.md`) and **uga4001** (GuardT4MothershipShatterer, see
`UNITS_GUARD.md`) both display "Tier 5" but are TECH4-tagged `4xxx` tpIds with
`NO_MODEL` status — an instance of the tier tag/displayName mismatch pattern
above, not evidence of a real T5 tier. Three known-good T4 tpIds for
spawn/matchup testing (already `true`/OK per `UNITS_STATUS.md`, no need to
duplicate their full records here): Chosen `ucl4004` (ChosenT4BotBig), Guard
`ugl4001` (GuardT4Bot), EDA `uel4001` (EDAT4RailgunSniper).

## Structures with combat-implying names/tags but NO `weapons` table
These read as weapon-bearing structures by name and tag, but their `.santp`
has no `weapons` array at all — treat any "expected" damage/range figure for
them as unknown, not zero and not to-be-inferred:
- Strategic launchers: ucs3102, ucs4102 (Chosen), ues3102, ues4101 (EDA
  — the "heavy artillery" one too), ugs3102, ugs4012, ugs4102 (Guard)
- Strategic/anti-projectile defense: ucs3402, ues3402 (Chosen/EDA), ugs3402 (Guard)
- Torpedo launchers (all factions, both tiers): ucs1301/2301, ues1301/2301, ugs1301/2301
- Drone launchers: ucs2102, ues2102, ugs2102
- Anti-drone defence: ucs2402, ues2402, ugs2402
- uga4801 (Guard T4 Mothership) — 50000 HP flying hull, zero weapons.
- uca2301/uea2301/uga2301 (T2 Torpedo Bombers, all 3 factions) — TORPEDO_BOMBER
  tag but no weapon defined.

## Naval tech tree is effectively non-functional
Every naval factory, frigate, sonar, and naval tech centre across all three
factions is `false` (mostly `NO_MODEL`, some `OK_PENDING_APPROVAL`). The
Chosen `ucn3001` Battleship additionally has a likely targeting bug: turret 3's
`layerWeaponLimits` is `Land` only (turrets 1–2 include `WaterSurface`) despite
the hull itself being a `WaterSurface` unit — it probably can't fire that
turret while afloat.

## Other one-off anomalies worth knowing
- **ugl3502** (Guard T3 Combat Engineer) uses movement type `"TracksWater"`,
  which isn't in the documented `MovementType` alias list (only
  `TracksLand`/`TracksSeabed`/`TracksAmphibious` are defined) — nonstandard value.
- **uea3701** ("Raven", EDA T3 Air Scout) has a weapon entry with `damage = 0`
  — present in the file but functionally unarmed.
- **uel3401** ("Chameleon", stealth+shield unit) and **uel4511** ("Grendel",
  harvester factory) are named/tagged for their special mechanic but carry
  none of the expected fields (no `shields`, no `stealthRadius`, no
  `construction`/harvest data) — bare unarmed shells consistent with their
  NO_MODEL status.
- **ues1111** (FreezeStation) has no faction tag, an order-of-magnitude larger
  cost/buildTime than everything else (15000/150000/15000s), a huge static
  hitbox, and zero weapons — reads as a scripted map object, not a normal
  buildable, matching its unique `BATTLE_NO_DAMAGE` status.
- **uws1000** (CryoTank) is UNCLAIMABLE, has no weapons/economy/construction,
  and is tagged GUARD despite being described as unfactioned — likely a
  neutral/scenario prop.

## What this means for strategy advice
When asked "what should I build to counter X," never suggest a unit from the
"no weapons table" or "NO_MODEL/BONE_MISSMATCH" lists above as if it were a
live option — always route through `UNITS_STATUS.md` first.
