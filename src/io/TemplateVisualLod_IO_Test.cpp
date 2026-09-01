// TemplateVisualLod_IO_Test.cpp — acceptance test for TemplateVisualLod_IO. Builds
// Sys::LuaTableValue fixtures directly (no live Lua evaluation), mirroring TemplateDialect_IO_Test's
// own fixture-construction style.
#include "TemplateVisualLod_IO.h"

#include <cstdio>
#include <string>
#include <utility>
#include <vector>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

Sys::LuaTableValue MakeText(const std::string& text) {
    Sys::LuaTableValue value;
    value.kind = Sys::LuaTableValueKind::Text;
    value.text = text;
    return value;
}
Sys::LuaTableValue MakeNumber(double number) {
    Sys::LuaTableValue value;
    value.kind = Sys::LuaTableValueKind::Number;
    value.number = number;
    return value;
}
Sys::LuaTableValue MakeTable(std::vector<std::pair<std::string, Sys::LuaTableValue>> entries) {
    Sys::LuaTableValue value;
    value.kind = Sys::LuaTableValueKind::Table;
    value.table = std::move(entries);
    return value;
}
Sys::LuaTableValue MakeArray(std::vector<Sys::LuaTableValue> elements) {
    Sys::LuaTableValue value;
    value.kind = Sys::LuaTableValueKind::Array;
    value.array = std::move(elements);
    return value;
}
Sys::LuaTableValue MakeLodEntry(double distance, const std::string& model, const std::string& material) {
    return MakeTable({ {"distance", MakeNumber(distance)}, {"model", MakeText(model)},
                       {"material", MakeText(material)} });
}

} // namespace

int main() {
    // Multi-LOD template, deliberately out of distance order — SelectLowestDistanceLod must pick
    // the minimum, never array index 0.
    {
        const Sys::LuaTableValue root = MakeTable({ {"visuals", MakeTable({
            {"lods", MakeArray({
                MakeLodEntry(50.0, "Environment/A/prop_lod2.sanmodel", "matB"),
                MakeLodEntry(0.0,  "Environment/A/prop_lod0.sanmodel", "matA"),
                MakeLodEntry(20.0, "Environment/A/prop_lod1.sanmodel", "matA"),
            }) }
        }) } });
        const std::vector<Io::TemplateVisualLodEntry> lods = Io::ExtractVisualLods(root);
        Check(lods.size() == 3, "multi-lod: extracts all three entries");
        const Io::TemplateVisualLodEntry* lowest = Io::SelectLowestDistanceLod(lods);
        Check(lowest != nullptr, "multi-lod: a lowest entry is found");
        Check(lowest != nullptr && lowest->model == "Environment/A/prop_lod0.sanmodel",
              "multi-lod: minimum-distance entry selected, not array index 0");
    }

    // Single-LOD template.
    {
        const Sys::LuaTableValue root = MakeTable({ {"visuals", MakeTable({
            {"lods", MakeArray({ MakeLodEntry(0.0, "Environment/B/only_lod0.sanmodel", "mat") }) }
        }) } });
        const std::vector<Io::TemplateVisualLodEntry> lods = Io::ExtractVisualLods(root);
        Check(lods.size() == 1, "single-lod: extracts the one entry");
        const Io::TemplateVisualLodEntry* lowest = Io::SelectLowestDistanceLod(lods);
        Check(lowest != nullptr && lowest->model == "Environment/B/only_lod0.sanmodel",
              "single-lod: the one entry is selected");
    }

    // Dialect-B / engine-lua template: no `visuals.lods` at all -> graceful empty skip, not a crash.
    {
        const Sys::LuaTableValue root = MakeTable({ {"general", MakeTable({ {"tpId", MakeText("exe0000")} })} });
        const std::vector<Io::TemplateVisualLodEntry> lods = Io::ExtractVisualLods(root);
        Check(lods.empty(), "no-visuals: empty result, no crash");
        Check(Io::SelectLowestDistanceLod(lods) == nullptr, "no-visuals: selecting from empty list returns null");
    }

    // `visuals` present but `lods` absent (e.g. ShatterHole_01.santp's deliberate omission).
    {
        const Sys::LuaTableValue root = MakeTable({ {"visuals", MakeTable({ {"isWreckage", MakeNumber(0.0)} })} });
        const std::vector<Io::TemplateVisualLodEntry> lods = Io::ExtractVisualLods(root);
        Check(lods.empty(), "visuals-without-lods: empty result");
    }

    // A malformed entry (missing `model`) is skipped, not a hard failure; the well-formed sibling
    // entry still comes through.
    {
        const Sys::LuaTableValue root = MakeTable({ {"visuals", MakeTable({
            {"lods", MakeArray({
                MakeTable({ {"distance", MakeNumber(0.0)} }),   // no `model` -- skipped
                MakeLodEntry(10.0, "Environment/C/good_lod0.sanmodel", "mat"),
            }) }
        }) } });
        const std::vector<Io::TemplateVisualLodEntry> lods = Io::ExtractVisualLods(root);
        Check(lods.size() == 1, "malformed-entry: the modelless entry is skipped");
        Check(!lods.empty() && lods[0].model == "Environment/C/good_lod0.sanmodel",
              "malformed-entry: the well-formed sibling survives");
    }

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
