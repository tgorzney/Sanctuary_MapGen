// StratumArt_DATA.h — the LOADED art of one stratum: its stored .sanmap mask and its albedo.
// Layer: DATA. ARCH §7.1 draws the line as "modes/thresholds -> PARAMS; loaded pixels -> DATA":
// the merge mode, the slope window and the remap are recipe settings (`Params::Stratum`), but
// the pixels an importer or the asset loader brings in are loaded input, never part of the
// recipe. They live here so no `_PARAMS` type ever carries a pixel buffer.
// Plain data + accessors; no behavior, no GPU handles (ARCH §3.2).
#pragma once
#include "FloatField_DATA.h"

namespace SanmapGen {
namespace Data {

struct StratumArt {
    // The stored stratum mask from a .sanmap import (0..1 surface weights, any resolution).
    // The Mask stage resamples it bilinearly onto the generated grid — the ONE resampler
    // (MASKING_SPEC 1.8). `importedMaskVersion` is bumped by the ONE production writer
    // (MapImporter_Fields_IO.cpp's LoadStratumMaskTga) every time it loads pixel content, so a
    // parameter hash can notice new/changed art without walking megabytes of texels — the exact
    // same "version counter stands in for content" contract `albedoVersion` below already
    // documents, now actually applied to this field too (STEP220 — this field's own hash used to
    // walk every texel, the confirmed root cause of a "dragging an Area is tremendously slow"
    // report, since NotifyParametersChanged() calls this hash on every drag frame).
    FloatField importedMask;
    int importedMaskVersion = 0;   // 0 = never loaded; the loader's own counter starts at 1

    // The stratum's albedo texture: RGBA8 packed little-endian (red in bits 0..7), row-major,
    // owned by the asset loader that produced it — this record only borrows the pointer.
    // `albedoVersion` is bumped by that loader when the pixels change, so a parameter hash can
    // notice new art without walking megabytes of texels.
    const unsigned int* albedoTexels = nullptr;
    int albedoWidth   = 0;
    int albedoHeight  = 0;
    int albedoVersion = 0;

    bool HasImportedMask() const { return !importedMask.IsEmpty(); }
    bool HasAlbedo() const { return albedoTexels != nullptr && albedoWidth > 0 && albedoHeight > 0; }
};

} // namespace Data
} // namespace SanmapGen
