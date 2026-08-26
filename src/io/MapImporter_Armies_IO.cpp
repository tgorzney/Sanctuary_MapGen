// MapImporter_Armies_IO.cpp — the top-level `.sanmap` `armies` dictionary -> `recipe.armies`.
// Layer: IO. The exact inverse of MapExporter_Armies_IO.cpp. `armies` -> `Army.groups` ->
// `UnitGroup.groups`/`UnitGroup.units`, three levels of name-keyed JSON objects (ENTITY_AUTHORING_
// PARAMS_SPEC finding 1) — the local `ReadNameKeyedObject` template below mirrors
// MapImporter_Rules_IO.cpp's file-local `ReadRuleArray`, kept file-local per the IO Architecture
// Expert ruling (promote only if a THIRD domain later needs the identical shape).
#include "JsonPrimitives_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

constexpr int factionCount = 3;   // Chosen, Guard, EDA

// The buffer is fixed width and must stay NUL-terminated whatever the document claimed — the same
// bounded copy ReadScatterTransformJson (MapImporter_ScatterTransform_IO.cpp) uses for `tpid`.
void CopyBoundedTemplateIdentifier(const std::string& text, char (&templateIdentifier)[8]) {
    const std::size_t capacity = sizeof(templateIdentifier) - 1u;
    const std::size_t copyLength = text.size() < capacity ? text.size() : capacity;
    for (std::size_t index = 0; index < sizeof(templateIdentifier); ++index)
        templateIdentifier[index] = index < copyLength ? text[index] : '\0';
}

// One name-keyed JSON object -> one vector, the folded-in dictionary key becoming ItemType::name
// (ENTITY_AUTHORING_PARAMS_SPEC's structural ruling). A non-object entry still yields a
// default-constructed (named) item instead of aborting the whole domain.
template <typename ItemType, typename ReadOneItemFunction>
void ReadNameKeyedObject(const nlohmann::json& parent, const char* key, std::vector<ItemType>& outItems,
                         ReadOneItemFunction ReadOneItem) {
    if (!parent.contains(key) || !parent[key].is_object()) return;
    outItems.clear();
    for (const auto& [name, valueJson] : parent[key].items()) {
        ItemType item;
        item.name = name;
        if (valueJson.is_object()) ReadOneItem(valueJson, item);
        outItems.push_back(item);
    }
}

// `position`/`rotation`/`scale` are nested {x,y,z}/{x,y,z,w} objects (finding 2); `type`/`tpid`
// are siblings, not nested. `positionZ` inverts the export-side flip (finding 3): `positionZ =
// mapSize - jsonZ - 1`.
void ReadUnitTransformJson(const nlohmann::json& json, Params::UnitTransform& unit, int mapSize) {
    if (json.contains("position") && json["position"].is_object()) {
        const nlohmann::json& position = json["position"];
        ReadJsonFloat(position, "x", unit.positionX);
        ReadJsonFloat(position, "y", unit.positionY);
        float jsonPositionZ = static_cast<float>(mapSize) - unit.positionZ - 1.0f;
        if (ReadJsonFloat(position, "z", jsonPositionZ))
            unit.positionZ = static_cast<float>(mapSize) - jsonPositionZ - 1.0f;
    }
    if (json.contains("rotation") && json["rotation"].is_object()) {
        const nlohmann::json& rotation = json["rotation"];
        // WATCH-ROTATION-FLIP: rotation round-trips verbatim, no coordinate transform (finding 4,
        // ratified item 2) — UNCONFIRMED whether this is correct. If in-game testing shows a
        // placed unit facing the wrong direction, this is where the fix goes. See
        // STEP2_ArmiesAreas_IO.md "Open items".
        ReadJsonFloat(rotation, "x", unit.rotationX);
        ReadJsonFloat(rotation, "y", unit.rotationY);
        ReadJsonFloat(rotation, "z", unit.rotationZ);
        ReadJsonFloat(rotation, "w", unit.rotationW);
    }
    if (json.contains("scale") && json["scale"].is_object()) {
        const nlohmann::json& scale = json["scale"];
        ReadJsonFloat(scale, "x", unit.scaleX);
        ReadJsonFloat(scale, "y", unit.scaleY);
        ReadJsonFloat(scale, "z", unit.scaleZ);
    }
    ReadJsonText(json, "type", unit.legacyTypeTag);
    std::string templateIdentifier;
    if (ReadJsonText(json, "tpid", templateIdentifier))
        CopyBoundedTemplateIdentifier(templateIdentifier, unit.templateIdentifier);
}

