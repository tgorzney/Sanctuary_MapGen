// SanmodelRead_SYS.h — bounds-checked binary parser for the `.sanmodel` mesh asset format. Layer:
// SYS. This is buffer-in/struct-out only, the same footing as any other untrusted third-party
// asset reader in this codebase (Constitution §6) — NOT Lua execution, no sandbox machinery needed.
//
// Format (ground truth, cross-checked against a real file's first bytes and the open-source
// Blender importer):
//   <name-string, NUL-terminated>
//   then 9 fixed segments, each: [count][count * N floats], little-endian throughout:
//     0 vertices(3)  1 normals(3)  2 tangents(4)  3 uv1(2)  4 uv2(2, =boneweights if skinned)
//     5 uv3(2)  6 colors(4)  7 indices(3, per triangle)  8 bindposes(16, only if skinned)
//
// CRITICAL, binding correctness requirement: the per-segment `count` field, and every value in
// segment 7 (indices), are the raw bit pattern of an int32 sitting in a 4-byte float slot — read
// via memcpy-based reinterpretation, NEVER `static_cast<int>`/`std::round` on the float value.
// Getting this wrong silently corrupts every segment boundary and every triangle index with no
// observable error. Only segments 0 and 7 are kept (see SanmodelMesh_SYS.h); segments 1-6 and 8
// are still walked (to stay correctly positioned) but their payload bytes are discarded.
#pragma once
#include "SanmodelMesh_SYS.h"
#include <cstddef>
#include <string>

namespace SanmapGen {
namespace Sys {

// Every cap is a setting with a sane default (Constitution §8) — generous relative to any real
// `.sanmodel` observed so far; no real corpus measurement exists yet, so this is deliberately not
// over-tuned.
struct SanmodelReadLimits {
    std::size_t maximumSourceByteSize = 64ull * 1024 * 1024;   // 64 MB
};

struct SanmodelReadResult {
    bool          bSucceeded = false;
    std::string   errorMessage;   // populated on any failure, names which segment/step failed
    SanmodelMesh  mesh;           // valid only when bSucceeded == true
};

// Parses `byteSize` bytes at `bytes` as a `.sanmodel` asset. Bounds-checks every read against the
// remaining buffer length before advancing — never trusts a `count` field to stay inside the
// buffer. Total: never throws, never asserts, never crashes on malformed/truncated input; any
// bounds violation or format error degrades to a clean `bSucceeded=false` result.
SanmodelReadResult ReadSanmodelMesh(const unsigned char* bytes, std::size_t byteSize,
                                     const SanmodelReadLimits& limits = SanmodelReadLimits());

} // namespace Sys
} // namespace SanmapGen
