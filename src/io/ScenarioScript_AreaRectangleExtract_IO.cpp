// ScenarioScript_AreaRectangleExtract_IO.cpp -- see the header for the full contract. Tokenization
// (comment/string-skipping, identifier/number/string/operator lexing) is delegated to the shared
// ScenarioScript_LuaLiteralLexer_IO primitive (STEP262) -- promoted out of this file once STEP263/
// STEP264 needed the same grammar surface. Everything below the tokenizer (the grammar itself: the
// closed keyed/positional shape, the caps, the near-miss/collision bookkeeping) remains private to
// this translation unit and specific to the area-rectangle grammar only.
#include "ScenarioScript_AreaRectangleExtract_IO.h"
#include "ScenarioScript_LuaLiteralLexer_IO.h"
#include <cmath>
#include <unordered_map>

namespace SanmapGen {
namespace Io {
namespace {

using Token     = LuaLiteralToken;
using TokenKind = LuaLiteralTokenKind;

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
