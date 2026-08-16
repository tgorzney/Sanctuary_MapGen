# CHOSEN Faction — Full Unit Data

Prefix `uc` (air `uca`, land `ucl`, naval `ucn`, structures `ucs`). Cross-check
`UNITS_STATUS.md` for playability before recommending any unit. DPS =
damage × muzzleSalvoSize / reloadTime unless noted otherwise.

---

# AIR (uca)

## uca1001 — "Comet", Tier 1: Bomber (ChosenT1Bomber) — TECH1 — playable: true (OK)
Cost 60/600, buildTime 240. HP 300, no armor/shields. Movement: Plane, speed 10, accel 10, rot 90°/s, refAlt GroundOrWater. Vision 15.
Weapon: dmg 60, reload 1s, range 60/0, Normal, AoE 0, salvo 1, projectile pxd002 → DPS 60.
Tags: AIR, ANTI_SURFACE, BOMBER, BUILDABLE_BY_T1/2/3_FACTORY, CHOSEN, DIRECT_FIRE, MOBILE, TECH1.
Role: cheap early direct-fire bomber, no AA capability.

## uca1201 — "Zephyr", Tier 1: Anti-Air Fighter (ChosenT1AAFighter) — TECH1 — playable: true (OK)
Cost 25/250, buildTime 100. HP 250. Movement: Plane, speed 15, accel 15, rot 90°/s. Vision 15.
Weapons: 2 identical turrets, dmg 19 each, reload 1.5s, range 25/0, salvo 2, projectile pca111 → DPS/weapon 25.33, combined ≈50.67. Targets Air/LandedAir.
Tags: AIR, ANTI_AIR, BUILDABLE_BY_T1/2/3_FACTORY, CHOSEN, FIGHTER, MOBILE, TECH1.
Role: T1 air-superiority interceptor.

## uca1502 — "Courier", Tier 1: Transport (ChosenT1Transport) — TECH1 — playable: false (NO_MODEL)
Cost 80/800, buildTime 320. HP 400. Movement: Gunship, speed 10, minSpeed 0. Vision 15. No weapons.
Tags: AIR, BUILDABLE_BY_T1/2/3_FACTORY, CHOSEN, DEMO_UI_ONLY, GUNSHIP, MOBILE, TECH1, TRANSPORT.

## uca1701 — "Seer", Tier 1: Air Scout (ChosenT1AirScout) — TECH1 — playable: true (OK)
Cost 20/200, buildTime 80. HP 60 (fragile). Movement: Plane, speed 18. Vision 30, radar 55. No weapons.
Tags: AIR, BUILDABLE_BY_T1/2/3_FACTORY, CHOSEN, INTEL, MOBILE, RADAR, SCOUT, TECH1.
Role: cheap fast scout.

## uca2011 — "Thunderbolt", Tier 2: Gunship (ChosenT2Gunship) — TECH2 — playable: false (OK_PENDING_APPROVAL)
Cost 150/1500, buildTime 600. HP 850. Movement: Gunship, speed 12. Vision 17.
Weapon: beam, dmg 84, reload 1.4s, range 22/0, salvo 1, beamLifetime 1 → DPS 60.
Tags: AIR, ANTI_SURFACE, BUILDABLE_BY_T2/3_FACTORY, CHOSEN, DIRECT_FIRE, GUNSHIP, MOBILE, TECH2.

## uca2101 — "Consort", Tier 2: Drone Carrier (ChosenT2DroneCarrier) — TECH2 — playable: false (NO_MODEL)
Cost 300/3000, buildTime 1200. HP 1500, regen 3. Movement: Gunship, speed 8. Vision 17, radar 44. No weapons in file (drone mechanic external).
Tags: AIR, BUILDABLE_BY_T2/3_FACTORY, CHOSEN, DEMO_UI_ONLY, DRONE_CARRIER, GUNSHIP, INTEL, MOBILE, RADAR, TECH2.

## uca2301 — "Typhoon", Tier 2: Torpedo Bomber (ChosenT2TorpedoBomber) — TECH2 — playable: false (NO_MODEL)
Cost 150/3000, buildTime 600. HP 830. Movement: Plane, speed 12, minSpeed 5. Vision 17, sonar 44. No weapons defined at all (data-incomplete, not just disabled).
Tags: AIR, BUILDABLE_BY_T2/3_FACTORY, CHOSEN, DEMO_UI_ONLY, INTEL, MOBILE, SONAR, TECH2, TORPEDO_BOMBER.

## uca2502 — "Envoy", Tier 2: Transport (ChosenT2Transport) — TECH2 — playable: false (NO_MODEL)
Cost 400/4000, buildTime 1600. HP 2000. Movement: Gunship, speed 15. Vision 17. No weapons.
Tags: AIR, BUILDABLE_BY_T2/3_FACTORY, CHOSEN, DEMO_UI_ONLY, GUNSHIP, MOBILE, TECH2, TRANSPORT.

## uca2701 — "Oracle", Tier 2: Air Scout (ChosenT2AirScout) — TECH2 — playable: false (OK_PENDING_APPROVAL)
Cost 100/1000, buildTime 400. HP 100. Movement: Plane, speed 21, minSpeed 5. Vision 40, radar 80, sonar 80. No weapons.
Tags: AIR, BUILDABLE_BY_T2/3_FACTORY, CHOSEN, INTEL, MOBILE, RADAR, SCOUT, SONAR, TECH2.

## uca2806 — "Cleric", Tier 2: Flying Shield Booster (ChosenT2FlyingShieldBooster) — TECH2 — playable: false (NO_MODEL)
Cost 150/3000, buildTime 600. HP 750, regen 3. Movement: Gunship, speed 8. Vision 17. No weapons (support unit, UTILITY tag).
Tags: AIR, BUILDABLE_BY_T2/3_FACTORY, CHOSEN, DEMO_UI_ONLY, GUNSHIP, MOBILE, TECH2, UTILITY.

