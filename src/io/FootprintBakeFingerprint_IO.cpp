// FootprintBakeFingerprint_IO.cpp — Build/Read/staleness-compare for
// Params::FootprintBakeFingerprint. Layer: IO.
// work_orders/STEP96_FootprintBakeAndStalenessCheck_IO.md §3/§5.
#include "FootprintBakeFingerprint_IO.h"
#include "AssetAtlasCache_IO.h"
#include "JsonPrimitives_IO.h"

namespace SanmapGen {
namespace Io {
namespace {

// A local, typed uint64 reader mirroring JsonPrimitives_IO.h's ReadJsonFloat/ReadJsonInteger idiom
// (is_number() gate, typed get<>()) -- no generic uint64 primitive exists there yet, and these three
// fields are specific enough to this one nested object that a new shared primitive for a single
// caller is not warranted; this stays the same class of typed-scalar read those primitives already
// model, not hand-rolled json surgery.
bool ReadJsonUnsigned64(const nlohmann::json& parent, const char* key, std::uint64_t& destination) {
    if (!parent.contains(key) || !parent[key].is_number()) return false;
    destination = parent[key].get<std::uint64_t>();
    return true;
}

} // namespace

nlohmann::ordered_json BuildFootprintBakeFingerprintJson(const Params::FootprintBakeFingerprint& fingerprint) {
    nlohmann::ordered_json json;
    json["SourcePath"]   = fingerprint.sourcePath;
    json["ByteSize"]     = fingerprint.byteSize;
    json["ModifiedTime"] = fingerprint.modifiedTime;
    json["ContentHash"]  = fingerprint.contentHash;
    return json;
}

void ReadFootprintBakeFingerprintJson(const nlohmann::json& parent, const char* key,
                                      Params::FootprintBakeFingerprint& out) {
    if (!parent.contains(key) || !parent[key].is_object()) return;
    const nlohmann::json& nested = parent[key];
    ReadJsonText(nested, "SourcePath", out.sourcePath);
    ReadJsonUnsigned64(nested, "ByteSize", out.byteSize);
    ReadJsonUnsigned64(nested, "ModifiedTime", out.modifiedTime);
    ReadJsonUnsigned64(nested, "ContentHash", out.contentHash);
}

bool FootprintBakeFingerprintIsStale(const Params::FootprintBakeFingerprint& baked,
                                     const SourceFingerprint& current) {
    return baked.sourcePath   != current.sourcePath
        || baked.byteSize     != current.byteSize
        || baked.modifiedTime != current.modifiedTime
        || baked.contentHash  != current.contentHash;
}

} // namespace Io
} // namespace SanmapGen
