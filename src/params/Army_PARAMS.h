// Army_PARAMS.h — the hand-placed army/unit roster: `Faction`, `UnitTransform`, `UnitGroup`,
// `Army`. Layer: PARAMS. Manually-authored, pass-through entity data (ENTITY_AUTHORING_PARAMS_SPEC
// "Scope" — the opposite kind of data from a procedural `UnitRule`): round-trip fidelity through
// the `.sanmap` `armies` dictionary is the entire purpose, no PROC stage computes or reinterprets
// these fields. Verbatim from ENTITY_AUTHORING_PARAMS_SPEC.md's "The types" section.
#pragma once
#include <cstddef>
#include <string>
#include <vector>

namespace SanmapGen {
namespace Params {

enum class Faction { Chosen, Guard, EDA };   // 0/1/2 — UNIT_PROP_MARKER_DATA_SPEC "Factions"

struct UnitTransform {
    std::string name;                  // folded-in dictionary key (UnitGroup.units[key])

    float positionX = 0.0f;            // world/game units, absolute (SANMAP_FORMAT_SPEC)
    float positionY = 0.0f;            // elevation
    float positionZ = 0.0f;
    float rotationX = 0.0f;            // quaternion (x, y, z, w)
    float rotationY = 0.0f;
    float rotationZ = 0.0f;
    float rotationW = 1.0f;
    float scaleX    = 1.0f;
    float scaleY    = 1.0f;
    float scaleZ    = 1.0f;

    // `tpid` — the identity SanGen actually uses. §1.8 named exception, NUL-safe bounded buffer,
    // same convention as ScatterTransform::templateIdentifier.
    char        templateIdentifier[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    // `type` — non-canonical passthrough only (see ENTITY_AUTHORING_PARAMS_SPEC's dedicated
    // section). Never computed, interpreted, or branched on by any SanGen stage.
    std::string legacyTypeTag;
};

struct UnitGroup {
    std::string name;                          // folded-in dictionary key (parent's `groups[key]`)
    std::vector<UnitTransform> units;           // this group's own units
    std::vector<UnitGroup>     groups;          // nested child groups (recursive)
};

struct Army {
    // ENGINE IDENTITY — `ARMY_XX`, the folded-in `armies[key]` (ARCH_01_08_ParamsFieldNamingByKind.md
    // §1.8). MACHINE-OWNED as of STEP76: minted and re-minted by AssignArmyIdentities
    // (Sanmap_ArmyIdentity_IO.h) from this army's 1-based roster position, so an ALPHABETICAL sort
    // of the roster's names equals roster order — the property `common/gameUtils.lua`'s
    // CreateArmies() relies on to assign lobby slots. NEVER bound to a text input, NEVER
    // user-settable, NEVER read from a `.sanmap` as authoritative (the importer re-mints it; see
    // MapImporter_ArmyIdentityNormalize_IO). To rename what a human sees, edit `displayName` — not
    // this.
    std::string name;

    // DISPLAY ONLY — the human-authored organization label. SanGen-internal; it reaches the
    // `.sanmap` as the lowerCamelCase `displayName` sibling (Correction 18) and is NEVER the
    // `armies` dictionary key. Free-form, may be empty, NOT required to be unique (two armies may
    // both be called "North" — they are still ARMY_01 and ARMY_02 to the engine).
    std::string displayName;

    Faction     faction = Faction::Chosen;
    float       alloys  = 500.0f;      // SANMAP_FORMAT_SPEC's confirmed export default
    float       energy  = 500.0f;      // same
    std::vector<UnitGroup> groups;     // recursive pre-placed unit tree

    float       armyColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };  // SanGen-added, Correction 11
    std::string alias;                                       // SanGen-added, Correction 11
};

// v1's rotating default palette, ported (ARCH_14_16_PerArmyUnitsOverlayRows.md §14.16-D). One
// definition shared by Add Army (ArmiesTab_UI.cpp) and the import backfill
// (MapImporter_Armies_IO.cpp) — rotation-by-index consumption, roster position mod 8. Verbatim
// from the ARCH ruling's own code block; do not retype by hand.
inline constexpr float kDefaultArmyColors[8][4] = {
    { 1.0f, 0.0f, 0.0f, 1.0f },   // 0 Red
    { 1.0f, 0.4f, 0.7f, 1.0f },   // 1 Pink
    { 1.0f, 0.5f, 0.0f, 1.0f },   // 2 Orange
    { 0.5f, 0.0f, 0.5f, 1.0f },   // 3 Purple
    { 0.0f, 0.0f, 1.0f, 1.0f },   // 4 Blue
    { 0.0f, 0.5f, 0.5f, 1.0f },   // 5 Teal
    { 0.0f, 0.5f, 0.0f, 1.0f },   // 6 Green
    { 0.2f, 0.8f, 0.2f, 1.0f },   // 7 LimeGreen
};

// The one shared mint: `rosterPosition % 8` into kDefaultArmyColors above. Both call sites §14.16-D
// names — Add Army (ArmiesTab_UI.cpp, UI) and the import backfill (MapImporter_Armies_IO.cpp, IO) —
// already legally depend on PARAMS, so this lives here rather than being copy-pasted twice.
inline void SeedDefaultArmyColor(float armyColor[4], int rosterPosition) {
    const std::size_t paletteIndex = static_cast<std::size_t>(rosterPosition) % 8u;
    for (int channel = 0; channel < 4; ++channel)
        armyColor[channel] = kDefaultArmyColors[paletteIndex][channel];
}

} // namespace Params
} // namespace SanmapGen