## uca3001 — "Meteor", Tier 3: Bomber (ChosenT3Bomber) — TECH3 — playable: false (NO_MODEL)
Cost 900/9000, buildTime 3600. HP 3800. Movement: Plane, speed 17, accel 14, minSpeed 5. Vision 20.
Weapon: dmg 375, reload 1s, range 90/0, salvo 1, projectile pxd002 → DPS 375.
Tags: AIR, ANTI_SURFACE, BOMBER, BUILDABLE_BY_T3_FACTORY, CHOSEN, DEMO_UI_ONLY, DIRECT_FIRE, MOBILE, TECH3.

## uca3011 — "Tempest", Tier 3: Gunship (ChosenT3Gunship) — TECH3 — playable: false (OK_PENDING_APPROVAL)
Cost 900/9000, buildTime 2400. HP 6000 (tanky). Movement: Gunship, speed 10. Vision 20.
Weapon: dmg 250, reload 1s, range 25/0, salvo 1, projectile pcd311 → DPS 250.
Tags: AIR, ANTI_SURFACE, BUILDABLE_BY_T3_FACTORY, CHOSEN, DIRECT_FIRE, GUNSHIP, MOBILE, TECH3.

## uca3201 — "Mistral", Tier 3: Anti-Air Fighter (ChosenT3AAFighter) — TECH3 — playable: false (OK_PENDING_APPROVAL)
Cost 250/2500, buildTime 1200. HP 2250. Movement: Plane, speed 22, accel 20, minSpeed 5. Vision 20.
Weapons: 2 identical turrets, dmg 250 each, reload 1s, range 30/0, salvo 1, projectile pca311, Air/LandedAir only → DPS 250 each, 500 combined.
Tags: AIR, ANTI_AIR, BUILDABLE_BY_T3_FACTORY, CHOSEN, FIGHTER, MOBILE, TECH3.
Role: endgame interceptor (pre-T4).

## uca3701 — "Prophet", Tier 3: Air Scout (ChosenT3AirScout) — TECH3 — playable: false (OK_PENDING_APPROVAL)
Cost 110/1100, buildTime 440. HP 1050. Movement: Plane, speed 27, accel 25, minSpeed 5. Vision 50, radar 110, sonar 100. No weapons.
Tags: AIR, BUILDABLE_BY_T3_FACTORY, CHOSEN, INTEL, MOBILE, RADAR, SCOUT, SONAR, TECH3.

## uca4011 — "Zeus", Tier 4: Gunship (ChosenT4Gunship) — TECH4 — playable: false (OK_PENDING_APPROVAL)
Cost 15000/150000, buildTime 15000. HP 50000. Movement: Gunship, speed 7, rot 60°/s. Vision 30.
Weapons: W1 dmg 50, reload 4s, range 25, AoE 1, salvo 6, → DPS 75. W2 dmg 175, reload 3s, AoE 2, salvo 3 → DPS 175. W3 identical to W2 → DPS 175. Combined ≈425.
Tags: AIR, ANTI_SURFACE, BUILDABLE_BY_T3_ENGINEER, CHOSEN, DEMO_UI_ONLY, DIRECT_FIRE, GUNSHIP, MOBILE, TECH4.
Role: experimental flying siege battery, built by T3 engineer not a factory.

---

# LAND (ucl)

## ucl0000 — Commander (ChosenCommander) — special/TECH1 tag — playable: true (OK)
Cost 100000/1000000, buildTime 400000. HP 15000, regen 15/s. Movement: LegsLand, speed 1.8. Vision 20.
Construction: buildPower 5, canBuild "CHOSEN * BUILDABLE_BY_T1_ENGINEER".
Weapons: main cannon dmg 50, reload 1s, range 22, salvo 2, projectile pcd121 → DPS 100. DeathExplosion dmg 5000, AoE 15, damageFriendly (one-shot on death, not sustained DPS).
Tags: ALLOYS_PRODUCTION, ALLOYS_STORAGE, ANTI_SURFACE, CAPTURE, CHOSEN, COMMAND, CONSTRUCTION, DIRECT_FIRE, ECONOMIC, ENERGY_PRODUCTION, ENERGY_STORAGE, HARVEST, LAND, MOBILE, TECH1.
Role: base/economy hub + builder + combatant; game ends if killed.

## ucl1001 — "Gladius", Tier 1: Tank (ChosenT1Tank) — TECH1 — playable: true (OK)
Cost 30/300, buildTime 120. HP 307. Movement: TracksLand, speed 3.1. Vision 20.
Weapon: dmg 24.44, reload 2s, range 20/0, salvo 1, projectile pcd111 → DPS 12.22.
Tags: ANTI_SURFACE, BUILDABLE_BY_T1/2/3_FACTORY, CHOSEN, DIRECT_FIRE, LAND, MOBILE, TANK, TECH1.
Role: baseline T1 frontline tank.

## ucl1002 — "Peltast", Tier 1: Raider (ChosenT1Bot / FastUnit) — TECH1 — playable: true (OK)
Cost 20/200, buildTime 80. HP 75. Movement: LegsLand, speed 4, accel 100, rot 300°/s. Vision 15.
Weapon: dmg 18.75, reload 1.5s, range 20/0, salvo 2, projectile pcd131 → DPS 25.
Tags: ANTI_SURFACE, BOT, BUILDABLE_BY_T1/2/3_FACTORY, CHOSEN, DIRECT_FIRE, LAND, MOBILE, TECH1.
Role: cheap fast harasser/flanker.

## ucl1101 — "Slinger", Tier 1: Mobile Artillery (ChosenT1MobileArty) — TECH1 — playable: true (OK)
Cost 26/260, buildTime 104. HP 220. Movement: TracksLand, speed 2.9. Vision 18.
Weapon: HighArc, dmg 180, reload 2s, range 30/0, AoE 1, salvo 1, projectile pci111 → DPS 90.
Tags: ARTILLERY, BUILDABLE_BY_T1/2/3_FACTORY, CHOSEN, INDIRECT_FIRE, LAND, MOBILE, TECH1.
Role: cheap indirect fire, outranges T1 tanks.

