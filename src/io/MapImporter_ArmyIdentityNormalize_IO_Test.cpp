// MapImporter_ArmyIdentityNormalize_IO_Test.cpp — STEP76_ArmyIdentityNaming_IO acceptance, part 2:
// the import-side normalizer, end to end through the real front door (ParseSanmapJsonText).
// Registered as ArmyIdentityNormalize_IO_Test. Headless: no imgui, no disk, no GL context.
#include "MapImporter_ArmyIdentityNormalize_IO.h"
#include "MapExporter_IO.h"
#include "MapImporter_IO.h"
#include "Sanmap_ArmyIdentity_IO.h"
#include "Sanmap_MigrationManifest_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include <nlohmann/json.hpp>
#include <cstdio>
#include <string>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

// The `"Spawn"` group, or nullptr when the fixture has none.
const Params::MarkerInstanceGroup* FindSpawnGroup(const Params::MapRecipe& recipe) {
    for (const Params::MarkerInstanceGroup& group : recipe.markers)
        if (group.name == "Spawn") return &group;
    return nullptr;
}

// (8) Legacy round-trip, end to end: a hand-built document whose `armies` keys AND whose
// `markers.Spawn.transforms` keys are the same three legacy strings.
void RunLegacyRoundTripChecks() {
    nlohmann::json document;
    document["SanGenVersion"] = Io::kCurrentSanGenVersion;
    document["armies"]["Army_0"] = nlohmann::json::object();
    document["armies"]["Army_1"] = nlohmann::json::object();
    document["armies"]["Army_2"] = nlohmann::json::object();
    document["markers"]["Spawn"]["resource"] = false;
    document["markers"]["Spawn"]["transforms"]["Army_0"] = nlohmann::json::object();
    document["markers"]["Spawn"]["transforms"]["Army_1"] = nlohmann::json::object();
    document["markers"]["Spawn"]["transforms"]["Army_2"] = nlohmann::json::object();

    Params::MapRecipe recipe;
    Io::MapImportResult result;
    Check(Io::MapImporter::ParseSanmapJsonText(document.dump(), recipe, Io::MapImportOptions(), result),
          "the legacy document imports without refusal");

    Check(recipe.armies.size() == 3u, "all three armies survive");
    Check(recipe.armies[0].name == "ARMY_01" && recipe.armies[1].name == "ARMY_02"
          && recipe.armies[2].name == "ARMY_03",
          "every army normalizes to its roster-position identity");
    Check(recipe.armies[0].displayName == "Army_0" && recipe.armies[1].displayName == "Army_1"
          && recipe.armies[2].displayName == "Army_2",
          "the legacy names survive as display labels");

    const Params::MarkerInstanceGroup* spawnGroup = FindSpawnGroup(recipe);
    bool bEverySpawnTransformMatchesAnArmy = spawnGroup != nullptr && spawnGroup->transforms.size() == 3u;
    if (spawnGroup != nullptr)
        for (const Params::MarkerTransform& transform : spawnGroup->transforms) {
            bool bMatchesAnArmy = false;
            for (const Params::Army& army : recipe.armies)
                if (army.name == transform.name) bMatchesAnArmy = true;
            if (!bMatchesAnArmy) bEverySpawnTransformMatchesAnArmy = false;
        }
    Check(bEverySpawnTransformMatchesAnArmy, "every Spawn transform name now matches an army name");

    Check(result.warningCount == 3, "exactly one warning per renamed army");
    Check(result.debugLog.find("Army_0") != std::string::npos
          && result.debugLog.find("ARMY_01") != std::string::npos,
          "the log names both the old and new identity strings, not just one");
}

// (9) Never-discard, occupied branch: one army already carries a human-authored displayName.
void RunNeverDiscardOccupiedBranchChecks() {
    nlohmann::json document;
    document["SanGenVersion"] = Io::kCurrentSanGenVersion;
    document["armies"]["Army_0"]["displayName"] = "North Ridge";
    document["armies"]["Army_1"] = nlohmann::json::object();

    Params::MapRecipe recipe;
    Io::MapImportResult result;
    Check(Io::MapImporter::ParseSanmapJsonText(document.dump(), recipe, Io::MapImportOptions(), result),
          "the document imports without refusal");
    Check(recipe.armies[0].name == "ARMY_01", "the name still normalizes");
    Check(recipe.armies[0].displayName == "North Ridge",
          "the already-occupied displayName is NOT overwritten with the machine-minted old name");
    Check(result.debugLog.find("Army_0") != std::string::npos,
          "the log still names the old identity, even though displayName was left alone");
}

// (10) An already-clean document is a silent no-op: zero renames, zero warnings, displayName and
// Spawn keys untouched.
void RunAlreadyCleanNoOpChecks() {
    nlohmann::json document;
    document["SanGenVersion"] = Io::kCurrentSanGenVersion;
    document["armies"]["ARMY_01"] = nlohmann::json::object();
    document["armies"]["ARMY_02"] = nlohmann::json::object();
    document["markers"]["Spawn"]["transforms"]["ARMY_01"] = nlohmann::json::object();
    document["markers"]["Spawn"]["transforms"]["ARMY_02"] = nlohmann::json::object();

    Params::MapRecipe recipe;
    Io::MapImportResult result;
    Check(Io::MapImporter::ParseSanmapJsonText(document.dump(), recipe, Io::MapImportOptions(), result),
          "an already-clean document imports without refusal");
    Check(result.warningCount == 0, "zero warnings on an already-canonical roster");
    Check(recipe.armies[0].displayName.empty() && recipe.armies[1].displayName.empty(),
          "displayName stays untouched (empty)");

    const Params::MarkerInstanceGroup* spawnGroup = FindSpawnGroup(recipe);
    Check(spawnGroup != nullptr && spawnGroup->transforms.size() == 2u
          && spawnGroup->transforms[0].name == "ARMY_01" && spawnGroup->transforms[1].name == "ARMY_02",
          "Spawn keys are untouched");
}

