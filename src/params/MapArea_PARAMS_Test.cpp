// MapArea_PARAMS_Test.cpp — STEP227/ARCH §14.19 acceptance: `MapAreaSize`/`InsertMapAreaSortedBySize`,
// the ONE shared function every insertion call site now routes through to keep `recipe.areas`
// continuously sorted ascending by size, smallest first. Pure checks; no imgui, no window, no GL
// context.
#include "MapArea_PARAMS.h"
#include <cstdio>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

Params::MapArea MakeArea(const char* name, float width, float length) {
    Params::MapArea area;
    area.name = name;
    area.width = width;
    area.length = length;
    return area;
}

void RunMapAreaSizeChecks() {
    Check(Params::MapAreaSize(MakeArea("A", 4.0f, 5.0f)) == 20.0f,
          "size is plain width * length, never a bounding diagonal or max(width, length)");
    Check(Params::MapAreaSize(MakeArea("B", 0.0f, 0.0f)) == 0.0f, "a zero-extent area has zero size");
    Check(Params::MapAreaSize(MakeArea("C", 10.0f, 10.0f)) == 100.0f, "a square area's size is the square");
}

// Ascending insertion order is maintained across several inserts of varying size, and the returned
// index matches the area's actual landing position.
void RunAscendingOrderChecks() {
    std::vector<Params::MapArea> areas;
    const std::size_t indexOfMedium = Params::InsertMapAreaSortedBySize(areas, MakeArea("Medium", 10.0f, 10.0f));
    Check(indexOfMedium == 0u, "the first insert always lands at index 0");
    Check(areas.size() == 1u && areas[0].name == "Medium", "and the returned index matches reality");

    const std::size_t indexOfLarge = Params::InsertMapAreaSortedBySize(areas, MakeArea("Large", 20.0f, 20.0f));
    Check(indexOfLarge == 1u, "a strictly larger area lands AFTER the existing (smaller) entry");
    Check(areas[0].name == "Medium" && areas[1].name == "Large",
          "ascending by size: Medium (100) then Large (400)");

    const std::size_t indexOfSmall = Params::InsertMapAreaSortedBySize(areas, MakeArea("Small", 1.0f, 1.0f));
    Check(indexOfSmall == 0u, "a strictly smaller area lands BEFORE every existing entry");
    Check(areas[0].name == "Small" && areas[1].name == "Medium" && areas[2].name == "Large",
          "the vector stays fully ascending by size after three inserts of varying size: "
          "Small (1), Medium (100), Large (400)");

    const std::size_t indexOfBetween =
        Params::InsertMapAreaSortedBySize(areas, MakeArea("Between", 12.0f, 12.0f));   // size 144
    Check(indexOfBetween == 2u, "an area sized between two existing entries lands between them");
    Check(areas[0].name == "Small" && areas[1].name == "Medium" && areas[2].name == "Between"
          && areas[3].name == "Large",
          "Small (1), Medium (100), Between (144), Large (400) — still fully ascending");
}

// A tie (equal size) inserts AFTER all existing equal-size entries — first-come-first-served, per
// the "inserts before the first STRICTLY LARGER entry" wording.
void RunTieBreakingChecks() {
    std::vector<Params::MapArea> areas;
    Params::InsertMapAreaSortedBySize(areas, MakeArea("First", 10.0f, 10.0f));    // size 100
    Params::InsertMapAreaSortedBySize(areas, MakeArea("Second", 10.0f, 10.0f));   // size 100, a tie
    Check(areas[0].name == "First" && areas[1].name == "Second",
          "an equal-size insert lands AFTER the existing equal-size entry, never before it");

    const std::size_t indexOfThird =
        Params::InsertMapAreaSortedBySize(areas, MakeArea("Third", 10.0f, 10.0f));   // a third tie
    Check(indexOfThird == 2u, "a THIRD equal-size insert lands after both prior equal-size entries");
    Check(areas[0].name == "First" && areas[1].name == "Second" && areas[2].name == "Third",
          "three equal-size entries keep strict first-come-first-served order");

    // A strictly larger entry inserted after the tied group must land AFTER all three ties, not
    // between them.
    const std::size_t indexOfLargest =
        Params::InsertMapAreaSortedBySize(areas, MakeArea("Largest", 20.0f, 20.0f));   // size 400
    Check(indexOfLargest == 3u, "a strictly larger entry lands after the entire tied group");
}

} // namespace

int main() {
    RunMapAreaSizeChecks();
    RunAscendingOrderChecks();
    RunTieBreakingChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
