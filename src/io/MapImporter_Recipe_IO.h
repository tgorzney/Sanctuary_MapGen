// MapImporter_Recipe_IO.h — MODULE-INTERNAL JSON readers behind MapImporter::ParseSanmapJsonText.
// Layer: IO. Split out under the ARCH §1.5 ceilings; declares no new public type (ARCH §8.4).
//
// EVERY reader here is total: it takes the parent object and a key, and it writes the destination
// ONLY when the key exists with the right JSON type. That is Constitution §6 in one pattern — a
// corrupt or partial document leaves the recipe on its own defaults instead of on garbage.
#pragma once
#include <nlohmann/json.hpp>
#include <string>

namespace SanmapGen {
namespace Params { struct MapRecipe; struct LayerStack; }
namespace Io {

struct MapImportOptions;
struct MapImportResult;

// --- The typed accessors every reader is built from. -------------------------------------------
inline bool ReadJsonFloat(const nlohmann::json& parent, const char* key, float& destination) {
    if (!parent.contains(key) || !parent[key].is_number()) return false;
    destination = parent[key].get<float>();
    return true;
}

inline bool ReadJsonInteger(const nlohmann::json& parent, const char* key, int& destination) {
    if (!parent.contains(key) || !parent[key].is_number()) return false;
    destination = parent[key].get<int>();
    return true;
}

inline bool ReadJsonBoolean(const nlohmann::json& parent, const char* key, bool& destination) {
    if (!parent.contains(key) || !parent[key].is_boolean()) return false;
    destination = parent[key].get<bool>();
    return true;
}

inline bool ReadJsonText(const nlohmann::json& parent, const char* key, std::string& destination) {
    if (!parent.contains(key) || !parent[key].is_string()) return false;
    destination = parent[key].get<std::string>();
    return true;
}

// An enum stored as its integer value. The value is FENCED to [0, valueCount) so a document
// written by a newer build can never index an enum out of range.
inline bool ReadJsonEnumeration(const nlohmann::json& parent, const char* key, int valueCount,
                                int& destination) {
    int value = destination;
    if (!ReadJsonInteger(parent, key, value)) return false;
    if (value < 0 || value >= valueCount) return false;
    destination = value;
    return true;
}

// A 4-component field stored as `{"x":.., "y":.., "z":.., "w":..}` (ARCH §7.2 item 10's Vector4
// shape). Each component is read independently, same as the scalar readers above, so a partial
// object still updates the components it has instead of discarding the whole field.
inline bool ReadJsonFloatVector4(const nlohmann::json& parent, const char* key,
                                 float destination[4]) {
    if (!parent.contains(key) || !parent[key].is_object()) return false;
    const nlohmann::json& vector = parent[key];
    bool bAnyComponentRead = false;
    bAnyComponentRead |= ReadJsonFloat(vector, "x", destination[0]);
    bAnyComponentRead |= ReadJsonFloat(vector, "y", destination[1]);
    bAnyComponentRead |= ReadJsonFloat(vector, "z", destination[2]);
    bAnyComponentRead |= ReadJsonFloat(vector, "w", destination[3]);
    return bAnyComponentRead;
}

// --- The block readers (MapImporter_Recipe_IO.cpp / MapImporter_Layers_IO.cpp). -----------------
void ReadGeometryJson(const nlohmann::json& generatorData, const MapImportOptions& options,
                      Params::MapRecipe& outRecipe, MapImportResult& result);
void ReadWaterJson(const nlohmann::json& generatorData, Params::MapRecipe& outRecipe);
void ReadLayerStackJson(const nlohmann::json& generatorData, Params::LayerStack& outLayerStack);
void ReadStrataSettingsJson(const nlohmann::json& generatorData, Params::MapRecipe& outRecipe);
void ReadPlacementRulesJson(const nlohmann::json& generatorData, Params::MapRecipe& outRecipe);

} // namespace Io
} // namespace SanmapGen
