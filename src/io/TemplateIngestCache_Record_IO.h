// TemplateIngestCache_Record_IO.h — private ARCH §1.5 aspect split off TemplateIngestCache_IO.cpp:
// one TemplateRecord's own JSON encode/decode, split out so TemplateIngestCache_IO.cpp stays under
// the hard 150-line ceiling (this ticket's own line-count estimate was optimistic, the same kind of
// self-correction its "Correction 2026-08-22" note already flags for ReadJsonInteger). NOT part of
// this domain's public surface — TemplateIngestCache_IO.h is; only TemplateIngestCache_IO.cpp and
// TemplateIngestCache_Record_IO.cpp include this.
#pragma once
#include "TemplateDialect_IO.h"
#include <cstdint>
#include <nlohmann/json.hpp>

namespace SanmapGen {
namespace Io {

// JsonPrimitives_IO.h has no std::uint64_t-capable accessor (ReadJsonInteger is a fixed `int&`
// out-parameter) — this ticket's own flagged compile-blocker correction. Same never-throwing,
// false-means-untouched contract as every other ReadJson* helper: an absent key or a signed/
// negative/non-numeric value leaves destination untouched.
inline bool ReadJsonUnsignedInteger64(const nlohmann::json& parent, const char* key, std::uint64_t& destination) {
    if (!parent.contains(key) || !parent[key].is_number_unsigned()) return false;
    destination = parent[key].get<std::uint64_t>();
    return true;
}

nlohmann::json RecordToJson(const TemplateRecord& record);

// False on ANY field failing its own type check — a single malformed record aborts the WHOLE
// load (Constitution §6's "never a partial success," per this ticket's own correction note: a
// half-loaded ingestion table would silently under-report real coverage). outRecord is untouched
// on a false return.
bool RecordFromJson(const nlohmann::json& object, TemplateRecord& outRecord);

} // namespace Io
} // namespace SanmapGen
