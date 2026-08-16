# EDA Faction — Full Unit Data

Prefix `ue` (air `uea`, land `uel`, naval `uen`, structures `ues`). Cross-check
`UNITS_STATUS.md` for playability. DPS = damage × muzzleSalvoSize / reloadTime
unless noted.

---

# AIR (uea)

## uea1001 — "Vulture", Tier 1: Bomber — TECH1 — playable: false (BONE_MISSMATCH)
Cost 60/1200, buildTime 240. HP 300. Movement: Plane, speed 10. Vision 15.
Weapon: dmg 37.5, reload 5s, range 60, AoE 10, salvo 1 (fired via 8 simultaneous muzzles) → nominal DPS 7.5 (actual burst larger via 8-muzzle group).
Tags: AIR, ANTI_SURFACE, BOMBER, BUILDABLE_BY_T1/2/3_FACTORY, DEMO_UI_ONLY, DIRECT_FIRE, EDA, MOBILE, TECH1.

## uea1201 — "Shrike", Tier 1: Anti-Air Fighter — TECH1 — playable: true (OK)
Cost 25/500, buildTime 100. HP 250. Movement: Plane, speed 15. Vision 15.
Weapon: dmg 15, reload 1.2s, range 25, salvo 4, leadTarget true → DPS 50.
Tags: AIR, ANTI_AIR, BUILDABLE_BY_T1/2/3_FACTORY, EDA, FIGHTER, MOBILE, TECH1.
Role: main T1 AA counter, only playable T1 EDA air combatant.

## uea1502 — "Stork", Tier 1: Transport — TECH1 — playable: false (NO_MODEL)
Cost 80/1600, buildTime 320. HP 400. Movement: Gunship, speed 10. No weapons.
Tags: AIR, BUILDABLE_BY_T1/2/3_FACTORY, DEMO_UI_ONLY, EDA, GUNSHIP, MOBILE, TECH1, TRANSPORT.

## uea1701 — "Magpie", Tier 1: Air Scout — TECH1 — playable: true (OK)
Cost 20/400, buildTime 80. HP 60. Movement: Gunship, speed 18. Vision 30, radar 45. No weapons.
Tags: AIR, BUILDABLE_BY_T1/2/3_FACTORY, EDA, GUNSHIP, INTEL, MOBILE, RADAR, SCOUT, TECH1.
Role: only working T1 recon option, cheap/fast/fragile.

## uea2011 — "Wasp", Tier 2: Gunship — TECH2 — playable: false (OK_PENDING_APPROVAL)
Cost 150/3000, buildTime 600. HP 850. Movement: Gunship, speed 8; orbits target while firing.
Weapon: dmg 240, reload 4s, range 22 → DPS 60.
Tags: AIR, ANTI_SURFACE, BUILDABLE_BY_T2/3_FACTORY, DIRECT_FIRE, EDA, GUNSHIP, MOBILE, TECH2.

## uea2101 — "Locust", Tier 2: Drone Carrier — TECH2 — playable: false (NO_MODEL)
Cost 300/6000, buildTime 1200. HP 1500, regen 3. Movement: Gunship, speed 8. Vision 17, radar 36. No weapons (drone mechanic external).
Tags: AIR, BUILDABLE_BY_T2/3_FACTORY, DEMO_UI_ONLY, DRONE_CARRIER, EDA, GUNSHIP, INTEL, MOBILE, RADAR, TECH2.

## uea2301 — "Kingfisher", Tier 2: Torpedo Bomber — TECH2 — playable: false (NO_MODEL)
Cost 150/3000, buildTime 600. HP 830. Vision 17, sonar 36. No weapons defined.
Tags: AIR, BUILDABLE_BY_T2/3_FACTORY, DEMO_UI_ONLY, EDA, INTEL, MOBILE, SONAR, TECH2, TORPEDO_BOMBER.

## uea2502 — "Pelican", Tier 2: Transport — TECH2 — playable: false (NO_MODEL)
Cost 400/8000, buildTime 1600. HP 2000. Movement: Gunship, speed 15. No weapons.
Tags: AIR, BUILDABLE_BY_T2/3_FACTORY, DEMO_UI_ONLY, EDA, GUNSHIP, MOBILE, TECH2, TRANSPORT.

## uea2806 — "Bumblebee", Tier 2: Flying Repair Station — TECH2 — playable: false (NO_MODEL)
Cost 150/3000, buildTime 600. HP 750, regen 3. No weapons; no repair-beam block present despite role.
Tags: AIR, BUILDABLE_BY_T2/3_FACTORY, DEMO_UI_ONLY, EDA, GUNSHIP, MOBILE, TECH2, UTILITY.

## uea3001 — "Condor", Tier 3: Bomber — TECH3 — playable: false (BONE_MISSMATCH)
Cost 900/18000, buildTime 3600. HP 3800. Movement: Plane, speed 17. Vision 20.
Weapon: dmg 187.5, reload 1s, range 90, salvo 1 (2 muzzles fire together) → DPS 187.5.
Tags: AIR, ANTI_SURFACE, BOMBER, BUILDABLE_BY_T3_FACTORY, DEMO_UI_ONLY, DIRECT_FIRE, EDA, MOBILE, TECH3.

