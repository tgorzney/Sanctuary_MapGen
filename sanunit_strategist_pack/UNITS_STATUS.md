# Unit Playability Status (source: `availableUnits.lua`, read 2026-08)

`true` = builds and functions in the current game. `false` = restricted; reason given.
Reasons seen: `OK` (validated but still gated off), `NO_MODEL` (no art asset),
`BONE_MISSMATCH` (skeleton doesn't match rig), `OK_PENDING_APPROVAL` (content-complete,
awaiting sign-off), `BATTLE_NO_DAMAGE` (special non-combat mechanic).

## EDA (`ue`)

### Land
| tpId | name | playable | reason |
|---|---|---|---|
| uel0000 | EDACommander | true | OK |
| uel1001 | EDAT1Tank | true | OK |
| uel1002 | EDAT1FastUnit | true | OK |
| uel1101 | EDAT1MobileArty | true | OK |
| uel1201 | EDAT1MAA | false | OK |
| uel1501 | EDAT1Engineer | true | OK |
| uel1701 | EDAT1LandScout | true | OK |
| uel1801 | EDAT1MobileBomp | false | NO_MODEL |
| uel2002 | EDAT2FastUnit | false | NO_MODEL |
| uel2101 | EDAT2LandDroneCarrier | false | NO_MODEL |
| uel2201 | EDAT2MAA | false | OK |
| uel2501 | EDAT2Engineer | true | OK |
| uel2806 | EDAT2MobileRepairStation | false | NO_MODEL |
| uel3001 | EDAT3Tank | true | OK |
| uel3002 | EDAT2FastUnit2 | true | OK |
| uel3101 | EDAT3MobileArty | true | OK |
| uel3401 | EDAT3MobileStealthShieldGenerator | false | NO_MODEL |
| uel3501 | EDAT3Engineer | true | OK |
| uel3801 | EDAT3SniperBot | true | OK |
| uel4001 | EDAT4RailgunSniper | true | OK |
| uel4002 | EDAT4MobileBattleStation | true | OK |
| uel4511 | Captain Crunch | false | NO_MODEL |

### Air
| tpId | name | playable | reason |
|---|---|---|---|
| uea1001 | EDAT1Bomber | false | BONE_MISSMATCH |
| uea1201 | EDAT1AAFighter | true | OK |
| uea1502 | EDAT1Transport | false | NO_MODEL |
| uea1701 | EDAT1AirScout | true | OK |
| uea2011 | EDAT2Gunship | false | OK_PENDING_APPROVAL |
| uea2101 | EDAT2DroneCarrier | false | NO_MODEL |
| uea2301 | EDAT2TorpedoBomber | false | NO_MODEL |
| uea2502 | EDAT2Transport | false | NO_MODEL |
| uea2806 | EDAT2FlyingRepairStation | false | NO_MODEL |
| uea3001 | EDAT3Bomber | false | BONE_MISSMATCH |
| uea3011 | EDAT3Gunship | false | OK_PENDING_APPROVAL |
| uea3201 | EDAT3AAFighter | false | BONE_MISSMATCH |
| uea3701 | EDAT3AirScout | false | BONE_MISSMATCH |
| uea4001 | EDAT4CarpetBomber | false | NO_MODEL |

### Naval + naval structures + anti-naval
| tpId | name | playable | reason |
|---|---|---|---|
| uen1001 | EDAT1Frigate | false | NO_MODEL |
| ues1513 | EDAT1NavalFactory | false | NO_MODEL |
| ues1803 | EDAT2NavalTechCentre | false | NO_MODEL |
| ues2513 | EDAT2NavalFactory | false | NO_MODEL |
| ues2803 | EDAT3NavalTechCentre | false | NO_MODEL |
| ues3513 | EDAT3NavalFactory | false | NO_MODEL |
| ues1301 | EDAT1TorpedoLauncher | false | NO_MODEL |
| ues2301 | EDAT2TorpedoLauncher | false | NO_MODEL |

