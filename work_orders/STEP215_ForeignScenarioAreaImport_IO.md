# STEP215 — Foreign scenario `.lua` area-rectangle import: `ScenarioScript_AreaRectangleExtract_IO` (pure grammar) + `ScenarioScript_AreaImport_IO` (disk-touching entry point)

**Layer:** IO. **Domain:** the `ScenarioScript_*_IO` family (new, physically separate translation units,
per `ARCH_15_11_ForeignScenarioAreaImport.md`'s own placement ruling), `recipe.areas`.
**Executor:** SanGen Coder. Authored by the SanGen Format Expert, per `ARCH_15_11_ForeignScenarioAreaImport.md`
(§15.11, ratified 2026-08-29) and the SanGen IO Architecture Expert's structural ruling on the exact
file/module split (relayed verbatim into this ticket's dispatch — not yet written to any ARCH file).
Every file this ticket cites was read directly against the live tree while drafting it; the real
scenario file (`map_scripts_backup/Pandemonium Isthmus_Scenarios_Script.lua.officialbak`) supplied the
grammar fixtures below verbatim — this ticket does not repeat `MAP_SCENARIO_SPEC.md`'s own inaccurate
`{846, 846, 356, 356}` positional-shorthand prose, which §15.11 itself flags as not matching the real
file's named-key syntax.

## Summary
Mechanizes the one narrow carve-out §15.11 grants against §15.3's otherwise-absolute "SanGen never
reads scenario content back" rule: a human-triggered, one-shot, non-executing extraction of
`Params::MapArea` rectangles ONLY from a foreign (non-SanGen-authored) scenario `.lua` file. Two new
translation-unit pairs, mirroring the family's existing pure/disk split
(`ScenarioScript_DataLua_IO` pure ↔ `ScenarioScript_Export_IO` disk-touching):

- **`ScenarioScript_AreaRectangleExtract_IO.h`/`.cpp`** — pure, disk-free, filename-agnostic. Takes
  an already-loaded `const std::string& sourceText`; returns extracted `std::vector<Params::MapArea>`
  plus a near-miss diagnostics list. Contains the ENTIRE closed literal-only grammar: comment/string
  skipping, named-key and positional forms, fixed field mapping, in-file last-write-wins collision
  resolution, and the value-level semantic caps (rectangle-count cap, non-finite/absurd-coordinate
  rejection). No Lua execution of any kind, ever.
- **`ScenarioScript_AreaImport_IO.h`/`.cpp`** — the disk-touching, human-triggered ONE entry point.
  Reads the file via `FilesystemPrimitives_IO::ReadTextFileBytes`, enforces the byte-size cap
  (checked via `std::filesystem::file_size` BEFORE the file is even read into memory — never loading
  an oversized file to find out it's oversized), runs the refusal guard by filename THEN banner-line
  BEFORE ever calling the pure extractor, then reconciles the returned rectangles against the live
  `Params::MapRecipe::areas` additively (skip-and-report on name collision), returning a report/result
  type mirroring `ScenarioExportResult`'s own shape.

The refusal guard (§15.11 item 1) lives ONLY in the disk-touching wrapper, never in the pure
extractor — mirrors `ScenarioScript_Export_IO.cpp`'s own private `StartsWithBanner` helper pattern.
This ticket keeps that helper as an independent second copy rather than promoting it into
`ScenarioScript_DataLua_IO.h` — see "Interpretation calls made" item 1 for the explicit scope-cut
justification. This ticket also stops at a fully-tested, UI-callable IO entry point and does NOT wire
the Areas-tab "Import Areas from Scenario File..." button — see "Interpretation calls made" item 2.

No `Sanmap_MigrationManifest_IO` entry, no `.sanmap` version bump, no `JsonPrimitives_IO` involvement
anywhere in this ticket — this is a hand-rolled Lua-table-literal grammar, not JSON, and not a schema
migration.

## Required reading
`ARCH_15_11_ForeignScenarioAreaImport.md` (read in full — it is the binding law here; all 11 numbered
items bind, and widening any of them is not this ticket's call), `ARCH_15_MapScenarioSystem.md` §15.3
(the export-only rule this carve-out narrows, not reverses) and §15.4 (the three-file on-disk shape;
confirms `<MapName>_data.lua`/the legacy `_Scenarios_Script.lua` are never written by SanGen, and this
carve-out reads neither of those — it reads a THIRD, foreign file the human points at).

---

## 1. New file: `src/io/ScenarioScript_AreaRectangleExtract_IO.h`

```cpp
// ScenarioScript_AreaRectangleExtract_IO.h -- the pure, disk-free, filename-agnostic closed
// literal-only grammar for extracting Params::MapArea rectangles from FOREIGN scenario .lua text
// (`ARCH_15_11_ForeignScenarioAreaImport.md` §15.11 items 2-7, 10). Layer: IO.
//
// This is NOT "the reader half" of ScenarioScript_DataLua_IO (§15.11 item 11) -- it is a physically
// separate translation unit that never touches a file SanGen itself wrote; the refusal guard that
// keeps those two file sets disjoint lives ONLY in the disk-touching caller
// (ScenarioScript_AreaImport_IO.h), never here. This header takes text, returns values, and performs
// NO Lua execution of any kind -- not LuaTableEvaluate_SYS, not a variant of it, ever (§15.11 item 3).
#pragma once
#include <cstddef>
#include <string>
#include <vector>
#include "../params/MapArea_PARAMS.h"

namespace SanmapGen {
namespace Io {

// One candidate that entered the grammar (matched `[local] IDENTIFIER = { ... }`) but failed it --
// never partially filled into a rectangle, never guessed (§15.11 item 6). `identifier` is always
// non-empty: a diagnostic is only ever created after an identifier token was already matched.
struct ScenarioAreaExtractionNearMiss {
    std::string identifier;
    std::string reason;
};

// Constitution §6 cap -- extraction stops (does not crash, does not unbounded-grow) once this many
// VALID rectangles have been extracted from one file (§15.11 item 10). Chosen generously above any
// real scenario script observed (the reference file defines four) while still bounding pathological
// input; not a map-design limit of any kind.
inline constexpr std::size_t kMaxScenarioAreaExtractionRectangleCount = 512;

// Constitution §6 "absurd coordinate" bound (§15.11 item 10). Real Sanctuary maps top out at a few
// thousand world units per side (the reference file's own AREA_FULL is 2048x2048); this is a
// generous backstop against garbage/overflow-adjacent values, not a tight map-size validator -- a
// later, map-size-aware check (if ever wanted) is a separate, unrelated concern from this
// grammar-level sanity gate. width/height must additionally be strictly positive -- a zero or
// negative extent is not a rectangle.
inline constexpr float kMaxScenarioAreaCoordinateMagnitude = 1.0e6f;

struct ScenarioAreaExtractionResult {
    std::vector<Params::MapArea>               areas;                 // in file order; in-file
                                                                        // collisions already resolved
                                                                        // last-write-wins (item 7)
    std::vector<std::string>                   collisionIdentifiers;  // identifiers reassigned within
                                                                        // THIS file (item 7, logged)
    std::vector<ScenarioAreaExtractionNearMiss> nearMisses;            // item 6
    bool bRectangleCountCapExceeded = false;    // kMaxScenarioAreaExtractionRectangleCount reached --
                                                 // extraction stopped early; the remainder of the
                                                 // source text was never scanned (item 10)
};

// Scans sourceText ONCE, skipping `--` line comments, `--[[ ]]` long comments, and single/double
// -quoted strings (item 5), for `[local] IDENTIFIER = { ... }` candidates whose body matches the
// closed grammar: EITHER all four keyed pairs x/y/width/height in any order, OR exactly four
// positional values read as x, y, width, height in that order (item 4). Arithmetic, identifiers,
// function calls, string keys, nesting, a fifth/missing key, and mixed keyed/positional in one table
// are all rejected outright, never evaluated. Field mapping is fixed and verbatim per item 7: Lua `x`
// -> MapArea::originX, `y` -> originZ, `width` -> width, `height` -> length; the Lua identifier
// becomes MapArea::name VERBATIM (never prettified/de-prefixed/invented -- it is a load-bearing
// gameplay identifier, GameUtils.GetArea(name)).
//
// Pure and total: never throws, never touches the filesystem, never partially mutates its output on
// a mid-scan failure (a rejected candidate contributes zero rectangles and exactly one near-miss).
ScenarioAreaExtractionResult ExtractAreaRectanglesFromScenarioScriptText(const std::string& sourceText);

} // namespace Io
} // namespace SanmapGen
```

## 2. New file: `src/io/ScenarioScript_AreaRectangleExtract_IO.cpp`

```cpp
// ScenarioScript_AreaRectangleExtract_IO.cpp -- see the header for the full contract. The tokenizer
// helpers below are private to this translation unit (anonymous namespace) by explicit IO
// Architecture Expert ruling -- this grammar has exactly one caller today, so no shared primitives
// file is created for them.
#include "ScenarioScript_AreaRectangleExtract_IO.h"
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <unordered_map>

namespace SanmapGen {
namespace Io {
namespace {

enum class TokenKind { Identifier, Number, Equals, LBrace, RBrace, Comma, Minus, Other };

struct Token {
    TokenKind   kind;
    std::string text;   // only meaningful for Identifier/Number
};

bool IsIdentifierStartChar(char c) { return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_'; }
bool IsIdentifierChar(char c)      { return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_'; }
bool IsDigitChar(char c)           { return std::isdigit(static_cast<unsigned char>(c)) != 0; }

// Skips whitespace, `--` line comments, and `--[[ ]]` long comments (item 5). An unterminated long
// comment consumes to end-of-text -- never an infinite loop, never a crash.
std::size_t SkipWhitespaceAndComments(const std::string& text, std::size_t cursor) {
    for (;;) {
        while (cursor < text.size() && std::isspace(static_cast<unsigned char>(text[cursor])) != 0) ++cursor;
        if (cursor + 1 < text.size() && text[cursor] == '-' && text[cursor + 1] == '-') {
            cursor += 2;
            if (cursor + 1 < text.size() && text[cursor] == '[' && text[cursor + 1] == '[') {
                const std::size_t closeIndex = text.find("]]", cursor + 2);
                cursor = (closeIndex == std::string::npos) ? text.size() : closeIndex + 2;
            } else {
                const std::size_t newlineIndex = text.find('\n', cursor);
                cursor = (newlineIndex == std::string::npos) ? text.size() : newlineIndex + 1;
            }
            continue;
        }
        break;
    }
    return cursor;
}

// Skips a single/double-quoted string literal (item 5) -- the live reference file's own comments
// mention rectangle identifiers by name, and a real scenario script's `name = "AlloyMarker_219"`
// fields contain braces nowhere, but a defensive scanner must never let a brace INSIDE a string
// confuse candidate/table-body detection either way. Handles a backslash-escaped quote.
std::size_t SkipQuotedString(const std::string& text, std::size_t cursor) {
    const char quoteChar = text[cursor];
    ++cursor;
    while (cursor < text.size() && text[cursor] != quoteChar) {
        if (text[cursor] == '\\' && cursor + 1 < text.size()) cursor += 2;
        else ++cursor;
    }
    if (cursor < text.size()) ++cursor;   // consume the closing quote
    return cursor;
}

std::vector<Token> Tokenize(const std::string& text) {
    std::vector<Token> tokens;
    std::size_t cursor = 0;
    for (;;) {
        cursor = SkipWhitespaceAndComments(text, cursor);
        if (cursor >= text.size()) break;
        const char c = text[cursor];

        if (c == '"' || c == '\'') { cursor = SkipQuotedString(text, cursor); continue; }

        if (IsIdentifierStartChar(c)) {
            const std::size_t start = cursor;
            while (cursor < text.size() && IsIdentifierChar(text[cursor])) ++cursor;
            tokens.push_back({ TokenKind::Identifier, text.substr(start, cursor - start) });
            continue;
        }

        if (IsDigitChar(c)) {
            const std::size_t start = cursor;
            while (cursor < text.size() && IsDigitChar(text[cursor])) ++cursor;
            if (cursor < text.size() && text[cursor] == '.' && cursor + 1 < text.size() && IsDigitChar(text[cursor + 1])) {
                ++cursor;
                while (cursor < text.size() && IsDigitChar(text[cursor])) ++cursor;
            }
            tokens.push_back({ TokenKind::Number, text.substr(start, cursor - start) });
            continue;
        }

        switch (c) {
            case '=': tokens.push_back({ TokenKind::Equals, "=" }); break;
            case '{': tokens.push_back({ TokenKind::LBrace, "{" }); break;
            case '}': tokens.push_back({ TokenKind::RBrace, "}" }); break;
            case ',': tokens.push_back({ TokenKind::Comma,  "," }); break;
            case '-': tokens.push_back({ TokenKind::Minus,  "-" }); break;
            default:  tokens.push_back({ TokenKind::Other,  std::string(1, c) }); break;
        }
        ++cursor;
    }
    return tokens;
}

// An optional leading Minus token followed by exactly one Number token, consuming the WHOLE
// [index, end) range -- a trailing stray token (e.g. a second number, an identifier) fails this,
// which is exactly the "single decimal numeric literal" requirement of item 4.
bool TryReadSignedNumberSpanningRange(const std::vector<Token>& tokens, std::size_t index, std::size_t end, float& outValue) {
    if (index >= end) return false;
    bool bNegative = false;
    if (tokens[index].kind == TokenKind::Minus) { bNegative = true; ++index; }
    if (index >= end || tokens[index].kind != TokenKind::Number) return false;
    const float magnitude = std::strtof(tokens[index].text.c_str(), nullptr);
    ++index;
    if (index != end) return false;   // extra trailing token in this segment -- reject
    outValue = bNegative ? -magnitude : magnitude;
    return true;
}

struct SegmentSpan { std::size_t start; std::size_t end; };

// Splits [bodyStart, bodyEnd) on every Comma token. Safe to split unconditionally because the
// caller has already rejected any body containing an LBrace (item 4's "nesting" rejection) -- there
// is no nested comma to protect against. A trailing comma before the closing brace yields no empty
// final segment (Lua tolerates a trailing comma); any OTHER empty segment (a double comma) is
// reported to the caller as an empty span, which the caller rejects.
std::vector<SegmentSpan> SplitCommaSegments(const std::vector<Token>& tokens, std::size_t bodyStart, std::size_t bodyEnd) {
    std::vector<SegmentSpan> segments;
    std::size_t segmentStart = bodyStart;
    for (std::size_t index = bodyStart; index < bodyEnd; ++index) {
        if (tokens[index].kind == TokenKind::Comma) {
            segments.push_back({ segmentStart, index });
            segmentStart = index + 1;
        }
    }
    if (segmentStart < bodyEnd) segments.push_back({ segmentStart, bodyEnd });
    return segments;
}

bool ParseSegmentAsKeyed(const std::vector<Token>& tokens, SegmentSpan segment,
                         std::string& outKey, float& outValue, std::string& outFailReason) {
    if (segment.end - segment.start < 3) { outFailReason = "malformed field (too few tokens)"; return false; }
    if (tokens[segment.start].kind != TokenKind::Identifier) { outFailReason = "expected a field name"; return false; }
    if (tokens[segment.start + 1].kind != TokenKind::Equals) { outFailReason = "expected '=' after field name"; return false; }
    if (!TryReadSignedNumberSpanningRange(tokens, segment.start + 2, segment.end, outValue)) {
        outFailReason = "field value is not a single numeric literal";
        return false;
    }
    outKey = tokens[segment.start].text;
    return true;
}

bool ParseSegmentAsPositional(const std::vector<Token>& tokens, SegmentSpan segment,
                              float& outValue, std::string& outFailReason) {
    if (!TryReadSignedNumberSpanningRange(tokens, segment.start, segment.end, outValue)) {
        outFailReason = "value is not a single numeric literal";
        return false;
    }
    return true;
}

// Parses ONE candidate table body against the closed grammar (item 4) and the value-level caps
// (item 10). Returns false (and fills outFailReason) for anything the grammar does not accept --
// the caller turns that into a near-miss, never a partial rectangle (item 6).
bool ParseCandidateBody(const std::vector<Token>& tokens, std::size_t bodyStart, std::size_t bodyEnd,
                        Params::MapArea& outArea, std::string& outFailReason) {
    if (bodyStart >= bodyEnd) { outFailReason = "empty table body"; return false; }

    for (std::size_t index = bodyStart; index < bodyEnd; ++index)
        if (tokens[index].kind == TokenKind::LBrace) { outFailReason = "nested table not permitted"; return false; }

    const std::vector<SegmentSpan> segments = SplitCommaSegments(tokens, bodyStart, bodyEnd);
    if (segments.size() != 4) {
        outFailReason = "expected exactly four fields, found " + std::to_string(segments.size());
        return false;
    }
    for (const SegmentSpan& segment : segments)
        if (segment.start >= segment.end) { outFailReason = "empty field between commas"; return false; }

    struct SegmentResult { bool bKeyed; std::string key; float value; };
    std::vector<SegmentResult> segmentResults;
    bool bAnyKeyed = false, bAnyPositional = false;

    for (const SegmentSpan& segment : segments) {
        const bool bLooksKeyed = tokens[segment.start].kind == TokenKind::Identifier
            && (segment.start + 1) < segment.end && tokens[segment.start + 1].kind == TokenKind::Equals;
        if (bLooksKeyed) {
            std::string key; float value;
            if (!ParseSegmentAsKeyed(tokens, segment, key, value, outFailReason)) return false;
            segmentResults.push_back({ true, key, value });
            bAnyKeyed = true;
        } else {
            float value;
            if (!ParseSegmentAsPositional(tokens, segment, value, outFailReason)) return false;
            segmentResults.push_back({ false, std::string(), value });
            bAnyPositional = true;
        }
    }
    if (bAnyKeyed && bAnyPositional) { outFailReason = "mixed keyed and positional fields not permitted"; return false; }

    float x = 0.0f, y = 0.0f, width = 0.0f, height = 0.0f;
    if (bAnyKeyed) {
        bool hasX = false, hasY = false, hasWidth = false, hasHeight = false;
        for (const SegmentResult& segmentResult : segmentResults) {
            if (segmentResult.key == "x") {
                if (hasX) { outFailReason = "duplicate field 'x'"; return false; }
                x = segmentResult.value; hasX = true;
            } else if (segmentResult.key == "y") {
                if (hasY) { outFailReason = "duplicate field 'y'"; return false; }
                y = segmentResult.value; hasY = true;
            } else if (segmentResult.key == "width") {
                if (hasWidth) { outFailReason = "duplicate field 'width'"; return false; }
                width = segmentResult.value; hasWidth = true;
            } else if (segmentResult.key == "height") {
                if (hasHeight) { outFailReason = "duplicate field 'height'"; return false; }
                height = segmentResult.value; hasHeight = true;
            } else {
                outFailReason = "unrecognized field '" + segmentResult.key + "'";
                return false;
            }
        }
        if (!(hasX && hasY && hasWidth && hasHeight)) {
            outFailReason = "missing one or more of x/y/width/height";
            return false;
        }
    } else {
        x = segmentResults[0].value; y = segmentResults[1].value;
        width = segmentResults[2].value; height = segmentResults[3].value;
    }

    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(width) || !std::isfinite(height)) {
        outFailReason = "non-finite coordinate value";
        return false;
    }
    if (std::fabs(x) > kMaxScenarioAreaCoordinateMagnitude || std::fabs(y) > kMaxScenarioAreaCoordinateMagnitude
        || width <= 0.0f || width > kMaxScenarioAreaCoordinateMagnitude
        || height <= 0.0f || height > kMaxScenarioAreaCoordinateMagnitude) {
        outFailReason = "coordinate or extent value out of the accepted range";
        return false;
    }

    outArea.originX = x; outArea.originZ = y; outArea.width = width; outArea.length = height;
    return true;
}

} // namespace

ScenarioAreaExtractionResult ExtractAreaRectanglesFromScenarioScriptText(const std::string& sourceText) {
    ScenarioAreaExtractionResult result;
    const std::vector<Token> tokens = Tokenize(sourceText);
    std::unordered_map<std::string, std::size_t> identifierToAreaIndex;   // for last-write-wins (item 7)

    std::size_t index = 0;
    while (index < tokens.size()) {
        std::size_t identifierIndex = index;
        if (tokens[identifierIndex].kind == TokenKind::Identifier && tokens[identifierIndex].text == "local") {
            ++identifierIndex;
        }
        if (identifierIndex >= tokens.size() || tokens[identifierIndex].kind != TokenKind::Identifier) { ++index; continue; }
        const std::size_t equalsIndex = identifierIndex + 1;
        if (equalsIndex >= tokens.size() || tokens[equalsIndex].kind != TokenKind::Equals) { ++index; continue; }
        const std::size_t braceIndex = equalsIndex + 1;
        if (braceIndex >= tokens.size() || tokens[braceIndex].kind != TokenKind::LBrace) { ++index; continue; }

        // Candidate found -- locate the matching closing brace via depth counting (handles a
        // nested table correctly for span purposes even though item 4 will reject it as content).
        std::size_t depth = 1;
        std::size_t scanCursor = braceIndex + 1;
        while (scanCursor < tokens.size() && depth > 0) {
            if (tokens[scanCursor].kind == TokenKind::LBrace) ++depth;
            else if (tokens[scanCursor].kind == TokenKind::RBrace) --depth;
            ++scanCursor;
        }
        const std::string identifierName = tokens[identifierIndex].text;
        if (depth != 0) {
            result.nearMisses.push_back({ identifierName, "unterminated table (no matching '}')" });
            break;   // nothing sane left to scan past an unterminated table
        }
        const std::size_t closeBraceIndex = scanCursor - 1;

        Params::MapArea candidateArea;
        std::string failReason;
        if (ParseCandidateBody(tokens, braceIndex + 1, closeBraceIndex, candidateArea, failReason)) {
            candidateArea.name = identifierName;   // verbatim -- item 7
            const auto existingEntry = identifierToAreaIndex.find(identifierName);
            if (existingEntry != identifierToAreaIndex.end()) {
                result.areas[existingEntry->second] = candidateArea;   // last-write-wins -- item 7
                result.collisionIdentifiers.push_back(identifierName);
            } else if (result.areas.size() >= kMaxScenarioAreaExtractionRectangleCount) {
                result.bRectangleCountCapExceeded = true;
                break;   // Constitution §6 cap -- item 10
            } else {
                identifierToAreaIndex[identifierName] = result.areas.size();
                result.areas.push_back(candidateArea);
            }
        } else {
            result.nearMisses.push_back({ identifierName, failReason });   // item 6
        }

        index = scanCursor;   // resume immediately after the whole candidate table, never re-enter it
    }
    return result;
}

} // namespace Io
} // namespace SanmapGen
```

---

## 3. New test file: `src/io/ScenarioScript_AreaRectangleExtract_IO_Test.cpp`

```cpp
// ScenarioScript_AreaRectangleExtract_IO_Test.cpp -- pure-logic acceptance test for the closed
// literal-only grammar (STEP215, ARCH §15.11 items 2-7,10). No filesystem, no disk, matches the
// header's own "pure and total" contract.
#include "ScenarioScript_AreaRectangleExtract_IO.h"
#include <cstdio>
#include <string>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

static bool NearFloat(float actual, float expected) { return std::fabs(actual - expected) < 0.001f; }

static const Params::MapArea* FindAreaByName(const Io::ScenarioAreaExtractionResult& result, const char* name) {
    for (const Params::MapArea& area : result.areas) if (area.name == name) return &area;
    return nullptr;
}

// 1. The real, byte-verbatim line from map_scripts_backup/Pandemonium Isthmus_Scenarios_Script.lua
//    .officialbak:61 (named-key form, all-integer values, a trailing "--" comment after the '}').
static void TestNamedKeyFormRealFixtureLine() {
    const std::string source =
        "local AREA_356 = { x = 846, y = 846, width = 356, height = 356 }               "
        "-- the map's own baked default\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    Check(result.areas.size() == 1, "NamedKeyRealFixture: exactly one area");
    Check(result.nearMisses.empty(), "NamedKeyRealFixture: no near-misses");
    const Params::MapArea* area = FindAreaByName(result, "AREA_356");
    Check(area != nullptr, "NamedKeyRealFixture: AREA_356 present");
    if (area != nullptr) {
        Check(NearFloat(area->originX, 846.0f), "NamedKeyRealFixture: originX");
        Check(NearFloat(area->originZ, 846.0f), "NamedKeyRealFixture: originZ (y->originZ mapping)");
        Check(NearFloat(area->width, 356.0f), "NamedKeyRealFixture: width");
        Check(NearFloat(area->length, 356.0f), "NamedKeyRealFixture: length (height->length mapping)");
    }
}

// 2. The real line :62 -- fractional (non-integer) float values, out-of-x/y/width/height order in
//    the source (x, y, width, height IS the declared order here, so this also exercises the "keys
//    in any order" clause via a THIRD synthetic case below rather than this one).
static void TestNamedKeyFormRealFractionalValues() {
    const std::string source =
        "local AREA_169 = { x = 668.4444444444445, y = 824, width = 711.1111111111111, height = 400 } "
        "-- 400 height, 16:9\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    const Params::MapArea* area = FindAreaByName(result, "AREA_169");
    Check(area != nullptr, "NamedKeyFractional: AREA_169 present");
    if (area != nullptr) {
        Check(NearFloat(area->originX, 668.4444444444445f), "NamedKeyFractional: originX");
        Check(NearFloat(area->originZ, 824.0f), "NamedKeyFractional: originZ");
        Check(NearFloat(area->width, 711.1111111111111f), "NamedKeyFractional: width");
        Check(NearFloat(area->length, 400.0f), "NamedKeyFractional: length");
    }
}

// 3. Keys in an order OTHER than x,y,width,height (item 4: "either all four keyed pairs ... in any
//    order").
static void TestNamedKeyFormOutOfOrderKeys() {
    const std::string source = "local AREA_ORDER = { height = 200, width = 100, y = 50, x = 25 }\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    const Params::MapArea* area = FindAreaByName(result, "AREA_ORDER");
    Check(area != nullptr, "OutOfOrderKeys: area present");
    if (area != nullptr) {
        Check(NearFloat(area->originX, 25.0f), "OutOfOrderKeys: originX");
        Check(NearFloat(area->originZ, 50.0f), "OutOfOrderKeys: originZ");
        Check(NearFloat(area->width, 100.0f), "OutOfOrderKeys: width");
        Check(NearFloat(area->length, 200.0f), "OutOfOrderKeys: length");
    }
}

// 4. Positional form (item 4's second accepted shape). NOT present in any real file scanned for
//    this ticket -- a synthetic fixture exercising the closed grammar's own second accepted branch,
//    values chosen to match TestNamedKeyFormRealFixtureLine's AREA_356 for an easy cross-check.
static void TestPositionalFormSynthetic() {
    const std::string source = "local AREA_POS = { 846, 846, 356, 356 }\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    const Params::MapArea* area = FindAreaByName(result, "AREA_POS");
    Check(area != nullptr, "PositionalSynthetic: area present");
    if (area != nullptr) {
        Check(NearFloat(area->originX, 846.0f), "PositionalSynthetic: originX from position 1 (x)");
        Check(NearFloat(area->originZ, 846.0f), "PositionalSynthetic: originZ from position 2 (y)");
        Check(NearFloat(area->width, 356.0f), "PositionalSynthetic: width from position 3");
        Check(NearFloat(area->length, 356.0f), "PositionalSynthetic: length from position 4");
    }
}

// 5. A negative-signed value (item 4: "optional sign").
static void TestNegativeSignedValue() {
    const std::string source = "local AREA_NEG = { x = -50, y = 10, width = 100, height = 100 }\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    const Params::MapArea* area = FindAreaByName(result, "AREA_NEG");
    Check(area != nullptr, "NegativeSigned: area present");
    if (area != nullptr) Check(NearFloat(area->originX, -50.0f), "NegativeSigned: originX is -50");
}

// 6. Line comments (`--`) hide a fake area entirely (item 5).
static void TestLineCommentIsSkipped() {
    const std::string source =
        "-- local AREA_FAKE = { x=1,y=1,width=1,height=1 }\n"
        "local AREA_REAL = { x = 5, y = 5, width = 5, height = 5 }\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    Check(FindAreaByName(result, "AREA_FAKE") == nullptr, "LineComment: commented area not extracted");
    Check(FindAreaByName(result, "AREA_REAL") != nullptr, "LineComment: real area still extracted");
}

// 7. A `--[[ ]]` long comment block hides a fake area entirely (item 5).
static void TestLongCommentIsSkipped() {
    const std::string source =
        "--[[ local AREA_FAKE2 = { x=1, y=1, width=1, height=1 } ]]\n"
        "local AREA_REAL2 = { x = 6, y = 6, width = 6, height = 6 }\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    Check(FindAreaByName(result, "AREA_FAKE2") == nullptr, "LongComment: commented area not extracted");
    Check(FindAreaByName(result, "AREA_REAL2") != nullptr, "LongComment: real area still extracted");
}

// 8. A string literal containing `{`/`}` and an `=` sign must not confuse the parser (item 5).
static void TestStringLiteralWithBracesDoesNotConfuseParser() {
    const std::string source =
        "local LABEL = \"some {weird} text with = signs and a fake area = { x=1,y=1,width=1,height=1 }\"\n"
        "local AREA_REAL3 = { x = 7, y = 7, width = 7, height = 7 }\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    Check(FindAreaByName(result, "AREA_REAL3") != nullptr, "StringBraces: real area after string extracted");
    // LABEL itself is a near-miss (its RHS is a string, not `{`), never an area -- confirms the
    // string's own content was never mistaken for the start of a table.
    Check(FindAreaByName(result, "LABEL") == nullptr, "StringBraces: LABEL itself never became an area");
}

// 9. In-file identifier collision resolves last-write-wins, and is logged (item 7).
static void TestInFileCollisionLastWriteWins() {
    const std::string source =
        "local AREA_X = { x = 1, y = 1, width = 1, height = 1 }\n"
        "local AREA_X = { x = 2, y = 2, width = 2, height = 2 }\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    Check(result.areas.size() == 1, "InFileCollision: exactly one AREA_X survives");
    Check(result.collisionIdentifiers.size() == 1 && result.collisionIdentifiers[0] == "AREA_X",
          "InFileCollision: collision logged by identifier");
    const Params::MapArea* area = FindAreaByName(result, "AREA_X");
    if (area != nullptr) Check(NearFloat(area->originX, 2.0f), "InFileCollision: second assignment wins");
}

// 10. A nested table is rejected outright (item 4).
static void TestNestedTableRejected() {
    const std::string source = "local AREA_BAD = { x = 1, y = 1, width = { 1 }, height = 1 }\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    Check(result.areas.empty(), "NestedTable: no area extracted");
    Check(result.nearMisses.size() == 1 && result.nearMisses[0].identifier == "AREA_BAD",
          "NestedTable: near-miss logged for AREA_BAD");
}

// 11. Mixed keyed/positional in one table is rejected (item 4).
static void TestMixedKeyedPositionalRejected() {
    const std::string source = "local AREA_BAD2 = { x = 1, 2, width = 3, height = 4 }\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    Check(result.areas.empty(), "MixedForm: no area extracted");
    Check(!result.nearMisses.empty() && result.nearMisses[0].identifier == "AREA_BAD2", "MixedForm: near-miss logged");
}

// 12. A missing key (only three fields) is rejected (item 4).
static void TestMissingFieldRejected() {
    const std::string source = "local AREA_BAD3 = { x = 1, y = 1, width = 1 }\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    Check(result.areas.empty(), "MissingField: no area extracted");
}

// 13. A duplicate key within one table is rejected (item 4).
static void TestDuplicateFieldRejected() {
    const std::string source = "local AREA_BAD4 = { x = 1, x = 2, width = 1, height = 1 }\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    Check(result.areas.empty(), "DuplicateField: no area extracted");
}

// 14. An unrecognized fifth-key-shaped field (right count, wrong key) is rejected (item 4).
static void TestUnrecognizedFieldRejected() {
    const std::string source = "local AREA_BAD5 = { x = 1, y = 1, w = 1, height = 1 }\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    Check(result.areas.empty(), "UnrecognizedField: no area extracted");
}

// 15. An absurd coordinate magnitude and a non-positive extent are both rejected (item 10).
static void TestAbsurdCoordinateAndNonPositiveExtentRejected() {
    const std::string sourceAbsurd = "local AREA_BAD6 = { x = 99999999, y = 1, width = 1, height = 1 }\n";
    Check(Io::ExtractAreaRectanglesFromScenarioScriptText(sourceAbsurd).areas.empty(),
          "AbsurdCoordinate: no area extracted");
    const std::string sourceZeroWidth = "local AREA_BAD7 = { x = 1, y = 1, width = 0, height = 1 }\n";
    Check(Io::ExtractAreaRectanglesFromScenarioScriptText(sourceZeroWidth).areas.empty(),
          "ZeroWidth: no area extracted");
}

// 16. The rectangle-count cap halts extraction (item 10) -- build 520 distinct valid area
//     assignments (> kMaxScenarioAreaExtractionRectangleCount == 512) and confirm the cap bites.
static void TestRectangleCountCapEnforced() {
    std::string source;
    for (int index = 0; index < 520; ++index) {
        source += "local AREA_GEN_" + std::to_string(index) + " = { x = 1, y = 1, width = 1, height = 1 }\n";
    }
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    Check(result.areas.size() == Io::kMaxScenarioAreaExtractionRectangleCount, "CountCap: extraction stops at the cap");
    Check(result.bRectangleCountCapExceeded, "CountCap: flag set");
}

// 17. A real multi-area block from the reference file (:61-64,68-69), verbatim, including two
//     genuinely-present non-rectangle tables (IDENTITY_ROTATION has a 'w' key + only 4 fields but
//     wrong keys; IDENTITY_SCALE has only 3 fields) that MUST be rejected as near-misses, never
//     imported -- proving item 2's "no other kind of value may ever be extracted" holds structurally,
//     not just by policy, even against real adjacent-in-file content shaped almost like a rectangle.
static void TestRealMultiAreaBlockFromReferenceFile() {
    const std::string source =
        "-- format: {x, y = world z, width, height}.\n"
        "local AREA_356 = { x = 846, y = 846, width = 356, height = 356 }               -- the map's own baked default\n"
        "local AREA_169 = { x = 668.4444444444445, y = 824, width = 711.1111111111111, height = 400 } -- 400 height, 16:9\n"
        "local AREA_1024 = { x = 537, y = 472, width = 974, height = 1104 }             -- 6-player, centered on map center\n"
        "local AREA_FULL = { x = 0, y = 0, width = 2048, height = 2048 }\n"
        "local IDENTITY_ROTATION = { w = 1.0, x = 0.0, y = 0.0, z = 0.0 }\n"
        "local IDENTITY_SCALE = { x = 1.0, y = 1.0, z = 1.0 }\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    Check(result.areas.size() == 4, "RealBlock: exactly the four real rectangles extracted");
    Check(FindAreaByName(result, "AREA_356") != nullptr, "RealBlock: AREA_356 present");
    Check(FindAreaByName(result, "AREA_169") != nullptr, "RealBlock: AREA_169 present");
    Check(FindAreaByName(result, "AREA_1024") != nullptr, "RealBlock: AREA_1024 present");
    Check(FindAreaByName(result, "AREA_FULL") != nullptr, "RealBlock: AREA_FULL present");
    Check(FindAreaByName(result, "IDENTITY_ROTATION") == nullptr, "RealBlock: IDENTITY_ROTATION never imported");
    Check(FindAreaByName(result, "IDENTITY_SCALE") == nullptr, "RealBlock: IDENTITY_SCALE never imported");
    Check(result.nearMisses.size() == 2, "RealBlock: exactly two near-misses logged");
}

// 18. A spawn-shaped 3-field x/y/z table (the real ARMY_01 shape, §15.11 item 2's own named concern)
//     is structurally excluded -- wrong field count AND wrong key set, never imported.
static void TestSpawnShapedTableNeverImported() {
    const std::string source = "ARMY_01 = { x = 855, y = 79.12979888916016, z = 920 }\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    Check(result.areas.empty(), "SpawnShaped: never imported as an area (item 2)");
}

int main() {
    TestNamedKeyFormRealFixtureLine();
    TestNamedKeyFormRealFractionalValues();
    TestNamedKeyFormOutOfOrderKeys();
    TestPositionalFormSynthetic();
    TestNegativeSignedValue();
    TestLineCommentIsSkipped();
    TestLongCommentIsSkipped();
    TestStringLiteralWithBracesDoesNotConfuseParser();
    TestInFileCollisionLastWriteWins();
    TestNestedTableRejected();
    TestMixedKeyedPositionalRejected();
    TestMissingFieldRejected();
    TestDuplicateFieldRejected();
    TestUnrecognizedFieldRejected();
    TestAbsurdCoordinateAndNonPositiveExtentRejected();
    TestRectangleCountCapEnforced();
    TestRealMultiAreaBlockFromReferenceFile();
    TestSpawnShapedTableNeverImported();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
```

---

## 4. New file: `src/io/ScenarioScript_AreaImport_IO.h`

```cpp
// ScenarioScript_AreaImport_IO.h -- the ONE disk-touching, human-triggered entry point for
// ARCH_15_11's foreign-scenario-.lua area-rectangle carve-out
// (`ARCH_15_11_ForeignScenarioAreaImport.md` §15.11). Layer: IO. Reads a FOREIGN (non-SanGen
// -authored) scenario .lua file, enforces the refusal guard (item 1) and the byte-size cap (item 10)
// BEFORE calling the pure extractor (ScenarioScript_AreaRectangleExtract_IO.h), then reconciles the
// returned rectangles into recipe.areas additively (item 9). Human-triggered, one-shot, no live
// binding, no provenance field, no re-sync action (item 8) -- never called from map open, export,
// generate, or any dirty-hash recompute.
//
// This is NOT "the reader half" of ScenarioScript_DataLua_IO (item 11) -- it reads a filename set
// DISJOINT from what that file (and ScenarioScript_Export_IO) ever write, and that disjointness is
// mechanically checked below, not merely conventional.
#pragma once
#include <cstddef>
#include <string>
#include <vector>
#include "ScenarioScript_AreaRectangleExtract_IO.h"

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Io {

// Constitution §6 byte cap (item 10) -- checked via a filesystem stat BEFORE the file is ever read
// into memory, so an oversized file is never loaded to find out it's oversized. Chosen generously
// above every real scenario script observed (map_scripts_backup's officialbak files are all well
// under 32 KB) while still bounding pathological input.
inline constexpr std::size_t kMaxScenarioAreaImportSourceBytes = 4u * 1024u * 1024u;   // 4 MiB

// Mirrors ScenarioExportResult's own shape (bool flags + string lists + Log()) -- a DISTINCT type,
// never merged with it: this is an import, not an export.
struct ScenarioAreaImportResult {
    bool bRefusedGeneratedFile      = false;   // filename or banner-line match -- item 1
    bool bRefusedUnreadableFile     = false;   // file missing, unreadable, or a stat failure
    bool bRefusedOversizedFile      = false;   // byte cap exceeded -- item 10; never scanned
    bool bRectangleCountCapExceeded = false;   // carried through from the pure extractor -- item 10

    std::vector<std::string>                    writtenNames;           // newly added to recipe.areas
    std::vector<std::string>                    skippedCollisionNames;  // name already existed --
                                                                          // skipped and reported, item 9
    std::vector<std::string>                    collisionIdentifiers;   // in-FILE reassignments,
                                                                          // carried through from the
                                                                          // extractor -- item 7
    std::vector<ScenarioAreaExtractionNearMiss>  nearMisses;             // carried through verbatim,
                                                                          // item 6

    std::string debugLog;
    void Log(const std::string& line) { debugLog += line; debugLog += '\n'; }
};

// sourceFilePath: any file the human picked (e.g. via FileDialog::OpenFilePath filtered to "*.lua"
// -- UI wiring is explicit follow-up, not built by this ticket). recipe.areas is mutated additively,
// in place, on success -- left completely untouched on every refusal path. This function itself
// never opens a platform dialog and never has a default/implicit invocation path (item 8).
ScenarioAreaImportResult ImportAreaRectanglesFromScenarioScriptFile(const std::string& sourceFilePath,
                                                                    Params::MapRecipe& recipe);

} // namespace Io
} // namespace SanmapGen
```

## 5. New file: `src/io/ScenarioScript_AreaImport_IO.cpp`

```cpp
// ScenarioScript_AreaImport_IO.cpp -- see the header for the full contract. Order of guards,
// cheapest-first: filename (zero I/O) -> file existence/size stat (no content read) -> byte cap ->
// read -> banner-line -> pure extraction -> additive reconciliation.
#include "ScenarioScript_AreaImport_IO.h"
#include "FilesystemPrimitives_IO.h"
#include "ScenarioScript_DataLua_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include <algorithm>
#include <filesystem>

namespace SanmapGen {
namespace Io {
namespace {

// Independent copy of ScenarioScript_Export_IO.cpp's own private StartsWithBanner helper --
// deliberately NOT promoted into ScenarioScript_DataLua_IO.h by this ticket (see the work-order's
// "Interpretation calls made" section for the explicit scope-cut rationale); the IO Architecture
// Expert's own ruling left that promotion optional.
bool StartsWithBanner(const std::string& text) {
    const std::string banner(kScenarioGeneratedFileBannerLine);
    return text.compare(0, banner.size(), banner) == 0;
}

// ARCH §15.11 item 1's second, independent half of the refusal guard: refuse the two SanGen-owned
// filenames by name, regardless of content. Suffix match on the filename only (path-separator
// -agnostic) -- "<MapName>" is not known to this function and is not needed for a suffix check.
bool HasSanGenOwnedScenarioFilenameSuffix(const std::string& filePath) {
    const std::size_t lastSlashIndex = filePath.find_last_of("/\\");
    const std::string fileName = (lastSlashIndex == std::string::npos) ? filePath : filePath.substr(lastSlashIndex + 1);
    static const char* const kOwnedSuffixes[] = { "_Scenarios_Runtime.lua", "_Scenarios_Data.lua" };
    for (const char* suffix : kOwnedSuffixes) {
        const std::string suffixText(suffix);
        if (fileName.size() >= suffixText.size()
            && fileName.compare(fileName.size() - suffixText.size(), suffixText.size(), suffixText) == 0) {
            return true;
        }
    }
    return false;
}

} // namespace

ScenarioAreaImportResult ImportAreaRectanglesFromScenarioScriptFile(const std::string& sourceFilePath,
                                                                    Params::MapRecipe& recipe) {
    ScenarioAreaImportResult result;

    // Item 1, half A -- filename refusal, checked before the file is even opened.
    if (HasSanGenOwnedScenarioFilenameSuffix(sourceFilePath)) {
        result.bRefusedGeneratedFile = true;
        result.Log("refused " + sourceFilePath +
                   " -- a SanGen-owned scenario filename is never read back (ARCH §15.11 item 1)");
        return result;
    }

    // Item 10 -- byte cap enforced via a filesystem stat, BEFORE any content is read into memory.
    std::error_code statError;
    const std::filesystem::path sourcePath(sourceFilePath);
    const std::uintmax_t fileSizeBytes = std::filesystem::file_size(sourcePath, statError);
    if (statError) {
        result.bRefusedUnreadableFile = true;
        result.Log("refused " + sourceFilePath + " -- file missing or unreadable (" + statError.message() + ")");
        return result;
    }
    if (fileSizeBytes > kMaxScenarioAreaImportSourceBytes) {
        result.bRefusedOversizedFile = true;
        result.Log("refused " + sourceFilePath + " -- source text exceeds the " +
                   std::to_string(kMaxScenarioAreaImportSourceBytes) + "-byte import cap (Constitution §6)");
        return result;
    }

    std::string sourceText;
    if (!ReadTextFileBytes(sourceFilePath, sourceText)) {
        result.bRefusedUnreadableFile = true;
        result.Log("refused " + sourceFilePath + " -- file could not be opened for read");
        return result;
    }

    // Item 1, half B -- banner-line refusal, checked BEFORE ever calling the pure extractor. This
    // is the load-bearing guard: it makes "SanGen never reads back what SanGen wrote" a checked
    // property rather than a convention (§15.11's own wording).
    if (StartsWithBanner(sourceText)) {
        result.bRefusedGeneratedFile = true;
        result.Log("refused " + sourceFilePath +
                   " -- first line matches Io::kScenarioGeneratedFileBannerLine (ARCH §15.11 item 1): "
                   "SanGen never reads back a file it wrote");
        return result;
    }

    // Items 2-7, 10 -- the pure, filename-agnostic extraction.
    const ScenarioAreaExtractionResult extraction = ExtractAreaRectanglesFromScenarioScriptText(sourceText);
    result.bRectangleCountCapExceeded = extraction.bRectangleCountCapExceeded;
    result.collisionIdentifiers       = extraction.collisionIdentifiers;
    result.nearMisses                 = extraction.nearMisses;
    for (const std::string& collisionIdentifier : extraction.collisionIdentifiers) {
        result.Log("in-file collision on '" + collisionIdentifier + "' -- last assignment wins (ARCH §15.11 item 7)");
    }
    for (const ScenarioAreaExtractionNearMiss& nearMiss : extraction.nearMisses) {
        result.Log("near-miss '" + nearMiss.identifier + "': " + nearMiss.reason);
    }
    if (extraction.bRectangleCountCapExceeded) {
        result.Log("rectangle-count cap reached -- extraction stopped early (Constitution §6)");
    }

    // Item 9 -- additive-never-destructive reconciliation into recipe.areas: a name collision is
    // skipped and reported, never a silent overwrite. Every non-colliding rectangle in the same file
    // is still imported (not all-or-nothing).
    for (const Params::MapArea& candidateArea : extraction.areas) {
        const bool bCollides = std::any_of(recipe.areas.begin(), recipe.areas.end(),
            [&candidateArea](const Params::MapArea& existingArea) { return existingArea.name == candidateArea.name; });
        if (bCollides) {
            result.skippedCollisionNames.push_back(candidateArea.name);
            result.Log("skipped '" + candidateArea.name +
                      "' -- an area with that name already exists in recipe.areas (ARCH §15.11 item 9)");
            continue;
        }
        recipe.areas.push_back(candidateArea);
        result.writtenNames.push_back(candidateArea.name);
    }

    return result;
}

} // namespace Io
} // namespace SanmapGen
```

---

## 6. New test file: `src/io/ScenarioScript_AreaImport_IO_Test.cpp`

```cpp
// ScenarioScript_AreaImport_IO_Test.cpp -- acceptance test for STEP215's disk-touching entry point.
// Scratch-directory pattern per ScenarioScript_Export_IO_Test.cpp's own precedent.
#include "ScenarioScript_AreaImport_IO.h"
#include "FilesystemPrimitives_IO.h"
#include "ScenarioScript_DataLua_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

static std::string ScratchFolderPath(const char* name) {
    std::error_code pathError;
    const std::filesystem::path folder =
        std::filesystem::temp_directory_path(pathError) / (std::string("SanGenAreaImportTest_") + name);
    std::filesystem::remove_all(folder, pathError);
    std::filesystem::create_directories(folder, pathError);
    return folder.string();
}

static std::string WriteScratchFile(const std::string& folder, const char* fileName, const std::string& contents) {
    const std::string filePath = Io::JoinExportPath(folder, fileName);
    std::ofstream outputStream(filePath, std::ios::binary | std::ios::trunc);
    outputStream << contents;
    outputStream.close();
    return filePath;
}

static const Params::MapArea* FindAreaByName(const Params::MapRecipe& recipe, const std::string& name) {
    for (const Params::MapArea& area : recipe.areas) if (area.name == name) return &area;
    return nullptr;
}

static const char* kRealNamedKeyBlock =
    "local AREA_356 = { x = 846, y = 846, width = 356, height = 356 }\n"
    "local AREA_169 = { x = 668.4444444444445, y = 824, width = 711.1111111111111, height = 400 }\n";

// 1. Filename refusal -- "_Scenarios_Runtime.lua" suffix, regardless of content.
static void TestRefusesSanGenOwnedFilenameRuntime() {
    const std::string folder = ScratchFolderPath("RefuseRuntimeFilename");
    const std::string filePath = WriteScratchFile(folder, "SomeMap_Scenarios_Runtime.lua", kRealNamedKeyBlock);
    Params::MapRecipe recipe;
    const Io::ScenarioAreaImportResult result = Io::ImportAreaRectanglesFromScenarioScriptFile(filePath, recipe);
    Check(result.bRefusedGeneratedFile, "RefuseRuntimeFilename: refused");
    Check(recipe.areas.empty(), "RefuseRuntimeFilename: recipe.areas untouched");
}

// 2. Filename refusal -- "_Scenarios_Data.lua" suffix, regardless of content.
static void TestRefusesSanGenOwnedFilenameData() {
    const std::string folder = ScratchFolderPath("RefuseDataFilename");
    const std::string filePath = WriteScratchFile(folder, "SomeMap_Scenarios_Data.lua", kRealNamedKeyBlock);
    Params::MapRecipe recipe;
    const Io::ScenarioAreaImportResult result = Io::ImportAreaRectanglesFromScenarioScriptFile(filePath, recipe);
    Check(result.bRefusedGeneratedFile, "RefuseDataFilename: refused");
    Check(recipe.areas.empty(), "RefuseDataFilename: recipe.areas untouched");
}

// 3. THE REQUIRED ACCEPTANCE TEST (ARCH §15.11 item 1): export a real _Scenarios_Data.lua via the
//    REAL renderer (BuildScenarioDataLuaText -- never a hand-typed banner string), feed it back in
//    under its OWN real filename, and confirm refusal.
static void TestAcceptanceExportedDataLuaIsRefusedUnderRealFilename() {
    Params::MapRecipe exportRecipe;
    exportRecipe.mapName = "AcceptanceMap";
    exportRecipe.geometry.mapSize = 4;
    const std::string realExportedText = Io::BuildScenarioDataLuaText(exportRecipe);

    const std::string folder = ScratchFolderPath("AcceptanceRealFilename");
    const std::string filePath = WriteScratchFile(folder, "AcceptanceMap_Scenarios_Data.lua", realExportedText);

    Params::MapRecipe importRecipe;
    const Io::ScenarioAreaImportResult result = Io::ImportAreaRectanglesFromScenarioScriptFile(filePath, importRecipe);
    Check(result.bRefusedGeneratedFile, "Acceptance/RealFilename: refused");
    Check(importRecipe.areas.empty(), "Acceptance/RealFilename: recipe.areas untouched");
}

// 4. Isolates the BANNER-LINE guard specifically (independent of the filename guard): the same real
//    exported text, written under a filename that does NOT match either owned suffix. Refusal here
//    can only be the banner-line check firing -- proving the guard is a checked property of the
//    FILE'S CONTENT, not merely a filename convention.
static void TestAcceptanceExportedDataLuaIsRefusedByBannerAloneUnderADifferentFilename() {
    Params::MapRecipe exportRecipe;
    exportRecipe.mapName = "AcceptanceMap";
    exportRecipe.geometry.mapSize = 4;
    const std::string realExportedText = Io::BuildScenarioDataLuaText(exportRecipe);

    const std::string folder = ScratchFolderPath("AcceptanceBannerOnly");
    const std::string filePath = WriteScratchFile(folder, "renamed_copy_of_export.lua", realExportedText);

    Params::MapRecipe importRecipe;
    const Io::ScenarioAreaImportResult result = Io::ImportAreaRectanglesFromScenarioScriptFile(filePath, importRecipe);
    Check(result.bRefusedGeneratedFile, "Acceptance/BannerOnly: refused by banner-line alone");
    Check(importRecipe.areas.empty(), "Acceptance/BannerOnly: recipe.areas untouched");
}

// 5. Unreadable/missing file.
static void TestRefusesUnreadableFile() {
    const std::string folder = ScratchFolderPath("RefuseUnreadable");
    const std::string filePath = Io::JoinExportPath(folder, "does_not_exist.lua");
    Params::MapRecipe recipe;
    const Io::ScenarioAreaImportResult result = Io::ImportAreaRectanglesFromScenarioScriptFile(filePath, recipe);
    Check(result.bRefusedUnreadableFile, "RefuseUnreadable: refused");
    Check(recipe.areas.empty(), "RefuseUnreadable: recipe.areas untouched");
}

// 6. Oversized file (item 10) -- refused before any rectangle could possibly be extracted, and the
//    stat-then-refuse path never needed to load the oversized content into a std::string at all.
static void TestRefusesOversizedFile() {
    const std::string folder = ScratchFolderPath("RefuseOversized");
    std::string oversizedText;
    oversizedText.reserve(Io::kMaxScenarioAreaImportSourceBytes + 1024);
    while (oversizedText.size() <= Io::kMaxScenarioAreaImportSourceBytes) oversizedText += "-- padding line\n";
    const std::string filePath = WriteScratchFile(folder, "oversized.lua", oversizedText);
    Params::MapRecipe recipe;
    const Io::ScenarioAreaImportResult result = Io::ImportAreaRectanglesFromScenarioScriptFile(filePath, recipe);
    Check(result.bRefusedOversizedFile, "RefuseOversized: refused");
    Check(recipe.areas.empty(), "RefuseOversized: recipe.areas untouched");
}

// 7. Successful import adds new areas additively into an empty recipe.
static void TestSuccessfulImportAddsNewAreas() {
    const std::string folder = ScratchFolderPath("SuccessfulImport");
    const std::string filePath = WriteScratchFile(folder, "ForeignMap_Scenarios_Script.lua", kRealNamedKeyBlock);
    Params::MapRecipe recipe;
    const Io::ScenarioAreaImportResult result = Io::ImportAreaRectanglesFromScenarioScriptFile(filePath, recipe);
    Check(!result.bRefusedGeneratedFile && !result.bRefusedUnreadableFile && !result.bRefusedOversizedFile,
          "SuccessfulImport: no refusal");
    Check(recipe.areas.size() == 2, "SuccessfulImport: both areas landed in recipe.areas");
    Check(result.writtenNames.size() == 2, "SuccessfulImport: both names reported written");
    const Params::MapArea* area356 = FindAreaByName(recipe, "AREA_356");
    Check(area356 != nullptr && area356->width == 356.0f, "SuccessfulImport: AREA_356 field values correct");
}

// 8. Name collision against an EXISTING recipe.areas entry is skipped and reported -- never a
//    silent overwrite -- while a non-colliding area from the same file is still imported (item 9).
static void TestNameCollisionSkippedAndReportedAdditively() {
    const std::string folder = ScratchFolderPath("NameCollision");
    const std::string filePath = WriteScratchFile(folder, "ForeignMap_Scenarios_Script.lua", kRealNamedKeyBlock);

    Params::MapRecipe recipe;
    Params::MapArea preExistingArea;
    preExistingArea.name = "AREA_356";
    preExistingArea.originX = 1.0f; preExistingArea.originZ = 1.0f;
    preExistingArea.width = 1.0f; preExistingArea.length = 1.0f;
    recipe.areas.push_back(preExistingArea);

    const Io::ScenarioAreaImportResult result = Io::ImportAreaRectanglesFromScenarioScriptFile(filePath, recipe);
    Check(result.skippedCollisionNames.size() == 1 && result.skippedCollisionNames[0] == "AREA_356",
          "NameCollision: AREA_356 reported skipped");
    Check(result.writtenNames.size() == 1 && result.writtenNames[0] == "AREA_169",
          "NameCollision: AREA_169 still imported (additive, not all-or-nothing)");
    Check(recipe.areas.size() == 2, "NameCollision: recipe now has the pre-existing area plus the new one");
    const Params::MapArea* stillPreExisting = FindAreaByName(recipe, "AREA_356");
    Check(stillPreExisting != nullptr && stillPreExisting->width == 1.0f,
          "NameCollision: pre-existing AREA_356 was never overwritten");
}

// 9. In-file collisions and grammar near-misses are carried through the wrapper's result verbatim.
static void TestNearMissesAndInFileCollisionsCarriedThrough() {
    const std::string source =
        "local AREA_DUP = { x = 1, y = 1, width = 1, height = 1 }\n"
        "local AREA_DUP = { x = 2, y = 2, width = 2, height = 2 }\n"
        "local AREA_BAD = { x = 1, y = 1, width = 1 }\n";   // missing 'height' -- a near-miss
    const std::string folder = ScratchFolderPath("PassThrough");
    const std::string filePath = WriteScratchFile(folder, "ForeignMap_Scenarios_Script.lua", source);
    Params::MapRecipe recipe;
    const Io::ScenarioAreaImportResult result = Io::ImportAreaRectanglesFromScenarioScriptFile(filePath, recipe);
    Check(result.collisionIdentifiers.size() == 1 && result.collisionIdentifiers[0] == "AREA_DUP",
          "PassThrough: in-file collision carried through");
    Check(result.nearMisses.size() == 1 && result.nearMisses[0].identifier == "AREA_BAD",
          "PassThrough: near-miss carried through");
    Check(recipe.areas.size() == 1, "PassThrough: only the resolved AREA_DUP landed");
}

int main() {
    TestRefusesSanGenOwnedFilenameRuntime();
    TestRefusesSanGenOwnedFilenameData();
    TestAcceptanceExportedDataLuaIsRefusedUnderRealFilename();
    TestAcceptanceExportedDataLuaIsRefusedByBannerAloneUnderADifferentFilename();
    TestRefusesUnreadableFile();
    TestRefusesOversizedFile();
    TestSuccessfulImportAddsNewAreas();
    TestNameCollisionSkippedAndReportedAdditively();
    TestNearMissesAndInFileCollisionsCarriedThrough();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
```

---

## 7. Modified: `CMakeLists.txt`

`src/io/*.cpp`/`*.h` are already covered by the existing `file(GLOB_RECURSE ... CONFIGURE_DEPENDS
"src/io/*.cpp" "src/io/*.h")` (`CMakeLists.txt:185`) that builds the `SanGenV2` library — the four new
production files (`ScenarioScript_AreaRectangleExtract_IO.h`/`.cpp`,
`ScenarioScript_AreaImport_IO.h`/`.cpp`) need **no CMakeLists.txt edit** to be compiled into the
library.

Only the two new **test executables** need explicit registration. Insert directly after the existing
`add_sangen_test(ScenarioScript_Export_IO_Test src/io/ScenarioScript_Export_IO_Test.cpp)` line
(currently `CMakeLists.txt:1025`), following that same block's own comment style:

```cmake

# STEP215_ScenarioScriptAreaRectangleExtract_IO: the pure, disk-free closed literal-only grammar for
# extracting Params::MapArea rectangles from a FOREIGN scenario .lua's text
# (`ARCH_15_11_ForeignScenarioAreaImport.md` §15.11 items 2-7,10). No filesystem, no Lua execution of
# any kind -- a hand-rolled tokenizer/grammar, not JSON, not LuaTableEvaluate_SYS.
add_sangen_test(ScenarioScript_AreaRectangleExtract_IO_Test src/io/ScenarioScript_AreaRectangleExtract_IO_Test.cpp)

# STEP215_ScenarioScriptAreaImport_IO: the disk-touching, human-triggered ONE entry point --
# filename + banner-line refusal guard (item 1, including the required acceptance test: a REAL
# BuildScenarioDataLuaText export fed back in must be refused), byte-size cap via a filesystem stat
# before any read (item 10), then additive-never-destructive reconciliation into recipe.areas
# (item 9). Links SanGenV2 for ScenarioScript_DataLua_IO's real BuildScenarioDataLuaText -- no
# hand-typed banner string anywhere in this test, same discipline as ScenarioScript_Export_IO_Test.
add_sangen_test(ScenarioScript_AreaImport_IO_Test src/io/ScenarioScript_AreaImport_IO_Test.cpp)
```

---

## ARCH rules invoked
Every one of `ARCH_15_11_ForeignScenarioAreaImport.md`'s 11 numbered items binds on this ticket:

1. **Item 1** — the mechanically-enforced refusal guard (filename suffix + `kScenarioGeneratedFileBannerLine`, referenced never re-typed), living ONLY in `ScenarioScript_AreaImport_IO.cpp` §5, with the required acceptance test at §6 tests 3/4.
2. **Item 2** — the extractor's ONLY output kind is `std::vector<Params::MapArea>` (§1); no scenario record/pattern/spawns/alloy/army field is ever readable through this grammar, proven structurally by §3 test 18 (a real `ARMY_01` spawn-shaped table is rejected on field count/key set, not merely by policy).
3. **Item 3** — no Lua execution anywhere; `ScenarioScript_AreaRectangleExtract_IO.cpp` (§2) is a hand-rolled, non-executing tokenizer/grammar, never `LuaTableEvaluate_SYS` or a variant of it.
4. **Item 4** — the closed grammar in `ParseCandidateBody` (§2): optional `local`, identifier, `=`, `{`, then all-four-keyed-any-order OR exactly-four-positional-in-x/y/width/height-order, `}`; nesting, mixed forms, arithmetic, and non-numeric values are all rejected (§3 tests 3, 4, 10, 11).
5. **Item 5** — comment/string skipping in `Tokenize`/`SkipWhitespaceAndComments`/`SkipQuotedString` (§2), verified against a real inline comment, a long comment, and a string literal containing braces (§3 tests 6-8).
6. **Item 6** — a failed candidate produces exactly one `ScenarioAreaExtractionNearMiss` (identifier + reason), never a partial/guessed rectangle (§3 tests 10-14, 17).
7. **Item 7** — fixed field mapping (`y`→`originZ`, `height`→`length`, verbatim `name`) and in-file last-write-wins collision resolution, logged (§2's `identifierToAreaIndex` map; §3 test 9, §6 test 9).
8. **Item 8** — human-triggered, one-shot, no provenance field, no re-sync action; `ScenarioScript_AreaImport_IO`'s entry point is the only door, called from nowhere in this ticket except a test harness (§4-§5).
9. **Item 9** — additive-never-destructive reconciliation in `ImportAreaRectanglesFromScenarioScriptFile` (§5): a name collision is skipped and reported (`skippedCollisionNames`), never overwritten (§6 test 8).
10. **Item 10** — the rectangle-count cap (`kMaxScenarioAreaExtractionRectangleCount`, §3 test 16), the coordinate/extent sanity bound (`kMaxScenarioAreaCoordinateMagnitude`, §3 test 15), and the byte-size cap enforced via a pre-read `std::filesystem::file_size` stat (`kMaxScenarioAreaImportSourceBytes`, §6 test 6).
11. **Item 11** — physically separate translation unit from `ScenarioScript_DataLua_IO` (§1's own header comment says so explicitly); never framed as, named as, or refactored into "the reader half" of that file.

Also invoked: **Constitution §6** (every external file validated: byte cap before read, rectangle-count cap, non-finite/absurd rejection, never a crash, never a partial write into `recipe.areas`) and **§15.3/§15.4** (this carve-out narrows §15.3 without reversing it; the guard mechanically enforces the filename-disjointness §15.4 already establishes for the two SanGen-owned files).

## Explicit out-of-scope
- **No Areas-tab UI wiring.** The "Import Areas from Scenario File..." action, its `FileDialog::OpenFilePath` call (filtered to `*.lua`), and any result-reporting UI (toast/dialog surfacing `ScenarioAreaImportResult`) are NOT built by this ticket — see "Interpretation calls made" item 2 for the scope-cut justification. `ScenarioScript_AreaImport_IO.h`'s entry point signature (`const std::string&` path + `Params::MapRecipe&`) is deliberately UI-callable with no further IO-layer work needed to wire it.
- **No promotion of `StartsWithBanner` into a shared `TextStartsWithScenarioGeneratedBanner` helper in `ScenarioScript_DataLua_IO.h`**, and no accompanying edit to `ScenarioScript_Export_IO.cpp`. Two independent copies ship instead — see "Interpretation calls made" item 1.
- **No `.sanmap` version bump, no `Sanmap_MigrationManifest_IO` entry, no `JsonPrimitives_IO` involvement of any kind** — per the IO Architecture Expert's ruling, this is a hand-rolled Lua-table-literal grammar, not a schema migration.
- **No writing of areas into any Lua file** (`<MapName>_data.lua`, the legacy `_Scenarios_Script.lua`, or `_Scenarios_Data.lua`) — this ticket is import-only; §15.11's own "Export direction" note confirms that would need its own, separate ratification and collides with §15.4 point 1.
- **No `PROC`/`PIPELINE` stage ever reads this extractor's output or the source `.lua`** — the imported rectangles become ordinary, fully hand-editable `recipe.areas` entries with zero ongoing link back to the file (item 8).
- **No hex/exponent numeric literals, no Lua string-escape decoding beyond the minimal backslash-skip needed to find a string's closing quote** — the grammar's own "single decimal numeric literal, optional sign, optional fractional part" wording (item 4) is taken literally; nothing wider is accepted.

## Acceptance test (end-to-end, in addition to §3's and §6's unit coverage)
1. Feeding a real `_Scenarios_Data.lua` exported via `Io::BuildScenarioDataLuaText` back into
   `ImportAreaRectanglesFromScenarioScriptFile`, under its own real filename, is refused
   (`bRefusedGeneratedFile == true`, `recipe.areas` untouched) — §6 test 3, the item-1-mandated test.
2. The SAME real exported text, written under a DIFFERENT filename (not matching either owned
   suffix), is STILL refused — proving the banner-line check is a real content guard, not a filename
   alias — §6 test 4.
3. A real foreign scenario file's own named-key rectangle lines (verbatim from
   `map_scripts_backup/Pandemonium Isthmus_Scenarios_Script.lua.officialbak:61-64`) import cleanly
   into an empty `recipe.areas`, field-for-field correct (`y`→`originZ`, `height`→`length`, name
   verbatim) — §3 test 1, §6 test 7.
4. A name collision against an existing `recipe.areas` entry is skipped and reported via
   `skippedCollisionNames`, while a non-colliding rectangle in the same file still imports — §6 test 8.
5. An in-file identifier reassignment resolves last-write-wins and is logged via
   `collisionIdentifiers`, on both the pure extractor and the wrapper — §3 test 9, §6 test 9.
6. A comment, a long comment, and a string literal containing `{`/`}`/`=` never produce a phantom
   area and never desynchronize the tokenizer's candidate detection for the real area that follows —
   §3 tests 6-8.
7. A nested table, a mixed keyed/positional table, a missing/duplicate/unrecognized field, and an
   absurd-magnitude or non-positive-extent rectangle are all rejected as near-misses, never imported
   — §3 tests 10-15.
8. Feeding more than `kMaxScenarioAreaExtractionRectangleCount` distinct valid rectangles halts
   extraction exactly at the cap and sets `bRectangleCountCapExceeded` — §3 test 16.
9. A file larger than `kMaxScenarioAreaImportSourceBytes` is refused via a filesystem stat, before
   its content is ever read into memory — §6 test 6.
10. A real spawn-shaped `ARMY_01 = { x=..., y=..., z=... }` table is never imported as an area (wrong
    field count and wrong key set both independently exclude it) — §3 test 18.
11. Full `SanGenV2` build stays clean; every existing test continues to pass; both new test binaries
    (`ScenarioScript_AreaRectangleExtract_IO_Test`, `ScenarioScript_AreaImport_IO_Test`) pass with
    `ALL PASS`.

---

## Interpretation calls made beyond the ratified text

1. **No promotion of the shared banner-check helper.** The IO Architecture Expert's ruling left
   promoting `StartsWithBanner` into `ScenarioScript_DataLua_IO.h` (as e.g.
   `TextStartsWithScenarioGeneratedBanner`) explicitly optional for this ticket. Given this ticket
   already lands two new production file pairs and two new test files, touching a third,
   already-shipped file (`ScenarioScript_DataLua_IO.h`) plus its one existing call site
   (`ScenarioScript_Export_IO.cpp`) purely for a 3-line de-duplication is deferred as clean,
   independently-schedulable follow-up work — it changes no behavior either way, and the two copies
   are guaranteed to stay in sync because both compare against the exact same
   `kScenarioGeneratedFileBannerLine` constant, never a re-typed literal.
2. **No UI button wiring in this ticket.** The dispatch explicitly left this scope cut to this
   ticket's own judgment. Given the IO-layer surface here is already substantial (a new closed
   grammar, a new disk-touching entry point, and comprehensive tests for both), and the UI-side work
   (an Areas-tab menu action, a `FileDialog::OpenFilePath` call, and result-reporting UI) is a
   distinct concern with its own design questions (where the action lives, how a large near-miss/
   collision report is surfaced to the human) better suited to a dedicated UI-layer ticket, this
   ticket stops at a fully-tested, UI-callable IO entry point. `ScenarioScript_AreaImport_IO.h`'s
   signature was verified UI-callable with no further IO changes needed (a `const std::string&` path
   plus `Params::MapRecipe&`, exactly the shape `FileDialog::OpenFilePath`'s own `outChosenPath` plus
   any tab's live recipe reference would supply).
3. **Byte-size cap value (4 MiB) and coordinate-magnitude bound (1.0e6).** Neither is named in
   §15.11's text (which only mandates that caps exist, item 10). Both are chosen generously above
   every real value observed in `map_scripts_backup`'s officialbak files (all well under 32 KB;
   coordinates never exceeding a few thousand) while still bounding pathological input — backstops,
   not tight validators, and not re-litigating any map-design limit.
4. **`width`/`height` (in Lua) — i.e. `MapArea::width`/`length` — must be strictly positive**, not
   merely finite. §15.11 item 10 says "reject non-finite or absurd coordinates"; a zero or negative
   extent is not itself non-finite, but it is not a rectangle either, so it is folded into the same
   "absurd" rejection bucket alongside the magnitude bound, per Constitution §6's general
   validate-then-reject discipline.
5. **`ScenarioAreaExtractionNearMiss` field naming and shape** (`identifier` + `reason`) is not
   specified by either ruling beyond "a near-miss diagnostics list" — a small, natural shape chosen
   to match item 6's own wording ("naming the identifier and reason") directly.
6. **Guard ordering inside `ImportAreaRectanglesFromScenarioScriptFile`**: filename check (zero I/O)
   before the file-size stat (no content read) before the byte-cap comparison before the actual read
   before the banner-line check. Not specified by either ruling; chosen cheapest-guard-first, and to
   avoid ever loading an oversized file's bytes into a `std::string` just to discover it's oversized.

## Key files read/cited while drafting
`D:\Projects\Sanctuary\Map Generator\ARCH_15_11_ForeignScenarioAreaImport.md`,
`D:\Projects\Sanctuary\Map Generator\ARCH_15_MapScenarioSystem.md`,
`D:\Projects\Sanctuary\Map Generator\src\io\ScenarioScript_DataLua_IO.h`,
`D:\Projects\Sanctuary\Map Generator\src\io\ScenarioScript_Export_IO.h`,
`D:\Projects\Sanctuary\Map Generator\src\io\ScenarioScript_Export_IO.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\io\ScenarioScript_Export_IO_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\io\FilesystemPrimitives_IO.h`,
`D:\Projects\Sanctuary\Map Generator\src\io\FileDialog_IO.h`,
`D:\Projects\Sanctuary\Map Generator\src\io\MapExporter_ScenarioAreaNameValidation_IO.h`,
`D:\Projects\Sanctuary\Map Generator\src\params\MapArea_PARAMS.h`,
`D:\Projects\Sanctuary\Map Generator\src\params\MapRecipe_PARAMS.h`,
`D:\Projects\Sanctuary\Map Generator\map_scripts_backup\Pandemonium Isthmus_Scenarios_Script.lua.officialbak`
(lines 42-68, the real named-key rectangle/quaternion/scale/spawn syntax used verbatim in every test
fixture),
`D:\Projects\Sanctuary\Map Generator\CMakeLists.txt` (the `add_sangen_test` macro, the `src/io/*.cpp`
glob, and the `ScenarioScript_DataLua_IO_Test`/`ScenarioScript_Export_IO_Test` registration
precedent),
and `work_orders\STEP210_AreaCanvasGesture_UI.md` / `work_orders\STEP209_ScenarioAreaNameReference_PARAMS_IO_UI.md`
used for this document's own structure/rigor precedent.
