// GameInstallLocation_IO.h — validates a candidate game install root for the Map Scenario Lua
// export leg (`ARCH_15_04_ThreeFileOnDiskShape.md` §15.4, DESIGN_MapScenarioIO_R1.md §1). Pure filesystem check, no platform
// API — unlike AppSettingsLocation_IO.h it needs no `_Shell_IO.cpp` split (Constitution §5): this
// is judgment on a path already chosen, not resolution of a platform-specific bootstrap location.
// UI owns the picker (FileDialog_IO.h's own rule, applied symmetrically); this file only validates.
#pragma once
#include <string>

namespace SanmapGen {
namespace Io {

struct GameInstallRootValidation {
    bool        bValid = false;
    std::string reason;   // populated only when bValid == false; empty on success
};

// A candidate root is valid iff BOTH <candidateRoot>/engine/LJ/lua AND
// <candidateRoot>/engine/Sanctuary_Data/Maps exist as directories (`ARCH_15_04_ThreeFileOnDiskShape.md` §15.4: the engine's script
// tree and the map asset package folder respectively). `reason` names whichever subpath(s) are
// missing -- never a generic "invalid" with no actionable detail (Constitution §6).
GameInstallRootValidation ValidateGameInstallRoot(const std::string& candidateRoot);

} // namespace Io
} // namespace SanmapGen