### Structures
| tpId | name | playable | reason |
|---|---|---|---|
| ues1001 | EDAT1PointDefence | true | OK |
| ues1201 | EDAT1AA | true | OK |
| ues1501 | EDAT1EngineeringStation | true | OK |
| ues1511 | EDAT1LandFactory | true | OK |
| ues1512 | EDAT1AirFactory | false | OK |
| ues1601 | EDAT1AlloyExtractor | true | OK |
| ues1602 | EDAT1AlloyStorage | true | OK |
| ues1611 | EDAT1EnergyGenerator | true | OK |
| ues1612 | EDAT1EnergyStorage | true | OK |
| ues1614 | EDAT1SolarConvertor | false | NO_MODEL |
| ues1701 | EDAT1Radar | true | OK |
| ues1702 | EDAT1Sonar | false | NO_MODEL |
| ues1801 | EDAT2LandTechCentre | false | OK_PENDING_APPROVAL |
| ues1802 | EDAT2AirTechCentre | false | OK_PENDING_APPROVAL |
| ues1804 | EDAT1Wall | false | NO_MODEL |
| ues2001 | EDAT2PointDefence | true | OK |
| ues2101 | EDAT2Arty | false | NO_MODEL |
| ues2102 | EDAT2DroneLauncher | false | NO_MODEL |
| ues2201 | EDAT2AA | true | OK |
| ues2401 | EDAT2Shield | true | OK |
| ues2402 | EDAT2AntiDroneDefence | false | NO_MODEL |
| ues2501 | EDAT2EngineeringStation | true | OK |
| ues2510 | EDAT2OperatingTheatre | false | NO_MODEL |
| ues2511 | EDAT2LandFactory | true | OK |
| ues2512 | EDAT2AirFactory | true | OK |
| ues2601 | EDAT2AlloyExtractor | true | OK |
| ues2611 | EDAT2EnergyGenerator | true | OK |
| ues2701 | EDAT2Radar | true | OK |
| ues2702 | EDAT2Sonar | false | NO_MODEL |
| ues2801 | EDAT3LandTechCentre | false | OK_PENDING_APPROVAL |
| ues2802 | EDAT3AirTechCentre | false | OK_PENDING_APPROVAL |
| ues2806 | EDAT2RepairStation | false | NO_MODEL |
| ues3102 | EDAT3StrategicLauncher | false | NO_MODEL |
| ues3201 | EDAT3AA | true | OK |
| ues3401 | EDAT3Shield | true | OK |
| ues3402 | EDAT3StrategicDefense | false | OK_PENDING_APPROVAL |
| ues3501 | EDAT3EngineeringStation | true | OK |
| ues3511 | EDAT3LandFactory | true | OK |
| ues3512 | EDAT3AirFactory | true | OK |
| ues3601 | EDAT3AlloyExtractor | true | OK |
| ues3603 | EDAT3AlloyFurnace | true | OK |
| ues3611 | EDAT3EnergyGenerator | true | OK |
| ues3701 | EDAT3Radar | true | OK |
| ues3702 | EDAT3Sonar | false | NO_MODEL |
| ues4101 | EDAT4HeavyArty | false | NO_MODEL |
| ues1111 | FreezeStation (unfactioned) | false | BATTLE_NO_DAMAGE |

## CHOSEN (`uc`)

### Land
| tpId | name | playable | reason |
|---|---|---|---|
| ucl0000 | ChosenCommander | true | OK |
| ucl1001 | ChosenT1Tank | true | OK |
| ucl1002 | ChosenT1FastUnit | true | OK |
| ucl1101 | ChosenT1MobileArty | true | OK |
| ucl1201 | ChosenT1MAA | false | OK |
| ucl1501 | ChosenT1Engineer | true | OK |
| ucl1701 | ChosenT1LandScout | true | OK |
| ucl2002 | ChosenT2FastUnit | true | OK |
| ucl2101 | ChosenT2LandDroneCarrier | false | OK_PENDING_APPROVAL |
| ucl2201 | ChosenT2MAA | false | OK |
| ucl2501 | ChosenT2Engineer | true | OK |
| ucl2806 | ChosenT2MobileShieldBooster | false | OK_PENDING_APPROVAL |
| ucl3001 | ChosenT3Tank | true | OK |
| ucl3101 | ChosenT3MobileArty | true | OK |
| ucl3501 | ChosenT3Engineer | true | OK |
| ucl3701 | ChosenT3LandScout | true | OK |
| ucl3801 | ChosenT3SniperBot | true | OK |
| ucl4001 | ChosenT4Bot | true | OK |
| ucl4002 | ChosenT4TripodBot | true | OK |
| ucl4003 | ChosenT4BotHover | true | OK |
| ucl4004 | ChosenT4BotBig | true | OK |
| ucl4005 | ChosenT4BotMega | false | NO_MODEL |
| ucl4401 | ChosenT4MobileShield | true | OK |