## uea3011 — "Hornet", Tier 3: Gunship — TECH3 — playable: false (OK_PENDING_APPROVAL)
Cost 900/18000, buildTime 2400. HP 6000. Vision 20.
Weapon: pulse beam, dmg 1000, reload 4s, range 25, beamLifetime 1 → DPS 250.
Tags: AIR, ANTI_SURFACE, BUILDABLE_BY_T3_FACTORY, DIRECT_FIRE, EDA, GUNSHIP, MOBILE, TECH3.

## uea3201 — "Peregrine", Tier 3: Anti-Air Fighter — TECH3 — playable: false (BONE_MISSMATCH)
Cost 250/5000, buildTime 1200. HP 2250. Movement: Plane, speed 22. Vision 20.
Weapons: two independent turrets — AA turret dmg 150, reload 3s, range 30, target Air/LandedAir → DPS 50; anti-surface turret identical stats, target Land/WaterSurface → DPS 50. Combined ≈100.
Tags: AIR, ANTI_AIR, BUILDABLE_BY_T3_FACTORY, EDA, FIGHTER, MOBILE, TECH3.
Role: dual-role AA + anti-surface fighter (unusual — most fighters are AA-only).

## uea3701 — "Raven", Tier 3: Air Scout — TECH3 — playable: false (BONE_MISSMATCH)
Cost 110/2200, buildTime 440. HP 1050. Movement: Gunship, speed 27. Vision 50, radar 90, sonar 80.
Weapon: dmg 0 (placeholder/non-functional stub weapon) → DPS 0.
Tags: AIR, ANTI_AIR, BUILDABLE_BY_T3_FACTORY, EDA, GUNSHIP, INTEL, MOBILE, RADAR, SCOUT, SONAR, TECH3.
Role: effectively unarmed premier recon platform.

## uea4001 — "Roc/Phoenix" (EDAT4CarpetBomber), Tier 4 — TECH4 — playable: false (NO_MODEL)
Cost 15000/150000, buildTime 15000. HP 50000. Movement: Gunship, speed 1 (near-stationary), orbits target.
Weapons: 4× railgun turrets, dmg 6000, reload 4s, range 40 → DPS 1500 each (6000 combined). 4× AA autocannon, dmg 20, reload 1s, salvo 4, Air only → DPS 80 each (320 combined). 1× cluster-bomb weapon, chargeTime 5s, reload 7s, AoE 35, salvo 56 bomblets — damage carried by submunition, not in this file (DPS n/a).
Tags: AIR, BOMBER, BUILDABLE_BY_T3_ENGINEER, DEMO_UI_ONLY, EDA, GUNSHIP, MOBILE, TECH4.
Role: near-stationary flying fortress/battlestation, not a maneuverable bomber.

---

# LAND (uel)

## uel0000 — Commander (EDACommander) — special/TECH1 tag — playable: true (OK)
Cost 100000/1000000, buildTime 400000. HP 16000, regen 15/s. Also: production 5 alloys/50 energy per sec, storage 500/5000, initial 250/2500 (unique to Commander). Movement: Hover, speed 1.8. Vision 20.
Construction: buildPower 5, canBuild "EDA * BUILDABLE_BY_T1_ENGINEER", range 10.
Weapons: dual turret dmg 100, reload 2s, range 22, salvo 2 → DPS 100. DeathExplosion dmg 5000, AoE 15 (one-shot, not sustained).
Tags: ALLOYS_PRODUCTION, ALLOYS_STORAGE, ANTI_SURFACE, CAPTURE, COMMAND, CONSTRUCTION, DIRECT_FIRE, ECONOMIC, EDA, ENERGY_PRODUCTION, ENERGY_STORAGE, HARVEST, HOVER, LAND, MOBILE, TECH1.

## uel1001 — "Puma", Tier 1: Tank — TECH1 — playable: true (OK)
Cost 30/300, buildTime 120. HP 275. Movement: TracksLand, speed 3.3. Vision 20.
Weapon: dmg 37.23, reload 1.6s, range 20, salvo 1 → DPS 23.27.
Tags: ANTI_SURFACE, BUILDABLE_BY_T1/2/3_FACTORY, DIRECT_FIRE, EDA, LAND, MOBILE, TANK, TECH1.

## uel1002 — "Dingo", Tier 1: Raider (FastUnit) — TECH1 — playable: true (OK)
Cost 20/200, buildTime 80. HP 145. Movement: LegsLand, speed 4, accel 100, rot 300°/s. Vision 15.
Weapon: dmg 6.25, reload 0.25s, range 15, salvo 1 → DPS 25.
Tags: ANTI_SURFACE, BOT, BUILDABLE_BY_T1/2/3_FACTORY, DIRECT_FIRE, EDA, LAND, MOBILE, TECH1.

