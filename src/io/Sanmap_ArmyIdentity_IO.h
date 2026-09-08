// Sanmap_ArmyIdentity_IO.h — the ONE canonical `ARMY_XX` engine identity helper. Layer: IO.
// STEP76_ArmyIdentityNaming_IO ruling 1: the `.sanmap` `armies` dictionary key is machine-owned by
// SanGen, never human-settable. This header is the single source of truth for minting it, so the
// UI re-mint (ArmiesTab_UI.cpp), the export-time guard (MapExporter_Armies_IO.cpp) and the
// import-time normalizer (MapImporter_ArmyIdentityNormalize_IO.cpp) can never disagree.
//
// Pure, headless, testable: zero includes beyond the standard library and Army_PARAMS.h — no
// imgui, no nlohmann (ARCH §1.5 — the identity itself is not a JSON concern, only its callers are).
#pragma once
#include <cctype>
#include <cstddef>
#include <string>
#include <vector>
#include "../params/Army_PARAMS.h"

namespace SanmapGen {
namespace Io {

// The `.sanmap` `armies` dictionary key for a 1-based roster position. `ARMY_` + at least two
// digits, zero-padded, so an ALPHABETICAL sort of a roster's keys equals roster order — the
// property common/gameUtils.lua's CreateArmies() relies on to assign lobby slots (STEP73 §0).
// The padding is FUNCTIONAL, not cosmetic: ARMY_1/ARMY_2/ARMY_10 sorts 1, 10, 2.
// Positions past 99 widen to three digits rather than truncating; the sort stays correct within
// each width band and no supported map reaches it (16 slots). A non-positive position is treated
// as 0 rather than crashing (Constitution §6) — never expected in real use.
inline std::string ArmyIdentityForRosterPosition(int oneBasedPosition) {
    const int clampedPosition = oneBasedPosition < 0 ? 0 : oneBasedPosition;
    std::string digits = std::to_string(clampedPosition);
    while (digits.size() < 2u) digits = "0" + digits;
    return "ARMY_" + digits;
}

// True when `name` is exactly `ARMY_` followed by two-or-more digits, all digits. Used by the
// export guard and by tests. Does NOT check that the number matches a position.
inline bool IsArmyIdentityWellFormed(const std::string& name) {
    static const std::string prefix = "ARMY_";
    if (name.size() <= prefix.size()) return false;
    if (name.compare(0, prefix.size(), prefix) != 0) return false;
    const std::string digits = name.substr(prefix.size());
    if (digits.size() < 2u) return false;
    for (char digitCharacter : digits)
        if (digitCharacter < '0' || digitCharacter > '9') return false;
    return true;
}

// True when `name` merely RESEMBLES an ARMY_XX identity, case-insensitively — a narrower question
// than IsArmyIdentityWellFormed's "is this a real, usable army identity" (which is case-sensitive,
// correct for STEP76 minting: the engine's Spawn.transforms lookup is exact-case, so a genuinely
// malformed lowercase name must still be flagged there). Used ONLY to forbid a
// Scenarios::spawnPoints row's spawnId from shadowing ARMY_XX (ARCH_15_12_ScenarioSpawnIdentity.md
// §15.12's Validator, corrected 2026-09-05) — never substituted for IsArmyIdentityWellFormed's own
// call sites (STEP76 army minting/well-formedness).
//
// CORRECTNESS NOTE (STEP252 coder, flagged per this ticket's own "flag the discrepancy" rule): the
// ratified §15.12 text and STEP252's own work-order both describe this predicate as folding `name`
// to lowercase and then "applying the identical prefix-plus-two-or-more-digits shape
// IsArmyIdentityWellFormed checks, on the folded string" — but IsArmyIdentityWellFormed itself
// hardcodes an UPPERCASE "ARMY_" prefix, so literally calling
// `IsArmyIdentityWellFormed(loweredName)` (the work-order's own literal code sample) can never
// return true for ANY input, lowered or not — it would silently defeat the entire reserved-namespace
// rule the ARCH ratifies (and fail this ticket's own acceptance test 1, which requires "ARMY_01"/
// "army_01"/"Army_01" all flagged). This reimplements the identical shape check directly against the
// folded (lowercase) string with a lowercase "army_" prefix instead of delegating to
// IsArmyIdentityWellFormed, which is the only way to deliver the case-insensitive behavior both the
// ARCH text and the acceptance tests actually require.
inline bool ResemblesArmyIdentityCaseInsensitive(const std::string& name) {
    std::string folded = name;
    for (char& c : folded) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    static const std::string prefix = "army_";
    if (folded.size() <= prefix.size()) return false;
    if (folded.compare(0, prefix.size(), prefix) != 0) return false;
    const std::string digits = folded.substr(prefix.size());
    if (digits.size() < 2u) return false;
    for (char digitCharacter : digits)
        if (digitCharacter < '0' || digitCharacter > '9') return false;
    return true;
}

// THE enforcement point. Rewrites every `armies[i].name` to ArmyIdentityForRosterPosition(i + 1),
// unconditionally and idempotently. Reports whether anything moved, so a UI caller can skip
// downstream work on a no-op frame. Deliberately positional and total — it does not inspect the
// current name, does not pattern-match, and does not try to preserve anything, so it stays correct
// across add / delete / reorder with one call and no per-operation logic. An empty roster is a
// no-op, never a crash.
inline bool AssignArmyIdentities(std::vector<Params::Army>& armies) {
    bool bAnyIdentityChanged = false;
    for (std::size_t armyIndex = 0u; armyIndex < armies.size(); ++armyIndex) {
        const std::string expectedIdentity =
            ArmyIdentityForRosterPosition(static_cast<int>(armyIndex) + 1);
        if (armies[armyIndex].name != expectedIdentity) {
            armies[armyIndex].name = expectedIdentity;
            bAnyIdentityChanged = true;
        }
    }
    return bAnyIdentityChanged;
}

} // namespace Io
} // namespace SanmapGen
