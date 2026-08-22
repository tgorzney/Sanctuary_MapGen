// IconAtlasPairing_UI.h — templateIdentifier -> {thumbnailIconId, strategicIconId}, the "adapter"
// IconGridWidget_UI.h's own header comment anticipated for when the real M5-4 asset manifest
// landed (IconGridWidget_UI.h:9-14). Deliberately separate from Ui::IconAtlasManifest
// (IconGridWidget_UI.h) rather than widening IconAtlasEntry — ARCH_14_03_IconRenderingLod.md §14.3 rules that manifest stays
// one iconId -> one UV rect because its other consumer, the icon-picker grid, wants exactly one
// slot per entry. Every id here still resolves through that unchanged single-slot manifest.
#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace SanmapGen {
namespace Ui {

// Reuses the sentinel convention IconAtlasEntry::iconId and IconGridState::selectedIconId already
// establish ("-1 = no entry / nothing selected") rather than inventing a second invalid-value
// convention (IconGridWidget_UI.h:26, :80).
inline constexpr int kInvalidIconId = -1;

struct IconIdentifierPairing {
    int thumbnailIconId = kInvalidIconId;
    int strategicIconId = kInvalidIconId;
};

// Caller-owned, DATA-free, GPU-free (ARCH_03_ModuleBoundaries.md §3.2) — a plain string-keyed lookup, no atlas pixels, no
// GL handle. Duplicate templateIdentifier inputs to SetThumbnailIconId are last-write-wins (the
// natural std::unordered_map assignment semantics below) — documented here so it is a known policy,
// not an unexamined accident, should two atlas entries ever share a file stem.
class IconAtlasPairingLookup {
public:
    void Clear() { pairingsByTemplateIdentifier.clear(); }

    void SetThumbnailIconId(const std::string& templateIdentifier, int iconId) {
        pairingsByTemplateIdentifier[templateIdentifier].thumbnailIconId = iconId;
    }

    // Unknown templateIdentifier resolves to a default-constructed pairing (both ids
    // kInvalidIconId) — mirrors Application::TemplateIdentifierOfIcon's own
    // empty-result-on-miss contract (Application_Assets_UI.cpp:75-79), never a thrown/asserted
    // failure.
    IconIdentifierPairing Resolve(const std::string& templateIdentifier) const {
        const auto found = pairingsByTemplateIdentifier.find(templateIdentifier);
        return found != pairingsByTemplateIdentifier.end() ? found->second : IconIdentifierPairing();
    }

    std::size_t Count() const { return pairingsByTemplateIdentifier.size(); }

private:
    std::unordered_map<std::string, IconIdentifierPairing> pairingsByTemplateIdentifier;
};

// Pure builder: one pairing per distinct templateIdentifier, thumbnailIconId = that identifier's
// index into the vector (the same "iconId is its own index" contract BuildIconAtlasManifest
// already establishes, Application_Assets_UI.cpp:57). strategicIconId is intentionally left at
// kInvalidIconId — no authored strategic-icon source exists yet (ARCH_14_03_IconRenderingLod.md §14.3, separate ticket).
// This posture — an entry still gets an explicit, obviously-a-placeholder id rather than being
// silently absent or crashing a future lookup — mirrors the Constitution §6 fallback discipline
// AssetAtlasCache_PropThumbnail_IO.cpp's MakePlaceholderImage already applies to a missing/corrupt
// thumbnail image (AssetAtlasCache_PropThumbnail_IO.cpp:1-3); no new fallback pattern invented.
inline IconAtlasPairingLookup BuildIconAtlasPairingLookup(
    const std::vector<std::string>& iconTemplateIdentifiers) {
    IconAtlasPairingLookup lookup;
    for (std::size_t iconId = 0; iconId < iconTemplateIdentifiers.size(); ++iconId)
        lookup.SetThumbnailIconId(iconTemplateIdentifiers[iconId], static_cast<int>(iconId));
    return lookup;
}

} // namespace Ui
} // namespace SanmapGen