## uel1101 — "Bison", Tier 1: Mobile Artillery — TECH1 — playable: true (OK)
Cost 28/280, buildTime 112. HP 220. Movement: TracksLand, speed 3.1. Vision 18.
Weapon: HighArc, dmg 90, reload 6s, range 30/min 5, AoE 0.5, salvo 6 → DPS 90.
Tags: ARTILLERY, BUILDABLE_BY_T1/2/3_FACTORY, EDA, INDIRECT_FIRE, LAND, MOBILE, TECH1.

## uel1201 — "Cobra", Tier 1: Mobile Anti-Air — TECH1 — playable: false (OK, gated)
Cost 20/200, buildTime 80. HP 200. Movement: TracksLand, speed 3.3. Vision 20.
Weapon: quad-barrel, dmg 10/muzzle × 4 simultaneous, reload 2s, range 32, Air only → volley 40, DPS ≈20.
Tags: ANTI_AIR, BUILDABLE_BY_T1/2/3_FACTORY, EDA, LAND, MOBILE, TECH1.

## uel1501 — Tier 1: Engineer — TECH1 — playable: true (OK)
Cost 75/750, buildTime 300. HP 750. Movement: TracksAmphibious, speed 2.7. Vision 15.
Construction: buildPower 5, range 5. No weapons.
Tags: AMPHIBIOUS, BUILDABLE_BY_T1/2/3_FACTORY, CONSTRUCTION, EDA, ENGINEER, HARVEST, LAND, MOBILE, TECH1.

## uel1701 — "Greyhound", Tier 1: Land Scout — TECH1 — playable: true (OK)
Cost 10/100, buildTime 40. HP 40. Movement: TracksLand, speed 4.4 (fastest T1 land mover). Vision 20, radar 38. No weapons.
Tags: BUILDABLE_BY_T1/2/3_FACTORY, EDA, INTEL, LAND, MOBILE, RADAR, SCOUT, TECH1.

## uel1801 — "EDAT1MobileBomp" — TECH1 — playable: false (NO_MODEL) — no .santp file exists on disk; metadata-only.

## uel2002 — "Jackal", Tier 2: Raider (FastUnit) — TECH2 — playable: false (NO_MODEL)
Cost 120/1200, buildTime 480. HP 1500. Movement: TracksSeabed, speed 3.2. Vision 15.
Weapon: dmg 15, reload 0.25s, range 22, salvo 1 → DPS 60.
Tags: ANTI_SURFACE, BUILDABLE_BY_T2/3_FACTORY, DEMO_UI_ONLY, DIRECT_FIRE, EDA, LAND, MOBILE, SEABED, TANK, TECH2.

## uel2101 — "Termite", Tier 2: Land Drone Carrier — TECH2 — playable: false (NO_MODEL)
Cost 150/1500, buildTime 600. HP 550. Vision 18. No construction/weapons (drone mechanic not represented).
Tags: BUILDABLE_BY_T2/3_FACTORY, DEMO_UI_ONLY, DRONE_CARRIER, EDA, LAND, MOBILE, TECH2.

## uel2201 — "Komodo", Tier 2: Mobile Anti-Air — TECH2 — playable: false (OK, gated)
Cost 100/1000, buildTime 400. HP 1100. Vision 20.
Weapon: beam/railgun, dmg 320, reload 2s, range 40, AoE 3, Air only → DPS 160.
Tags: ANTI_AIR, BUILDABLE_BY_T2/3_FACTORY, EDA, LAND, MOBILE, TECH2.

## uel2501 — Tier 2: Engineer — TECH2 — playable: true (OK)
Cost 150/1500, buildTime 600. HP 1500. Movement: TracksAmphibious, speed 2.5. Vision 17.
Construction: buildPower 10, range 7.5. No weapons.
Tags: AMPHIBIOUS, BUILDABLE_BY_T2/3_FACTORY, CONSTRUCTION, EDA, ENGINEER, HARVEST, LAND, MOBILE, TECH2.

## uel2806 — "Capybara", Tier 2: Mobile Repair Station — TECH2 — playable: false (NO_MODEL)
Cost 180/1800, buildTime 720. HP 500. No repair/buildPower field present despite role.
Tags: BUILDABLE_BY_T2/3_FACTORY, DEMO_UI_ONLY, EDA, LAND, MOBILE, TECH2, UTILITY.

## uel3001 — "Kodiak", Tier 3: Tank — TECH3 — playable: true (OK)
Cost 600/6000, buildTime 2400. HP 11239. Movement: TracksLand, speed 2. Vision 20.
Weapon: 3-muzzle burst, dmg 697.25, reload 6s, range 34, salvo 3 → DPS 348.63.
Tags: ANTI_SURFACE, BUILDABLE_BY_T3_FACTORY, DIRECT_FIRE, EDA, LAND, MOBILE, TANK, TECH3.

