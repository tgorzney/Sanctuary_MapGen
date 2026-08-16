# GUARD Faction — Full Unit Data

Prefix `ug` (air `uga`, land `ugl`, naval `ugn`, structures `ugs`), plus the
unfactioned `uws1000`. Cross-check `UNITS_STATUS.md` for playability. DPS =
damage × muzzleSalvoSize / reloadTime unless noted.

---

# AIR (uga)

## uga1001 — "Inertia", Tier 1: Bomber — TECH1 — playable: false (BONE_MISSMATCH)
Cost 60/1200, buildTime 240. HP 300, regen 1. Movement: Plane, speed 8. Vision 15.
Weapon: dmg 60, reload 1s, range 60, salvo 1 → DPS 60.
Tags: AIR, ANTI_SURFACE, BOMBER, BUILDABLE_BY_T1/2/3_FACTORY, DEMO_UI_ONLY, DIRECT_FIRE, GUARD, MOBILE, TECH1.

## uga1011 — "CRISPR", Tier 1: Gunship — TECH1 — playable: false (OK_PENDING_APPROVAL)
Cost 70/1400, buildTime 280. HP 350, regen 1. Movement: Gunship, speed 8, hovers. Vision 15.
Weapon: dmg 10, reload 0.3s, range 15, salvo 1 → DPS 33.33.
Tags: AIR, ANTI_SURFACE, BUILDABLE_BY_T1/2/3_FACTORY, DIRECT_FIRE, GUARD, GUNSHIP, MOBILE, TECH1.

## uga1201 — "Aerofoil", Tier 1: Anti-Air Fighter — TECH1 — playable: true (OK)
Cost 25/500, buildTime 100. HP 250, regen 1. Movement: Plane, speed 12. Vision 15.
Weapons: 2 identical turrets, dmg 25 each, reload 1s, range 25, targets Air/LandedAir → DPS 25 each, 50 combined.
Tags: AIR, ANTI_AIR, BUILDABLE_BY_T1/2/3_FACTORY, FIGHTER, GUARD, MOBILE, TECH1.
Role: cheap dedicated T1 interceptor, playable.

## uga1502 — "Shuttle", Tier 1: Transport — TECH1 — playable: false (NO_MODEL)
Cost 80/1600, buildTime 320. HP 400, regen 1. No weapons.
Tags: AIR, BUILDABLE_BY_T1/2/3_FACTORY, DEMO_UI_ONLY, GUARD, GUNSHIP, MOBILE, TECH1, TRANSPORT.

## uga1701 — "Tranceiver", Tier 1: Air Scout — TECH1 — playable: true (OK)
Cost 20/400, buildTime 80. HP 60, regen 1. Movement: Plane, speed 15. Vision 30, radar 50. No weapons.
Tags: AIR, BUILDABLE_BY_T1/2/3_FACTORY, GUARD, INTEL, MOBILE, RADAR, SCOUT, TECH1.
Role: playable, cost-efficient recon.

## uga2011 — "CAR-T", Tier 2: Gunship — TECH2 — playable: false (OK_PENDING_APPROVAL)
Cost 150/3000, buildTime 600. HP 850, regen 3. Vision 17.
Weapon: dmg 30, reload 2.5s, range 22, salvo 5 (burst) → DPS 60.
Tags: AIR, ANTI_SURFACE, BUILDABLE_BY_T2/3_FACTORY, DIRECT_FIRE, GUARD, GUNSHIP, MOBILE, TECH2.

## uga2101 — "Botnet", Tier 2: Drone Carrier — TECH2 — playable: false (OK_PENDING_APPROVAL)
Cost 300/6000, buildTime 1200. HP 1500, regen 3. No weapons (relies on unimplemented drones).
Tags: AIR, BUILDABLE_BY_T2/3_FACTORY, DEMO_UI_ONLY, DRONE_CARRIER, GUARD, GUNSHIP, MOBILE, TECH2.

## uga2301 — "Barometer", Tier 2: Torpedo Bomber — TECH2 — playable: false (OK_PENDING_APPROVAL)
Cost 150/3000, buildTime 600. HP 830, regen 3. No weapons present.
Tags: AIR, BUILDABLE_BY_T2/3_FACTORY, DEMO_UI_ONLY, GUARD, MOBILE, TECH2, TORPEDO_BOMBER.

## uga2502 — "Conveyer", Tier 2: Transport — TECH2 — playable: false (OK_PENDING_APPROVAL)
Cost 400/8000, buildTime 1600. HP 2000, regen 3. No weapons.
Tags: AIR, BUILDABLE_BY_T2/3_FACTORY, DEMO_UI_ONLY, GUARD, GUNSHIP, MOBILE, TECH2, TRANSPORT.

## uga2806 — "Broadcast", Tier 2: Flying Transmitter — TECH2 — playable: false (NO_MODEL)
Cost 150/3000, buildTime 600. HP 750, regen 3. No weapons — buff/support unit.
Tags: AIR, BUILDABLE_BY_T2/3_FACTORY, DEMO_UI_ONLY, GUARD, GUNSHIP, MOBILE, TECH2, UTILITY.

## uga3001 — "Impulse", Tier 3: Bomber — TECH3 — playable: false (BONE_MISSMATCH)
Cost 900/18000, buildTime 3600. HP 3800, regen 5. Vision 20.
Weapon: dmg 375, reload 1s, range 90 → DPS 375.
Tags: AIR, ANTI_SURFACE, BOMBER, BUILDABLE_BY_T3_FACTORY, DEMO_UI_ONLY, DIRECT_FIRE, GUARD, MOBILE, TECH3.

