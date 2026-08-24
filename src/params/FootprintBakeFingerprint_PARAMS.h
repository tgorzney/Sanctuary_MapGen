// FootprintBakeFingerprint_PARAMS.h — a source snapshot captured at bake time, compared later to
// detect staleness (ARCH_18_02_IngestedDataDeterminism.md; work_orders/STEP96_FootprintBakeAndStalenessCheck_IO.md).
// Layer: PARAMS. Deliberately field-for-field IDENTICAL to Io::SourceFingerprint
// (src/io/AssetAtlasCache_IO.h:38-48, real shipped code) but NOT the same type and NOT shared via
// include -- Constitution §1 layering is IO -> {DATA, PARAMS}, never the reverse, so PARAMS cannot
// depend on an IO header. This is intentional duplication of a SHAPE, not of a TYPE.
#pragma once
#include <cstdint>
#include <string>

namespace SanmapGen {
namespace Params {

struct FootprintBakeFingerprint {
    std::string   sourcePath;
    std::uint64_t byteSize     = 0;
    std::uint64_t modifiedTime = 0;
    std::uint64_t contentHash  = 0;

    // Empty sourcePath == never baked (ARCH_18_02_IngestedDataDeterminism.md §18.2 rule 4's "not an
    // error state").
    bool IsValid() const { return !sourcePath.empty() && byteSize > 0; }
};

} // namespace Params
} // namespace SanmapGen
