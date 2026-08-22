// Sanmap_ArmyIdentity_IO_Test.cpp — STEP76_ArmyIdentityNaming_IO acceptance, part 1: the pure
// `ARMY_XX` identity helpers (Sanmap_ArmyIdentity_IO.h). Registered as ArmyIdentity_IO_Test.
// Headless: no imgui, no disk, no GL context.
#include "Sanmap_ArmyIdentity_IO.h"
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

using namespace SanmapGen;
using namespace SanmapGen::Io;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

std::vector<Params::Army> MakeRoster(int armyCount) {
    std::vector<Params::Army> armies(static_cast<std::size_t>(armyCount));
    return armies;
}

// (1) THE load-bearing test — ruling 4: an alphabetical sort of the minted names equals roster
// order, at exactly the sizes where the old unpadded `Army_N` scheme broke (9, 10, 11) and at the
// full 16-slot ceiling.
void RunAlphabeticalOrderMatchesRosterOrderChecks() {
    for (int armyCount : { 9, 10, 11, 16 }) {
        std::vector<Params::Army> armies = MakeRoster(armyCount);
        AssignArmyIdentities(armies);

        std::vector<std::string> rosterOrderNames;
        for (const Params::Army& army : armies) rosterOrderNames.push_back(army.name);

        std::vector<std::string> sortedNames = rosterOrderNames;
        std::sort(sortedNames.begin(), sortedNames.end());

        bool bIndexForIndexIdentical = true;
        for (std::size_t index = 0u; index < rosterOrderNames.size(); ++index)
            if (sortedNames[index] != rosterOrderNames[index]) bIndexForIndexIdentical = false;
        Check(bIndexForIndexIdentical,
              "an alphabetical sort of the minted names is index-for-index identical to roster order");

        if (armyCount >= 10)
            Check(rosterOrderNames[9] == "ARMY_10",
                  "the 10th army is ARMY_10 — the exact size where the old Army_N scheme broke");
    }
}

// (2) ArmyIdentityForRosterPosition: exact values, and widening past 99 (never truncating).
void RunIdentityForPositionChecks() {
    Check(ArmyIdentityForRosterPosition(1)  == "ARMY_01", "position 1 mints ARMY_01");
    Check(ArmyIdentityForRosterPosition(9)  == "ARMY_09", "position 9 mints ARMY_09");
    Check(ArmyIdentityForRosterPosition(10) == "ARMY_10", "position 10 mints ARMY_10");
    Check(ArmyIdentityForRosterPosition(16) == "ARMY_16", "position 16 mints ARMY_16");
    Check(ArmyIdentityForRosterPosition(100) == "ARMY_100", "position 100 WIDENS rather than truncating");
}

// (3) Idempotence: calling AssignArmyIdentities twice in a row reports no move and no change the
// second time.
void RunIdempotenceChecks() {
    std::vector<Params::Army> armies = MakeRoster(5);
    Check(AssignArmyIdentities(armies), "the first call mints identities and reports the move");
    const std::vector<Params::Army> firstResult = armies;
    Check(!AssignArmyIdentities(armies), "the second call reports no move");
    for (std::size_t index = 0u; index < armies.size(); ++index)
        Check(armies[index].name == firstResult[index].name, "and changes nothing");
}

// (4) Reorder: identity is POSITIONAL, display is NOT — proven by swapping two rows and re-minting.
void RunReorderChecks() {
    std::vector<Params::Army> armies = MakeRoster(3);
    armies[0].displayName = "North";
    armies[1].displayName = "Center";
    armies[2].displayName = "South";
    AssignArmyIdentities(armies);   // ARMY_01/02/03, matching declaration order

    std::swap(armies[0], armies[2]);   // the ArmiesTab_UI reorder repair's own erase/insert pattern
    Check(AssignArmyIdentities(armies), "re-minting after a reorder reports the move");
    Check(armies[0].name == "ARMY_01" && armies[1].name == "ARMY_02" && armies[2].name == "ARMY_03",
          "identities are re-minted purely from POSITION, with no memory of the old assignment");
    Check(armies[0].displayName == "South" && armies[2].displayName == "North",
          "display labels followed their own row through the reorder — identity moved, display didn't");
}

// (5) Delete: names stay a contiguous ARMY_01..ARMY_03 with no gap, and the surviving display
// labels are the right three in the right order.
void RunDeleteChecks() {
    std::vector<Params::Army> armies = MakeRoster(4);
    armies[0].displayName = "Alpha";
    armies[1].displayName = "Bravo";
    armies[2].displayName = "Charlie";
    armies[3].displayName = "Delta";
    AssignArmyIdentities(armies);

    armies.erase(armies.begin() + 1);   // remove Bravo (ARMY_02)
    Check(AssignArmyIdentities(armies), "re-minting after a delete reports the move");
    Check(armies.size() == 3u, "the roster is 3 armies after the delete");
    Check(armies[0].name == "ARMY_01" && armies[1].name == "ARMY_02" && armies[2].name == "ARMY_03",
          "names are a contiguous ARMY_01..ARMY_03 with no gap");
    Check(armies[0].displayName == "Alpha" && armies[1].displayName == "Charlie"
          && armies[2].displayName == "Delta",
          "the surviving display labels are the right three, in the right order");
}

// (6) IsArmyIdentityWellFormed: exactly `ARMY_` + two-or-more digits, all digits.
void RunWellFormedChecks() {
    Check(IsArmyIdentityWellFormed("ARMY_01"), "ARMY_01 is well formed");
    Check(IsArmyIdentityWellFormed("ARMY_100"), "ARMY_100 (3 digits) is well formed");
    Check(!IsArmyIdentityWellFormed("ARMY_1"), "ARMY_1 (one digit) is NOT well formed");
    Check(!IsArmyIdentityWellFormed("Army_01"), "Army_01 (wrong case) is NOT well formed");
    Check(!IsArmyIdentityWellFormed("ARMY01"), "ARMY01 (no underscore) is NOT well formed");
    Check(!IsArmyIdentityWellFormed("ARMY_0X"), "ARMY_0X (non-digit) is NOT well formed");
    Check(!IsArmyIdentityWellFormed("Bob"), "Bob is NOT well formed");
    Check(!IsArmyIdentityWellFormed(""), "an empty string is NOT well formed");
}

// (7) An empty roster is a no-op, never a crash.
void RunEmptyRosterChecks() {
    std::vector<Params::Army> armies;
    Check(!AssignArmyIdentities(armies), "an empty roster reports no move");
    Check(armies.empty(), "and stays empty — no crash");
}

} // namespace

int main() {
    RunAlphabeticalOrderMatchesRosterOrderChecks();
    RunIdentityForPositionChecks();
    RunIdempotenceChecks();
    RunReorderChecks();
    RunDeleteChecks();
    RunWellFormedChecks();
    RunEmptyRosterChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
