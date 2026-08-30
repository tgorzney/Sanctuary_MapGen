// MapCanvas_SelectionSet_UI_Test.cpp — headless acceptance coverage for ARCH §21.1's ordered
// multi-select set: primary-as-last-element, Replace/Toggle/Union's exact contracts, and the
// order-sensitive equality the release-time gesture resolver depends on to skip a no-op callback.
#include "MapCanvas_SelectionSet_UI.h"
#include <cstdio>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

int failureCount = 0;
void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

OverlayInstanceKey_UI MakeKey(std::int32_t instanceIndex, bool bManual = true,
                              PlacementCollectionKind_UI collection = PlacementCollectionKind_UI::Markers) {
    return OverlayInstanceKey_UI{collection, instanceIndex, /*bValid=*/true, bManual};
}

void RunPrimaryAndContainsChecks() {
    OverlayInstanceKeySet_UI set;
    Check(!PrimaryOfSelectionSet(set).bValid, "an empty set's primary is the default invalid key");
    Check(!SelectionSetContains(set, MakeKey(1)), "an empty set contains nothing");

    set.keys.push_back(MakeKey(1));
    set.keys.push_back(MakeKey(2));
    Check(OverlayInstanceKeysEqual(PrimaryOfSelectionSet(set), MakeKey(2)),
          "primary is the LAST element, not the first or some other derivation");
    Check(SelectionSetContains(set, MakeKey(1)) && SelectionSetContains(set, MakeKey(2)),
          "both inserted keys are contained");
    Check(!SelectionSetContains(set, MakeKey(3)), "a never-inserted key is not contained");
    // Two keys with the same instanceIndex but different bManual are DIFFERENT keys (§19.25's own
    // manual-vs-procedural number-space distinction) — containment must respect the full key.
    Check(!SelectionSetContains(set, MakeKey(1, /*bManual=*/false)),
          "the same instanceIndex under a different bManual is a distinct key, not a match");
}

void RunSelectionSetsEqualChecks() {
    OverlayInstanceKeySet_UI a, b;
    Check(SelectionSetsEqual(a, b), "two empty sets are equal");
    a.keys.push_back(MakeKey(1)); a.keys.push_back(MakeKey(2));
    b.keys.push_back(MakeKey(1)); b.keys.push_back(MakeKey(2));
    Check(SelectionSetsEqual(a, b), "two sets with identical keys in the same order are equal");
    OverlayInstanceKeySet_UI c;
    c.keys.push_back(MakeKey(2)); c.keys.push_back(MakeKey(1));
    Check(!SelectionSetsEqual(a, c), "the SAME keys in a different order are NOT equal — order matters (primary differs)");
    OverlayInstanceKeySet_UI d;
    d.keys.push_back(MakeKey(1));
    Check(!SelectionSetsEqual(a, d), "different sizes are never equal");
}

void RunReplaceChecks() {
    OverlayInstanceKeySet_UI set;
    set.keys.push_back(MakeKey(99));   // stale prior selection
    ReplaceSelectionSet(set, {MakeKey(1), MakeKey(2), MakeKey(3)});
    Check(set.keys.size() == 3, "Replace becomes the whole given list, discarding whatever was there");
    Check(OverlayInstanceKeysEqual(PrimaryOfSelectionSet(set), MakeKey(3)),
          "primary is whichever key the caller's own list ends with");
    ReplaceSelectionSet(set, {});
    Check(set.keys.empty(), "Replace with an empty list clears the selection");
}

void RunToggleChecks() {
    OverlayInstanceKeySet_UI set;
    ToggleInSelectionSet(set, MakeKey(1));
    Check(set.keys.size() == 1 && OverlayInstanceKeysEqual(PrimaryOfSelectionSet(set), MakeKey(1)),
          "toggling an absent key appends it, becoming primary");
    ToggleInSelectionSet(set, MakeKey(2));
    Check(set.keys.size() == 2 && OverlayInstanceKeysEqual(PrimaryOfSelectionSet(set), MakeKey(2)),
          "toggling a second absent key appends it, becoming the new primary");
    ToggleInSelectionSet(set, MakeKey(1));
    Check(set.keys.size() == 1 && OverlayInstanceKeysEqual(PrimaryOfSelectionSet(set), MakeKey(2)),
          "toggling a PRESENT, non-primary key erases it; primary is unaffected (still back())");
    ToggleInSelectionSet(set, MakeKey(2));
    Check(set.keys.empty(), "toggling the last remaining (and primary) key empties the set");
    Check(!PrimaryOfSelectionSet(set).bValid, "an emptied set's primary is the default invalid key again");
}