## uga3011 — "TALEN" (tpId slot displayName says "Tier 1" but stats are T3-caliber — data anomaly) — tag TECH1 — playable: false (NO_MODEL)
Cost 900/18000, buildTime 2400. HP 6000, regen 5.
Weapon: dmg 250, reload 1s, range 25 → DPS 250.
Tags: AIR, ANTI_SURFACE, BUILDABLE_BY_T3_FACTORY, DIRECT_FIRE, GUARD, GUNSHIP, MOBILE, TECH1.
Note: treat as unfinished/miscategorized heavy gunship, not a real low-tier unit.

## uga3201 — "Contrail", Tier 3: Anti-Air Fighter — TECH3 — playable: false (BONE_MISSMATCH)
Cost 250/5000, buildTime 1200. HP 2250, regen 5. Movement: Plane, speed 20. Vision 20.
Weapon: dmg 500, reload 1s, range 30, targets Air/LandedAir → DPS 500.
Tags: AIR, ANTI_AIR, BUILDABLE_BY_T3_FACTORY, FIGHTER, GUARD, MOBILE, TECH3.
Role: would be the premier AA counter if functional — very high single-hit damage.

## uga3701 — "Monitor", Tier 3: Air Scout — TECH3 — playable: false (OK_PENDING_APPROVAL)
Cost 110/2200, buildTime 440. HP 1050, regen 5. Movement: Plane, speed 27 (fastest scout). Vision 50, radar 100. No weapons.
Tags: AIR, BUILDABLE_BY_T3_FACTORY, GUARD, INTEL, MOBILE, RADAR, SCOUT, TECH3.

## uga4001 — "GuardT4MothershipShatterer" (displayName "Tier 5: Laser Bomber") — tag TECH4 — playable: false (NO_MODEL)
Cost 30000/300000, buildTime 30000. HP 540000 (massive). Movement type "Hovership" (non-standard/undocumented value). Vision 90.
Weapons: AOEBeam close-range, chargeTime 4.66s, dmg 600, damageBox instead of radius, reload 2s → DPS ≈300. DeathExplosion dmg 50000, radius 20 (one-shot).
Tags: AIR, ANTI_SURFACE, BUILDABLE_BY_T3_ENGINEER, DEMO_UI_ONLY, DIRECT_FIRE, GUARD, GUNSHIP, MOBILE, TECH4.
Note: multiple placeholder markers present — treat stats as unfinished, not balanced.

## uga4801 — "Gravity" (GuardT4Mothership) — TECH4 — playable: false (NO_MODEL)
Cost 15000/150000, buildTime 15000. HP 50000. Movement: Gunship, speed 7, airHover. Vision 30. **No weapons table at all** — pure fortress-HP hull, likely intended drone/support carrier flagship, template incomplete.
Tags: AIR, BUILDABLE_BY_T3_ENGINEER, DEMO_UI_ONLY, GUARD, GUNSHIP, MOBILE, TECH4.

---

# LAND (ugl)

## ugl0000 — Commander (GuardCommander) — special/TECH1 tag — playable: true (OK)
Cost 100000/1000000, buildTime 400000. HP 14000, regen 20. Movement: LegsLand, speed 1.8. Vision 20.
Construction: buildPower 5, range 10.
Weapons: 9-muzzle ring, dmg 100, reload 3s, range 22, salvo 3 → DPS 100. DeathExplosion dmg 5000, radius 15 (one-shot).
Tags: ALLOYS_PRODUCTION, ALLOYS_STORAGE, ANTI_SURFACE, CAPTURE, COMMAND, CONSTRUCTION, DIRECT_FIRE, ECONOMIC, ENERGY_PRODUCTION, ENERGY_STORAGE, GUARD, HARVEST, LAND, MOBILE, TECH1.

## ugl1001 — "Gimlet", Tier 1: Tank — TECH1 — playable: true (OK)
Cost 30/300, buildTime 120. HP 285, regen 1. Movement: TracksLand, speed 3.3. Vision 20.
Weapon: dmg 23.53, reload 1s, range 20 → DPS 23.53.
Tags: ANTI_SURFACE, BUILDABLE_BY_T1/2/3_FACTORY, DIRECT_FIRE, GUARD, LAND, MOBILE, TANK, TECH1.

## ugl1002 — "Vector", Tier 1: Raider (FastUnit) — TECH1 — playable: true (OK)
Cost 20/200, buildTime 80. HP 145, regen 1. Movement: LegsLand, speed 4, accel 100, rot 300°/s. Vision 15.
Weapon: dmg 6.25, reload 0.25s, range 15 → DPS 25.
Tags: ANTI_SURFACE, BOT, BUILDABLE_BY_T1/2/3_FACTORY, DIRECT_FIRE, GUARD, LAND, MOBILE, TECH1.

## ugl1101 — "Parabola", Tier 1: Mobile Artillery — TECH1 — playable: true (OK)
Cost 32/320, buildTime 128. HP 220, regen 1. Vision 18.
Weapon: HighArc, dmg 360, reload 4s, range 30, AoE 2 → DPS 90.
Tags: ARTILLERY, BUILDABLE_BY_T1/2/3_FACTORY, GUARD, INDIRECT_FIRE, LAND, MOBILE, TECH1.

