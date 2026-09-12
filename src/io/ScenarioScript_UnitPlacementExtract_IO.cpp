// ScenarioScript_UnitPlacementExtract_IO.cpp -- see the header for the full contract. Tokenization is
// delegated to the shared ScenarioScript_LuaLiteralLexer_IO primitive (STEP262); the array-of-tables
// recognition below duplicates ScenarioScript_SlotPatternExtract_IO.cpp's own
// FindMatchingCloseBraceIndex/SplitTopLevelCommaSegments helpers verbatim (each sibling extractor
// owns its own private copy -- established precedent, see that file's own top-of-file note); the
// closed grammar itself (the five/six-key element shape, the armyIndex guard, the caps) is private to
// this translation unit and specific to the unit-placement grammar only.
#include "ScenarioScript_UnitPlacementExtract_IO.h"
#include "ScenarioScript_LuaLiteralLexer_IO.h"
#include "ScenarioScript_AreaRectangleExtract_IO.h"   // kMaxScenarioAreaCoordinateMagnitude, reused verbatim
#include <cmath>

namespace SanmapGen {
namespace Io {
namespace {

using Token     = LuaLiteralToken;
using TokenKind = LuaLiteralTokenKind;

struct SegmentSpan { std::size_t start; std::size_t end; };

// Returns the index of the `}` matching the `{` at tokens[openBraceIndex], or tokens.size() if the
// table never closes.
std::size_t FindMatchingCloseBraceIndex(const std::vector<Token>& tokens, std::size_t openBraceIndex) {
    int depth = 0;
    for (std::size_t index = openBraceIndex; index < tokens.size(); ++index) {
        if (tokens[index].kind == TokenKind::LBrace) ++depth;
        else if (tokens[index].kind == TokenKind::RBrace) {
            --depth;
            if (depth == 0) return index;
        }
    }
    return tokens.size();
}

// Splits [start, end) on every top-level comma -- "top-level" meaning outside any nested `{...}`, so
// the nested `rotation = { x=.., y=.., z=.., w=.. }` sub-table stays ONE segment instead of being torn
// apart by its own internal commas.
std::vector<SegmentSpan> SplitTopLevelCommaSegments(const std::vector<Token>& tokens, std::size_t start, std::size_t end) {
    std::vector<SegmentSpan> segments;
    std::size_t segmentStart = start;
    int depth = 0;
    for (std::size_t index = start; index < end; ++index) {
        const Token& token = tokens[index];
        if (token.kind == TokenKind::LBrace) ++depth;
        else if (token.kind == TokenKind::RBrace) --depth;
        else if (token.kind == TokenKind::Comma && depth == 0) {
            segments.push_back({ segmentStart, index });
            segmentStart = index + 1;
        }
    }
    if (segmentStart < end) segments.push_back({ segmentStart, end });
    return segments;
}

// A segment is `IDENT = ...`; returns false (leaving outKey untouched) for anything else, e.g. an
// empty segment from a stray comma.
bool SegmentIsKeyedField(const std::vector<Token>& tokens, SegmentSpan segment, std::string& outKey) {
    if (segment.end - segment.start < 3) return false;
    if (tokens[segment.start].kind != TokenKind::Identifier) return false;
    if (tokens[segment.start + 1].kind != TokenKind::Equals) return false;
    outKey = tokens[segment.start].text;
    return true;
}

bool ValueIsSingleString(const std::vector<Token>& tokens, SegmentSpan segment, std::string& outValue) {
    if (segment.end - segment.start != 3) return false;
    if (tokens[segment.start + 2].kind != TokenKind::String) return false;
    outValue = tokens[segment.start + 2].text;
    return true;
}

bool ValueIsBoundedNumber(const std::vector<Token>& tokens, SegmentSpan segment, float& outValue) {
    if (!TryReadSignedNumberSpanningRange(tokens, segment.start + 2, segment.end, outValue)) return false;
    return std::isfinite(outValue) && std::fabs(outValue) <= kMaxScenarioAreaCoordinateMagnitude;
}

// Grades the nested `rotation = { x=.., y=.., z=.., w=.. }` sub-table [bodyStart, bodyEnd) (the span
// strictly BETWEEN the sub-table's own braces): all four sub-fields required together, no extras, no
// duplicates, each a plain signed numeric literal (no magnitude bound -- a quaternion component is
// not a map-scale coordinate, so kMaxScenarioAreaCoordinateMagnitude does not apply here). A partial
// or malformed rotation table is reported via outFailReason and is a near-miss for the WHOLE row, per
// the header's "all four sub-fields required together" rule.
bool GradeRotationSubTable(const std::vector<Token>& tokens, std::size_t bodyStart, std::size_t bodyEnd,
                           Params::ScenarioUnitPlacement& outPlacement, std::string& outFailReason) {
    const std::vector<SegmentSpan> segments = SplitTopLevelCommaSegments(tokens, bodyStart, bodyEnd);
    bool bHasX = false, bHasY = false, bHasZ = false, bHasW = false;
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;

    for (const SegmentSpan& segment : segments) {
        std::string key;
        if (!SegmentIsKeyedField(tokens, segment, key)) { outFailReason = "rotation table has a malformed field"; return false; }
        float value = 0.0f;
        if (!TryReadSignedNumberSpanningRange(tokens, segment.start + 2, segment.end, value) || !std::isfinite(value)) {
            outFailReason = "rotation field '" + key + "' is not a numeric literal";
            return false;
        }
        if      (key == "x") { if (bHasX) { outFailReason = "duplicate rotation field 'x'"; return false; } x = value; bHasX = true; }
        else if (key == "y") { if (bHasY) { outFailReason = "duplicate rotation field 'y'"; return false; } y = value; bHasY = true; }
        else if (key == "z") { if (bHasZ) { outFailReason = "duplicate rotation field 'z'"; return false; } z = value; bHasZ = true; }
        else if (key == "w") { if (bHasW) { outFailReason = "duplicate rotation field 'w'"; return false; } w = value; bHasW = true; }
        else { outFailReason = "unrecognized rotation field '" + key + "'"; return false; }
    }
    if (!(bHasX && bHasY && bHasZ && bHasW)) {
        outFailReason = "partial rotation table -- x/y/z/w are only accepted all four together";
        return false;
    }
    outPlacement.rotationX = x; outPlacement.rotationY = y; outPlacement.rotationZ = z; outPlacement.rotationW = w;
    return true;
}

// Grades ONE positional `{ ... }` candidate element [bodyStart, bodyEnd) against Shape 3's rule.
// `armyIndex`'s guard is checked FIRST, before any other grading, per the header's own "checked and
// reported before any other grading" ordering. Returns false (never a partial ScenarioUnitPlacement)
// for anything the grammar does not accept; outArmyNameContext carries the row's own `armyName` string
// if one was found and parsed cleanly, else stays empty, purely for a human-facing near-miss tag.
bool GradeCandidateElement(const std::vector<Token>& tokens, std::size_t bodyStart, std::size_t bodyEnd, int mapSize,
                           Params::ScenarioUnitPlacement& outPlacement, std::string& outFailReason,
                           std::string& outArmyNameContext) {
    const std::vector<SegmentSpan> segments = SplitTopLevelCommaSegments(tokens, bodyStart, bodyEnd);

    bool bHasArmyIndex = false, bHasArmyName = false, bHasTemplateIdentifier = false;
    bool bHasX = false, bHasY = false, bHasZ = false, bHasRotation = false;
    std::string armyNameValue, templateIdentifierValue;
    float xValue = 0.0f, yValue = 0.0f, zValue = 0.0f;
    SegmentSpan armyNameSegment{ 0, 0 }, templateIdentifierSegment{ 0, 0 };
    SegmentSpan xSegment{ 0, 0 }, ySegment{ 0, 0 }, zSegment{ 0, 0 }, rotationSegment{ 0, 0 };

    for (const SegmentSpan& segment : segments) {
        std::string key;
        if (!SegmentIsKeyedField(tokens, segment, key)) { outFailReason = "malformed field"; return false; }

        if (key == "armyName") { if (bHasArmyName) { outFailReason = "duplicate field 'armyName'"; return false; } bHasArmyName = true; armyNameSegment = segment; }
        else if (key == "templateIdentifier") { if (bHasTemplateIdentifier) { outFailReason = "duplicate field 'templateIdentifier'"; return false; } bHasTemplateIdentifier = true; templateIdentifierSegment = segment; }
        else if (key == "x") { if (bHasX) { outFailReason = "duplicate field 'x'"; return false; } bHasX = true; xSegment = segment; }
        else if (key == "y") { if (bHasY) { outFailReason = "duplicate field 'y'"; return false; } bHasY = true; ySegment = segment; }
        else if (key == "z") { if (bHasZ) { outFailReason = "duplicate field 'z'"; return false; } bHasZ = true; zSegment = segment; }
        else if (key == "rotation") { if (bHasRotation) { outFailReason = "duplicate field 'rotation'"; return false; } bHasRotation = true; rotationSegment = segment; }
        else if (key == "armyIndex") { bHasArmyIndex = true; }
        else { outFailReason = "unrecognized field '" + key + "'"; return false; }
    }
    // Best-effort human-facing context: whatever armyName parses to, even on a row that is about to
    // be rejected for some OTHER reason (including the armyIndex guard below). Never itself a reason
    // to fail this row.
    if (bHasArmyName) ValueIsSingleString(tokens, armyNameSegment, outArmyNameContext);

    // The one genuinely new risk this shape introduces (ARCH_15_14 Part A "Corrections" item 3):
    // a raw armyIndex integer is never reinterpreted as a stable ARMY_XX identity, regardless of
    // whatever else is present or missing on the same row.
    if (bHasArmyIndex) {
        outFailReason = "row is keyed by 'armyIndex' instead of 'armyName' -- a raw pairs(Armies) "
                        "runtime handle is not a stable ARMY_XX identity and is never reinterpreted "
                        "(ARCH_15_14 Part A, 'Corrections' item 3)";
        return false;
    }

    std::vector<std::string> missingKeys;
    if (!bHasArmyName) missingKeys.push_back("armyName");
    if (!bHasTemplateIdentifier) missingKeys.push_back("templateIdentifier");
    if (!bHasX) missingKeys.push_back("x");
    if (!bHasY) missingKeys.push_back("y");
    if (!bHasZ) missingKeys.push_back("z");
    if (!missingKeys.empty()) {
        outFailReason = "missing required field(s):";
        for (const std::string& missingKey : missingKeys) outFailReason += " " + missingKey;
        return false;
    }

    if (!ValueIsSingleString(tokens, armyNameSegment, armyNameValue)) { outFailReason = "field 'armyName' is not a string literal"; return false; }
    if (!ValueIsSingleString(tokens, templateIdentifierSegment, templateIdentifierValue)) { outFailReason = "field 'templateIdentifier' is not a string literal"; return false; }
    if (!ValueIsBoundedNumber(tokens, xSegment, xValue)) { outFailReason = "field 'x' is not a bounded numeric literal"; return false; }
    if (!ValueIsBoundedNumber(tokens, ySegment, yValue)) { outFailReason = "field 'y' is not a bounded numeric literal"; return false; }
    if (!ValueIsBoundedNumber(tokens, zSegment, zValue)) { outFailReason = "field 'z' is not a bounded numeric literal"; return false; }
    outArmyNameContext = armyNameValue;

    outPlacement = Params::ScenarioUnitPlacement{};
    outPlacement.armyName = armyNameValue;
    outPlacement.templateIdentifier = templateIdentifierValue;
    outPlacement.positionX = xValue;
    outPlacement.positionY = yValue;
    outPlacement.positionZ = static_cast<float>(mapSize) - zValue - 1.0f;   // self-inverse of FlipPositionZ

    if (bHasRotation) {
        // rotationSegment spans `rotation = { ... }` -- the sub-table body is strictly between its
        // own braces, i.e. [rotationSegment.start + 3, rotationSegment.end - 1) provided it opens
        // with `{` and closes with `}` spanning the whole remaining segment.
        if (rotationSegment.end - rotationSegment.start < 4
            || tokens[rotationSegment.start + 2].kind != TokenKind::LBrace
            || tokens[rotationSegment.end - 1].kind != TokenKind::RBrace
            || FindMatchingCloseBraceIndex(tokens, rotationSegment.start + 2) != rotationSegment.end - 1) {
            outFailReason = "field 'rotation' is not a nested table";
            return false;
        }
        if (!GradeRotationSubTable(tokens, rotationSegment.start + 3, rotationSegment.end - 1, outPlacement, outFailReason)) {
            return false;
        }
    }
    // else: outPlacement's default-constructed rotationX/Y/Z=0, rotationW=1 already IS identity --
    // absence of the whole `rotation` key is not a near-miss (header's own rule).

    return true;
}

} // namespace

ScenarioUnitPlacementExtractionResult
ExtractUnitPlacementsFromScenarioScriptText(const std::string& sourceText, int mapSize) {
    ScenarioUnitPlacementExtractionResult result;
    const std::vector<Token> tokens = Tokenize(sourceText);

    std::size_t index = 0;
    while (index < tokens.size()) {
        std::size_t identifierIndex = index;
        if (tokens[identifierIndex].kind == TokenKind::Identifier && tokens[identifierIndex].text == "local") ++identifierIndex;
        if (identifierIndex >= tokens.size() || tokens[identifierIndex].kind != TokenKind::Identifier) { ++index; continue; }
        const std::size_t equalsIndex = identifierIndex + 1;
        if (equalsIndex >= tokens.size() || tokens[equalsIndex].kind != TokenKind::Equals) { ++index; continue; }
        const std::size_t braceIndex = equalsIndex + 1;
        if (braceIndex >= tokens.size() || tokens[braceIndex].kind != TokenKind::LBrace) { ++index; continue; }

        const std::size_t outerCloseIndex = FindMatchingCloseBraceIndex(tokens, braceIndex);
        if (outerCloseIndex >= tokens.size()) break;   // unterminated outer table -- nothing sane left

        // Scan the outer body for POSITIONAL `{ ... }` sub-tables only -- a `{` immediately preceded
        // by `=` is a KEYED sub-table (e.g. `spawns = { ARMY_01 = {...} }`), skipped whole and never
        // treated as a candidate element for this shape.
        std::size_t scanIndex = braceIndex + 1;
        while (scanIndex < outerCloseIndex) {
            if (tokens[scanIndex].kind == TokenKind::LBrace) {
                const bool bKeyedSubTable = scanIndex > braceIndex + 1 && tokens[scanIndex - 1].kind == TokenKind::Equals;
                const std::size_t elementCloseIndex = FindMatchingCloseBraceIndex(tokens, scanIndex);
                if (elementCloseIndex >= tokens.size()) { scanIndex = outerCloseIndex; break; }   // unterminated -- stop this array
                if (!bKeyedSubTable) {
                    const std::size_t elementBodyStart = scanIndex + 1;
                    const std::size_t elementBodyEnd = elementCloseIndex;

                    // Candidacy: a `templateIdentifier` or `armyIndex` key marks this element as a
                    // Shape-3 candidate (header's own doc comment explains why NOT bare `armyName` --
                    // that would misfire against Params::ScenarioSpawnPoint's own SCENARIO_SPAWN_POINTS
                    // array shape, which shares armyName/x/y/z but carries neither of those two keys).
                    bool bIsCandidate = false;
                    for (std::size_t scanKeyIndex = elementBodyStart; scanKeyIndex < elementBodyEnd && !bIsCandidate; ++scanKeyIndex) {
                        if (tokens[scanKeyIndex].kind == TokenKind::Identifier
                            && (tokens[scanKeyIndex].text == "templateIdentifier" || tokens[scanKeyIndex].text == "armyIndex")
                            && scanKeyIndex + 1 < elementBodyEnd && tokens[scanKeyIndex + 1].kind == TokenKind::Equals) {
                            bIsCandidate = true;
                        }
                    }

                    if (bIsCandidate) {
                        Params::ScenarioUnitPlacement placement;
                        std::string failReason, armyNameContext;
                        if (GradeCandidateElement(tokens, elementBodyStart, elementBodyEnd, mapSize, placement, failReason, armyNameContext)) {
                            if (result.placements.size() >= kMaxScenarioUnitPlacementExtractionCount) {
                                result.bUnitPlacementCountCapExceeded = true;
                                return result;   // Constitution §6 cap
                            }
                            result.placements.push_back(placement);
                        } else {
                            result.nearMisses.push_back({ armyNameContext, failReason });
                        }
                    }
                }
                scanIndex = elementCloseIndex + 1;
            } else {
                ++scanIndex;
            }
        }

        index = outerCloseIndex + 1;
    }
    return result;
}

} // namespace Io
} // namespace SanmapGen