void RunUnionChecks() {
    OverlayInstanceKeySet_UI set;
    set.keys.push_back(MakeKey(1));
    UnionIntoSelectionSet(set, {MakeKey(1), MakeKey(2), MakeKey(3)});
    Check(set.keys.size() == 3, "Union appends every not-already-present key, skips the already-present one");
    Check(OverlayInstanceKeysEqual(set.keys[0], MakeKey(1)),
          "an already-present key keeps its EXISTING position, never re-touched/reordered by a union");
    Check(OverlayInstanceKeysEqual(PrimaryOfSelectionSet(set), MakeKey(3)),
          "the LAST newly-appended key is the final primary");

    OverlayInstanceKeySet_UI unchanged;
    unchanged.keys.push_back(MakeKey(5));
    UnionIntoSelectionSet(unchanged, {MakeKey(5)});
    Check(OverlayInstanceKeysEqual(PrimaryOfSelectionSet(unchanged), MakeKey(5)) && unchanged.keys.size() == 1,
          "if every key in the union list was already present, the primary (and set) is unchanged");
}

// ARCH §21.1 (STEP230) — ToggleEachInSelectionSet is Toggle applied per element, in order, NOT a
// hybrid of Toggle+Union: each key present->erase, absent->append, exactly as a loop of
// ToggleInSelectionSet calls would produce (which is in fact its real implementation).
void RunToggleEachChecks() {
    // Fresh batch, all absent: every key appended in order, last becomes primary.
    OverlayInstanceKeySet_UI freshSet;
    ToggleEachInSelectionSet(freshSet, {MakeKey(1), MakeKey(2), MakeKey(3)});
    Check(freshSet.keys.size() == 3, "toggling a fresh batch in appends every key");
    Check(OverlayInstanceKeysEqual(PrimaryOfSelectionSet(freshSet), MakeKey(3)),
          "the LAST key in a fresh all-absent batch becomes primary");

    // Mixed batch, last operation is an append: present key(s) erased, absent key(s) appended, the
    // appended (present) key is the final primary — the ordinary case.
    OverlayInstanceKeySet_UI mixedAppendLast;
    mixedAppendLast.keys.push_back(MakeKey(5));
    ToggleEachInSelectionSet(mixedAppendLast, {MakeKey(5), MakeKey(6)});
    Check(mixedAppendLast.keys.size() == 1 && OverlayInstanceKeysEqual(mixedAppendLast.keys[0], MakeKey(6)),
          "a mixed batch erases the present key and appends the absent one, ending with just the appended key");
    Check(OverlayInstanceKeysEqual(PrimaryOfSelectionSet(mixedAppendLast), MakeKey(6)),
          "the last key that ENDS UP present (the appended one) is the final primary");

    // Mixed batch, last operation is an ERASE: the last key's own toggle removes it from the set, so
    // primary must fall back to the set's new back() — NOT to the just-erased key, and NOT left
    // pointing at a key no longer in the set.
    OverlayInstanceKeySet_UI mixedEraseLast;
    mixedEraseLast.keys.push_back(MakeKey(1));
    mixedEraseLast.keys.push_back(MakeKey(2));   // primary starts as 2
    ToggleEachInSelectionSet(mixedEraseLast, {MakeKey(2), MakeKey(3), MakeKey(1)});
    // Step by step: 2 present->erase (set={1}); 3 absent->append (set={1,3}, primary 3);
    // 1 present->erase (set={3}) — the LAST key's own toggle (key 1) erased it, so primary falls
    // back to the set's new back(), which is 3.
    Check(mixedEraseLast.keys.size() == 1 && OverlayInstanceKeysEqual(mixedEraseLast.keys[0], MakeKey(3)),
          "erase-fallback case: the final set is exactly {3} after the 3-step mixed toggle sequence");
    Check(OverlayInstanceKeysEqual(PrimaryOfSelectionSet(mixedEraseLast), MakeKey(3)),
          "erase-fallback case: when the LAST key's own toggle erases it, primary falls back to the "
          "set's new back() (3), matching ToggleInSelectionSet's own erase-case contract, not the "
          "erased key (1) and not the batch's own last-listed key blindly");
}

} // namespace

int main() {
    RunPrimaryAndContainsChecks();
    RunSelectionSetsEqualChecks();
    RunReplaceChecks();
    RunToggleChecks();
    RunUnionChecks();
    RunToggleEachChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