## ugl1201 — "Pulse", Tier 1: Mobile Anti-Air — TECH1 — playable: false (OK, gated)
Cost 20/200, buildTime 80. HP 200, regen 1. Vision 20.
Weapon: continuous beam (beamLifetime -1), dmg 2, reload 1s, range 32, Air only → DPS 2 (tick-based, low nominal figure).
Tags: ANTI_AIR, BUILDABLE_BY_T1/2/3_FACTORY, GUARD, LAND, MOBILE, TECH1.

## ugl1501 — Tier 1: Engineer — TECH1 — playable: true (OK)
Cost 75/750, buildTime 300. HP 750, regen 1. Construction: buildPower 5, range 5. No weapons.
Tags: AMPHIBIOUS, BUILDABLE_BY_T1/2/3_FACTORY, CONSTRUCTION, ENGINEER, GUARD, HARVEST, LAND, MOBILE, TECH1.

## ugl1701 — "Monocycle", Tier 1: Land Scout — TECH1 — playable: true (OK)
Cost 10/100, buildTime 40. HP 40, regen 1. Movement: TracksLand, speed 4.4. Vision 20, radar 30. No weapons.
Tags: BUILDABLE_BY_T1/2/3_FACTORY, GUARD, INTEL, LAND, MOBILE, RADAR, SCOUT, TECH1.

## ugl2002 — "Torque", Tier 2: Raider (FastUnit) — TECH2 — playable: true (OK)
Cost 120/1200, buildTime 480. HP 1750, regen 3. Movement: Hover (land+sea), speed 3.2. Vision 15.
Weapon: dmg 51.39, reload 4s, range 22, salvo 2 (burst) → DPS 25.7.
Tags: ANTI_SURFACE, BUILDABLE_BY_T2/3_FACTORY, DIRECT_FIRE, GUARD, HOVER, LAND, MOBILE, TANK, TECH2.

## ugl2101 — "Wrecker", Tier 2: Grenade Bot — TECH2 — playable: true (OK)
Cost 150/1500, buildTime 600. HP 1000, regen 3. Movement: LegsLand, speed 2.8. Vision 18.
Weapon: HighArc, dmg 100, reload 4s, range 65/min 5, AoE 1.5, salvo 2 (2 groups) → DPS 50.
Tags: BOT, BUILDABLE_BY_T2/3_FACTORY, GUARD, INDIRECT_FIRE, LAND, MOBILE, TECH2.
Role: mobile mini-artillery, long range/min-range for a bot.

## ugl2201 — "Perforator", Tier 2: Mobile Anti-Air — TECH2 — playable: false (OK, gated)
Cost 100/1000, buildTime 400. HP 1100, regen 3. Vision 20.
Weapon: dmg 80, reload 1s, range 40, AoE 10 (unusually large for AA), Air only → DPS 80.
Tags: ANTI_AIR, BUILDABLE_BY_T2/3_FACTORY, GUARD, LAND, MOBILE, TECH2.

## ugl2501 — Tier 2: Engineer — TECH2 — playable: true (OK)
Cost 150/1500, buildTime 600. HP 1500, regen 3. Construction: buildPower 10, range 7.5. No weapons.
Tags: AMPHIBIOUS, BUILDABLE_BY_T2/3_FACTORY, CONSTRUCTION, ENGINEER, GUARD, HARVEST, LAND, MOBILE, TECH2.

## ugl2806 — "Relay", Tier 2: Mobile Transmitter — TECH2 — playable: false (OK_PENDING_APPROVAL)
Cost 180/1800, buildTime 720. HP 500, regen 3. No weapons.
**Data bug:** unitTypeName literally reads "ChosenT2MobileShieldBooster" and carries a CHOSEN tag instead of GUARD — copy-paste leftover from the Chosen faction file.
Tags: BUILDABLE_BY_T2/3_FACTORY, CHOSEN(sic), DEMO_UI_ONLY, LAND, MOBILE, TECH2, UTILITY.

## ugl3001 — "Auger", Tier 3: Tank — TECH3 — playable: true (OK)
Cost 600/6000, buildTime 2400. HP 8270, regen 5. Movement: LegsSeabed, speed 2. Vision 20.
Weapon: continuous plasma beam, dmg 25.64, reload 3s, range 34, AoE 1 → nominal DPS 8.55 (continuous-beam tick formula; real sustained output differs).
Tags: ANTI_SURFACE, BUILDABLE_BY_T3_FACTORY, DIRECT_FIRE, GUARD, LAND, MOBILE, SEABED, TANK, TECH3.

## ugl3002 — "Nitro", Tier 3: Raider — TECH3 — playable: true (OK)
Cost 250/2500, buildTime 1000. HP 2250, regen 5. Movement: LegsLand, speed 3.3. Vision 20.
Weapon: dmg 148, reload 2s, range 24, salvo 2 (2-muzzle group) → DPS 74.
Tags: ANTI_SURFACE, BOT, BUILDABLE_BY_T3_FACTORY, DIRECT_FIRE, GUARD, LAND, MOBILE, TECH3.

## ugl3101 — "Malware", Tier 3: Mobile Drone Carrier — TECH3 — playable: false (OK_PENDING_APPROVAL)
Cost 460/4600, buildTime 1840. HP 850, regen 5. Vision 20.
Weapon: HighArc, dmg 105, reload 6s, range 90/min 10, AoE 3, salvo 4 (4 groups) → DPS 70.
Tags: BUILDABLE_BY_T3_FACTORY, DEMO_UI_ONLY, DRONE_LAUNCHER, GUARD, INDIRECT_FIRE, LAND, MOBILE, TECH3.
Role: long-range (90) indirect launcher-carrier hybrid, disabled.