## ucl1201 — "Dart", Tier 1: Mobile Anti-Air (ChosenT1MAA) — TECH1 — playable: false (OK, gated)
Cost 20/200, buildTime 80. HP 200. Movement: Hover, speed 3.1. Vision 20.
Weapon: dmg 10, reload 1s, range 32/0, salvo 1, Air only → DPS 10.
Tags: ANTI_AIR, BUILDABLE_BY_T1/2/3_FACTORY, CHOSEN, DEMO_UI_ONLY, HOVER, LAND, MOBILE, TECH1.

## ucl1501 — Tier 1: Engineer (ChosenT1Engineer) — TECH1 — playable: true (OK)
Cost 75/750, buildTime 300. HP 750. Movement: Hover, speed 2.7. Vision 15.
Construction: buildPower 5, canBuild "CHOSEN * BUILDABLE_BY_T1_ENGINEER". No weapons.
Tags: BUILDABLE_BY_T1/2/3_FACTORY, CHOSEN, CONSTRUCTION, ENGINEER, HARVEST, HOVER, LAND, MOBILE, TECH1.

## ucl1701 — "Monocle", Tier 1: Land Scout (ChosenT1LandScout) — TECH1 — playable: true (OK)
Cost 10/100, buildTime 40. HP 40. Movement: Hover, speed 4.2. Vision 20, radar 42. No weapons.
Tags: BUILDABLE_BY_T1/2/3_FACTORY, CHOSEN, HOVER, INTEL, LAND, MOBILE, RADAR, SCOUT, TECH1.

## ucl2002 — "Jager", Tier 2: Raider (ChosenT2Bot / FastUnit) — TECH2 — playable: true (OK)
Cost 120/1200, buildTime 480. HP 2173. Movement: Hover, speed 3.2, rot 75°/s. Vision 15.
Weapon: dmg 50.4, reload 0.5s, range 22/0, salvo 1, projectile pxd002 → DPS 100.8.
Tags: ANTI_SURFACE, BOT, BUILDABLE_BY_T2/3_FACTORY, CHOSEN, DIRECT_FIRE, HOVER, LAND, MOBILE, TECH2.
Role: high DPS-per-cost skirmisher/harasser.

## ucl2101 — "Daimyo", Tier 2: Land Drone Carrier (ChosenT2LandDroneCarrier) — TECH2 — playable: false (OK_PENDING_APPROVAL)
Cost 150/1500, buildTime 600. HP 550. Movement: Hover, speed 2.8. Vision 18. No weapons (drone mechanic external).
Tags: BUILDABLE_BY_T2/3_FACTORY, CHOSEN, DRONE_CARRIER, HOVER, LAND, MOBILE, TECH2.

## ucl2201 — "Javelin", Tier 2: Mobile Anti-Air (ChosenT2MAA) — TECH2 — playable: false (OK, gated)
Cost 100/1000, buildTime 400. HP 1100. Movement: TracksLand, speed 2.8, rot 75°/s. Vision 20.
Weapon: dmg 80, reload 1s, range 40, AoE 3, salvo 1, Air only → DPS 80.
Tags: ANTI_AIR, BUILDABLE_BY_T2/3_FACTORY, CHOSEN, LAND, MOBILE, TECH2.

## ucl2501 — Tier 2: Engineer (ChosenT2Engineer) — TECH2 — playable: true (OK)
Cost 150/1500, buildTime 600. HP 1500. Movement: Hover, speed 2.5. Vision 17.
Construction: buildPower 10. No weapons.
Tags: BUILDABLE_BY_T2/3_FACTORY, CHOSEN, CONSTRUCTION, ENGINEER, HARVEST, HOVER, LAND, MOBILE, TECH2.

## ucl2806 — "Paladin", Tier 2: Mobile Shield Booster (ChosenT2MobileShieldBooster) — TECH2 — playable: false (OK_PENDING_APPROVAL)
Cost 180/1800, buildTime 720. HP 500. Movement: Hover, speed 2.6. Vision 18. No weapons; support aura mechanic not represented in template.
Tags: BUILDABLE_BY_T2/3_FACTORY, CHOSEN, DEMO_UI_ONLY, HOVER, LAND, MOBILE, TECH2, UTILITY.

## ucl3001 — "Glaive", Tier 3: Tank (ChosenT3Tank) — TECH3 — playable: true (OK)
Cost 600/6000, buildTime 2400. HP 8696. Movement: LegsLand, speed 2, rot 60°/s. Vision 20.
Weapon: dual-mount, dmg 268.8, reload 2s, range 34/0, salvo 2, projectile pcd311 → DPS 268.8.
Tags: ANTI_SURFACE, BUILDABLE_BY_T3_FACTORY, CHOSEN, DIRECT_FIRE, LAND, MOBILE, TANK, TECH3.
Role: heavy frontline tank.

## ucl3101 — "Catapault", Tier 3: Mobile Artillery (ChosenT3MobileArty) — TECH3 — playable: true (OK)
Cost 376/3760, buildTime 1504. HP 850. Movement: Hover, speed 2.5, rot 60°/s. Vision 20.
Weapon: HighArc, dmg 350, reload 5s, range 90/0, AoE 4, salvo 1, projectile pci211 → DPS 70.
Tags: ARTILLERY, BUILDABLE_BY_T3_FACTORY, CHOSEN, HOVER, INDIRECT_FIRE, LAND, MOBILE, TECH3.
Role: long-range (90) siege artillery.

## ucl3501 — Tier 3: Engineer (ChosenT3Engineer) — TECH3 — playable: true (OK)
Cost 300/3000, buildTime 1200. HP 3000. Movement: Hover, speed 2.3, rot 60°/s. Vision 20.
Construction: buildPower 20. No weapons.
Tags: BUILDABLE_BY_T3_FACTORY, CHOSEN, CONSTRUCTION, ENGINEER, HARVEST, HOVER, LAND, MOBILE, TECH3.