## uel3002 — "Hyena", Tier 2: Raider (FastUnit2) — TECH2 — playable: true (OK)
Cost 120/1200, buildTime 480. HP 1428. Movement: TracksSeabed, speed 3.3. Vision 20.
Weapon: dual autocannon, dmg 7.86, reload 0.2s, range 22, salvo 2 → DPS 78.6.
Tags: ANTI_SURFACE, BOT, BUILDABLE_BY_T2/3_FACTORY, DIRECT_FIRE, EDA, LAND, MOBILE, SEABED, TECH2.

## uel3101 — "Longhorn", Tier 3: Mobile Artillery — TECH3 — playable: true (OK)
Cost 320/3200, buildTime 1280. HP 850. Vision 20.
Weapon: HighArc, dmg 70, reload 12s, range 90/min 10, AoE 2, salvo 12 → DPS 70.
Tags: ARTILLERY, BUILDABLE_BY_T3_FACTORY, EDA, INDIRECT_FIRE, LAND, MOBILE, TECH3.

## uel3401 — "Chameleon", Tier 3: Mobile Stealth Shield Generator — TECH3 — playable: false (NO_MODEL)
Cost 400/4000, buildTime 1600. HP 750. No shields/stealth fields present despite the name — bare unarmed shell.
Tags: BUILDABLE_BY_T3_FACTORY, DEMO_UI_ONLY, EDA, LAND, MOBILE, TECH3 (notably missing SHIELD/STEALTH_FIELD tags).

## uel3501 — Tier 3: Engineer — TECH3 — playable: true (OK)
Cost 300/3000, buildTime 1200. HP 3000. Construction: buildPower 20 (highest of EDA engineers), range 10.
Tags: AMPHIBIOUS, BUILDABLE_BY_T3_FACTORY, CONSTRUCTION, EDA, ENGINEER, HARVEST, LAND, MOBILE, TECH3.

## uel3801 — "Scorpion", Tier 3: Sniper Bot — TECH3 — playable: true (OK)
Cost 500/5000, buildTime 2000. HP 1330. Movement: LegsLand, speed 2.8. Vision 20.
Weapon: railgun/beam, dmg 750, reload 2.5s, range 60, AoE 2 → DPS 300.
Tags: ANTI_SURFACE, BUILDABLE_BY_T3_FACTORY, DIRECT_FIRE, EDA, LAND, MOBILE, SNIPER, TECH3.

## uel4001 — "Centaur", Tier 4: Railgun Sniper — TECH4 — playable: true (OK)
Cost 15000/150000, buildTime 15000. HP 55000. Movement: LegsLand, speed 2. Vision 20.
Weapons: main railgun/beam, dmg 4000, reload 2.5s, range 40, AoE 2 → DPS 1600. AA autocannon, dmg 40, reload 1s, salvo 4, Air only → DPS 160.
Tags: BOT, BUILDABLE_BY_T3_ENGINEER, EDA, LAND, MOBILE, TECH4.
Role: massive-HP walking sniper, short reload for its class — premier siege/anti-everything unit.

## uel4002 — "Behemoth", Tier 4: Mech — TECH4 — playable: true (OK)
Cost 15000/150000, buildTime 15000. HP 72000. Vision 20.
Weapons: indirect mortar (8 slots), dmg 480, reload 8s, range 30/min 5, AoE 0.5, salvo 16 → DPS 960. Two direct cannons (2-muzzle groups), dmg 345×2 volley, reload 2.2s, AoE 1 → DPS 313.64 each (627.3 combined). AA missile, dmg 100, reload 5s, range 60, AoE 2, salvo 2, Air only → DPS 40.
Tags: BUILDABLE_BY_T3_ENGINEER, EDA, LAND, MOBILE, SEABED, TANK, TECH4.
Role: 4-weapon-system "battle station" mech, combined ground DPS ≈1587.

## uel4511 — "Grendel"/"Captain Crunch", Tier 4: Mobile Harvester Factory — TECH4 — playable: false (NO_MODEL)
Cost 15000/150000, buildTime 15000. HP 75000 (highest HP in EDA land set). No harvest/production/construction/weapons fields present despite the name — bare unarmed shell.
Tags: AMPHIBIOUS, BUILDABLE_BY_T3_ENGINEER, DEMO_UI_ONLY, EDA, LAND, MOBILE, TANK, TECH4.

---

# NAVAL (uen) + STRUCTURES (ues)

## uen1001 — "Sea Lion", Tier 1: Frigate — TECH1 — playable: false (NO_MODEL)
Cost 150/1500, buildTime 600. HP 2000. No weapons — unarmed placeholder hull.
Tags: BUILDABLE_BY_T1/2/3_FACTORY, DEMO_UI_ONLY, EDA, FRIGATE, MOBILE, NAVAL, TECH1.

## ues1001 — "Hedgehog", Tier 1: Point Defence — TECH1 — playable: true (OK)
Cost 100/1000, buildTime 100. HP 1000. Weapon: dmg 32, reload 0.2s, range 25, 2 alternating muzzle groups → DPS ≈160.
Tags: ANTI_SURFACE, BUILDABLE_BY_T1/2/3_ENGINEER, DEFENCE, DIRECT_FIRE, EDA, LAND, STRUCTURE, TECH1.