## ugl3501 — Tier 3: Engineer — TECH3 — playable: true (OK)
Cost 300/3000, buildTime 1200. HP 3000, regen 5. Construction: buildPower 20, range 10. No weapons.
Tags: AMPHIBIOUS, BUILDABLE_BY_T3_FACTORY, CONSTRUCTION, ENGINEER, GUARD, HARVEST, LAND, MOBILE, TECH3.

## ugl3502 — Tier 3: Combat Engineer — TECH3 — playable: false (OK_PENDING_APPROVAL)
Cost 500/5000, buildTime 2000. HP 6000, regen 5 (very tanky). Movement type literally "TracksWater" (nonstandard, not in documented MovementType enum). Construction: buildPower 40 (highest of Guard engineers), range 15. **No weapons table despite COMBAT_ENGINEER tag.**
Tags: AMPHIBIOUS, BUILDABLE_BY_T3_FACTORY, COMBAT_ENGINEER, CONSTRUCTION, DEMO_UI_ONLY, GUARD, HARVEST, LAND, MOBILE, TECH3.

## ugl4001 — Tier 4: Bot (experimental) — TECH4 — playable: true (OK)
Cost 16500/165000, buildTime 16500. HP 50000, regen 10. Movement: LegsSeabed, speed 2. Vision 25.
Weapon: triple-turret continuous beam, dmg 50/muzzle (2 fire together), reload 2s, range 35, AoE 1 → DPS ≈25 per single-muzzle basis (two muzzles fire simultaneously). DeathExplosion dmg 8000, radius 8.
Tags: ANTI_SURFACE, BOT, BUILDABLE_BY_T3_ENGINEER, DIRECT_FIRE, GUARD, LAND, MOBILE, SEABED, TECH4.

## ugl4011 — "Quasar", Tier 4: Bot — TECH4 — playable: true (OK)
Cost 20000/200000, buildTime 20000. HP 50000, regen 10. Vision 20.
Weapon: HighArc rapid barrage, dmg 300, reload 16s, range 90, AoE 3, salvo 20 (0.03s delay) → DPS 375. DeathExplosion dmg 8000, radius 8.
Tags: BOT, BUILDABLE_BY_T3_ENGINEER, GUARD, INDIRECT_FIRE, LAND, MOBILE, SEABED, TECH4.
Role: devastating 20-round indirect barrage, seabed-capable.

---

# NAVAL (ugn) + STRUCTURES (ugs) + uws1000

## ugn1001 — "Ballast", Tier 1: Frigate — TECH1 — playable: false (NO_MODEL)
Cost 150/1500, buildTime 600. HP 2000. Movement: WaterSurface, speed 6. No weapons.
Tags: BUILDABLE_BY_T1/2/3_FACTORY, FRIGATE, GUARD, MOBILE, NAVAL, TECH1.

## ugs1001 — "Stitcher", Tier 1: Point Defence — TECH1 — playable: true (OK)
Cost 100/1000, buildTime 100. HP 1000, regen 1. Weapon: dmg 80, reload 0.5s, range 25, target Land+WaterSurface → DPS 160. Threat 346.5.
Tags: ANTI_SURFACE, BUILDABLE_BY_T1/2/3_ENGINEER, DEFENCE, DIRECT_FIRE, GUARD, LAND, STRUCTURE, TECH1.

## ugs1201 — "Emitter", Tier 1: Anti-Air — TECH1 — playable: true (OK)
Cost 100/1000, buildTime 100. HP 1000, regen 1. Weapon: continuous beam, dmg 8, reload 1s, range 40, Air only → DPS ≈8 (floor value; continuous beam likely ticks differently). Threat 414.
Tags: ANTI_AIR, BUILDABLE_BY_T1/2/3_ENGINEER, DEFENCE, GUARD, STRUCTURE, TECH1.

## ugs1301 — "Hook", Tier 1: Torpedo Launcher — TECH1 — playable: false (OK_PENDING_APPROVAL)
Cost 250/2500, buildTime 250. HP 1650, regen 1. No weapons defined.
Tags: ANTI_NAVAL, BUILDABLE_BY_T1/2/3_ENGINEER, DEFENCE, DEMO_UI_ONLY, GUARD, STRUCTURE, TECH1.

## ugs1501 — Tier 1: Engineering Station — TECH1 — playable: true (OK)
Cost 150/1500, buildTime 150. HP 750, regen 1. Construction: buildPower 10, range 10, upgradesTo ugs2501.
Tags: BUILDABLE_BY_T1/2/3_ENGINEER, CONSTRUCTION, ENGINEERING_STATION, GUARD, LAND, STRUCTURE, TECH1.

## ugs1511 — Tier 1: Land Factory — TECH1 — playable: true (OK)
Cost 150/1500, buildTime 150. HP 5000, regen 1. Construction: buildPower 10, upgradesTo ugs2511.
Tags: BUILDABLE_BY_T1/2/3_ENGINEER, CONSTRUCTION, FACTORY, GUARD, LAND, LAND_FACTORY, STRUCTURE, TECH1.

## ugs1512 — Tier 1: Air Factory — TECH1 — playable: false (OK, gated)
Cost 150/1500, buildTime 150. HP 4000, regen 1. transport.storage 10. Construction buildPower 10, upgradesTo ugs2512.
Tags: AIR_FACTORY, BUILDABLE_BY_T1/2/3_ENGINEER, CONSTRUCTION, DEMO_UI_ONLY, FACTORY, GUARD, LAND, STRUCTURE, TECH1.

