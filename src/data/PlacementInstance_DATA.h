// PlacementInstance_DATA.h — one resolved placed entity (the record view of the SoA).
// Layer: DATA (computed output). This is the FULL round-trip state PLACEMENT_SCATTER_SPEC
// says the old PropInstance could not carry: template id (tpId), rotation, biome stratum,
// collision, and the symmetry group. It is only the append/read *view* — the storage is the
// parallel-array SoA in PlacementInstances_DATA.h (the old struct was documented SoA and
// implemented AoS; here the record never backs the storage).
// DATA-pure: plain scalars, no PARAMS/PROC/GPU dependency (ARCH §3.1).
#pragma once

namespace SanmapGen {
namespace Data {

// A 7-character game template id plus its terminator (UNIT_PROP_MARKER_DATA_SPEC "tpId
// scheme"). `tpId` is one of the naming law's verbatim exceptions; fixed-size so the SoA
// column stays a flat POD array with no per-instance allocation.
struct TemplateIdentifier {
    char characters[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
};

inline TemplateIdentifier MakeTemplateIdentifier(const char* text) {
    TemplateIdentifier identifier;
    if (text == nullptr) return identifier;
    for (int index = 0; index < 7 && text[index] != '\0'; ++index)
        identifier.characters[index] = text[index];
    return identifier;
}

inline bool TemplateIdentifiersEqual(const TemplateIdentifier& first, const TemplateIdentifier& second) {
    for (int index = 0; index < 8; ++index)
        if (first.characters[index] != second.characters[index]) return false;
    return true;
}

struct PlacementInstance {
    float positionX = 0.0f;      // world/game units (absolute, SANMAP_FORMAT_SPEC)
    float positionY = 0.0f;      // height in game units — terrainMaxHeight comes from the map
    float positionZ = 0.0f;
    float rotationX = 0.0f;      // quaternion (x, y, z, w)
    float rotationY = 0.0f;
    float rotationZ = 0.0f;
    float rotationW = 1.0f;
    float scaleX    = 1.0f;
    float scaleY    = 1.0f;
    float scaleZ    = 1.0f;
    TemplateIdentifier templateIdentifier;
    int  ruleIndex         = 0;  // which rule produced it
    int  category          = 0;  // Params::MarkerCategory as int (DATA never includes PARAMS)
    int  symmetryIdentifier = 0; // clones of one source share this id
    int  biomeStratumIndex = 0;  // dominant material mask at the position
    int  armyIndex         = -1; // units only; -1 for markers/props/decals
    int  manualLayerId     = -1; // manual props/decals only; -1 for procedurally-scattered instances
    bool bCollidable       = false;  // collidable props are gameplay (AI_HOSTCLIENT_SPEC)
};

} // namespace Data
} // namespace SanmapGen