## ues1111 — FreezeStation — TECH1 tag, no faction tag — playable: false (BATTLE_NO_DAMAGE, special)
Cost 15000/150000, buildTime 15000 (order-of-magnitude above normal units). HP 20000. No weapons at all — cannot deal/take combat damage. Huge static hitbox (16.3×22.6×84.6), a map-feature object not a normal buildable.
Tags: BUILDABLE_BY_T1/2/3_ENGINEER, DEMO_UI_ONLY, FREEZE_CONTROLLER, LAND, STRUCTURE, TECH1.

## ues1201 — Tier 1: Anti-Air — TECH1 — playable: true (OK)
Cost 100/1000, buildTime 100. HP 1000. Weapon: dmg 20, reload 1s, range 40, salvo 4 (quad), Air only → DPS 80.
Tags: ANTI_AIR, BUILDABLE_BY_T1/2/3_ENGINEER, DEFENCE, EDA, STRUCTURE, TECH1.

## ues1301 — "Urchin", Tier 1: Torpedo Launcher — TECH1 — playable: false (NO_MODEL)
Cost 250/2500, buildTime 250. HP 1650. No weapons.
Tags: ANTI_NAVAL, BUILDABLE_BY_T1/2/3_ENGINEER, DEFENCE, DEMO_UI_ONLY, EDA, STRUCTURE, TECH1.

## ues1501 — Tier 1: Engineering Station — TECH1 — playable: true (OK)
Cost 150/1500, buildTime 150. HP 750. Construction: buildPower 10, range 10, upgradesTo ues2501.
Tags: BUILDABLE_BY_T1/2/3_ENGINEER, CONSTRUCTION, EDA, ENGINEERING_STATION, LAND, STRUCTURE, TECH1.

## ues1511 — Tier 1: Land Factory — TECH1 — playable: true (OK)
Cost 150/1500, buildTime 150. HP 5000. Economy: storage energy 1000. Construction: buildPower 10, upgradesTo ues2511.
Tags: BUILDABLE_BY_T1/2/3_ENGINEER, CONSTRUCTION, EDA, ENERGY_STORAGE, FACTORY, LAND, LAND_FACTORY, STRUCTURE, TECH1.

## ues1512 — Tier 1: Air Factory — TECH1 — playable: false (OK, gated)
Cost 150/1500, buildTime 150. HP 4000. Economy storage energy 1000. transport.storage 10. Construction buildPower 10, upgradesTo ues2512.
Tags: AIR_FACTORY, BUILDABLE_BY_T1/2/3_ENGINEER, CONSTRUCTION, DEMO_UI_ONLY, EDA, ENERGY_STORAGE, FACTORY, LAND, STRUCTURE, TECH1.

## ues1513 — Tier 1: Naval Factory — TECH1 — playable: false (NO_MODEL)
Cost 150/1500, buildTime 150. HP 6000. transport.storage 10. Construction buildPower 10, upgradesTo ues2513.
Tags: BUILDABLE_BY_T1/2/3_ENGINEER, CONSTRUCTION, DEMO_UI_ONLY, EDA, FACTORY, NAVAL, NAVAL_FACTORY, STRUCTURE, TECH1.

## ues1601 — Tier 1: Alloy Extractor — TECH1 — playable: true (OK)
Cost 50/500, buildTime 50. HP 600. Production alloys 1/s. Upgrades to ues2601.
Tags: ALLOYS_EXTRACTION, BUILDABLE_BY_T1/2/3_ENGINEER, CONSTRUCTION, ECONOMIC, EDA, STRUCTURE, TECH1.

## ues1602 — Tier 1: Alloy Storage — TECH1 — playable: true (OK)
Cost 80/800, buildTime 80. HP 800. Storage alloys 1000.
Tags: ALLOYS_STORAGE, BUILDABLE_BY_T1/2/3_ENGINEER, ECONOMIC, EDA, STRUCTURE, TECH1.

## ues1611 — Tier 1: Energy Generator — TECH1 — playable: true (OK)
Cost 50/500, buildTime 50. HP 800. Production energy 10/s.
Tags: BUILDABLE_BY_T1/2/3_ENGINEER, ECONOMIC, EDA, ENERGY_PRODUCTION, STRUCTURE, TECH1.

## ues1612 — Tier 1: Energy Storage — TECH1 — playable: true (OK)
Cost 80/800, buildTime 80. HP 800. Storage energy 10000.
Tags: BUILDABLE_BY_T1/2/3_ENGINEER, ECONOMIC, EDA, ENERGY_STORAGE, STRUCTURE, TECH1.

## ues1614 — Tier 1: Solar Convertor — TECH1 — playable: false (NO_MODEL)
Cost 50/500, buildTime 50. HP 800. Larger footprint (4×4) but no production field present — unimplemented.
Tags: BUILDABLE_BY_T1/2/3_ENGINEER, DEMO_UI_ONLY, ECONOMIC, EDA, STRUCTURE, TECH1.

