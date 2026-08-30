// MapCanvas_SelectionSet_UI.cpp — see MapCanvas_SelectionSet_UI.h for the full rationale.
#include "MapCanvas_SelectionSet_UI.h"

namespace SanmapGen {
namespace Ui {

const OverlayInstanceKey_UI& PrimaryOfSelectionSet(const OverlayInstanceKeySet_UI& set) {
    static const OverlayInstanceKey_UI kEmptyKey;
    return set.keys.empty() ? kEmptyKey : set.keys.back();
}

bool SelectionSetContains(const OverlayInstanceKeySet_UI& set, const OverlayInstanceKey_UI& key) {
    for (const OverlayInstanceKey_UI& existing : set.keys)
        if (OverlayInstanceKeysEqual(existing, key)) return true;
    return false;
}

bool SelectionSetsEqual(const OverlayInstanceKeySet_UI& a, const OverlayInstanceKeySet_UI& b) {
    if (a.keys.size() != b.keys.size()) return false;
    for (std::size_t index = 0; index < a.keys.size(); ++index)
        if (!OverlayInstanceKeysEqual(a.keys[index], b.keys[index])) return false;
    return true;
}

void ReplaceSelectionSet(OverlayInstanceKeySet_UI& set, const std::vector<OverlayInstanceKey_UI>& keys) {
    set.keys = keys;
}

void ToggleInSelectionSet(OverlayInstanceKeySet_UI& set, const OverlayInstanceKey_UI& key) {
    for (std::size_t index = 0; index < set.keys.size(); ++index) {
        if (OverlayInstanceKeysEqual(set.keys[index], key)) {
            set.keys.erase(set.keys.begin() + static_cast<std::ptrdiff_t>(index));
            return;
        }
    }
    set.keys.push_back(key);
}

void ToggleEachInSelectionSet(OverlayInstanceKeySet_UI& set, const std::vector<OverlayInstanceKey_UI>& keys) {
    for (const OverlayInstanceKey_UI& key : keys) ToggleInSelectionSet(set, key);
}

void UnionIntoSelectionSet(OverlayInstanceKeySet_UI& set, const std::vector<OverlayInstanceKey_UI>& keys) {
    for (const OverlayInstanceKey_UI& key : keys)
        if (!SelectionSetContains(set, key)) set.keys.push_back(key);
}

} // namespace Ui
} // namespace SanmapGen