## ucl3701 — "Emissary", Tier 3: Land Scout (ChosenT3LandScout) — TECH3 — playable: true (OK)
Cost 8/80, buildTime 32. HP 100. Movement: Hover, speed 3.5, rot 60°/s. Vision 20, radar 80. No weapons.
Tags: BUILDABLE_BY_T3_FACTORY, CHOSEN, HOVER, INTEL, LAND, MOBILE, RADAR, SCOUT, TECH3.

## ucl3801 — "Longbow", Tier 3: Sniper Bot (ChosenT3SniperBot) — TECH3 — playable: true (OK)
Cost 500/5000, buildTime 2000. HP 2600. Movement: Hover, speed 2.8, rot 60°/s. Vision 20.
Weapon: dmg 1800, reload 6s, range 60/0, salvo 1, projectile pcd312 → DPS 300.
Tags: ANTI_SURFACE, BUILDABLE_BY_T3_FACTORY, CHOSEN, DIRECT_FIRE, HOVER, LAND, MOBILE, SNIPER, TECH3.
Role: long-range high-alpha burst — snipe high-value/low-HP targets.

## ucl4001 — "Ares", Tier 4: Bot (ChosenT4Bot) — TECH4 — playable: true (OK)
Cost 15000/150000, buildTime 15000. HP 75000. Movement: LegsSeabed, speed 2, rot 40°/s. Vision 20.
Weapons: main cannon dmg 4000, reload 4s, range 30, AoE 3, salvo 1 → DPS 1000. Two plasma point-defense mounts, dmg 100, reload 0.8s, salvo 2 → DPS 250 each (500 combined). DeathExplosion dmg 6000, AoE 6.
Tags: ANTI_SURFACE, BOT, BUILDABLE_BY_T3_ENGINEER, CHOSEN, DIRECT_FIRE, LAND, MOBILE, SEABED, TECH4.
Role: T4 assault experimental, combined DPS ≈1500.

## ucl4002 — Tier 4: Tripod Bot (ChosenT4Bot / TripodBot) — TECH4 — playable: true (OK)
Cost 15000/150000, buildTime 15000. HP 50000. Movement: LegsSeabed, speed 2, rot 40°/s. Vision 20.
Weapons: long-range DoT artillery, dmg 1000 (+400 DoT via 10×40 pulses), reload 15s, range 100, AoE 4, salvo 3 → base DPS 200 (+DoT). Two AT beams, dmg 160, reload 1s → DPS 160 each (320 combined). DeathExplosion dmg 15000, AoE 12.
Note: lacks ANTI_SURFACE/DIRECT_FIRE tag despite having weapons — likely a tagging omission.
Tags: BOT, BUILDABLE_BY_T3_ENGINEER, CHOSEN, LAND, MOBILE, TECH4.
Role: long-range (100) siege platform with close-range self-defense beams.

## ucl4003 — "Djinn", Tier 4: Bot Hover (ChosenT4Bot / BotHover) — TECH4 — playable: true (OK)
Cost 18000/180000, buildTime 18000. HP 100000. Movement: Hover, speed 2, rot 40°/s. Vision 20.
Weapons: heavy plasma dual-muzzle, dmg 1000, reload 1.8s, range 40, AoE 3, salvo 2 → DPS 1111.11. Two AA missile batteries, dmg 150, reload 3s, range 60, AoE 5, salvo 3 → DPS 150 each (300 combined). DeathExplosion dmg 8000, AoE 8.
Tags: ANTI_AIR, ANTI_SURFACE, BOT, BUILDABLE_BY_T3_ENGINEER, CHOSEN, DIRECT_FIRE, HOVER, LAND, MOBILE, TECH4.
Role: highest-HP T4 hover experimental, combined-arms (anti-surface + dedicated AA).

## ucl4004 — Tier 4: Bot Big (ChosenT4Bot / BotBig) — TECH4 — playable: true (OK)
Cost 20000/200000, buildTime 20000. HP 100000. Movement: LegsSeabed, speed 2, rot 40°/s. Vision 20.
Weapons: heavy triple-turret plasma (6 muzzles, salvo 3), dmg 1567.94, reload 2s → DPS 2351.92. Two AA point-defense, dmg 31.36, reload 0.4s → DPS 78.4 each (156.8 combined). Two long-range howitzers, dmg 836.24, reload 10s, range 100, AoE 3 → DPS 83.62 each (167.25 combined). DeathExplosion dmg 15000, AoE 12.
Tags: ANTI_AIR, ANTI_SURFACE, BOT, BUILDABLE_BY_T3_ENGINEER, CHOSEN, DIRECT_FIRE, INDIRECT_FIRE, LAND, MOBILE, SEABED, TECH4.
Role: largest/most expensive Chosen combat experimental — 5 weapon systems, combined non-explosion DPS ≈2676.

## ucl4005 — Tier 5 (displayName)/TECH4 (tag): Hovertank Mega Bot (ChosenT4BotMega) — playable: false (NO_MODEL)
Cost 400000/4000000, buildTime 400000. HP 1000000. Movement: LegsSeabed, speed 1.5, rot 20°/s. Vision 60.
Weapons: main siege cannon dmg 12000, reload 12s, range 70, AoE 9 → DPS 1000. 3× AA point-defense dmg 30, reload 0.4s → 75 each (225 combined). 6× AT beam turrets dmg 160, reload 1s → 160 each (960 combined). AOEPulse special: dmg 12000, radius 10, damageFriendly true, reload 22s → ~545.45 cycle DPS.
Tags: ANTI_AIR, ANTI_SURFACE, BOT, BUILDABLE_BY_T3_ENGINEER, CHOSEN, DEMO_UI_ONLY, DIRECT_FIRE, LAND, MOBILE, SEABED, TECH4.
Role: ultimate/unfinished super-heavy, 11 weapon systems, currently non-functional (no model).