### Air
| tpId | name | playable | reason |
|---|---|---|---|
| uca1001 | ChosenT1Bomber | true | OK |
| uca1201 | ChosenT1AAFighter | true | OK |
| uca1502 | ChosenT1Transport | false | NO_MODEL |
| uca1701 | ChosenT1AirScout | true | OK |
| uca2011 | ChosenT2Gunship | false | OK_PENDING_APPROVAL |
| uca2101 | ChosenT2DroneCarrier | false | NO_MODEL |
| uca2301 | ChosenT2TorpedoBomber | false | NO_MODEL |
| uca2502 | ChosenT2Transport | false | NO_MODEL |
| uca2701 | ChosenT2AirScout | false | OK_PENDING_APPROVAL |
| uca2806 | ChosenT2FlyingShieldBooster | false | NO_MODEL |
| uca3001 | ChosenT3Bomber | false | NO_MODEL |
| uca3011 | ChosenT3Gunship | false | OK_PENDING_APPROVAL |
| uca3201 | ChosenT3AAFighter | false | OK_PENDING_APPROVAL |
| uca3701 | ChosenT3AirScout | false | OK_PENDING_APPROVAL |
| uca4011 | ChosenT4Gunship | false | OK_PENDING_APPROVAL |

### Naval + naval structures + anti-naval
| tpId | name | playable | reason |
|---|---|---|---|
| ucn1001 | ChosenT1Frigate | false | OK_PENDING_APPROVAL |
| ucn3001 | ChosenT3Battleship | false | OK_PENDING_APPROVAL |
| ucs1513 | ChosenT1NavalFactory | false | OK_PENDING_APPROVAL |
| ucs1803 | ChosenT2NavalTechCentre | false | NO_MODEL |
| ucs2513 | ChosenT2NavalFactory | false | OK_PENDING_APPROVAL |
| ucs2803 | ChosenT3NavalTechCentre | false | NO_MODEL |
| ucs3513 | ChosenT3NavalFactory | false | OK_PENDING_APPROVAL |
| ucs1301 | ChosenT1TorpedoLauncher | false | NO_MODEL |
| ucs2301 | ChosenT2TorpedoLauncher | false | NO_MODEL |

### Structures
| tpId | name | playable | reason |
|---|---|---|---|
| ucs1001 | ChosenT1PointDefence | true | OK |
| ucs1201 | ChosenT1AA | false | OK |
| ucs1501 | ChosenT1EngineeringStation | true | OK |
| ucs1511 | ChosenT1LandFactory | true | OK |
| ucs1512 | ChosenT1AirFactory | false | OK |
| ucs1601 | ChosenT1AlloyExtractor | true | OK |
| ucs1602 | ChosenT1AlloyStorage | true | OK |
| ucs1611 | ChosenT1EnergyGenerator | true | OK |
| ucs1612 | ChosenT1EnergyStorage | true | OK |
| ucs1614 | ChosenT1SolarConvertor | false | NO_MODEL |
| ucs1701 | ChosenT1Radar | true | OK |
| ucs1702 | ChosenT1Sonar | false | NO_MODEL |
| ucs1801 | ChosenT2LandTechCentre | false | OK_PENDING_APPROVAL |
| ucs1802 | ChosenT2AirTechCentre | false | OK_PENDING_APPROVAL |
| ucs1804 | ChosenT1Wall | false | NO_MODEL |
| ucs2001 | ChosenT2PointDefence | true | OK |
| ucs2101 | ChosenT2Arty | true | OK |
| ucs2102 | ChosenT2DroneLauncher | false | OK_PENDING_APPROVAL |
| ucs2201 | ChosenT2AA | true | OK |
| ucs2401 | ChosenT2Shield | true | OK |
| ucs2402 | ChosenT2AntiDroneDefence | false | NO_MODEL |
| ucs2501 | ChosenT2EngineeringStation | true | OK |
| ucs2510 | ChosenT2OperatingTheatre | false | NO_MODEL |
| ucs2511 | ChosenT2LandFactory | true | OK |
| ucs2512 | ChosenT2AirFactory | true | OK |
| ucs2601 | ChosenT2AlloyExtractor | true | OK |
| ucs2611 | ChosenT2EnergyGenerator | true | OK |
| ucs2701 | ChosenT2Radar | true | OK |
| ucs2702 | ChosenT2Sonar | false | NO_MODEL |
| ucs2711 | ChosenT2StealthField | false | OK_PENDING_APPROVAL |
| ucs2801 | ChosenT3LandTechCentre | false | OK_PENDING_APPROVAL |
| ucs2802 | ChosenT3AirTechCentre | false | OK_PENDING_APPROVAL |
| ucs2806 | ChosenT2ShieldBoosterStation | false | NO_MODEL |
| ucs3001 | ChosenT3PointDefence | true | OK |
| ucs3101 | ChosenT3Arty | true | OK |
| ucs3102 | ChosenT3StrategicLauncher | false | OK_PENDING_APPROVAL |
| ucs3201 | ChosenT3AA | true | OK |
| ucs3401 | ChosenT3Shield | true | OK |
| ucs3402 | ChosenT3StrategicDefense | false | OK_PENDING_APPROVAL |
| ucs3501 | ChosenT3EngineeringStation | true | OK |
| ucs3511 | ChosenT3LandFactory | true | OK |
| ucs3512 | ChosenT3AirFactory | true | OK |
| ucs3601 | ChosenT3AlloyExtractor | true | OK |
| ucs3603 | ChosenT3AlloyFurnace | true | OK |
| ucs3611 | ChosenT3EnergyGenerator | true | OK |
| ucs3701 | ChosenT3Radar | true | OK |
| ucs3702 | ChosenT3Sonar | false | NO_MODEL |
| ucs4102 | ChosenT4StrategicLauncher | false | OK_PENDING_APPROVAL |

