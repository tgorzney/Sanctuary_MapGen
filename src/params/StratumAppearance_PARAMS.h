// StratumAppearance_PARAMS.h — the material identity and shader appearance of ONE stratum.
// Layer: PARAMS. A MEMBER file of `Stratum_PARAMS.h` (ARCH §7.1: composition is allowed, rival
// top-level per-stratum types are not); it is reached only as `Params::Stratum::appearance`.
//
// Every field is a `.sanmap` stratum key (SANMAP_FORMAT_SPEC "Stratum"): name; albedo / normal /
// mask TextureLoader paths; tileSize (+Far); triplanar tile sizes; normalScale (+Far); normal and
// height farNearBlend; farColorRemap color (`diffuseRemap` is written from Stratum::tint*, not a
// field on this struct — see below). Names are spelled in full (ARCH §1.1) except where the format
// dictates the word.
//
// NOT DUPLICATED HERE (they already live on `Params::Stratum` and stages consume them there —
// a second copy is exactly the rival-array defect ARCH §7.2.5 was written about):
//   - the PREVIEW BASE COLOR is `tintRed/tintGreen/tintBlue` (Bake multiplies it onto the albedo),
//   - the NEAR TILE SIZE is `tileCount`,
//   - the mask remap window is `maskRemapMinimum/maskRemapMaximum`.
//
// SCOPE NOTE: no generation stage reads the fields below yet — they are the shader-side appearance
// the `.sanmap` round-trip (WO D) writes and the game renderer consumes. They are settings, so they
// live in PARAMS from the moment they are settable (Constitution §8).
#pragma once
#include <string>

namespace SanmapGen {
namespace Params {

// Linear RGBA, the house color convention (matches Params::GradientStop::color).
enum : int { kStratumColorChannelCount = 4 };

struct StratumAppearance {
    // --- Identity. `name` is the designer-facing label and the `.sanmap` stratum name.
    std::string name;                      // empty = the tab shows "Stratum <index>"
    std::string environmentName;           // the sanpack environment this material came from
    std::string materialName;              // the material inside that environment

    // --- Texture sources. Paths only: the loaded texels are DATA, never PARAMS (ARCH §7.1).
    std::string albedoTexturePath;
    std::string normalTexturePath;
    std::string compositeTexturePath;      // the format's `mask` TextureLoader (v1 "Composite")

    // --- Shader color remap (the preview base color is Stratum::tint*, see the note above; the
    // diffuseRemap shader key is written FROM tint*, not from a second color field here — the
    // dead, round-tripping-nothing `diffuseRemapColor` field this comment used to describe was
    // deleted per SANMAP_FORMAT_SPEC Correction 13).
    float farColorRemapColor[kStratumColorChannelCount]  = { 1.0f, 1.0f, 1.0f, 1.0f };

    // --- Tiling. The NEAR tile count is Stratum::tileCount; these are its far/triplanar partners.
    float farTileCount           = 1.0f;
    float triplanarTileCount     = 1.0f;
    float farTriplanarTileCount  = 1.0f;

    // --- Normal / height detail and the far-to-near crossfade.
    float normalScale            = 1.0f;
    float farNormalScale         = 1.0f;
    float normalFarNearBlend     = 0.0f;
    float heightFarNearBlend     = 0.0f;
};

} // namespace Params
} // namespace SanmapGen
