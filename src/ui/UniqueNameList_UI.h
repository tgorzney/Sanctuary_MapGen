// UniqueNameList_UI.h — the generic "keep every row's name unique" repair. Layer: UI.
// Shared across every entity list keyed by NAME on export (armies, areas, and any future one):
// a `.sanmap` dictionary section is a JSON object keyed by `name`, so two rows sharing a name would
// silently collide and one would clobber the other on export — Constitution §6 applied to a name
// the designer typed. Constrained only to "has a `std::string name` member"; no base class, no
// virtual dispatch, no widget of its own (ARCH Expert ruling, STEP20) — same headless posture as
// WidgetHelpers_UI.h / DraggableListWidget_UI.h: no imgui include, pure and testable without a
// window.
#pragma once
#include <cstddef>
#include <string>
#include <vector>

namespace SanmapGen {
namespace Ui {

// True when some EARLIER row already answers to `name` — the half of the uniqueness rule that
// decides which of two clashing rows is the one that gets renamed (the later one).
template<typename T>
bool NameIsTakenBefore(const std::vector<T>& rows, std::size_t rowIndex, const std::string& name) {
    const std::size_t rowCount = rowIndex < rows.size() ? rowIndex : rows.size();
    for (std::size_t earlierRowIndex = 0u; earlierRowIndex < rowCount; ++earlierRowIndex)
        if (rows[earlierRowIndex].name == name) return true;
    return false;
}

// Repairs duplicate names by suffixing the later row (`Base`, `Base_1`, `Base_2` ...). Reports
// whether any name moved, so a caller only re-runs export-side work on the frames a name settled.
template<typename T>
bool MakeNamesUnique(std::vector<T>& rows) {
    bool bNamesMoved = false;
    for (std::size_t rowIndex = 0u; rowIndex < rows.size(); ++rowIndex) {
        if (!NameIsTakenBefore(rows, rowIndex, rows[rowIndex].name)) continue;
        const std::string baseName = rows[rowIndex].name;
        int suffix = 1;
        do {
            rows[rowIndex].name = baseName + "_" + std::to_string(suffix++);
        } while (NameIsTakenBefore(rows, rowIndex, rows[rowIndex].name));
        bNamesMoved = true;
    }
    return bNamesMoved;
}

// The name a fresh row is seeded with before the uniqueness repair runs — `baseLabel` plus the
// count of rows that already existed, so a v1 project (which coined names this exact way for
// areas) reads the same.
inline std::string NextUniqueLabel(const char* baseLabel, int existingCount) {
    return std::string(baseLabel != nullptr ? baseLabel : "") + "_"
         + std::to_string(existingCount < 0 ? 0 : existingCount);
}

} // namespace Ui
} // namespace SanmapGen