## GUARD (`ug`) + unfactioned

### Land
| tpId | name | playable | reason |
|---|---|---|---|
| ugl0000 | GuardCommander | true | OK |
| ugl1001 | GuardT1Tank | true | OK |
| ugl1002 | GuardT1FastUnit | true | OK |
| ugl1101 | GuardT1MobileArty | true | OK |
| ugl1201 | GuardT1MAA | false | OK |
| ugl1501 | GuardT1Engineer | true | OK |
| ugl1701 | GuardT1LandScout | true | OK |
| ugl2002 | GuardT2FastUnit | true | OK |
| ugl2101 | GuardT2GranadeBot | true | OK |
| ugl2201 | GuardT2MAA | false | OK |
| ugl2501 | GuardT2Engineer | true | OK |
| ugl2806 | GuardT2MobileTransmitter | false | OK_PENDING_APPROVAL |
| ugl3001 | GuardT3Tank | true | OK |
| ugl3002 | GuardT3FastUnit | true | OK |
| ugl3101 | GuardT3MobileDroneCarrier | false | OK_PENDING_APPROVAL |
| ugl3501 | GuardT3Engineer | true | OK |
| ugl3502 | GuardT3CombatEngineer | false | OK_PENDING_APPROVAL |
| ugl4001 | GuardT4Bot | true | OK |
| ugl4011 | GuardT4Bot ("Quasar") | true | OK |

### Air
| tpId | name | playable | reason |
|---|---|---|---|
| uga1001 | GuardT1Bomber | false | BONE_MISSMATCH |
| uga1011 | GuardT1Gunship | false | OK_PENDING_APPROVAL |
| uga1201 | GuardT1AAFighter | true | OK |
| uga1502 | GuardT1Transport | false | NO_MODEL |
| uga1701 | GuardT1AirScout | true | OK |
| uga2011 | GuardT2Gunship | false | OK_PENDING_APPROVAL |
| uga2101 | GuardT2DroneCarrier | false | OK_PENDING_APPROVAL |
| uga2301 | GuardT2TorpedoBomber | false | OK_PENDING_APPROVAL |
| uga2502 | GuardT2Transport | false | OK_PENDING_APPROVAL |
| uga2806 | GuardT2FlyingTransmitter | false | NO_MODEL |
| uga3001 | GuardT3Bomber | false | BONE_MISSMATCH |
| uga3011 | GuardT1Gunship (dup tpId slot, T3-caliber stats — data anomaly) | false | NO_MODEL |
| uga3201 | GuardT3AAFighter | false | BONE_MISSMATCH |
| uga3701 | GuardT3AirScout | false | OK_PENDING_APPROVAL |
| uga4001 | GuardT4MothershipShatterer | false | NO_MODEL |
| uga4801 | GuardT4Mothership | false | NO_MODEL |

### Naval + naval structures + anti-naval
| tpId | name | playable | reason |
|---|---|---|---|
| ugn1001 | GuardT1Frigate | false | NO_MODEL |
| ugs1513 | GuardT1NavalFactory | false | OK_PENDING_APPROVAL |
| ugs1803 | GuardT2NavalTechCentre | false | OK_PENDING_APPROVAL |
| ugs2513 | GuardT2NavalFactory | false | OK_PENDING_APPROVAL |
| ugs2803 | GuardT3NavalTechCentre | false | OK_PENDING_APPROVAL |
| ugs3513 | GuardT3NavalFactory | false | OK_PENDING_APPROVAL |
| ugs1301 | GuardT1TorpedoLauncher | false | OK_PENDING_APPROVAL |
| ugs2301 | GuardT2TorpedoLauncher | false | OK_PENDING_APPROVAL |