## ugs1513 — Tier 1: Naval Factory — TECH1 — playable: false (OK_PENDING_APPROVAL)
Cost 150/1500, buildTime 150. HP 6000, regen 1. transport.storage 10. Construction buildPower 10, upgradesTo ugs2513.
Tags: BUILDABLE_BY_T1/2/3_ENGINEER, CONSTRUCTION, DEMO_UI_ONLY, FACTORY, GUARD, NAVAL, NAVAL_FACTORY, STRUCTURE, TECH1.

## ugs1601 — Tier 1: Alloy Extractor — TECH1 — playable: true (OK)
Cost 50/500, buildTime 50. HP 600, regen 1. Production alloys 1/s. Upgrades to ugs2601.
Tags: ALLOYS_EXTRACTION, BUILDABLE_BY_T1/2/3_ENGINEER, CONSTRUCTION, ECONOMIC, GUARD, STRUCTURE, TECH1.

## ugs1602 — Tier 1: Alloy Storage — TECH1 — playable: true (OK)
Cost 80/800, buildTime 80. HP 800, regen 1. Storage alloys 1000.
Tags: ALLOYS_STORAGE, BUILDABLE_BY_T1/2/3_ENGINEER, ECONOMIC, GUARD, STRUCTURE, TECH1.

## ugs1611 — Tier 1: Energy Generator — TECH1 — playable: true (OK)
Cost 50/500, buildTime 50. HP 800, regen 1. Production energy 10/s.
Tags: BUILDABLE_BY_T1/2/3_ENGINEER, ECONOMIC, ENERGY_PRODUCTION, GUARD, STRUCTURE, TECH1.

## ugs1612 — Tier 1: Energy Storage — TECH1 — playable: true (OK)
Cost 80/800, buildTime 80. HP 800, regen 1. Storage energy 10000.
Tags: BUILDABLE_BY_T1/2/3_ENGINEER, ECONOMIC, ENERGY_STORAGE, GUARD, STRUCTURE, TECH1.

## ugs1614 — Tier 1: Solar Convertor — TECH1 — playable: false (NO_MODEL)
Cost 50/500, buildTime 50. HP 800, regen 1. No production field present — unimplemented.
Tags: BUILDABLE_BY_T1/2/3_ENGINEER, DEMO_UI_ONLY, ECONOMIC, GUARD, STRUCTURE, TECH1.

## ugs1701 — Tier 1: Radar — TECH1 — playable: true (OK)
Cost 60/600, buildTime 60. HP 300, regen 1. Radar radius 100. Upgrades to ugs2701.
Tags: BUILDABLE_BY_T1/2/3_ENGINEER, CONSTRUCTION, GUARD, INTEL, RADAR, STRUCTURE, TECH1.

## ugs1702 — Tier 1: Sonar — TECH1 — playable: false (OK_PENDING_APPROVAL)
Cost 60/600, buildTime 60. HP 300, regen 1. Sonar radius 90. Upgrades to ugs2702.
Tags: BUILDABLE_BY_T1/2/3_ENGINEER, CONSTRUCTION, DEMO_UI_ONLY, GUARD, INTEL, SONAR, STRUCTURE, TECH1.

## ugs1801/1802/1803 — Tier 2 Land/Air/Naval Tech Centres — tag TECH1 (mismatch) — playable: false (all OK_PENDING_APPROVAL)
Cost 360/3600, buildTime 360. HP 2500/2000/3000.
Tags include LAND_TECH_CENTRE / AIR_TECH_CENTRE / NAVAL_TECH_CENTRE + TECH_CENTRE.

## ugs1804 — Tier 1: Wall — TECH1 — playable: false (OK_PENDING_APPROVAL)
Cost 50/500, buildTime 50. HP 500, regen 1. Vision 0. No weapons.
Tags: BUILDABLE_BY_T1/2/3_ENGINEER, DEMO_UI_ONLY, GUARD, STRUCTURE, TECH1, WALL.

## ugs1805 — Tier 1: Airfield — TECH1 — playable: false (OK_PENDING_APPROVAL)
Cost 75/750, buildTime 75. HP 750, regen 1. transport.storage 10. Construction buildPower 5, upgradesTo ugs2805.
Tags: AIRFIELD, BUILDABLE_BY_T1/2/3_ENGINEER, CONSTRUCTION, GUARD, STRUCTURE, TECH1.

## ugs1806 — Tier 1: Transmitter — TECH1 — playable: false (OK_PENDING_APPROVAL)
Cost 150/1500, buildTime 150. HP 150, regen 1 (fragile). Construction buildPower 5, upgradesTo ugs2806.
Tags: BUILDABLE_BY_T1/2/3_ENGINEER, DEMO_UI_ONLY, GUARD, STRUCTURE, TECH1, TRANSMITTER.

## ugs2001 — "Nailer", Tier 2: Point Defence — TECH2 — playable: true (OK)
Cost 250/2500, buildTime 250. HP 2000, regen 2. Weapon: dmg 160, reload 2s, range 50, AoE 2 → DPS 80. Threat 1629.8.
Tags: ANTI_SURFACE, BUILDABLE_BY_T2/3_ENGINEER, DEFENCE, DIRECT_FIRE, GUARD, LAND, STRUCTURE, TECH2.

