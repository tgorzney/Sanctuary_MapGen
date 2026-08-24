// MapImporter_ArmyIdentityNormalize_IO.cpp — see the header for the policy and its ⚠️ ASSUMPTION
// flag. Layer: IO.
#include "MapImporter_ArmyIdentityNormalize_IO.h"
#include "MapImporter_IO.h"
#include "Sanmap_ArmyIdentity_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../params/MarkerInstance_PARAMS.h"
#include <cstddef>
#include <string>
#include <vector>

namespace SanmapGen {
namespace Io {
namespace {

// One army's old->new identity move, plus enough of its own displayName decision to word the
// report correctly (STEP76 §4a step 5's two message shapes).
struct ArmyIdentityRename {
    std::string oldName;
    std::string newName;
    bool        bDisplayNamePreserved = false;   // true: oldName was stashed into displayName
    std::string existingDisplayName;              // meaningful only when NOT preserved
};

// Positional and total, exactly mirroring Sanmap_ArmyIdentity_IO.h's AssignArmyIdentities — the
// SAME expected-identity formula, so import and UI can never disagree. Mutates `armies` in place and
// returns the subset that actually moved, in roster order, for the marker rewrite and report below.
std::vector<ArmyIdentityRename> RenameArmiesToCanonicalIdentities(std::vector<Params::Army>& armies) {
    std::vector<ArmyIdentityRename> renames;
    for (std::size_t armyIndex = 0u; armyIndex < armies.size(); ++armyIndex) {
        Params::Army& army = armies[armyIndex];
        const std::string expectedIdentity =
            ArmyIdentityForRosterPosition(static_cast<int>(armyIndex) + 1);
        if (army.name == expectedIdentity) continue;

        ArmyIdentityRename rename;
        rename.oldName = army.name;
        rename.newName = expectedIdentity;
        if (army.displayName.empty()) {
            army.displayName = army.name;
            rename.bDisplayNamePreserved = true;
        } else {
            rename.existingDisplayName = army.displayName;
        }
        army.name = expectedIdentity;
        renames.push_back(rename);
    }
    return renames;
}

// Rewrites `markers["Spawn"].transforms[*].name` using the WHOLE mapping in one pass over the
// (already-frozen) `renames` list — never army-by-army, which could chain a rename through two
// mappings (e.g. on-disk `ARMY_02`/`ARMY_03` normalizing to `ARMY_01`/`ARMY_02`: a naive per-army
// loop could re-match an already-rewritten transform against the SECOND rename). A missing/empty
// `"Spawn"` group, or a transform matching no rename, is not this function's concern — STEP82's.
int RewriteSpawnMarkerKeys(std::vector<Params::MarkerInstanceGroup>& markers,
                           const std::vector<ArmyIdentityRename>& renames) {
    int rewrittenCount = 0;
    for (Params::MarkerInstanceGroup& group : markers) {
        if (group.name != Params::kSpawnMarkerGroupName) continue;
        for (Params::MarkerTransform& transform : group.transforms) {
            for (const ArmyIdentityRename& rename : renames) {
                if (transform.name != rename.oldName) continue;
                transform.name = rename.newName;
                ++rewrittenCount;
                break;
            }
        }
    }
    return rewrittenCount;
}

// One WARNING line per renamed army, naming both strings — Constitution §6: the human must be able
// to read the import log and see exactly what moved, even in the "displayName already occupied"
// branch where nothing about the label itself changed.
void ReportArmyIdentityRenames(const std::vector<ArmyIdentityRename>& renames, MapImportResult& result) {
    for (const ArmyIdentityRename& rename : renames) {
        std::string message = "SANGEN: army \"" + rename.oldName + "\" renamed to engine identity \""
                             + rename.newName + "\" (SanGen owns the armies key; the engine assigns "
                             "lobby slots by alphabetical name order).";
        if (rename.bDisplayNamePreserved)
            message += " The original name was preserved as this army's display name.";
        else
            message += " The original name was NOT preserved as a display name because this army "
                       "already had one (\"" + rename.existingDisplayName + "\").";
        result.Warn(message);
    }
}

} // namespace

void NormalizeArmyIdentities(Params::MapRecipe& outRecipe, MapImportResult& result) {
    const std::vector<ArmyIdentityRename> renames =
        RenameArmiesToCanonicalIdentities(outRecipe.armies);
    if (renames.empty()) return;   // the healthy path (§4a step 2): silent, nothing to report

    ReportArmyIdentityRenames(renames, result);
    const int rewrittenCount = RewriteSpawnMarkerKeys(outRecipe.markers, renames);
    result.Log("SANGEN: rewrote " + std::to_string(rewrittenCount)
              + " Spawn marker key reference(s) to match the new army identities.");
}

} // namespace Io
} // namespace SanmapGen