void ReadUnitGroupJson(const nlohmann::json& json, Params::UnitGroup& group, int mapSize) {
    ReadNameKeyedObject(json, "units", group.units,
                        [mapSize](const nlohmann::json& unitJson, Params::UnitTransform& unit) {
                            ReadUnitTransformJson(unitJson, unit, mapSize);
                        });
    ReadNameKeyedObject(json, "groups", group.groups,
                        [mapSize](const nlohmann::json& groupJson, Params::UnitGroup& nestedGroup) {
                            ReadUnitGroupJson(groupJson, nestedGroup, mapSize);
                        });
}

// Reports whether "armyColor" was present at all (ARCH_14_16_PerArmyUnitsOverlayRows.md §14.16-D's
// key-presence discriminator, never a value comparison against white) — same bool-reporting pattern
// ReadJsonFloat already uses elsewhere in this file.
bool ReadArmyColorJson(const nlohmann::json& armyJson, float armyColor[4]) {
    if (!armyJson.contains("armyColor") || !armyJson["armyColor"].is_object()) return false;
    const nlohmann::json& color = armyJson["armyColor"];
    ReadJsonFloat(color, "r", armyColor[0]);
    ReadJsonFloat(color, "g", armyColor[1]);
    ReadJsonFloat(color, "b", armyColor[2]);
    ReadJsonFloat(color, "a", armyColor[3]);
    return true;
}

void ReadArmyJson(const nlohmann::json& armyJson, Params::Army& army, int mapSize) {
    int factionValue = static_cast<int>(army.faction);
    if (ReadJsonEnumeration(armyJson, "faction", factionCount, factionValue))
        army.faction = static_cast<Params::Faction>(factionValue);
    ReadJsonFloat(armyJson, "alloys", army.alloys);
    ReadJsonFloat(armyJson, "energy", army.energy);
    ReadNameKeyedObject(armyJson, "groups", army.groups,
                        [mapSize](const nlohmann::json& groupJson, Params::UnitGroup& group) {
                            ReadUnitGroupJson(groupJson, group, mapSize);
                        });
    ReadArmyColorJson(armyJson, army.armyColor);   // an absent key is backfilled by the second pass below
    ReadJsonText(armyJson, "alias", army.alias);
    // STEP76 §2/§8: `displayName` merges into the format-native `armies[<ARMY_XX>]` object, a
    // sibling of `alias`. `name` itself (the outer dict key) is re-minted unconditionally right
    // after this by NormalizeArmyIdentities — never trusted as authoritative from the document.
    ReadJsonText(armyJson, "displayName", army.displayName);
}

// ARCH_14_16_PerArmyUnitsOverlayRows.md §14.16-D: a SECOND pass over the same `armies` JSON object
// (not a counter threaded through ReadNameKeyedObject's per-item lambda above, which skips
// ReadOneItem for a malformed non-object entry and would desync from `armies`' real vector index).
// Same iteration order/source as the first pass, so position i here IS armies[i]'s roster position.
void BackfillMissingArmyColors(const nlohmann::json& armiesJson, std::vector<Params::Army>& armies) {
    std::size_t rosterPosition = 0;
    for (const auto& [name, armyJson] : armiesJson.items()) {
        if (rosterPosition >= armies.size()) break;
        const bool bHasColorKey = armyJson.is_object() && armyJson.contains("armyColor")
                                 && armyJson["armyColor"].is_object();
        if (!bHasColorKey)
            Params::SeedDefaultArmyColor(armies[rosterPosition].armyColor, static_cast<int>(rosterPosition));
        ++rosterPosition;
    }
}

} // namespace

void ReadArmiesJson(const nlohmann::json& document, Params::MapRecipe& outRecipe) {
    if (!document.contains("armies") || !document["armies"].is_object()) return;
    const int mapSize = outRecipe.geometry.mapSize;   // already populated from top-level `width`
                                                       // before this is called — see the "Critical
                                                       // wiring correction" note in MapImporter_IO.cpp.
    ReadNameKeyedObject(document, "armies", outRecipe.armies,
                        [mapSize](const nlohmann::json& armyJson, Params::Army& army) {
                            ReadArmyJson(armyJson, army, mapSize);
                        });
    BackfillMissingArmyColors(document["armies"], outRecipe.armies);
}

} // namespace Io
} // namespace SanmapGen