## ugs2101 — "Cycler", Tier 2: Artillery — TECH2 — playable: true (OK)
Cost 1000/10000, buildTime 1000. HP 3000, regen 2. Weapon: HighArc, dmg 1100, reload 10s, range 115, AoE 3 → DPS 110. Threat 1344.8.
Tags: ARTILLERY, BUILDABLE_BY_T2/3_ENGINEER, GUARD, INDIRECT_FIRE, STRUCTURE, TECH2.

## ugs2102 — "Assembly", Tier 2: Drone Launcher — TECH2 — playable: false (OK_PENDING_APPROVAL)
Cost 400/4000, buildTime 400. HP 2000, regen 2. No weapons.
Tags: BUILDABLE_BY_T2/3_ENGINEER, DEMO_UI_ONLY, DRONE_LAUNCHER, GUARD, STRUCTURE, TECH2.

## ugs2201 — "Triode", Tier 2: Anti-Air — TECH2 — playable: true (OK)
Cost 250/2500, buildTime 250. HP 2500, regen 2. Weapon: dmg 50/muzzle × 3 muzzles, reload 1s, range 50, AoE 5, Air only → burst ≈150 DPS. Threat 1203.
Tags: ANTI_AIR, BUILDABLE_BY_T2/3_ENGINEER, DEFENCE, GUARD, STRUCTURE, TECH2.

## ugs2301 — "Snare", Tier 2: Torpedo Launcher — TECH2 — playable: false (OK_PENDING_APPROVAL)
Cost 250/2500, buildTime 250. HP 2500, regen 2. No weapons.
Tags: ANTI_NAVAL, BUILDABLE_BY_T2/3_ENGINEER, DEFENCE, DEMO_UI_ONLY, GUARD, STRUCTURE, TECH2.

## ugs2401 — Tier 2: Shield — TECH2 — playable: true (OK)
Cost 150/1500, buildTime 150. HP 750, regen 2. Shield: max 10000, regen 100/s, regenDelay 3s, rechargeTime 20s, radius 12. Maintenance 100 energy/s. Upgrades to ugs3401.
Tags: BUILDABLE_BY_T2/3_ENGINEER, CONSTRUCTION, DEFENCE, GUARD, SHIELD, STRUCTURE, TECH2.

## ugs2402 — "Repellent", Tier 2: Anti-Drone Defence — TECH2 — playable: false (OK_PENDING_APPROVAL)
Cost 150/1500, buildTime 150. HP 750, regen 2. No weapons.
Tags: ANTI_DRONE, BUILDABLE_BY_T2/3_ENGINEER, DEFENCE, DEMO_UI_ONLY, GUARD, LAND, STRUCTURE, TECH2.

## ugs2501 — Tier 2: Engineering Station — TECH2 — playable: true (OK)
Cost 450/4500, buildTime 450. HP 2250, regen 2. Construction: buildPower 20, range 15, upgradesTo ugs3501.
Tags: BUILDABLE_BY_T2/3_ENGINEER, CONSTRUCTION, ENGINEERING_STATION, GUARD, LAND, STRUCTURE, TECH2.

## ugs2510 — Tier 2: Operating Theatre — TECH2 — playable: false (NO_MODEL)
Cost 900/9000, buildTime 900. HP 10000, regen 2. No weapons/construction — unimplemented support stub.
Tags: BUILDABLE_BY_T2/3_ENGINEER, DEMO_UI_ONLY, GUARD, LAND, STRUCTURE, TECH2.

## ugs2511 — Tier 2: Land Factory — TECH2 — playable: true (OK)
Cost 500/5000, buildTime 800. HP 10000, regen 2. Construction: buildPower 20, upgradesTo ugs3511.
Tags: BUILDABLE_BY_T1_FACTORY, BUILDABLE_BY_T2/3_ENGINEER, CONSTRUCTION, FACTORY, GUARD, LAND, LAND_FACTORY, STRUCTURE, TECH2.

## ugs2512 — Tier 2: Air Factory — TECH2 — playable: true (OK)
Cost 500/5000, buildTime 800. HP 8000, regen 2. transport.storage 25. Construction buildPower 20, upgradesTo ugs3512.
Note: unitTypeName mislabeled "GuardT2LandFactory".
Tags: AIR_FACTORY, BUILDABLE_BY_T1_FACTORY, BUILDABLE_BY_T2/3_ENGINEER, CONSTRUCTION, DEMO_UI_ONLY, FACTORY, GUARD, LAND, STRUCTURE, TECH2.

## ugs2513 — Tier 2: Naval Factory — TECH2 — playable: false (OK_PENDING_APPROVAL)
Cost 500/5000, buildTime 800. HP 12000, regen 2. Construction buildPower 20, upgradesTo ugs3513.
Tags: BUILDABLE_BY_T1_FACTORY, BUILDABLE_BY_T2/3_ENGINEER, CONSTRUCTION, FACTORY, GUARD, NAVAL, NAVAL_FACTORY, STRUCTURE, TECH2.

## ugs2601 — Tier 2: Alloy Extractor — TECH2 — playable: true (OK)
Cost 600/6000, buildTime 600. HP 2400, regen 2. Production alloys 4/s. Upgrades to ugs3601.
Tags: ALLOYS_EXTRACTION, BUILDABLE_BY_T2/3_ENGINEER, CONSTRUCTION, ECONOMIC, GUARD, STRUCTURE, TECH2.

## ugs2611 — Tier 2: Energy Generator — TECH2 — playable: true (OK)
Cost 1000/10000, buildTime 1000. HP 1600, regen 2. Production energy 200/s.
Tags: BUILDABLE_BY_T2/3_ENGINEER, ECONOMIC, ENERGY_PRODUCTION, GUARD, STRUCTURE, TECH2.

