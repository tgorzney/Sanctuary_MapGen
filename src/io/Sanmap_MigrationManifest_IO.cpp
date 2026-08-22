// Sanmap_MigrationManifest_IO.cpp — the manifest table itself. See the header for the full
// contract. STEP40F: one real step, sourceVersion 2 -> 3, wiring in all 9 `<Domain>_Migrate_V2_IO`
// migration functions STEP40B-E already shipped and individually tested. Ordering inside
// `migrations` is load-bearing law (header, §2 rule 2): `SlopeDefaults_Migrate_V2` MUST run before
// `StratumGenerationSettings_Migrate_V2` — both share their read source
// (`mapGeneratorData.Stratums[]`) and write destination (`StratumGenerationSettings[i]`); see
// either migration's own header for the additive-write discipline that ordering protects.
//
// STEP26A: `bLosslessIfSkipped` audited for all 9 shipped migrations against the real reader
// architecture (`MapImporter_IO.h`, `MapImporter_ParseDocument_IO.cpp`) — block readers are
// current-shape-only by law; only Geometry/Water/StrataSettings have a legacy-`mapGeneratorData`-
// gated fallback reader, and none of these 9 migrations target those three domains, so only a
// migration that relocates NOTHING (pure key-reservation) can honestly claim `true`. Only
// `Accumulation_Migrate_V2` qualifies. Each `false` declaration below carries its own one-line
// reason, per this same audit.
#include "Sanmap_MigrationManifest_IO.h"
#include "GeneralMapSettings_Migrate_V2_IO.h"
#include "Symmetry_Migrate_V2_IO.h"
#include "Accumulation_Migrate_V2_IO.h"
#include "DetailNormal_Migrate_V2_IO.h"
#include "Flow_Migrate_V2_IO.h"
#include "GlobalMarkerSettings_Migrate_V2_IO.h"
#include "SlopeDefaults_Migrate_V2_IO.h"
#include "StratumGenerationSettings_Migrate_V2_IO.h"
#include "EntityCollections_Migrate_V2_IO.h"
#include "MarkersStack_Migrate_V3_IO.h"

namespace SanmapGen {
namespace Io {

const std::vector<MigrationStep>& SanmapMigrationManifest() {
    static const std::vector<MigrationStep> manifest = {
        MigrationStep{
            /*sourceVersion=*/ 2,
            /*migrations=*/ {
                MigrationEntry{ GeneralMapSettings_Migrate_V2, "GeneralMapSettings_Migrate_V2",
                    "Relocates Seed/ScaleFeaturesToMapSize/TerrainMinHeight/WorldUnitsPerCell out of "
                    "mapGeneratorData into GeneralMapSettings.", /*bIndependentlySelectable=*/ true,
                    // bLosslessIfSkipped = false: relocates real fields, no fallback reader.
                    /*bLosslessIfSkipped=*/ false },
                MigrationEntry{ Symmetry_Migrate_V2, "Symmetry_Migrate_V2",
                    "Relocates the 9 legacy global symmetry scalars out of mapGeneratorData into "
                    "Symmetry.", /*bIndependentlySelectable=*/ true,
                    // bLosslessIfSkipped = false: relocates real fields, no fallback reader for Symmetry.
                    /*bLosslessIfSkipped=*/ false },
                MigrationEntry{ Accumulation_Migrate_V2, "Accumulation_Migrate_V2",
                    "Reserves the empty Accumulation top-level key.", /*bIndependentlySelectable=*/ true,
                    // bLosslessIfSkipped = true: pure key reservation — nothing to relocate.
                    /*bLosslessIfSkipped=*/ true },
                MigrationEntry{ DetailNormal_Migrate_V2, "DetailNormal_Migrate_V2",
                    "Relocates DetailNormalMapSize out of mapGeneratorData into DetailNormal.",
                    /*bIndependentlySelectable=*/ true,
                    // bLosslessIfSkipped = false: relocates real fields, no fallback reader for
                    // DetailNormal.
                    /*bLosslessIfSkipped=*/ false },
                MigrationEntry{ Flow_Migrate_V2, "Flow_Migrate_V2",
                    "Converts the legacy FlowMapColor array into the {r,g,b,a} object shape under "
                    "Flow.", /*bIndependentlySelectable=*/ false,
                    // bLosslessIfSkipped = false: relocates real fields, no fallback reader for Flow.
                    /*bLosslessIfSkipped=*/ false },
                MigrationEntry{ GlobalMarkerSettings_Migrate_V2, "GlobalMarkerSettings_Migrate_V2",
                    "Relocates the 9 legacy global marker fields (icons/colors/scales) into "
                    "GlobalMarkerSettings.", /*bIndependentlySelectable=*/ false,
                    // bLosslessIfSkipped = false: relocates real fields, no fallback reader.
                    /*bLosslessIfSkipped=*/ false },
                MigrationEntry{ SlopeDefaults_Migrate_V2, "SlopeDefaults_Migrate_V2",
                    "Synthesizes a global SlopeDefaults from each stratum's legacy slope-gate fields "
                    "and sets each stratum's SlopeUseGlobal.", /*bIndependentlySelectable=*/ false,
                    // bLosslessIfSkipped = false: relocates real fields, no fallback reader.
                    /*bLosslessIfSkipped=*/ false },
                MigrationEntry{ StratumGenerationSettings_Migrate_V2, "StratumGenerationSettings_Migrate_V2",
                    "Relocates each stratum's legacy slope-gate fields verbatim into "
                    "StratumGenerationSettings, index-aligned and padded to 9.",
                    /*bIndependentlySelectable=*/ false,
                    // bLosslessIfSkipped = false: relocates real fields, no fallback reader.
                    /*bLosslessIfSkipped=*/ false },
                MigrationEntry{ EntityCollections_Migrate_V2, "EntityCollections_Migrate_V2",
                    "Converts each army's legacy Color array into armyColor, and folds the legacy "
                    "mapGeneratorData.Aliases dictionary into markers[].alias.",
                    /*bIndependentlySelectable=*/ false,
                    // bLosslessIfSkipped = false: relocates real fields, no fallback reader.
                    /*bLosslessIfSkipped=*/ false },
            },
            /*legacyKeysToDelete=*/ { "mapGeneratorData", "MapGeneratorDataVersion", "mapGeneratorDataVersion" }
        },
        // STEP67: the V3->V4 step. `MarkersStack` reshapes in place (flat array -> two-level
        // `MarkerRuleLayer`); nothing moves to a different top-level key, so `legacyKeysToDelete`
        // stays empty.
        MigrationStep{
            /*sourceVersion=*/ 3,
            /*migrations=*/ {
                MigrationEntry{ MarkersStack_Migrate_V3, "MarkersStack_Migrate_V3",
                    "Groups the flat MarkersStack rule array into MarkerRuleLayer objects by "
                    "contiguous shared symmetry triplet, moving each group's SymmetryUseGlobal/"
                    "SymmetryMask/RadialSymmetryRepeatCount up onto the layer.",
                    /*bIndependentlySelectable=*/ true,
                    // bLosslessIfSkipped = false: reshapes real, load-bearing per-rule fields (the
                    // symmetry triplet) with no fallback reader for the old flat shape once STEP66's
                    // importer expects the layer-wrapped one.
                    /*bLosslessIfSkipped=*/ false },
            },
            /*legacyKeysToDelete=*/ {}
        }
    };
    return manifest;
}

} // namespace Io
} // namespace SanmapGen
