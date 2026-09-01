// TemplateVisualLod_IO.h — extracts a prop template's `visuals.lods[]` array (each entry
// {distance, model, material}) from an already-evaluated Sys::LuaTableValue tree. Layer: IO. A
// new, additive sibling file — deliberately NOT an extension of TemplateDialect_IO's own shipped
// TemplateRecord (reopening a shipped type's scope for one feature is exactly what this avoids).
// Reuses Io::DetectTemplateRootTable for dialect detection rather than re-deriving it.
#pragma once
#include "../sys/LuaTableValue_SYS.h"
#include <string>
#include <vector>

namespace SanmapGen {
namespace Io {

struct TemplateVisualLodEntry {
    float       distance = 0.0f;
    // Literal pack-relative path, e.g. "Environment/01_Highlands/Props/edmm0101/edms0103_lod0.sanmodel"
    // — NEVER synthesized from tpId/folder name (confirmed real defect: a model filename stem can
    // differ from its owning folder's name).
    std::string model;
    std::string material;
};

// Walks `rootTable`'s `visuals.lods` array (each element a table with distance/model/material
// scalar fields); non-table elements and elements missing a non-empty `model` are skipped, never a
// hard failure (Constitution §6 — a shipped .santp's malformed field is documented reality). Empty
// result for a template with no `lods[]` at all (a graceful skip, not an error — the four
// `exe0000`-`exe0002`/`defaultWreckage` engine-lua test templates have none).
std::vector<TemplateVisualLodEntry> ExtractVisualLods(const Sys::LuaTableValue& rootTable);

// The entry this feature actually wants: MINIMUM `distance`, never array index 0 (self-correcting
// against a malformed/reordered file). Returns nullptr for an empty list.
const TemplateVisualLodEntry* SelectLowestDistanceLod(const std::vector<TemplateVisualLodEntry>& lods);

} // namespace Io
} // namespace SanmapGen