// (11) Spawn group absent entirely: armies still normalize, zero crashes, and — the explicit
// negative check — NO warning about the missing group (STEP82's subject, not this ticket's).
void RunSpawnGroupAbsentChecks() {
    nlohmann::json document;
    document["SanGenVersion"] = Io::kCurrentSanGenVersion;
    document["armies"]["Army_0"] = nlohmann::json::object();

    Params::MapRecipe recipe;
    Io::MapImportResult result;
    Check(Io::MapImporter::ParseSanmapJsonText(document.dump(), recipe, Io::MapImportOptions(), result),
          "a document with no Spawn group at all still imports");
    Check(recipe.armies.size() == 1u && recipe.armies[0].name == "ARMY_01",
          "the army still normalizes with no Spawn group present");
    Check(result.warningCount == 1,
          "exactly one warning (the army rename) — nothing about the missing group");
}

// (12) Chained-rename hazard: on-disk keys ARMY_02/ARMY_03 — position 1 becomes ARMY_01, so an OLD
// name and a NEW name collide across rows. Proves the build-mapping-then-apply-once discipline.
void RunChainedRenameHazardChecks() {
    nlohmann::json document;
    document["SanGenVersion"] = Io::kCurrentSanGenVersion;
    document["armies"]["ARMY_02"] = nlohmann::json::object();
    document["armies"]["ARMY_03"] = nlohmann::json::object();
    document["markers"]["Spawn"]["transforms"]["ARMY_02"] = nlohmann::json::object();
    document["markers"]["Spawn"]["transforms"]["ARMY_03"] = nlohmann::json::object();

    Params::MapRecipe recipe;
    Io::MapImportResult result;
    Check(Io::MapImporter::ParseSanmapJsonText(document.dump(), recipe, Io::MapImportOptions(), result),
          "the chained-rename fixture imports without refusal");
    Check(recipe.armies.size() == 2u && recipe.armies[0].name == "ARMY_01"
          && recipe.armies[1].name == "ARMY_02",
          "position 1 becomes ARMY_01 — an old name (ARMY_02) and a new name collide across rows");

    const Params::MarkerInstanceGroup* spawnGroup = FindSpawnGroup(recipe);
    Check(spawnGroup != nullptr && spawnGroup->transforms.size() == 2u
          && spawnGroup->transforms[0].name == "ARMY_01" && spawnGroup->transforms[1].name == "ARMY_02",
          "the transform originally keyed ARMY_02 resolves to ARMY_01 and the one originally keyed "
          "ARMY_03 resolves to ARMY_02 — neither is re-matched by the OTHER rename in the same pass, "
          "which a naive per-army loop would get wrong");
}

// (13) Full `.sanmap` round-trip parity: a canonical (already re-minted, as ArmiesTab_UI would leave
// it) roster with non-empty displayName values survives export -> import exactly, and the raw JSON
// confirms the wire shape from §2.
void RunFullRoundTripParityChecks() {
    Params::MapRecipe recipe;
    for (int index = 0; index < 3; ++index) {
        Params::Army army;
        army.displayName = "Army Label " + std::to_string(index);
        recipe.armies.push_back(army);
    }
    Io::AssignArmyIdentities(recipe.armies);   // ArmiesTab_UI's own re-mint, run once before export

    const std::string documentText = Io::MapExporter::BuildSanmapJsonText(recipe);
    const nlohmann::json rawDocument = nlohmann::json::parse(documentText);

    Params::MapRecipe loaded;
    Io::MapImportResult result;
    Check(Io::MapImporter::ParseSanmapJsonText(documentText, loaded, Io::MapImportOptions(), result),
          "the exported document parses");
    Check(result.warningCount == 0, "a canonical roster round-trips with no warning at all");

    Check(loaded.armies.size() == 3u, "all three armies survive");
    for (std::size_t index = 0u; index < 3u; ++index) {
        Check(loaded.armies[index].name == recipe.armies[index].name, "name survives exactly");
        Check(loaded.armies[index].displayName == recipe.armies[index].displayName,
              "displayName survives exactly");
    }

    Check(rawDocument["armies"].contains("ARMY_01") && rawDocument["armies"].contains("ARMY_02")
          && rawDocument["armies"].contains("ARMY_03"), "the raw JSON keys are ARMY_01...");
    Check(rawDocument["armies"]["ARMY_01"].contains("displayName")
          && rawDocument["armies"]["ARMY_01"].contains("armyColor")
          && rawDocument["armies"]["ARMY_01"].contains("alias"),
          "displayName sits INSIDE each army object, a sibling of armyColor/alias");
}

} // namespace

int main() {
    RunLegacyRoundTripChecks();
    RunNeverDiscardOccupiedBranchChecks();
    RunAlreadyCleanNoOpChecks();
    RunSpawnGroupAbsentChecks();
    RunChainedRenameHazardChecks();
    RunFullRoundTripParityChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