## ues1701 — Tier 1: Radar — TECH1 — playable: true (OK)
Cost 60/600, buildTime 60. HP 300. Radar radius 90. Maintenance 10 energy/s. Upgrades to ues2701.
Tags: BUILDABLE_BY_T1/2/3_ENGINEER, CONSTRUCTION, EDA, INTEL, RADAR, STRUCTURE, TECH1.

## ues1702 — Tier 1: Sonar — TECH1 — playable: false (NO_MODEL)
Cost 60/600, buildTime 60. HP 300. Sonar radius 90. Upgrades to ues2702.
Tags: BUILDABLE_BY_T1/2/3_ENGINEER, CONSTRUCTION, DEMO_UI_ONLY, EDA, INTEL, SONAR, STRUCTURE, TECH1.

## ues1801/1802/1803 — Tier 2 Land/Air/Naval Tech Centres — tag TECH1 (mismatch) — playable: false (OK_PENDING_APPROVAL / OK_PENDING_APPROVAL / NO_MODEL)
Cost 360/3600, buildTime 360. HP 2500/2000/3000. Internal unitTypeName says "EDAT1..." for all three.
Tags include LAND_TECH_CENTRE / AIR_TECH_CENTRE / NAVAL_TECH_CENTRE + TECH_CENTRE.

## ues1804 — Tier 1: Wall — TECH1 — playable: false (NO_MODEL)
Cost 50/500, buildTime 50. HP 500. Vision 0. No weapons.
Tags: BUILDABLE_BY_T1/2/3_ENGINEER, DEMO_UI_ONLY, EDA, STRUCTURE, TECH1, WALL.

## ues2001 — "Snapper", Tier 2: Point Defence — TECH2 — playable: true (OK)
Cost 250/2500, buildTime 250. HP 2000. Weapon: pulse beam, dmg 400, reload 2.5s, range 50, AoE 2 → DPS 160.
Tags: ANTI_SURFACE, BUILDABLE_BY_T2/3_ENGINEER, DEFENCE, DIRECT_FIRE, EDA, LAND, STRUCTURE, TECH2.

## ues2101 — "Spitter", Tier 2: Artillery — TECH2 — playable: false (NO_MODEL)
Cost 1000/10000, buildTime 1000. HP 3000. Weapon: HighArc, dmg 1100, reload 10s, range 115, AoE 3 → DPS 110.
Tags: ARTILLERY, BUILDABLE_BY_T2/3_ENGINEER, EDA, INDIRECT_FIRE, STRUCTURE, TECH2.

## ues2102 — "Brood", Tier 2: Drone Launcher — TECH2 — playable: false (NO_MODEL)
Cost 400/4000, buildTime 400. HP 2000. No weapons.
Tags: BUILDABLE_BY_T2/3_ENGINEER, DEMO_UI_ONLY, DRONE_LAUNCHER, EDA, STRUCTURE, TECH2.

## ues2201 — Tier 2: Anti-Air — TECH2 — playable: true (OK)
Cost 250/2500, buildTime 250. HP 2500. Weapon: flak, dmg 300, reload 2s, range 50, AoE 5, Air only → DPS 150.
Tags: ANTI_AIR, BUILDABLE_BY_T2/3_ENGINEER, DEFENCE, EDA, STRUCTURE, TECH2.

## ues2301 — "Moray", Tier 2: Torpedo Launcher — TECH2 — playable: false (NO_MODEL)
Cost 250/2500, buildTime 250. HP 2500. No weapons.
Tags: ANTI_NAVAL, BUILDABLE_BY_T2/3_ENGINEER, DEFENCE, DEMO_UI_ONLY, EDA, STRUCTURE, TECH2.

## ues2401 — Tier 2: Shield — TECH2 — playable: true (OK)
Cost 150/1500, buildTime 150. HP 750. Shield: max 10000, regen 100/s, regenDelay 3s, rechargeTime 20s, radius 12. Maintenance 100 energy/s. Upgrades to ues3401. Also grants CounterIntel toggle.
Tags: BUILDABLE_BY_T2/3_ENGINEER, CONSTRUCTION, DEFENCE, EDA, SHIELD, STRUCTURE, TECH2.

## ues2402 — "Bullfrog", Tier 2: Anti-Drone Defence — TECH2 — playable: false (NO_MODEL)
Cost 150/1500, buildTime 150. HP 750. No weapons.
Tags: ANTI_DRONE, BUILDABLE_BY_T2/3_ENGINEER, DEFENCE, DEMO_UI_ONLY, EDA, LAND, STRUCTURE, TECH2.

## ues2501 — Tier 2: Engineering Station — TECH2 — playable: true (OK)
Cost 450/4500, buildTime 450. HP 2250. Construction: buildPower 20, range 15, upgradesTo ues3501.
Tags: BUILDABLE_BY_T2/3_ENGINEER, CONSTRUCTION, EDA, ENGINEERING_STATION, LAND, STRUCTURE, TECH2.

