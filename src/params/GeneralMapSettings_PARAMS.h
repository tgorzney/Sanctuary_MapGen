// GeneralMapSettings_PARAMS.h — the durable home for `globalGravity`, the UI's per-stratum
// bulk-write mirror. Layer: PARAMS. `SANMAP_FORMAT_SPEC.md` Correction 2: "a genuine new-field
// addition ... not a relocation" — NOT geometry (`Geometry_PARAMS.h`'s own SCOPE stays untouched),
// and NOT a rival store for `ErosionLayerSettings::gravity`, which stays the one real per-stratum
// value (ARCH §7.1 "no rival settings type"). This struct exists purely so the UI's last bulk-set
// value survives a save/load instead of resetting to its hardcoded default every session
// (HeightmapTab_UI.h:70, that file's own SCOPE NOTE 2; Constitution §8).
#pragma once

namespace SanmapGen {
namespace Params {

struct GeneralMapSettings {
    // Default matches HeightmapTabState::globalGravity's current hardcoded default
    // (HeightmapTab_UI.h:70). UI wiring (reading/writing this field instead of its own local
    // default) is a separate, already-tracked follow-up — not this ticket.
    float globalGravity = 4.0f;
};

} // namespace Params
} // namespace SanmapGen
