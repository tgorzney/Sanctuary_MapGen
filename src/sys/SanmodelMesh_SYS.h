// SanmodelMesh_SYS.h — the owned result shape for a parsed `.sanmodel` binary asset. Layer: SYS.
// Positions + triangle indices only: this reader's only confirmed consumer (the Props-tab mesh
// preview, and later the Navmesh Blocker plane-slice) needs shape alone, never normals/tangents/
// UVs/colors/bindposes — see SanmodelRead_SYS.h for the parser that fills this in.
#pragma once
#include <cstdint>
#include <vector>

namespace SanmapGen {
namespace Sys {

struct SanmodelMesh {
    // x,y,z triples, LOCAL/MODEL space, verbatim — no Y-up/Z-up axis conversion performed here
    // (that is a rendering-time concern, never this reader's job).
    std::vector<float> positions;

    // 3 per triangle, already bit-reinterpreted from the source file's int32-bit-pattern-in-a-
    // float-slot encoding (see SanmodelRead_SYS.cpp) — safe to use directly as vertex indices.
    std::vector<std::uint32_t> triangleIndices;

    // True when the source file's optional bindpose segment (segment 8) was present in the byte
    // stream — i.e. the source mesh was skinned. positions/triangleIndices above are always the
    // mesh's rest-pose geometry regardless; this flag is informational only (every real
    // Environment prop's skeleton field is confirmed empty, so this is expected to stay false in
    // practice, but the byte format allows it).
    bool bWasSkinned = false;
};

} // namespace Sys
} // namespace SanmapGen