## ues2510 — Tier 2: Operating Theatre — TECH2 — playable: false (NO_MODEL)
Cost 900/9000, buildTime 900. HP 10000. No construction/weapon/production data — unimplemented stub.
Tags: BUILDABLE_BY_T2/3_ENGINEER, DEMO_UI_ONLY, EDA, LAND, STRUCTURE, TECH2.

## ues2511 — Tier 2: Land Factory — TECH2 — playable: true (OK)
Cost 500/5000, buildTime 800. HP 10000. Economy storage energy 2000. Construction buildPower 20, upgradesTo ues3511.
Tags: BUILDABLE_BY_T1_FACTORY, BUILDABLE_BY_T2/3_ENGINEER, CONSTRUCTION, EDA, ENERGY_STORAGE, FACTORY, LAND, LAND_FACTORY, STRUCTURE, TECH2.

## ues2512 — Tier 2: Air Factory — TECH2 — playable: true (OK)
Cost 500/5000, buildTime 800. HP 8000. Economy storage energy 2000. transport.storage 25. Construction buildPower 20, upgradesTo ues3512.
Note: unitTypeName mislabeled "EDAT2LandFactory".
Tags: AIR_FACTORY, BUILDABLE_BY_T1_FACTORY, BUILDABLE_BY_T2/3_ENGINEER, CONSTRUCTION, DEMO_UI_ONLY, EDA, ENERGY_STORAGE, FACTORY, LAND, STRUCTURE, TECH2.

## ues2513 — Tier 2: Naval Factory — TECH2 — playable: false (NO_MODEL)
Cost 500/5000, buildTime 800. HP 12000. Construction buildPower 20, upgradesTo ues3513.
Tags: BUILDABLE_BY_T1_FACTORY, BUILDABLE_BY_T2/3_ENGINEER, CONSTRUCTION, DEMO_UI_ONLY, EDA, FACTORY, NAVAL, NAVAL_FACTORY, STRUCTURE, TECH2.

## ues2601 — Tier 2: Alloy Extractor — TECH2 — playable: true (OK)
Cost 600/6000, buildTime 600. HP 2400. Production alloys 4/s. Upgrades to ues3601.
Tags: ALLOYS_EXTRACTION, BUILDABLE_BY_T2/3_ENGINEER, CONSTRUCTION, ECONOMIC, EDA, STRUCTURE, TECH2.

## ues2611 — Tier 2: Energy Generator — TECH2 — playable: true (OK)
Cost 1000/10000, buildTime 1000. HP 1600. Production energy 200/s.
Tags: BUILDABLE_BY_T2/3_ENGINEER, ECONOMIC, EDA, ENERGY_PRODUCTION, STRUCTURE, TECH2.

## ues2701 — Tier 2: Radar — TECH2 — playable: true (OK)
Cost 250/2500, buildTime 250. HP 1250. Radar radius 180. Upgrades to ues3701.
Tags: BUILDABLE_BY_T2/3_ENGINEER, CONSTRUCTION, EDA, INTEL, RADAR, STRUCTURE, TECH2.

## ues2702 — Tier 2: Sonar — TECH2 — playable: false (NO_MODEL)
Cost 250/2500, buildTime 250. HP 1250. Sonar radius 180. Upgrades to ues3702.
Tags: BUILDABLE_BY_T2/3_ENGINEER, CONSTRUCTION, DEMO_UI_ONLY, EDA, INTEL, SONAR, STRUCTURE, TECH2.

## ues2801/2802 — Tier 3 Land/Air Tech Centre — tag TECH2 (displayName says "Tier 3") — playable: false (OK_PENDING_APPROVAL both)
Cost 1200/12000, buildTime 1200. HP 5000/4000.
Tags include LAND_TECH_CENTRE / AIR_TECH_CENTRE + TECH_CENTRE.
Note: internal unitTypeName "EDAT2LandTechCentre" on both (copy-paste bug for the air one).

## ues2806 — Tier 2: Repair Station — TECH2 — playable: false (NO_MODEL)
Cost 450/4500, buildTime 450. HP 2250. No repair/heal mechanic table present — data-incomplete.
Tags: BUILDABLE_BY_T2/3_ENGINEER, DEMO_UI_ONLY, EDA, LAND, STRUCTURE, TECH2, UTILITY.

## ues3102 — "Basilisk", Tier 3: Strategic Launcher — TECH3 — playable: false (NO_MODEL)
Cost 8000/80000, buildTime 8000. HP 8000. No weapons table despite STRATEGIC tag.
Tags: BUILDABLE_BY_T3_ENGINEER, DEMO_UI_ONLY, EDA, STRATEGIC, STRUCTURE, TECH3.

## ues3201 — Tier 3: Anti-Air — TECH3 — playable: true (OK)
Cost 450/4500, buildTime 450. HP 5000. Weapon: dmg 555.556, reload 5s, range 60, AoE 2, salvo 3 (12 muzzle slots) → DPS 333.33. Threat 4158.5.
Tags: ANTI_AIR, BUILDABLE_BY_T3_ENGINEER, DEFENCE, EDA, STRUCTURE, TECH3.