## ucl4401 — "Athena", Tier 4: Mobile Shield (ChosenT4MobileShield) — TECH4 — playable: true (OK)
Cost 18000/180000, buildTime 18000. HP 75000. Shield: "Bubble Shield" max 40000, spawn value 20000, regen 150/s, regenDelay 5s, rechargeTime 30s, rechargeHPRatio 0.5, radii 25/25/25. Movement: Hover, speed 2, rot 40°/s. Vision 20. No weapons.
Tags: BOT, BUILDABLE_BY_T3_ENGINEER, CHOSEN, HOVER, LAND, MOBILE, SHIELD, TECH4.
Role: unarmed mobile shield generator, protects army/base with a large bubble.

---

# NAVAL (ucn) + STRUCTURES (ucs)

## ucn1001 — "Escort", Tier 1: Frigate (ChosenT1Frigate) — TECH1 — playable: false (OK_PENDING_APPROVAL)
Cost 150/1500, buildTime 600. HP 2000. Vision 20.
Weapon: dmg 120, AoE 1, reload 2s, range 48/0, LowArc, target Land+WaterSurface, fires from WaterSurface → DPS 60.
Tags: ANTI_SURFACE, BUILDABLE_BY_T1/2/3_FACTORY, CHOSEN, DIRECT_FIRE, FRIGATE, MOBILE, NAVAL, TECH1.

## ucn3001 — "Dreadnought", Tier 3: Battleship (ChosenT3Battleship) — TECH3 — playable: false (OK_PENDING_APPROVAL)
Cost 5000/50000, buildTime 15000. HP 48000. Vision 20.
Weapons: 3 turrets, dmg 583.33 each, reload 10s, range 140, AoE 3, salvo 3 (delay 0.5s) → DPS 175 each (525 total nominal). **Data bug: turret 3's fire-layer is Land only, not WaterSurface — likely can't fire while afloat; real DPS may be ≈350.**
Tags: ANTI_SURFACE, BATTLESHIP, BUILDABLE_BY_T3_FACTORY, CHOSEN, DIRECT_FIRE, MOBILE, NAVAL, TECH3.

## ucs1001 — "Watchtower", Tier 1: Point Defence — TECH1 — playable: true (OK)
Cost 100/1000, buildTime 100. HP 1000. Vision 15.
Weapon: beam, dmg 160, reload 1s, range 25, salvo 1, fires from Land, targets Land+WaterSurface → DPS 160.
Tags: ANTI_SURFACE, BUILDABLE_BY_T1/2/3_ENGINEER, CHOSEN, DEFENCE, DIRECT_FIRE, LAND, STRUCTURE, TECH1.

## ucs1201 — "Lookout", Tier 1: Anti-Air — TECH1 — playable: false (listed OK but restricted)
Cost 100/1000, buildTime 100. HP 1000. Vision 15.
Weapon: dmg 40/muzzle × 2 simultaneous muzzles, reload 1s, range 40, Air only → DPS 80.
Tags: ANTI_AIR, BUILDABLE_BY_T1/2/3_ENGINEER, CHOSEN, DEFENCE, STRUCTURE, TECH1.

## ucs1301 — "Trapper", Tier 1: Torpedo Launcher — TECH1 — playable: false (NO_MODEL)
Cost 250/2500, buildTime 250. HP 1650. Vision 15. No weapons in file (stub).
Tags: ANTI_NAVAL, BUILDABLE_BY_T1/2/3_ENGINEER, CHOSEN, DEFENCE, DEMO_UI_ONLY, STRUCTURE, TECH1.

## ucs1501 — Tier 1: Engineering Station — TECH1 — playable: true (OK)
Cost 150/1500, buildTime 150. HP 750. Construction: buildPower 10, range 10, upgradesTo ucs2501.
Tags: BUILDABLE_BY_T1/2/3_ENGINEER, CHOSEN, CONSTRUCTION, ENGINEERING_STATION, LAND, STRUCTURE, TECH1.

## ucs1511 — Tier 1: Land Factory — TECH1 — playable: true (OK)
Cost 150/1500, buildTime 150. HP 5000. Construction: buildPower 10, 2 rollOffPoints, upgradesTo ucs2511. Toggles RepeatBuild.
Tags: BUILDABLE_BY_T1/2/3_ENGINEER, CHOSEN, CONSTRUCTION, FACTORY, LAND, LAND_FACTORY, STRUCTURE, TECH1.

## ucs1512 — Tier 1: Air Factory — TECH1 — playable: false (listed OK but restricted)
Cost 150/1500, buildTime 150. HP 4000. Construction: buildPower 10, upgradesTo ucs2512. transport.storage 10.
Note: unitTypeName mislabeled "ChosenT1LandFactory" (copy-paste bug).
Tags: AIR_FACTORY, BUILDABLE_BY_T1/2/3_ENGINEER, CHOSEN, CONSTRUCTION, DEMO_UI_ONLY, FACTORY, LAND, STRUCTURE, TECH1.

## ucs1513 — Tier 1: Naval Factory — TECH1 — playable: false (OK_PENDING_APPROVAL)
Cost 150/1500, buildTime 150. HP 6000. Construction: buildPower 10, upgradesTo ucs2513. transport.storage 10.
Note: unitTypeName mislabeled "ChosenT1LandFactory".
Tags: BUILDABLE_BY_T1/2/3_ENGINEER, CHOSEN, CONSTRUCTION, DEMO_UI_ONLY, FACTORY, NAVAL, NAVAL_FACTORY, STRUCTURE, TECH1.

## ucs1601 — Tier 1: Alloy Extractor — TECH1 — playable: true (OK)
Cost 50/500, buildTime 50. HP 600. Economy: production alloys 1/s (site-locked). Upgrades to ucs2601.
Tags: ALLOYS_EXTRACTION, BUILDABLE_BY_T1/2/3_ENGINEER, CHOSEN, CONSTRUCTION, ECONOMIC, STRUCTURE, TECH1.

## ucs1602 — Tier 1: Alloy Storage — TECH1 — playable: true (OK)
Cost 80/800, buildTime 80. HP 800. Economy: storage alloys 1000.
Tags: ALLOYS_STORAGE, BUILDABLE_BY_T1/2/3_ENGINEER, CHOSEN, ECONOMIC, STRUCTURE, TECH1.

