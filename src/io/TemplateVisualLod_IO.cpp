// TemplateVisualLod_IO.cpp — see the header for the contract. Consumes an already-evaluated
// Sys::LuaTableValue tree purely in-memory: no file I/O, no Lua evaluation of its own (same
// posture as TemplateDialect_IO.cpp).
#include "TemplateVisualLod_IO.h"

namespace SanmapGen {
namespace Io {

std::vector<TemplateVisualLodEntry> ExtractVisualLods(const Sys::LuaTableValue& rootTable) {
    std::vector<TemplateVisualLodEntry> result;
    const Sys::LuaTableValue* visuals = rootTable.Find("visuals");
    if (visuals == nullptr) return result;
    const Sys::LuaTableValue* lods = visuals->Find("lods");
    if (lods == nullptr || lods->kind != Sys::LuaTableValueKind::Array) return result;

    for (const Sys::LuaTableValue& element : lods->array) {
        if (element.kind != Sys::LuaTableValueKind::Table) continue;
        const Sys::LuaTableValue* modelField = element.Find("model");
        if (modelField == nullptr || modelField->kind != Sys::LuaTableValueKind::Text
            || modelField->text.empty())
            continue;   // no literal model path -- nothing this feature can resolve

        TemplateVisualLodEntry entry;
        entry.model = modelField->text;
        const Sys::LuaTableValue* distanceField = element.Find("distance");
        entry.distance = distanceField != nullptr ? static_cast<float>(distanceField->AsNumber(0.0)) : 0.0f;
        const Sys::LuaTableValue* materialField = element.Find("material");
        if (materialField != nullptr) entry.material = materialField->AsText("");
        result.push_back(std::move(entry));
    }
    return result;
}

const TemplateVisualLodEntry* SelectLowestDistanceLod(const std::vector<TemplateVisualLodEntry>& lods) {
    const TemplateVisualLodEntry* lowest = nullptr;
    for (const TemplateVisualLodEntry& entry : lods)
        if (lowest == nullptr || entry.distance < lowest->distance) lowest = &entry;
    return lowest;
}

} // namespace Io
} // namespace SanmapGen
