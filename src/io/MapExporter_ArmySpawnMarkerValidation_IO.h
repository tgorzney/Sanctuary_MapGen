// MapExporter_ArmySpawnMarkerValidation_IO.h — `ArmySpawnMarkerValidationReport` + the export-time
// army->Spawn-marker membership scan (STEP82). Layer: IO. Modelled directly on the sibling
// MapExporter_BlueprintValidation_IO.h: a report struct with a one-wording `SummaryText()`, plus a
// pure `Validate*` free function, same tier as `recipe.IsValid()`, never called from inside
// BuildSanmapJsonText.
#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Io {

// The format-fixed marker TYPE name whose inner dictionary is keyed by army name
// (SANMAP_FORMAT_SPEC; STEP49_ManualMarkersUI.md:34-35). A named setting, never a literal at a
// check site (Constitution §8). IO-side and deliberately NOT shared with the UI layer's own
// "Spawn" label: ARCH_16_09_NonArchItems.md §16.9 rules the UI-side constant is UI-internal
// naming hygiene owned by the UI Expert. This is format truth, and it lives in IO.
inline const char* const spawnMarkerGroupName = "Spawn";

// One export-time army->spawn-marker membership pass. WARN-ONLY: this REPORTS and nothing else.
// An army with no matching Spawn marker is a LEGAL, TOLERATED state (ARCH_16_08_SpawnArmyShrink.md
// §16.8) -- never auto-created, never auto-deleted, never blocking. UX polish, not correctness.
struct ArmySpawnMarkerValidationReport {
    std::vector<std::string> armyNamesWithoutSpawnMarker;   // distinct, in recipe.armies order
    bool bSpawnMarkerGroupPresent = false;                  // false = no "Spawn" group authored at all
    bool AllArmiesHaveSpawnMarkers() const { return armyNamesWithoutSpawnMarker.empty(); }
    std::string SummaryText() const;   // ONE wording -- shared by every call site
};

// Pure/read-only, touches no disk, never called from inside BuildSanmapJsonText -- same tier as
// recipe.IsValid() (the MapExporter_BlueprintValidation_IO sibling's own posture).
ArmySpawnMarkerValidationReport ValidateArmiesHaveSpawnMarkers(const Params::MapRecipe& recipe);

} // namespace Io
} // namespace SanmapGen
