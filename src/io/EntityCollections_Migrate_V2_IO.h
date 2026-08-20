// EntityCollections_Migrate_V2_IO.h — V2->V3 migration (IO_MIGRATION_SPEC.md §1/§7,
// SANMAP_FORMAT_SPEC Correction 11): two unrelated legacy `mapGeneratorData` fold-ins into the
// format-native `armies`/`markers` collections.
// 1. mapGeneratorData.Armies[key].Color (legacy [r,g,b,a] array) -> armies[key].armyColor object.
// 2. The legacy mapGeneratorData.Aliases dict (aliasName -> markerTransformName) folds into the
//    matching marker transform's own `alias` field, searched across every marker-type group under
//    the top-level `markers` collection.
// NOTE (coder finding, STEP40E): the work-order describes the legacy `Aliases` dict as a TOP-LEVEL
// document key. That does not match this codebase's ground truth — the reference importer/exporter
// (core/MapImporter.cpp:730, core/export/Export_Metadata.cpp:435), IO_PARITY_REPORT.md §3.3 (lists
// `Aliases` alongside `Armies` as "present in v1's [mapGeneratorData] block"), and this spec's own
// §7 worked example (EntityCollections_Migrate_V2_IO's comment: "deletes the old global Aliases
// block" — covered entirely by the step's shared `mapGeneratorData` deletion, with no separate
// `Aliases` entry in `legacyKeysToDelete`) all agree `Aliases` is nested at
// `mapGeneratorData.Aliases`, exactly like `Armies`. Implemented against that ground truth instead.
#pragma once
#include <nlohmann/json.hpp>

namespace SanmapGen {
namespace Io {

// bIndependentlySelectable = false (IO_MIGRATION_SPEC.md §3): no sibling migration in this step
// reads or writes `armies`/marker `alias` fields, but the two-unrelated-sub-tasks-in-one-function
// shape makes a confident independence claim premature for a first ship (work-order STEP40E).
//
// Total and idempotent: a safe no-op when `mapGeneratorData`, `Armies`, or `Aliases` is absent; an
// `Aliases` entry pointing at a transform name that exists nowhere is a safe no-op for that one
// entry (never crashes, never creates a phantom transform).
void EntityCollections_Migrate_V2(nlohmann::json& document);

} // namespace Io
} // namespace SanmapGen