## ugs2701 — Tier 2: Radar — TECH2 — playable: true (OK)
Cost 250/2500, buildTime 250. HP 1250, regen 2. Radar radius 250. Upgrades to ugs3701.
Tags: BUILDABLE_BY_T2/3_ENGINEER, CONSTRUCTION, GUARD, INTEL, RADAR, STRUCTURE, TECH2.

## ugs2702 — Tier 2: Sonar — TECH2 — playable: false (OK_PENDING_APPROVAL)
Cost 250/2500, buildTime 250. HP 1250, regen 2. Sonar radius 180. Upgrades to ugs3702.
Tags: BUILDABLE_BY_T2/3_ENGINEER, CONSTRUCTION, DEMO_UI_ONLY, GUARD, INTEL, SONAR, STRUCTURE, TECH2.

## ugs2711 — Tier 2: Stealth Field — TECH2 — playable: false (OK_PENDING_APPROVAL)
Cost 150/1500, buildTime 150. HP 750, regen 2. No weapons.
Tags: BUILDABLE_BY_T2/3_ENGINEER, DEFENCE, DEMO_UI_ONLY, GUARD, STEALTH_FIELD, STRUCTURE, TECH2.

## ugs2801/2802/2803 — Tier 3 Land/Air/Naval Tech Centres — tag TECH2 (mismatch) — playable: false (all OK_PENDING_APPROVAL)
Cost 1200/12000, buildTime 1200. HP 5000/4000/6000.
Tags include LAND_TECH_CENTRE / AIR_TECH_CENTRE / NAVAL_TECH_CENTRE + TECH_CENTRE.

## ugs2805 — Tier 2: Airfield — TECH2 — playable: false (OK_PENDING_APPROVAL)
Cost 300/3000, buildTime 300. HP 3000, regen 2. transport.storage 25. Construction buildPower 5, upgradesTo ugs3805.
Tags: AIRFIELD, BUILDABLE_BY_T2/3_ENGINEER, CONSTRUCTION, DEMO_UI_ONLY, GUARD, STRUCTURE, TECH2.

## ugs2806 — Tier 2: Transmitter — TECH2 — playable: false (OK_PENDING_APPROVAL)
Cost 600/6000, buildTime 600. HP 300, regen 2. No weapons.
Tags: BUILDABLE_BY_T2/3_ENGINEER, DEMO_UI_ONLY, GUARD, STRUCTURE, TECH2, TRANSMITTER.

## ugs2807 — "Terminal" (transmitter variant, same internal type as ugs2806) — TECH2 — playable: false (NO_MODEL)
Cost 600/6000, buildTime 600. HP 300, regen 2. No weapons.
Tags: BUILDABLE_BY_T2/3_ENGINEER, DEMO_UI_ONLY, GUARD, STRUCTURE, TECH2, TRANSMITTER.

## ugs3101 — "Grinder", Tier 3: Artillery — TECH3 — playable: true (OK)
Cost 40000/400000, buildTime 40000 (very expensive endgame-tier). HP 13000, regen 3. Weapon: HighArc, dmg 5250, reload 10s, range 825 (very long), AoE 4 → DPS 525. Threat 63519.2.
Tags: ARTILLERY, BUILDABLE_BY_T3_ENGINEER, GUARD, INDIRECT_FIRE, STRUCTURE, TECH3.

## ugs3102 — "Autoclave", Tier 3: Strategic Launcher — TECH3 — playable: false (OK_PENDING_APPROVAL)
Cost 8000/80000, buildTime 8000. HP 8000, regen 3. No weapons defined.
Tags: BUILDABLE_BY_T3_ENGINEER, DEMO_UI_ONLY, GUARD, STRATEGIC, STRUCTURE, TECH3.

## ugs3201 — "Engraver", Tier 3: Anti-Air — TECH3 — playable: true (OK)
Cost 450/4500, buildTime 450. HP 5000, regen 3. Weapon: continuous beam, dmg 333.33/tick, reload 1s, range 60, Air only → DPS 333.33. Threat 1833.2.
Tags: ANTI_AIR, BUILDABLE_BY_T3_ENGINEER, DEFENCE, GUARD, STRUCTURE, TECH3.

## ugs3401 — Tier 3: Shield — TECH3 — playable: true (OK)
Cost 600/6000, buildTime 600. HP 3000, regen 3. Shield: max 20000, regen 150/s, regenDelay 5s, rechargeTime 30s, radius 20. Maintenance 250 energy/s.
Tags: BUILDABLE_BY_T3_ENGINEER, DEFENCE, GUARD, SHIELD, STRUCTURE, TECH3.

## ugs3402 — "Conservatory", Tier 3: Strategic Defense — TECH3 — playable: false (OK_PENDING_APPROVAL)
Cost 3500/35000, buildTime 3500. HP 3500, regen 3. No weapons.
Tags: ANTI_PROJECTILE, BUILDABLE_BY_T3_ENGINEER, DEFENCE, DEMO_UI_ONLY, GUARD, STRUCTURE, TECH3.

## ugs3501 — Tier 3: Engineering Station — TECH3 — playable: true (OK)
Cost 1200/12000, buildTime 1200. HP 6000, regen 3. Construction: buildPower 40, range 20.
Tags: BUILDABLE_BY_T3_ENGINEER, CONSTRUCTION, ENGINEERING_STATION, GUARD, LAND, STRUCTURE, TECH3.

