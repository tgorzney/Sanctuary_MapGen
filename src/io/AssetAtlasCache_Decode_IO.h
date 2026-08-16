// AssetAtlasCache_Decode_IO.h — image decoding + the safe fallbacks, private to the atlas cache
// (ARCH §1.5 aspect split). Constitution §6 in one place: nothing here trusts a byte of the
// input — the header is sanity-checked, the dimensions are capped, the payload must be exactly
// as long as the format says, and any failure returns false so the caller packs a placeholder
// instead. GAMEDATA_LAYOUT_SPEC: unit thumbnails and strategic icons are stored .dds (DXT5/DXT1
// or plain 32-bit); prop folders store no thumbnail at all, so one is rendered on demand.
#pragma once
#include "AssetAtlasCache_Atlas_IO.h"
#include <cstddef>
#include <string>

namespace SanmapGen {
namespace Io {
namespace Decode {

// Validated .dds -> RGBA8. Rejects (returns false, with a reason) on a bad magic/header, a
// non-positive or over-cap size, an unsupported pixel format, or a short payload.
bool DecodeDirectDrawSurface(const unsigned char* bytes, std::size_t byteSize,
                             int maximumWidth, int maximumHeight,
                             AtlasImage& outImage, std::string& outRejectionReason);

// DXT1 (8-byte blocks) / DXT5 (16-byte blocks) 4x4 block decode into a tightly packed RGBA8
// surface. Returns false when the payload is shorter than the block grid requires.
bool DecodeBlockCompressedSurface(const unsigned char* blocks, std::size_t byteSize,
                                  int width, int height, bool bHasAlphaBlock, AtlasImage& outImage);

// The "never load an unverified file into the UI" fallback: a self-evident magenta/black
// checker so a missing icon is visible as missing rather than as a silent blank.
AtlasImage MakePlaceholderImage(int width, int height);

// Prop thumbnail plumbing (ASSET_LOADING_SPEC: prop thumbnails are NOT stored). Produces a
// valid, deterministic, cacheable image from the model bytes so the pipeline end-to-end is
// real; render QUALITY is explicitly out of scope for M5-4 and this is not a mesh rasterizer.
AtlasImage RenderPropThumbnail(const unsigned char* modelBytes, std::size_t byteSize,
                               int width, int height);

} // namespace Decode
} // namespace Io
} // namespace SanmapGen