## ucs1611 — Tier 1: Energy Generator — TECH1 — playable: true (OK)
Cost 50/500, buildTime 50. HP 800. Economy: production energy 10/s.
Tags: BUILDABLE_BY_T1/2/3_ENGINEER, CHOSEN, ECONOMIC, ENERGY_PRODUCTION, STRUCTURE, TECH1.

## ucs1612 — Tier 1: Energy Storage — TECH1 — playable: true (OK)
Cost 80/800, buildTime 80. HP 800. Economy: storage energy 10000.
Tags: BUILDABLE_BY_T1/2/3_ENGINEER, CHOSEN, ECONOMIC, ENERGY_STORAGE, STRUCTURE, TECH1.

## ucs1614 — Tier 1: Solar Convertor — TECH1 — playable: false (NO_MODEL)
Cost 50/500, buildTime 50. HP 800. No economy fields present — unimplemented stub.
Tags: BUILDABLE_BY_T1/2/3_ENGINEER, CHOSEN, DEMO_UI_ONLY, ECONOMIC, STRUCTURE, TECH1.

## ucs1701 — Tier 1: Radar — TECH1 — playable: true (OK)
Cost 60/600, buildTime 60. HP 300. Radar radius 110. Maintenance 10 energy/s. Upgrades to ucs2701.
Tags: BUILDABLE_BY_T1/2/3_ENGINEER, CHOSEN, CONSTRUCTION, INTEL, RADAR, STRUCTURE, TECH1.

## ucs1702 — Tier 1: Sonar — TECH1 — playable: false (NO_MODEL)
Cost 60/600, buildTime 60. HP 300. Sonar radius 110. Maintenance 10 energy/s. Upgrades to ucs2702.
Tags: BUILDABLE_BY_T1/2/3_ENGINEER, CHOSEN, CONSTRUCTION, DEMO_UI_ONLY, INTEL, SONAR, STRUCTURE, TECH1.

## ucs1801/1802/1803 — Tier 2 Land/Air/Naval Tech Centres — tag TECH1 (mismatch vs "Tier 2" name) — playable: false (OK_PENDING_APPROVAL / OK_PENDING_APPROVAL / NO_MODEL)
Cost 360/3600, buildTime 360. HP 2500/2000/3000 respectively. Unlock higher-tier units at factories. Internal unitTypeName says "ChosenT1...TechCentre" for all three.
Tags include LAND_TECH_CENTRE / AIR_TECH_CENTRE / NAVAL_TECH_CENTRE + TECH_CENTRE + TECH1.

## ucs1804 — Tier 1: Wall — TECH1 — playable: false (NO_MODEL)
Cost 50/500, buildTime 50. HP 500. Vision 0. No weapons.
Tags: BUILDABLE_BY_T1/2/3_ENGINEER, CHOSEN, DEMO_UI_ONLY, STRUCTURE, TECH1, WALL.

## ucs2001 — "Redoubt", Tier 2: Point Defence — TECH2 — playable: true (OK)
Cost 250/2500, buildTime 250. HP 2000. Weapon: dmg 480, AoE 2, reload 3s, range 50 → DPS 160.
Tags: ANTI_SURFACE, BUILDABLE_BY_T2/3_ENGINEER, CHOSEN, DEFENCE, DIRECT_FIRE, LAND, STRUCTURE, TECH2.

## ucs2101 — "Ballista", Tier 2: Artillery — TECH2 — playable: true (OK)
Cost 1000/10000, buildTime 1000. HP 3000. Weapon: HighArc, dmg 1100, AoE 3, reload 10s, range 115 → DPS 110.
Tags: ARTILLERY, BUILDABLE_BY_T2/3_ENGINEER, CHOSEN, INDIRECT_FIRE, STRUCTURE, TECH2.

## ucs2102 — "Totem", Tier 2: Drone Launcher — TECH2 — playable: false (OK_PENDING_APPROVAL)
Cost 400/4000, buildTime 400. HP 2000. No weapons in file.
Tags: BUILDABLE_BY_T2/3_ENGINEER, CHOSEN, DEMO_UI_ONLY, DRONE_LAUNCHER, STRUCTURE, TECH2.

## ucs2201 — "Sentry", Tier 2: Anti-Air — TECH2 — playable: true (OK)
Cost 250/2500, buildTime 250. HP 2500. Weapon: dmg 112.5×2 muzzles, AoE 2, reload 1.5s, range 50, Air only → DPS 150.
Tags: ANTI_AIR, BUILDABLE_BY_T2/3_ENGINEER, CHOSEN, DEFENCE, STRUCTURE, TECH2.

## ucs2301 — "Angler", Tier 2: Torpedo Launcher — TECH2 — playable: false (NO_MODEL)
Cost 250/2500, buildTime 250. HP 2500. No weapons defined.
Tags: ANTI_NAVAL, BUILDABLE_BY_T2/3_ENGINEER, CHOSEN, DEFENCE, DEMO_UI_ONLY, STRUCTURE, TECH2.

## ucs2401 — Tier 2: Shield — TECH2 — playable: true (OK)
Cost 150/1500, buildTime 150. HP 750. Shield: max 10000, regen 100/s, regenDelay 3s, rechargeTime 20s, radius 12. Maintenance 100 energy/s. Upgrades to ucs3401.
Tags: BUILDABLE_BY_T2/3_ENGINEER, CHOSEN, CONSTRUCTION, DEFENCE, SHIELD, STRUCTURE, TECH2.

## ucs2402 — "Warden", Tier 2: Anti-Drone Defence — TECH2 — playable: false (NO_MODEL)
Cost 150/1500, buildTime 150. HP 750. No weapons.
Tags: ANTI_DRONE, BUILDABLE_BY_T2/3_ENGINEER, CHOSEN, DEFENCE, DEMO_UI_ONLY, STRUCTURE, TECH2.

