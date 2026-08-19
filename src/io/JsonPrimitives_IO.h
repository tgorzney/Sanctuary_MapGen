// JsonPrimitives_IO.h — the shared, IO/BRIDGE-scoped JSON toolkit (IO_MIGRATION_SPEC.md §5).
// Header-only, `inline` free functions, no `.cpp` — same header-only-pure-function precedent as
// `WidgetHelpers_UI.h` and `src/math/`'s one-file-per-concept convention. JSON manipulation happens
// nowhere else in the tree (UI never touches a document — ARCH §3.2), so this is the one honest home.
//
// Two families:
//  - The typed READ accessors every block reader is built from (relocated verbatim out of
//    MapImporter_Recipe_IO.h — they were mis-homed there: cross-included by half a dozen other
//    domains purely to reach a category of function that has nothing to do with the Recipe domain
//    specifically). `ReadJsonFloatVector4` stayed behind — it is Stratum/Recipe-domain-specific
//    shape, not a generic primitive.
//  - The TRANSFORM primitives every future `<Domain>_Migrate_V<N>_IO` migration composes from
//    (rename / move / wrap / default / delete). All are TOTAL and IDEMPOTENT (§5's own requirement):
//    calling one twice is always safe and produces the same end state as calling it once.
#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <utility>

namespace SanmapGen {
namespace Io {

// --- The typed accessors every reader is built from. -------------------------------------------
inline bool ReadJsonFloat(const nlohmann::json& parent, const char* key, float& destination) {
    if (!parent.contains(key) || !parent[key].is_number()) return false;
    destination = parent[key].get<float>();
    return true;
}

inline bool ReadJsonInteger(const nlohmann::json& parent, const char* key, int& destination) {
    if (!parent.contains(key) || !parent[key].is_number()) return false;
    destination = parent[key].get<int>();
    return true;
}

inline bool ReadJsonBoolean(const nlohmann::json& parent, const char* key, bool& destination) {
    if (!parent.contains(key) || !parent[key].is_boolean()) return false;
    destination = parent[key].get<bool>();
    return true;
}

inline bool ReadJsonText(const nlohmann::json& parent, const char* key, std::string& destination) {
    if (!parent.contains(key) || !parent[key].is_string()) return false;
    destination = parent[key].get<std::string>();
    return true;
}

// An enum stored as its integer value. The value is FENCED to [0, valueCount) so a document
// written by a newer build can never index an enum out of range.
inline bool ReadJsonEnumeration(const nlohmann::json& parent, const char* key, int valueCount,
                                int& destination) {
    int value = destination;
    if (!ReadJsonInteger(parent, key, value)) return false;
    if (value < 0 || value >= valueCount) return false;
    destination = value;
    return true;
}

// Reads an integer and CLAMPS it into [minimum, maximum], OVERWRITING `destination` with the
// clamped result — unlike `ReadJsonEnumeration`, which REJECTS an out-of-range value and leaves
// `destination` at its prior/default. A saved 500 imports as `maximum`, not as whatever the
// struct's own default happened to be (STEP23 ruling #6, e.g. `radialSymmetryRepeatCount`). Silent,
// no logging — same posture as `ReadJsonEnumeration`'s idiom (no logging subsystem exists in
// src/sys, Constitution §6); human-facing surfacing belongs on a future UI widget ticket.
inline bool ReadJsonIntegerClamped(const nlohmann::json& parent, const char* key, int minimum,
                                   int maximum, int& destination) {
    int value = destination;
    if (!ReadJsonInteger(parent, key, value)) return false;
    if (value < minimum) value = minimum;
    if (value > maximum) value = maximum;
    destination = value;
    return true;
}

// --- The transform primitives every migration composes from (IO_MIGRATION_SPEC.md §5). ---------

// Moves a value to a new key in the SAME object. No-op — total and idempotent — if `oldKey` is
// absent (covers both "never had it" and "already renamed it once").
inline void RenameKey(nlohmann::json& parent, const char* oldKey, const char* newKey) {
    if (!parent.contains(oldKey)) return;
    parent[newKey] = std::move(parent[oldKey]);
    parent.erase(oldKey);
}

// Moves a value ACROSS objects — the cross-domain primitive (pulling a field out of the legacy
// `mapGeneratorData` blob into a new top-level section). No-op if `sourceKey` is absent.
inline void MoveKey(nlohmann::json& sourceParent, const char* sourceKey,
                    nlohmann::json& destinationParent, const char* destinationKey) {
    if (!sourceParent.contains(sourceKey)) return;
    destinationParent[destinationKey] = std::move(sourceParent[sourceKey]);
    sourceParent.erase(sourceKey);
}

// Replaces a scalar at `key` with a single-element array containing it — the exact tool for a
// global -> per-layer cardinality change. Idempotent by construction: a `key` that is ALREADY an
// array is left untouched (a second call must never double-wrap into `[[x]]`).
inline void WrapScalarAsVector(nlohmann::json& parent, const char* key) {
    if (!parent.contains(key)) return;
    if (parent[key].is_array()) return;
    nlohmann::json wrapped = nlohmann::json::array();
    wrapped.push_back(std::move(parent[key]));
    parent[key] = std::move(wrapped);
}

// Sets `key` only if absent; never overwrites a value already present (idempotent by construction).
inline void DefaultIfMissing(nlohmann::json& parent, const char* key, nlohmann::json defaultValue) {
    if (parent.contains(key)) return;
    parent[key] = std::move(defaultValue);
}

// Erases `key` if present, no-op otherwise. The primitive the manifest's legacy-key cleanup is
// built from (§3).
inline void DeleteKeyIfPresent(nlohmann::json& parent, const char* key) {
    if (!parent.contains(key)) return;
    parent.erase(key);
}

} // namespace Io
} // namespace SanmapGen