## ues3401 — Tier 3: Shield — TECH3 — playable: true (OK)
Cost 600/6000, buildTime 600. HP 3000. Shield: max 20000, regen 150/s, regenDelay 5s, rechargeTime 30s, radius 20. Maintenance 250 energy/s. Also CounterIntel toggle.
Tags: BUILDABLE_BY_T3_ENGINEER, DEFENCE, EDA, SHIELD, STRUCTURE, TECH3.

## ues3402 — "Canopy", Tier 3: Strategic Defense — TECH3 — playable: false (OK_PENDING_APPROVAL)
Cost 3500/35000, buildTime 3500. HP 3500. No weapon table despite ANTI_PROJECTILE tag.
Tags: ANTI_PROJECTILE, BUILDABLE_BY_T3_ENGINEER, DEFENCE, DEMO_UI_ONLY, EDA, STRUCTURE, TECH3.

## ues3501 — Tier 3: Engineering Station — TECH3 — playable: true (OK)
Cost 1200/12000, buildTime 1200. HP 6000. Construction: buildPower 40, range 20 — strongest static builder in EDA.
Tags: BUILDABLE_BY_T3_ENGINEER, CONSTRUCTION, EDA, ENGINEERING_STATION, LAND, STRUCTURE, TECH3.

## ues3511 — Tier 3: Land Factory — TECH3 — playable: true (OK)
Cost 2000/20000, buildTime 4200. HP 20000. Economy storage energy 5000. Construction buildPower 40.
Tags: BUILDABLE_BY_T2_FACTORY, BUILDABLE_BY_T3_ENGINEER, CONSTRUCTION, EDA, ENERGY_STORAGE, FACTORY, LAND, LAND_FACTORY, STRUCTURE, TECH3.

## ues3512 — Tier 3: Air Factory — TECH3 — playable: true (OK)
Cost 2000/20000, buildTime 4200. HP 16000. Economy storage energy 5000. transport.storage 40. Construction buildPower 40.
Note: unitTypeName mislabeled "EDAT3LandFactory".
Tags: AIR_FACTORY, BUILDABLE_BY_T2_FACTORY, BUILDABLE_BY_T3_ENGINEER, CONSTRUCTION, DEMO_UI_ONLY, EDA, ENERGY_STORAGE, FACTORY, LAND, STRUCTURE, TECH3.

## ues3513 — Tier 3: Naval Factory — TECH3 — playable: false (NO_MODEL)
Cost 2000/20000, buildTime 4200. HP 24000. Construction buildPower 40.
Tags: BUILDABLE_BY_T2_FACTORY, BUILDABLE_BY_T3_ENGINEER, CONSTRUCTION, DEMO_UI_ONLY, EDA, FACTORY, NAVAL, NAVAL_FACTORY, STRUCTURE, TECH3.

## ues3601 — Tier 3: Alloy Extractor — TECH3 — playable: true (OK)
Cost 2000/20000, buildTime 2000. HP 8000. Production alloys 10/s (top of chain).
Tags: ALLOYS_EXTRACTION, BUILDABLE_BY_T3_ENGINEER, ECONOMIC, EDA, STRUCTURE, TECH3.

## ues3603 — Tier 3: Alloy Furnace — TECH3 — playable: true (OK)
Cost 2000/20000, buildTime 2000. HP 8000. Production alloys 10/s, maintenance energy 1000/s (build-anywhere, not site-locked).
Tags: ALLOYS_PRODUCTION, BUILDABLE_BY_T3_ENGINEER, ECONOMIC, EDA, STRUCTURE, TECH3.

## ues3611 — Tier 3: Energy Generator — TECH3 — playable: true (OK)
Cost 5000/50000, buildTime 5000. HP 8000. Production energy 1000/s.
Tags: BUILDABLE_BY_T3_ENGINEER, ECONOMIC, EDA, ENERGY_PRODUCTION, STRUCTURE, TECH3.

## ues3701 — Tier 3: Radar — TECH3 — playable: true (OK)
Cost 700/7000, buildTime 700. HP 3500. Radar radius 450 (top of chain).
Tags: BUILDABLE_BY_T3_ENGINEER, EDA, INTEL, RADAR, STRUCTURE, TECH3.

## ues3702 — Tier 3: Sonar — TECH3 — playable: false (NO_MODEL)
Cost 700/7000, buildTime 700. HP 3500. Sonar radius 150.
Tags: BUILDABLE_BY_T3_ENGINEER, DEMO_UI_ONLY, EDA, INTEL, SONAR, STRUCTURE, TECH3.

## ues4101 — "Jötunn", Tier 4: Heavy Artillery — TECH4 — playable: false (NO_MODEL)
Cost 30000/300000, buildTime 30000. HP 30000. No weapons table at all despite ARTILLERY tag — entirely non-functional.
Tags: ARTILLERY, BUILDABLE_BY_T3_ENGINEER, DEMO_UI_ONLY, EDA, STRUCTURE, TECH4.
