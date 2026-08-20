// EntityCollections_Migrate_V2_IO_Test.cpp — acceptance test for EntityCollections_Migrate_V2
// (STEP40E). One migration, one hand-built OLD-shape fixture per case, exact NEW-shape assertions
// (IO_MIGRATION_SPEC.md §1). Legacy `Armies`/`Aliases` fixtures are nested at
// `mapGeneratorData.Armies`/`mapGeneratorData.Aliases` — see the header's ground-truth note on why
// this diverges from the work-order's "top-level Aliases" description.
#include "EntityCollections_Migrate_V2_IO.h"
#include <cstdio>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++failureCount; }
}

// Acceptance test 1: 2+ armies, each with a legacy Color array, produce the correct armyColor
// object on each corresponding top-level armies[key] entry.
void CheckArmyColorsMigrateToTopLevelArmies() {
    nlohmann::json document = {
        {"mapGeneratorData", {
            {"Armies", {
                {"ARMY_1", {{"Color", {1.0f, 0.0f, 0.0f, 1.0f}}}},
                {"ARMY_2", {{"Color", {0.0f, 1.0f, 0.0f, 0.5f}}}},
            }}
        }},
        {"armies", {
            {"ARMY_1", {{"faction", 0}}},
            {"ARMY_2", {{"faction", 1}}},
        }}
    };
    Io::EntityCollections_Migrate_V2(document);

    Check(document["armies"]["ARMY_1"]["armyColor"].is_object() &&
          document["armies"]["ARMY_1"]["armyColor"]["r"] == 1.0f &&
          document["armies"]["ARMY_1"]["armyColor"]["g"] == 0.0f &&
          document["armies"]["ARMY_1"]["armyColor"]["b"] == 0.0f &&
          document["armies"]["ARMY_1"]["armyColor"]["a"] == 1.0f,
          "EntityCollections_Migrate_V2 sets ARMY_1's armyColor correctly");
    Check(document["armies"]["ARMY_2"]["armyColor"].is_object() &&
          document["armies"]["ARMY_2"]["armyColor"]["r"] == 0.0f &&
          document["armies"]["ARMY_2"]["armyColor"]["g"] == 1.0f &&
          document["armies"]["ARMY_2"]["armyColor"]["b"] == 0.0f &&
          document["armies"]["ARMY_2"]["armyColor"]["a"] == 0.5f,
          "EntityCollections_Migrate_V2 sets ARMY_2's armyColor correctly");
    Check(document["armies"]["ARMY_1"]["faction"] == 0 && document["armies"]["ARMY_2"]["faction"] == 1,
          "EntityCollections_Migrate_V2 leaves the rest of each top-level army entry untouched");
    Check(!document["mapGeneratorData"]["Armies"]["ARMY_1"].contains("Color") &&
          !document["mapGeneratorData"]["Armies"]["ARMY_2"].contains("Color"),
          "EntityCollections_Migrate_V2 removes the legacy Color array once moved");
}

// Acceptance test 2: aliases resolve correctly across at least 2 different marker-type collections
// (groups), proving the search is not scoped too narrowly to just one.
void CheckAliasesResolveAcrossMultipleMarkerGroups() {
    nlohmann::json document = {
        {"mapGeneratorData", {
            {"Aliases", {
                {"MyMex",   "Mex 0"},
                {"MySpawn", "Spawn 0"},
            }}
        }},
        {"markers", {
            {"Alloys", {
                {"resource", true},
                {"transforms", {
                    {"Mex 0", {{"position", {{"x", 1.0f}, {"y", 0.0f}, {"z", 1.0f}}}}},
                }},
            }},
            {"Spawn", {
                {"resource", false},
                {"transforms", {
                    {"Spawn 0", {{"position", {{"x", 2.0f}, {"y", 0.0f}, {"z", 2.0f}}}}},
                }},
            }},
        }}
    };
    Io::EntityCollections_Migrate_V2(document);

    Check(document["markers"]["Alloys"]["transforms"]["Mex 0"]["alias"] == "MyMex",
          "EntityCollections_Migrate_V2 sets alias on a transform in the Alloys marker group");
    Check(document["markers"]["Spawn"]["transforms"]["Spawn 0"]["alias"] == "MySpawn",
          "EntityCollections_Migrate_V2 sets alias on a transform in the (different) Spawn marker group");
}

// Acceptance test 3: an alias pointing at a transform name that doesn't exist anywhere is a safe
// no-op for that one entry — no crash, no phantom transform created anywhere.
void CheckUnmatchedAliasIsSafeNoOp() {
    nlohmann::json document = {
        {"mapGeneratorData", {
            {"Aliases", { {"Ghost", "Nonexistent Transform"} }}
        }},
        {"markers", {
            {"Alloys", {
                {"resource", true},
                {"transforms", {
                    {"Mex 0", {{"position", {{"x", 1.0f}, {"y", 0.0f}, {"z", 1.0f}}}}},
                }},
            }},
        }}
    };
    Io::EntityCollections_Migrate_V2(document);

    Check(!document["markers"]["Alloys"]["transforms"].contains("Nonexistent Transform"),
          "EntityCollections_Migrate_V2 creates no phantom transform for an unmatched alias");
    Check(!document["markers"]["Alloys"]["transforms"]["Mex 0"].contains("alias"),
          "EntityCollections_Migrate_V2 leaves an unrelated transform's alias untouched");
    Check(document["markers"]["Alloys"]["transforms"].size() == 1,
          "EntityCollections_Migrate_V2 does not grow the transforms collection for an unmatched alias");
}

// Acceptance test 4: a document with no legacy Armies/Aliases data at all is a safe no-op.
void CheckNoOpWhenNoLegacyData() {
    nlohmann::json emptyDocument = nlohmann::json::object();
    nlohmann::json beforeEmpty   = emptyDocument;
    Io::EntityCollections_Migrate_V2(emptyDocument);
    Check(emptyDocument == beforeEmpty,
          "EntityCollections_Migrate_V2 is a no-op on a document with no mapGeneratorData at all");

    nlohmann::json documentWithoutEntities = {
        {"mapGeneratorData", { {"SomeOtherField", 1} }},
        {"armies", { {"ARMY_1", {{"faction", 0}}} }},
        {"markers", { {"Alloys", {{"resource", true}, {"transforms", { {"Mex 0", nlohmann::json::object()} }}}} }},
    };
    nlohmann::json beforeWithoutEntities = documentWithoutEntities;
    Io::EntityCollections_Migrate_V2(documentWithoutEntities);
    Check(documentWithoutEntities == beforeWithoutEntities,
          "EntityCollections_Migrate_V2 is a no-op when mapGeneratorData has neither Armies nor Aliases");
}

} // namespace

int main() {
    CheckArmyColorsMigrateToTopLevelArmies();
    CheckAliasesResolveAcrossMultipleMarkerGroups();
    CheckUnmatchedAliasIsSafeNoOp();
    CheckNoOpWhenNoLegacyData();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
