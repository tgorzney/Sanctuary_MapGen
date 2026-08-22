// WorldFootprintSizeTable_IO.h — templateIdentifier -> real-world ground-plane footprint size.
// ARCH_14_03_IconRenderingLod.md §14.3: "No world-footprint-size data exists anywhere in the codebase
// today... needs a new templateIdentifier -> baseFootprintWidth/Depth table, IO-layer,
// asset-derived not PARAMS-authored." This is that table's data shape + a manual/placeholder-
// seeded lookup -- NOT a live .santp/Lua reader. No Lua-table reader exists anywhere in src/io/
// (or src/ at all) today -- confirmed by grep for "santp"/"lua_State"/"luaL_"/"LuaTable"; the only
// hits are unrelated blueprintPath string literals in MapImporter/MapExporter test fixtures
// (e.g. MapImporter_IO_Test.cpp:999). Wiring a real .santp Lua-table reader is the separate,
// unscoped asset-importer effort -- see BuildPlaceholderWorldFootprintSizeTable()'s own comment.
#pragma once
#include <string>
#include <unordered_map>

namespace SanmapGen {
namespace Io {

// Ground-plane extent in real world units -- mirrors the game's own UnitTemplate/PropTemplate
// footprint = {x, y} field verbatim (UNIT_PROP_MARKER_DATA_SPEC.md, confirmed identical shape on
// unit templates and both prop-template dialects). x -> baseFootprintWidth, y -> baseFootprintDepth.
// The game's collisionInfo/collider 3D box (collisionSize.y = height) is a separate, out-of-scope
// concern -- this table is ground-plane size only, the exact input §14.3's icon LOD formula needs.
struct WorldFootprintSize_IO {
    float baseFootprintWidth = 0.0f;
    float baseFootprintDepth = 0.0f;
};

// Reasoned-placeholder fallback for any templateIdentifier not (yet) seeded below --
// Constitution §7 basis tag: REASONED-PLACEHOLDER, not measured, not final. Domain guess uses the
// tpId scheme's own char1 (UNIT_PROP_MARKER_DATA_SPEC.md: 'u'=Unit, 'e'=Prop -- a real,
// already-documented game convention, not an invented split) so an unseeded lookup never silently
// returns a zero-size (invisible) icon -- mirrors AssetAtlasCache_PropThumbnail_IO.cpp's
// MakePlaceholderImage "always an explicit stand-in" discipline (Constitution §6).
inline constexpr WorldFootprintSize_IO kDefaultUnitFootprintSize{2.0f, 2.0f};
inline constexpr WorldFootprintSize_IO kDefaultPropFootprintSize{4.0f, 4.0f};
inline constexpr WorldFootprintSize_IO kDefaultUnknownFootprintSize{2.0f, 2.0f};

// Caller-owned, DATA-free, GPU-free -- same posture as Ui::IconAtlasPairingLookup
// (IconAtlasPairing_UI.h). Duplicate SetFootprint calls for one templateIdentifier are
// last-write-wins (documented policy, not an unexamined accident).
class WorldFootprintSizeTable {
public:
    void Clear() { footprintsByTemplateIdentifier.clear(); }

    void SetFootprint(const std::string& templateIdentifier, float baseFootprintWidth,
                       float baseFootprintDepth) {
        footprintsByTemplateIdentifier[templateIdentifier] =
            WorldFootprintSize_IO{baseFootprintWidth, baseFootprintDepth};
    }

    // Unknown templateIdentifier resolves to a domain-guessed default, never a thrown/asserted
    // failure. Domain guess uses the tpId scheme's char1 directly -- NOT Ui::OverlayDomainKind_UI;
    // IO must not depend upward on UI (Constitution §1 layering).
    WorldFootprintSize_IO Resolve(const std::string& templateIdentifier) const {
        const auto found = footprintsByTemplateIdentifier.find(templateIdentifier);
        if (found != footprintsByTemplateIdentifier.end()) return found->second;
        if (!templateIdentifier.empty()) {
            if (templateIdentifier.front() == 'u') return kDefaultUnitFootprintSize;
            if (templateIdentifier.front() == 'e') return kDefaultPropFootprintSize;
        }
        return kDefaultUnknownFootprintSize;
    }

    std::size_t Count() const { return footprintsByTemplateIdentifier.size(); }

private:
    std::unordered_map<std::string, WorldFootprintSize_IO> footprintsByTemplateIdentifier;
};

// MANUAL / PLACEHOLDER SEED ONLY -- not a .santp parse. Hand-entered from the real footprint
// values a Format Expert consult already confirmed this session by reading the shipped Lua
// directly. Exists so STEP52/STEP53 have a real, non-zero table to consume today; ingesting the
// full ~280 unit + ~98+4 prop template set is separate, unscoped asset-importer work (no
// Lua-table reader exists anywhere in src/io/ today -- see this file's own top comment). Do not
// silently grow a hand-rolled Lua parser into this function to "finish" it.
inline WorldFootprintSizeTable BuildPlaceholderWorldFootprintSizeTable() {
    WorldFootprintSizeTable table;
    table.SetFootprint("uca1001", 1.2f, 1.2f);
    table.SetFootprint("ucl4005", 18.4f, 18.4f);
    return table;
}

} // namespace Io
} // namespace SanmapGen
