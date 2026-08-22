// IconAtlasPairing_UI_Test.cpp — acceptance test for STEP52's pairing lookup: the
// templateIdentifier -> {thumbnailIconId, strategicIconId} adapter. Pure/no-atlas (ARCH §8.1) —
// the real end-to-end wiring through Application::LoadAssetAtlas() is covered separately in
// ApplicationShell_IconBridge_UI_Test.cpp.
#include "IconAtlasPairing_UI.h"
#include <cstdio>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

static void TestDefaultLookupResolvesToAllInvalid() {
    const Ui::IconAtlasPairingLookup lookup;
    const Ui::IconIdentifierPairing pairing = lookup.Resolve("anything");
    Check(pairing.thumbnailIconId == Ui::kInvalidIconId, "default lookup thumbnail id invalid");
    Check(pairing.strategicIconId == Ui::kInvalidIconId, "default lookup strategic id invalid");
}

static void TestBuildResolvesEachTemplateIdentifier() {
    const std::vector<std::string> identifiers = { "unitA", "unitB" };
    const Ui::IconAtlasPairingLookup lookup = Ui::BuildIconAtlasPairingLookup(identifiers);

    const Ui::IconIdentifierPairing unitAPairing = lookup.Resolve("unitA");
    Check(unitAPairing.thumbnailIconId == 0, "unitA thumbnail id is its index");
    Check(unitAPairing.strategicIconId == Ui::kInvalidIconId, "unitA strategic id still invalid");

    const Ui::IconIdentifierPairing unitBPairing = lookup.Resolve("unitB");
    Check(unitBPairing.thumbnailIconId == 1, "unitB thumbnail id is its index");
    Check(unitBPairing.strategicIconId == Ui::kInvalidIconId, "unitB strategic id still invalid");

    const Ui::IconIdentifierPairing unlistedPairing = lookup.Resolve("unlistedIdentifier");
    Check(unlistedPairing.thumbnailIconId == Ui::kInvalidIconId, "unlisted identifier thumbnail id invalid");
    Check(unlistedPairing.strategicIconId == Ui::kInvalidIconId, "unlisted identifier strategic id invalid");

    Check(lookup.Count() == 2u, "two distinct identifiers were recorded");
}

static void TestDuplicateIdentifiersAreLastWriteWins() {
    const std::vector<std::string> identifiers = { "unitA", "unitA" };
    const Ui::IconAtlasPairingLookup lookup = Ui::BuildIconAtlasPairingLookup(identifiers);

    const Ui::IconIdentifierPairing pairing = lookup.Resolve("unitA");
    Check(pairing.thumbnailIconId == 1, "duplicate identifier resolves to the LAST index (last-write-wins)");
    Check(lookup.Count() == 1u, "duplicate identifiers collapse to one entry");
}

int main() {
    TestDefaultLookupResolvesToAllInvalid();
    TestBuildResolvesEachTemplateIdentifier();
    TestDuplicateIdentifiersAreLastWriteWins();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