## ucs2501 — Tier 2: Engineering Station — TECH2 — playable: true (OK)
Cost 450/4500, buildTime 450. HP 2250. Construction: buildPower 20, range 15, upgradesTo ucs3501.
Tags: BUILDABLE_BY_T2/3_ENGINEER, CHOSEN, CONSTRUCTION, ENGINEERING_STATION, LAND, STRUCTURE, TECH2.

## ucs2510 — Tier 2: Operating Theatre — TECH2 — playable: false (NO_MODEL)
Cost 900/9000, buildTime 900. HP 10000. No production/construction/weapons — unimplemented support stub.
Tags: BUILDABLE_BY_T2/3_ENGINEER, CHOSEN, DEMO_UI_ONLY, LAND, STRUCTURE, TECH2.

## ucs2511 — Tier 2: Land Factory — TECH2 — playable: true (OK)
Cost 500/5000, buildTime 800. HP 10000. Construction: buildPower 20, 2 rollOffPoints, upgradesTo ucs3511.
Tags: BUILDABLE_BY_T1_FACTORY, BUILDABLE_BY_T2/3_ENGINEER, CHOSEN, CONSTRUCTION, FACTORY, LAND, LAND_FACTORY, STRUCTURE, TECH2.

## ucs2512 — Tier 2: Air Factory — TECH2 — playable: true (OK)
Cost 500/5000, buildTime 800. HP 8000. Construction: buildPower 20, upgradesTo ucs3512. transport.storage 25.
Note: unitTypeName mislabeled "ChosenT2LandFactory".
Tags: AIR_FACTORY, BUILDABLE_BY_T1_FACTORY, BUILDABLE_BY_T2/3_ENGINEER, CHOSEN, CONSTRUCTION, DEMO_UI_ONLY, FACTORY, LAND, STRUCTURE, TECH2.

## ucs2513 — Tier 2: Naval Factory — TECH2 — playable: false (OK_PENDING_APPROVAL)
Cost 500/5000, buildTime 800. HP 12000. Construction: buildPower 20, upgradesTo ucs3513.
Tags: BUILDABLE_BY_T1_FACTORY, BUILDABLE_BY_T2/3_ENGINEER, CHOSEN, CONSTRUCTION, DEMO_UI_ONLY, FACTORY, NAVAL, NAVAL_FACTORY, STRUCTURE, TECH2.

## ucs2601 — Tier 2: Alloy Extractor — TECH2 — playable: true (OK)
Cost 600/6000, buildTime 600. HP 2400. Production alloys 4/s. Upgrades to ucs3601.
Tags: ALLOYS_EXTRACTION, BUILDABLE_BY_T2/3_ENGINEER, CHOSEN, CONSTRUCTION, ECONOMIC, STRUCTURE, TECH2.

## ucs2611 — Tier 2: Energy Generator — TECH2 — playable: true (OK)
Cost 1000/10000, buildTime 1000. HP 1600. Production energy 200/s.
Tags: BUILDABLE_BY_T2/3_ENGINEER, CHOSEN, ECONOMIC, ENERGY_PRODUCTION, STRUCTURE, TECH2.

## ucs2701 — Tier 2: Radar — TECH2 — playable: true (OK)
Cost 250/2500, buildTime 250. HP 1250. Radar radius 220. Maintenance 100 energy/s. Upgrades to ucs3701.
Tags: BUILDABLE_BY_T2/3_ENGINEER, CHOSEN, CONSTRUCTION, INTEL, RADAR, STRUCTURE, TECH2.

## ucs2702 — Tier 2: Sonar — TECH2 — playable: false (NO_MODEL)
Cost 250/2500, buildTime 250. HP 1250. Sonar radius 220. Upgrades to ucs3702.
Tags: BUILDABLE_BY_T2/3_ENGINEER, CHOSEN, CONSTRUCTION, DEMO_UI_ONLY, INTEL, SONAR, STRUCTURE, TECH2.

## ucs2711 — Tier 2: Stealth Field — TECH2 — playable: false (OK_PENDING_APPROVAL)
Cost 150/1500, buildTime 150. HP 750, regen 2. No weapons.
Tags: BUILDABLE_BY_T2/3_ENGINEER, CHOSEN, DEFENCE, DEMO_UI_ONLY, STEALTH_FIELD, STRUCTURE, TECH2.

## ucs2801/2802/2803 — Tier 3 Land/Air/Naval Tech Centres — tag TECH2 (mismatch) — playable: false (OK_PENDING_APPROVAL / OK_PENDING_APPROVAL / NO_MODEL)
Cost 1200/12000, buildTime 1200. HP 5000/4000/6000 respectively.
Tags include LAND_TECH_CENTRE / AIR_TECH_CENTRE / NAVAL_TECH_CENTRE + TECH_CENTRE + TECH2.

## ucs2806 — Tier 2: Shield Booster Station — TECH2 — playable: false (NO_MODEL)
Cost 450/4500, buildTime 450. HP 2250. No weapons — utility structure, mechanic unimplemented.
Tags: BUILDABLE_BY_T2/3_ENGINEER, CHOSEN, DEMO_UI_ONLY, LAND, STRUCTURE, TECH2, UTILITY.

## ucs3001 — "Bastion", Tier 3: Point Defence — TECH3 — playable: true (OK)
Cost 1000/10000, buildTime 1000. HP 6500. Weapon: dmg 825, AoE 1, reload 3s, range 70 → DPS 275. Threat 4161.2.
Tags: ANTI_SURFACE, BUILDABLE_BY_T3_ENGINEER, CHOSEN, DEFENCE, DIRECT_FIRE, LAND, STRUCTURE, TECH3.

## ucs3101 — "Onager", Tier 3: Artillery — TECH3 — playable: true (OK)
Cost 40000/400000, buildTime 40000 (very expensive). HP 13000. Weapon: dmg 5250 direct + 4000 DoT (100×40 pulses), AoE 4, reload 10s → DPS (direct) 525. Range 825 (very long). Threat 63486.3.
Tags: ARTILLERY, BUILDABLE_BY_T3_ENGINEER, CHOSEN, INDIRECT_FIRE, STRUCTURE, TECH3.

