// IconGridWidget_TestSupport_UI.h — shared scaffolding for the M5-3 icon-grid acceptance test.
// Test-only. The atlas build is M5-4 and is NOT in this work-order, so the manifest here is a
// MOCK the test constructs itself: 4096-pixel atlas pages of 64x64 thumbnails, 64 per atlas row,
// 4096 per page — the shape ASSET_LOADING_SPEC describes. Expected UV rects inside the aspect
// tests are written out independently, never read back from the code under test.
#pragma once
#include <cstddef>
#include <cstdio>
#include "IconGridWidget_UI.h"

namespace SanmapGen {
namespace IconGridTest {

inline int& FailureCount() { static int failures = 0; return failures; }

inline void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++FailureCount(); }
}

inline bool IsNear(float value, float expected) {
    const float difference = value - expected;
    return difference < 1.0e-6f && difference > -1.0e-6f;
}

enum : int { kIconsPerAtlasRow = 64, kIconsPerAtlasPage = kIconsPerAtlasRow * kIconsPerAtlasRow };
const float kUvStep = 1.0f / static_cast<float>(kIconsPerAtlasRow);

// Icon `i` gets id 1000 + i so an assertion can never confuse an id with an index.
inline Ui::IconAtlasManifest MakeMockAtlasManifest(int iconCount) {
    Ui::IconAtlasManifest manifest;
    manifest.entries.resize(static_cast<std::size_t>(iconCount));
    for (int iconIndex = 0; iconIndex < iconCount; ++iconIndex) {
        Ui::IconAtlasEntry& entry = manifest.entries[static_cast<std::size_t>(iconIndex)];
        entry.iconId = 1000 + iconIndex;
        entry.atlasPage = iconIndex / kIconsPerAtlasPage;
        const int withinPage = iconIndex - entry.atlasPage * kIconsPerAtlasPage;
        entry.uvMinimumX = static_cast<float>(withinPage % kIconsPerAtlasRow) * kUvStep;
        entry.uvMinimumY = static_cast<float>(withinPage / kIconsPerAtlasRow) * kUvStep;
        entry.uvMaximumX = entry.uvMinimumX + kUvStep;
        entry.uvMaximumY = entry.uvMinimumY + kUvStep;
    }
    manifest.pageTextureIdentifiers = {11u, 12u, 13u};   // supplied by the atlas owner (IO/SYS)
    return manifest;
}

// 64px cells with 4px spacing, so both strides are exactly 68px.
inline Ui::IconGridLayout MakeLayout(int columnCount) {
    Ui::IconGridLayout layout;
    layout.columnCount = columnCount;
    return layout;
}

inline bool PlacementHasUvRect(const Ui::IconGridCellPlacement& placement, float minimumX,
                               float minimumY) {
    return IsNear(placement.uvMinimumX, minimumX) && IsNear(placement.uvMinimumY, minimumY) &&
           IsNear(placement.uvMaximumX, minimumX + kUvStep) &&
           IsNear(placement.uvMaximumY, minimumY + kUvStep);
}

// Aspect test entry points (one translation unit each, ARCH §1.5).
void TestOnlyVisibleCellsAreVisited();
void TestColumnCountResolution();

} // namespace IconGridTest
} // namespace SanmapGen