### Structures
| tpId | name | playable | reason |
|---|---|---|---|
| ugs1001 | GuardT1PointDefence | true | OK |
| ugs1201 | GuardT1AA | true | OK |
| ugs1501 | GuardT1EngineeringStation | true | OK |
| ugs1511 | GuardT1LandFactory | true | OK |
| ugs1512 | GuardT1AirFactory | false | OK |
| ugs1601 | GuardT1AlloyExtractor | true | OK |
| ugs1602 | GuardT1AlloyStorage | true | OK |
| ugs1611 | GuardT1EnergyGenerator | true | OK |
| ugs1612 | GuardT1EnergyStorage | true | OK |
| ugs1614 | GuardT1SolarConvertor | false | NO_MODEL |
| ugs1701 | GuardT1Radar | true | OK |
| ugs1702 | GuardT1Sonar | false | OK_PENDING_APPROVAL |
| ugs1801 | GuardT2LandTechCentre | false | OK_PENDING_APPROVAL |
| ugs1802 | GuardT2AirTechCentre | false | OK_PENDING_APPROVAL |
| ugs1804 | GuardT1Wall | false | OK_PENDING_APPROVAL |
| ugs1805 | GuardT1Airfield | false | OK_PENDING_APPROVAL |
| ugs1806 | GuardT1Transmitter | false | OK_PENDING_APPROVAL |
| ugs2001 | GuardT2PointDefence | true | OK |
| ugs2101 | GuardT2Arty | true | OK |
| ugs2102 | GuardT2DroneLauncher | false | OK_PENDING_APPROVAL |
| ugs2201 | GuardT2AA | true | OK |
| ugs2401 | GuardT2Shield | true | OK |
| ugs2402 | GuardT2AntiDroneDefence | false | OK_PENDING_APPROVAL |
| ugs2501 | GuardT2EngineeringStation | true | OK |
| ugs2510 | GuardT2OperatingTheatre | false | NO_MODEL |
| ugs2511 | GuardT2LandFactory | true | OK |
| ugs2512 | GuardT2AirFactory | true | OK |
| ugs2601 | GuardT2AlloyExtractor | true | OK |
| ugs2611 | GuardT2EnergyGenerator | true | OK |
| ugs2701 | GuardT2Radar | true | OK |
| ugs2702 | GuardT2Sonar | false | OK_PENDING_APPROVAL |
| ugs2711 | GuardT2StealthField | false | OK_PENDING_APPROVAL |
| ugs2801 | GuardT3LandTechCentre | false | OK_PENDING_APPROVAL |
| ugs2802 | GuardT3AirTechCentre | false | OK_PENDING_APPROVAL |
| ugs2805 | GuardT2Airfield | false | OK_PENDING_APPROVAL |
| ugs2806 | GuardT2Transmitter | false | OK_PENDING_APPROVAL |
| ugs2807 | Terminal (GuardT2Transmitter variant) | false | NO_MODEL |
| ugs3101 | GuardT3Arty ("Grinder") | true | OK |
| ugs3102 | GuardT3StrategicLauncher | false | OK_PENDING_APPROVAL |
| ugs3201 | GuardT3AA | true | OK |
| ugs3401 | GuardT3Shield | true | OK |
| ugs3402 | GuardT3StrategicDefense | false | OK_PENDING_APPROVAL |
| ugs3501 | GuardT3EngineeringStation | true | OK |
| ugs3511 | GuardT3LandFactory | true | OK |
| ugs3512 | GuardT3AirFactory | true | OK |
| ugs3601 | GuardT3AlloyExtractor | true | OK |
| ugs3603 | GuardT3AlloyFurnace | true | OK |
| ugs3611 | GuardT3EnergyGenerator | true | OK |
| ugs3701 | GuardT3Radar | true | OK |
| ugs3702 | GuardT3Sonar | false | OK_PENDING_APPROVAL |
| ugs3805 | GuardT3Airfield | false | NO_MODEL |
| ugs4012 | GuardT4Arty ("Tachyon") | false | OK_PENDING_APPROVAL |
| ugs4102 | GuardT4HiveHole | false | NO_MODEL |
| ugs4621 | GuardT4ExperimentalGenerator ("Crucible") | true | OK |
| uws1000 | CryoTank (unfactioned map object, GUARD-tagged, UNCLAIMABLE) | false | OK_PENDING_APPROVAL |

## Totals
~283 templates exist; roughly half are playable (`true`) at snapshot time.
Playable skews heavily toward T1/T2/T3 land units and economy/defence
structures. Nearly the entire naval tree (all 3 factions) and most T2+ air
support units (transports, drone carriers, torpedo bombers) are disabled.