## ucs3102 — "Trebuchet", Tier 3: Strategic Launcher — TECH3 — playable: false (OK_PENDING_APPROVAL)
Cost 8000/80000, buildTime 8000. HP 8000. No weapons table at all — launch mechanic not wired.
Tags: BUILDABLE_BY_T3_ENGINEER, CHOSEN, DEMO_UI_ONLY, STRATEGIC, STRUCTURE, TECH3.

## ucs3201 — "Guardian", Tier 3: Anti-Air — TECH3 — playable: true (OK)
Cost 450/4500, buildTime 450. HP 5000. Weapon: 6-muzzle burst, dmg 55.556/muzzle, reload 1s, range 60, Air only → DPS ≈333.3. Threat 2400.9.
Tags: ANTI_AIR, BUILDABLE_BY_T3_ENGINEER, CHOSEN, DEFENCE, STRUCTURE, TECH3.

## ucs3401 — Tier 3: Shield — TECH3 — playable: true (OK)
Cost 600/6000, buildTime 600. HP 3000. Shield: max 20000, regen 150/s, regenDelay 5s, rechargeTime 30s, radius 20. Maintenance 250 energy/s.
Tags: BUILDABLE_BY_T3_ENGINEER, CHOSEN, DEFENCE, SHIELD, STRUCTURE, TECH3.

## ucs3402 — "Aegis", Tier 3: Strategic Defense — TECH3 — playable: false (OK_PENDING_APPROVAL)
Cost 3500/35000, buildTime 3500. HP 3500. No weapons table (anti-projectile logic not defined).
Tags: ANTI_PROJECTILE, BUILDABLE_BY_T3_ENGINEER, CHOSEN, DEFENCE, DEMO_UI_ONLY, STRUCTURE, TECH3.

## ucs3501 — Tier 3: Engineering Station — TECH3 — playable: true (OK)
Cost 1200/12000, buildTime 1200. HP 6000. Construction: buildPower 40, range 20.
Tags: BUILDABLE_BY_T3_ENGINEER, CHOSEN, CONSTRUCTION, ENGINEERING_STATION, LAND, STRUCTURE, TECH3.

## ucs3511 — Tier 3: Land Factory — TECH3 — playable: true (OK)
Cost 2000/20000, buildTime 4200. HP 20000. Construction: buildPower 40, 2 rollOffPoints.
Tags: BUILDABLE_BY_T2_FACTORY, BUILDABLE_BY_T3_ENGINEER, CHOSEN, CONSTRUCTION, FACTORY, LAND, LAND_FACTORY, STRUCTURE, TECH3.

## ucs3512 — Tier 3: Air Factory — TECH3 — playable: true (OK)
Cost 2000/20000, buildTime 4200. HP 16000. Construction: buildPower 40. transport.storage 40.
Note: unitTypeName mislabeled "ChosenT3LandFactory".
Tags: AIR_FACTORY, BUILDABLE_BY_T2_FACTORY, BUILDABLE_BY_T3_ENGINEER, CHOSEN, CONSTRUCTION, DEMO_UI_ONLY, FACTORY, LAND, STRUCTURE, TECH3.

## ucs3513 — Tier 3: Naval Factory — TECH3 — playable: false (OK_PENDING_APPROVAL)
Cost 2000/20000, buildTime 4200. HP 24000. Construction: buildPower 40.
Tags: BUILDABLE_BY_T2_FACTORY, BUILDABLE_BY_T3_ENGINEER, CHOSEN, CONSTRUCTION, DEMO_UI_ONLY, FACTORY, NAVAL, NAVAL_FACTORY, STRUCTURE, TECH3.

## ucs3601 — Tier 3: Alloy Extractor — TECH3 — playable: true (OK)
Cost 2000/20000, buildTime 2000. HP 8000. Production alloys 10/s (top of extractor chain).
Tags: ALLOYS_EXTRACTION, BUILDABLE_BY_T3_ENGINEER, CHOSEN, ECONOMIC, STRUCTURE, TECH3.

## ucs3603 — Tier 3: Alloy Furnace — TECH3 — playable: true (OK)
Cost 2000/20000, buildTime 2000. HP 8000. Production alloys 10/s, maintenance energy 1000/s — build-anywhere alloy converter. Toggle Production. Large death explosion (5000 dmg, radius 10).
Tags: ALLOYS_PRODUCTION, BUILDABLE_BY_T3_ENGINEER, CHOSEN, ECONOMIC, STRUCTURE, TECH3.

## ucs3611 — Tier 3: Energy Generator — TECH3 — playable: true (OK)
Cost 5000/50000, buildTime 5000. HP 8000. Production energy 1000/s.
Tags: BUILDABLE_BY_T3_ENGINEER, CHOSEN, ECONOMIC, ENERGY_PRODUCTION, STRUCTURE, TECH3.

## ucs3701 — Tier 3: Radar — TECH3 — playable: true (OK)
Cost 700/7000, buildTime 700. HP 3500. Radar radius 550 (top of radar chain). Maintenance 100 energy/s.
Tags: BUILDABLE_BY_T3_ENGINEER, CHOSEN, INTEL, RADAR, STRUCTURE, TECH3.

## ucs3702 — Tier 3: Sonar — TECH3 — playable: false (NO_MODEL)
Cost 700/7000, buildTime 700. HP 3500. Sonar radius 550. Maintenance 500 energy/s.
Tags: BUILDABLE_BY_T3_ENGINEER, CHOSEN, DEMO_UI_ONLY, INTEL, SONAR, STRUCTURE, TECH3.

## ucs4102 — "Babel", Tier 4: Strategic Launcher — TECH4 — playable: false (OK_PENDING_APPROVAL)
Cost 50000/500000, buildTime 50000. HP 20000. StrategicManualLaunch order exists but no weapon/warhead data defined.
Tags: BUILDABLE_BY_T3_ENGINEER, CHOSEN, DEMO_UI_ONLY, STRATEGIC, STRUCTURE, TECH4.
