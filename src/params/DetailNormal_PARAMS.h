// DetailNormal_PARAMS.h — the reserved home for the future layered-heightmap-delta system's
// settings (SANMAP_FORMAT_SPEC Correction 8). Layer: PARAMS. Deliberately minimal: the one field
// the spec actually names, `DetailNormalMapSize`, and nothing else — the layered-heightmap-delta
// system itself (a stack of heightmaps producing a delta normal map) is out of scope for this
// ticket (no PROC consumer yet).
//
// Confirmed distinct from `DetailNormalTab_UI.h`'s own `DetailNormalTabState::detailNormalSizeIndex`
// (that stays caller-owned tab state, untouched by this ticket — see that file's own SCOPE NOTE 2).
#pragma once

namespace SanmapGen {
namespace Params {

struct DetailNormal {
    // 1024 matches the v1 default already live in DetailNormalTab_UI.h:45 (DetailNormalTabState's
    // detailNormalSizeIndex defaulting to row 2, which names 1024) — not invented.
    int mapSize = 1024;
};

} // namespace Params
} // namespace SanmapGen