## ugs3511 — Tier 3: Land Factory — TECH3 — playable: true (OK)
Cost 2000/20000, buildTime 4200. HP 20000, regen 3. Construction: buildPower 40.
Tags: BUILDABLE_BY_T2_FACTORY, BUILDABLE_BY_T3_ENGINEER, CONSTRUCTION, FACTORY, GUARD, LAND, LAND_FACTORY, STRUCTURE, TECH3.

## ugs3512 — Tier 3: Air Factory — TECH3 — playable: true (OK)
Cost 2000/20000, buildTime 4200. HP 16000, regen 3. transport.storage 40. Construction buildPower 40.
Note: unitTypeName mislabeled "GuardT3LandFactory".
Tags: AIR_FACTORY, BUILDABLE_BY_T2_FACTORY, BUILDABLE_BY_T3_ENGINEER, CONSTRUCTION, DEMO_UI_ONLY, FACTORY, GUARD, LAND, STRUCTURE, TECH3.

## ugs3513 — Tier 3: Naval Factory — TECH3 — playable: false (OK_PENDING_APPROVAL)
Cost 2000/20000, buildTime 4200. HP 24000, regen 3. Construction buildPower 40.
Tags: BUILDABLE_BY_T2_FACTORY, BUILDABLE_BY_T3_ENGINEER, CONSTRUCTION, FACTORY, GUARD, NAVAL, NAVAL_FACTORY, STRUCTURE, TECH3.

## ugs3601 — Tier 3: Alloy Extractor — TECH3 — playable: true (OK)
Cost 2000/20000, buildTime 2000. HP 8000, regen 3. Production alloys 10/s.
Tags: ALLOYS_EXTRACTION, BUILDABLE_BY_T3_ENGINEER, ECONOMIC, GUARD, STRUCTURE, TECH3.

## ugs3603 — Tier 3: Alloy Furnace — TECH3 — playable: true (OK)
Cost 2000/20000, buildTime 2000. HP 8000, regen 3. Production alloys 10/s, maintenance energy 1000/s.
Tags: ALLOYS_PRODUCTION, BUILDABLE_BY_T3_ENGINEER, ECONOMIC, GUARD, STRUCTURE, TECH3.

## ugs3611 — Tier 3: Energy Generator — TECH3 — playable: true (OK)
Cost 5000/50000, buildTime 5000. HP 8000, regen 3. Production energy 1000/s.
Tags: BUILDABLE_BY_T3_ENGINEER, ECONOMIC, ENERGY_PRODUCTION, GUARD, STRUCTURE, TECH3.

## ugs3701 — Tier 3: Radar — TECH3 — playable: true (OK)
Cost 700/7000, buildTime 700. HP 3500, regen 3. Radar radius 1000 (top-tier intel structure).
Tags: BUILDABLE_BY_T3_ENGINEER, GUARD, INTEL, RADAR, STRUCTURE, TECH3.

## ugs3702 — Tier 3: Sonar — TECH3 — playable: false (OK_PENDING_APPROVAL)
Cost 700/7000, buildTime 700. HP 3500, regen 3. Sonar radius 450.
Tags: BUILDABLE_BY_T3_ENGINEER, DEMO_UI_ONLY, GUARD, INTEL, SONAR, STRUCTURE, TECH3.

## ugs3805 — Tier 3: Airfield — TECH3 — playable: false (NO_MODEL)
Cost 500/5000, buildTime 500. HP 5000, regen 3. transport.storage 50.
Tags: AIRFIELD, BUILDABLE_BY_T3_ENGINEER, GUARD, STRUCTURE, TECH3.

## ugs4012 — "Tachyon", Tier 4: Artillery — TECH4 — playable: false (OK_PENDING_APPROVAL)
Cost 50000/500000, buildTime 50000. HP 20000, regen 10. **No weapons table at all** despite ARTILLERY tag and "Tier 4: Artillery" displayName — no reliable range/damage/DPS available. Class field oddly reads "GuardEnergyGenerator".
Tags: ARTILLERY, BUILDABLE_BY_T3_ENGINEER, DEMO_UI_ONLY, GUARD, STRUCTURE, TECH4.

## ugs4102 — "Hive Hole", Tier 4: Strategic Launcher — TECH4 — playable: false (NO_MODEL)
Cost 50000/500000, buildTime 50000. HP 20000. No weapons listed.
Tags: BUILDABLE_BY_T3_ENGINEER, DEMO_UI_ONLY, GUARD, STRATEGIC, STRUCTURE, TECH4.

## ugs4621 — "Crucible", Tier 4: Experimental Generator — TECH4 — playable: true (OK)
Cost 50000/500000, buildTime 50000. HP 30000, regen 9. **Production: alloys 500/s AND energy 50000/s** (dual-resource, the game's premier late-economy structure). No weapons.
Tags: ALLOYS_PRODUCTION, BUILDABLE_BY_T3_ENGINEER, ECONOMIC, ENERGY_PRODUCTION, GUARD, STRUCTURE, TECH4.

## uws1000 — "CryoTank" (unfactioned, GUARD-tagged) — TECH4 — playable: false (OK_PENDING_APPROVAL)
Cost 50000/500000, buildTime 50000. HP 10000, regen 2. No weapons/economy/construction. Tag **UNCLAIMABLE** (cannot be captured). Reads as a neutral/scenario map prop rather than a normal buildable unit.
Tags: DEMO_UI_ONLY, GUARD, STRUCTURE, TECH4, UNCLAIMABLE.
