// MapExporter_Armies_IO.cpp — `recipe.armies` -> the top-level `.sanmap` `armies` dictionary.
// Layer: IO. Own file (not shared with Areas): the more complex of the two domains — three levels
// of recursion (Army -> UnitGroup -> UnitGroup*/UnitTransform*), a Color-shaped field, and the
// bounded `tpid` buffer copy (IO Architecture Expert ruling, STEP2_ArmiesAreas_IO).
#include "MapExporter_IO.h"
#include "MapExporter_Recipe_IO.h"
#include "Sanmap_ArmyIdentity_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

// `tpid` reuses the exact bounded-buffer convention MapExporter_Rules_IO.cpp already uses for
// ScatterTransform::templateIdentifier (char[8], NUL-safe copy) — same field, same type.
std::string BoundedTemplateIdentifier(const char (&templateIdentifier)[8]) {
    std::size_t identifierLength = 0;
    while (identifierLength < sizeof(templateIdentifier) && templateIdentifier[identifierLength] != '\0')
        ++identifierLength;
    return std::string(templateIdentifier, identifierLength);
}

nlohmann::ordered_json BuildUnitTransformJson(const Params::UnitTransform& unit, int mapSize) {
    nlohmann::ordered_json json;
    // The coordinate flip applies to UnitTransform.positionZ (finding 3): `world.z = mapSize -
    // positionZ - 1`. positionX/positionY and rotation/scale are untouched by the flip.
    json["position"] = { { "x", unit.positionX }, { "y", unit.positionY },
                         { "z", static_cast<float>(mapSize) - unit.positionZ - 1.0f } };
    // WATCH-ROTATION-FLIP: rotation round-trips verbatim, no coordinate transform (finding 4,
    // human-ratified item 2) — UNCONFIRMED whether this is correct. If in-game testing shows a
    // placed unit facing the wrong direction, this is where the fix goes (mirror rotationZ/W or
    // whichever component the dev's answer identifies). See STEP2_ArmiesAreas_IO.md "Open items".
    json["rotation"] = { { "x", unit.rotationX }, { "y", unit.rotationY },
                         { "z", unit.rotationZ }, { "w", unit.rotationW } };
    json["scale"]    = { { "x", unit.scaleX }, { "y", unit.scaleY }, { "z", unit.scaleZ } };
    json["type"]     = unit.legacyTypeTag;
    json["tpid"]     = BoundedTemplateIdentifier(unit.templateIdentifier);
    return json;
}

nlohmann::ordered_json BuildUnitGroupJson(const Params::UnitGroup& group, int mapSize) {
    nlohmann::ordered_json units = nlohmann::ordered_json::object();
    for (const Params::UnitTransform& unit : group.units)
        units[unit.name] = BuildUnitTransformJson(unit, mapSize);

    nlohmann::ordered_json nestedGroups = nlohmann::ordered_json::object();
    for (const Params::UnitGroup& nestedGroup : group.groups)
        nestedGroups[nestedGroup.name] = BuildUnitGroupJson(nestedGroup, mapSize);

    nlohmann::ordered_json json;
    json["units"]  = units;
    json["groups"] = nestedGroups;
    return json;
}

nlohmann::ordered_json BuildArmyGroupsJson(const Params::Army& army, int mapSize) {
    nlohmann::ordered_json groups = nlohmann::ordered_json::object();
    for (const Params::UnitGroup& group : army.groups)
        groups[group.name] = BuildUnitGroupJson(group, mapSize);
    return groups;
}

} // namespace

nlohmann::ordered_json BuildArmiesJson(const Params::MapRecipe& recipe) {
    const int mapSize = recipe.geometry.mapSize;
    nlohmann::ordered_json armies = nlohmann::ordered_json::object();
    for (const Params::Army& army : recipe.armies) {
        nlohmann::ordered_json armyJson;
        armyJson["faction"] = static_cast<int>(army.faction);
        armyJson["alloys"]  = army.alloys;
        armyJson["energy"]  = army.energy;
        armyJson["groups"]  = BuildArmyGroupsJson(army, mapSize);
        armyJson["armyColor"] = { { "r", army.armyColor[0] }, { "g", army.armyColor[1] },
                                  { "b", army.armyColor[2] }, { "a", army.armyColor[3] } };
        armyJson["alias"] = army.alias;
        // STEP76 §2: `displayName` merges directly into the format-native `armies[<ARMY_XX>]`
        // object, lowerCamelCase, alongside `armyColor`/`alias` (ARCH_01_06_SanmapKeyCasing.md §1.6,
        // §1.8's direct-injection branch) — NOT a new top-level section.
        armyJson["displayName"] = army.displayName;
        armies[army.name] = armyJson;
    }
    return armies;
}

// STEP76 §3c — the export-time guard. `ArmiesTab_UI.cpp`'s own re-mint (§3b) keeps
// `recipe.armies` correct on every roster mutation reachable through the tab; this exists only for
// the paths that don't go through it — a recipe built by a test, a future headless/CLI export, or
// any caller that mutates `recipe.armies` directly. Option (ii) from the ticket: VERIFIES and warns,
// never rewrites — `BuildArmiesJson` above stays pure/const, and this never needs a `const_cast`.
// Should fire zero times on a recipe that ever passed through the tab (AssignArmyIdentities is
// idempotent, so a caller that DID go through the tab sees no warning here either).
void CheckArmyIdentitiesWellFormed(const std::vector<Params::Army>& armies, MapExportResult& result) {
    for (std::size_t armyIndex = 0u; armyIndex < armies.size(); ++armyIndex) {
        const std::string expectedIdentity =
            ArmyIdentityForRosterPosition(static_cast<int>(armyIndex) + 1);
        if (armies[armyIndex].name == expectedIdentity) continue;
        result.Log("SANGEN: army \"" + armies[armyIndex].name + "\" (roster position "
                   + std::to_string(armyIndex + 1) + ") is not the expected engine identity \""
                   + expectedIdentity + "\". The engine assigns lobby slots by ALPHABETICAL name "
                   "order, so a non-conforming name silently maps armies to the wrong slots once a "
                   "map has 10 or more armies. Scenario spawn positions and alloy occupancy will be "
                   "assigned to the wrong armies.");
    }
}

} // namespace Io
} // namespace SanmapGen
