// ScenarioScript_MatchConditionExtract_IO.cpp -- see the header for the full contract. Tokenization
// is delegated to the shared ScenarioScript_LuaLiteralLexer_IO primitive (STEP262); everything below
// (the two closed templates, the caps, the near-miss/context bookkeeping) is private to this
// translation unit and specific to the match-condition grammar only.
#include "ScenarioScript_MatchConditionExtract_IO.h"
#include "ScenarioScript_LuaLiteralLexer_IO.h"
#include <cstdlib>
#include <utility>

namespace SanmapGen {
namespace Io {
namespace {

using Token     = LuaLiteralToken;
using TokenKind = LuaLiteralTokenKind;

struct SegmentSpan { std::size_t start; std::size_t end; };

bool IsOtherTokenWithText(const Token& token, const char* text) {
    return token.kind == TokenKind::Other && token.text == text;
}

// An optional leading Minus token followed by exactly one Number token whose text contains no '.'
// (a genuine INTEGER literal, per Template A/B's "N a signed integer literal" requirement -- a
// literal like `2.0` is deliberately NOT accepted here even though the shared lexer would tokenize
// it as a single Number), consuming the WHOLE [index, end) range.
bool TryReadSignedIntegerSpanningRange(const std::vector<Token>& tokens, std::size_t index,
                                       std::size_t end, int& outValue) {
    if (index >= end) return false;
    bool bNegative = false;
    if (tokens[index].kind == TokenKind::Minus) { bNegative = true; ++index; }
    if (index >= end || tokens[index].kind != TokenKind::Number) return false;
    if (tokens[index].text.find('.') != std::string::npos) return false;
    const long magnitude = std::strtol(tokens[index].text.c_str(), nullptr, 10);
    ++index;
    if (index != end) return false;   // extra trailing token in this segment -- reject
    outValue = bNegative ? -static_cast<int>(magnitude) : static_cast<int>(magnitude);
    return true;
}

// Splits [start, end) on every top-level `and` identifier token. Safe to split unconditionally --
// the caller has already rejected any remaining parentheses in this range, so there is no nested
// `and` to protect against.
std::vector<SegmentSpan> SplitOnTopLevelAnd(const std::vector<Token>& tokens, std::size_t start, std::size_t end) {
    std::vector<SegmentSpan> segments;
    std::size_t segmentStart = start;
    for (std::size_t index = start; index < end; ++index) {
        if (tokens[index].kind == TokenKind::Identifier && tokens[index].text == "and") {
            segments.push_back({ segmentStart, index });
            segmentStart = index + 1;
        }
    }
    segments.push_back({ segmentStart, end });
    return segments;
}

bool BodyContainsIdentifier(const std::vector<Token>& tokens, std::size_t start, std::size_t end, const char* text) {
    for (std::size_t index = start; index < end; ++index)
        if (tokens[index].kind == TokenKind::Identifier && tokens[index].text == text) return true;
    return false;
}

bool MapComparator(TokenKind kind, Params::ScenarioComparator& outComparator) {
    switch (kind) {
        case TokenKind::EqualsEquals:  outComparator = Params::ScenarioComparator::Equal; return true;
        case TokenKind::TildeEquals:   outComparator = Params::ScenarioComparator::NotEqual; return true;
        case TokenKind::GreaterThan:   outComparator = Params::ScenarioComparator::GreaterThan; return true;
        case TokenKind::GreaterEquals: outComparator = Params::ScenarioComparator::GreaterOrEqual; return true;
        case TokenKind::LessThan:      outComparator = Params::ScenarioComparator::LessThan; return true;
        case TokenKind::LessEquals:    outComparator = Params::ScenarioComparator::LessOrEqual; return true;
        default: return false;
    }
}

bool MapVarField(const std::string& varText, Params::ScenarioCountField& outField) {
    if (varText == "t") { outField = Params::ScenarioCountField::Total; return true; }
    if (varText == "h") { outField = Params::ScenarioCountField::HumanCount; return true; }
    if (varText == "a") { outField = Params::ScenarioCountField::AiCount; return true; }
    return false;
}

// Template B's fixed tail -- everything after the `sub(...)` call's closing ')': `:find("[^-]") ~=
// nil`, byte-for-byte.
bool MatchTemplateBTail(const std::vector<Token>& tokens, std::size_t start, std::size_t end) {
    struct TailToken { TokenKind kind; const char* text; };
    static const TailToken kTail[] = {
        { TokenKind::Other, ":" }, { TokenKind::Identifier, "find" }, { TokenKind::Other, "(" },
        { TokenKind::String, "[^-]" }, { TokenKind::Other, ")" },
        { TokenKind::TildeEquals, "~=" }, { TokenKind::Identifier, "nil" },
    };
    constexpr std::size_t kTailCount = sizeof(kTail) / sizeof(kTail[0]);
    if (end - start != kTailCount) return false;
    for (std::size_t index = 0; index < kTailCount; ++index) {
        if (tokens[start + index].kind != kTail[index].kind || tokens[start + index].text != kTail[index].text) return false;
    }
    return true;
}

// Attempts Template B against the WHOLE body span [start, end): `pattern:sub(N, M):find("[^-]") ~=
// nil`. On success fills outCondition with the ONE fixed pairing this template ever produces.
bool TryMatchTemplateB(const std::vector<Token>& tokens, std::size_t start, std::size_t end,
                       Params::ScenarioCountCondition& outCondition, std::string& outFailReason) {
    if (end - start < 5 || !(tokens[start].kind == TokenKind::Identifier && tokens[start].text == "pattern"
        && IsOtherTokenWithText(tokens[start + 1], ":")
        && tokens[start + 2].kind == TokenKind::Identifier && tokens[start + 2].text == "sub"
        && IsOtherTokenWithText(tokens[start + 3], "("))) {
        outFailReason = "does not open with pattern:sub(";
        return false;
    }

    std::size_t commaIndex = end;
    for (std::size_t index = start + 4; index < end; ++index) {
        if (tokens[index].kind == TokenKind::Comma) { commaIndex = index; break; }
        if (IsOtherTokenWithText(tokens[index], ")")) break;   // closed before a comma was found -- fail below
    }
    if (commaIndex == end) { outFailReason = "sub(...) does not take exactly two comma-separated arguments"; return false; }

    std::size_t closeParenIndex = end;
    for (std::size_t index = commaIndex + 1; index < end; ++index) {
        if (IsOtherTokenWithText(tokens[index], ")")) { closeParenIndex = index; break; }
    }
    if (closeParenIndex == end) { outFailReason = "sub(...) never closes"; return false; }

    int slotRangeStart = 0, slotRangeEnd = 0;
    if (!TryReadSignedIntegerSpanningRange(tokens, start + 4, commaIndex, slotRangeStart)) {
        outFailReason = "sub(...)'s first argument is not a signed integer literal";
        return false;
    }
    if (!TryReadSignedIntegerSpanningRange(tokens, commaIndex + 1, closeParenIndex, slotRangeEnd)) {
        outFailReason = "sub(...)'s second argument is not a signed integer literal";
        return false;
    }
    if (!MatchTemplateBTail(tokens, closeParenIndex + 1, end)) {
        outFailReason = "does not close with :find(\"[^-]\") ~= nil verbatim";
        return false;
    }

    outCondition = Params::ScenarioCountCondition{};
    outCondition.field          = Params::ScenarioCountField::SlotRangeOccupiedCount;
    outCondition.comparator     = Params::ScenarioComparator::GreaterOrEqual;
    outCondition.value          = 1;
    outCondition.slotRangeStart = slotRangeStart;
    outCondition.slotRangeEnd   = slotRangeEnd;
    return true;
}

// Attempts Template A against the WHOLE body span [start, end): an AND-chain of `VAR OP N`
// conjuncts, one optional pair of parentheses tolerated around the whole chain.
bool TryMatchTemplateA(const std::vector<Token>& tokens, std::size_t start, std::size_t end,
                       std::vector<Params::ScenarioCountCondition>& outConditions, std::string& outFailReason) {
    if (start >= end) { outFailReason = "empty match body"; return false; }

    std::size_t bodyStart = start, bodyEnd = end;
    if (IsOtherTokenWithText(tokens[bodyStart], "(") && IsOtherTokenWithText(tokens[bodyEnd - 1], ")")) {
        int depth = 0;
        std::size_t matchIndex = bodyEnd;   // sentinel: "no matching close found within range"
        for (std::size_t index = bodyStart; index < bodyEnd; ++index) {
            if (IsOtherTokenWithText(tokens[index], "(")) ++depth;
            else if (IsOtherTokenWithText(tokens[index], ")")) {
                --depth;
                if (depth == 0) { matchIndex = index; break; }
            }
        }
        if (matchIndex == bodyEnd - 1) { ++bodyStart; --bodyEnd; }   // the pair spans the whole chain -- strip it
    }

    for (std::size_t index = bodyStart; index < bodyEnd; ++index) {
        if (IsOtherTokenWithText(tokens[index], "(") || IsOtherTokenWithText(tokens[index], ")")) {
            outFailReason = "unexpected parentheses in the AND-chain";
            return false;
        }
    }

    const std::vector<SegmentSpan> segments = SplitOnTopLevelAnd(tokens, bodyStart, bodyEnd);
    if (segments.size() > kMaxScenarioMatchConditionConjunctCountPerChain) {
        outFailReason = "AND-chain exceeds the conjunct-count cap";
        return false;
    }

    std::vector<Params::ScenarioCountCondition> conditions;
    conditions.reserve(segments.size());
    for (const SegmentSpan& segment : segments) {
        if (segment.start >= segment.end) { outFailReason = "empty conjunct"; return false; }
        if (segment.end - segment.start < 3) { outFailReason = "conjunct has too few tokens"; return false; }
        if (tokens[segment.start].kind != TokenKind::Identifier) { outFailReason = "conjunct does not start with t/h/a"; return false; }

        Params::ScenarioCountField field;
        if (!MapVarField(tokens[segment.start].text, field)) { outFailReason = "left-hand side is not t, h, or a"; return false; }

        Params::ScenarioComparator comparator;
        if (!MapComparator(tokens[segment.start + 1].kind, comparator)) {
            outFailReason = "conjunct's operator is not one of ==/~=/>/>=/</<=";
            return false;
        }

        int value = 0;
        if (!TryReadSignedIntegerSpanningRange(tokens, segment.start + 2, segment.end, value)) {
            outFailReason = "conjunct's right-hand side is not a signed integer literal";
            return false;
        }

        Params::ScenarioCountCondition condition;
        condition.field = field;
        condition.comparator = comparator;
        condition.value = value;
        conditions.push_back(condition);
    }

    outConditions = std::move(conditions);
    return true;
}

} // namespace

ScenarioMatchConditionExtractionResult ExtractMatchConditionsFromScenarioScriptText(const std::string& sourceText) {
    ScenarioMatchConditionExtractionResult result;
    const std::vector<Token> tokens = Tokenize(sourceText);
    std::string lastSeenNameContext;   // best-effort human-facing tag only, never interpreted further

    std::size_t index = 0;
    while (index < tokens.size()) {
        if (tokens[index].kind == TokenKind::Identifier && tokens[index].text == "name"
            && index + 2 < tokens.size() && tokens[index + 1].kind == TokenKind::Equals
            && tokens[index + 2].kind == TokenKind::String) {
            lastSeenNameContext = tokens[index + 2].text;
            index += 3;
            continue;
        }

        // Recognition anchor: the exact fixed preamble `match = function(t, h, a, pattern)`. A field
        // whose parameter list differs is not recognized as a candidate at all here.
        const bool bPreambleMatches =
            tokens[index].kind == TokenKind::Identifier && tokens[index].text == "match"
            && index + 11 < tokens.size()
            && tokens[index + 1].kind == TokenKind::Equals
            && tokens[index + 2].kind == TokenKind::Identifier && tokens[index + 2].text == "function"
            && IsOtherTokenWithText(tokens[index + 3], "(")
            && tokens[index + 4].kind == TokenKind::Identifier && tokens[index + 4].text == "t"
            && tokens[index + 5].kind == TokenKind::Comma
            && tokens[index + 6].kind == TokenKind::Identifier && tokens[index + 6].text == "h"
            && tokens[index + 7].kind == TokenKind::Comma
            && tokens[index + 8].kind == TokenKind::Identifier && tokens[index + 8].text == "a"
            && tokens[index + 9].kind == TokenKind::Comma
            && tokens[index + 10].kind == TokenKind::Identifier && tokens[index + 10].text == "pattern"
            && IsOtherTokenWithText(tokens[index + 11], ")");

        if (!bPreambleMatches) { ++index; continue; }

        const std::size_t returnIndex = index + 12;
        if (returnIndex >= tokens.size() || tokens[returnIndex].kind != TokenKind::Identifier
            || tokens[returnIndex].text != "return") {
            result.nearMisses.push_back({ lastSeenNameContext, "match function body does not start with 'return'" });
            index = returnIndex;
            continue;
        }

        // Template A/B bodies never contain the identifier "end" themselves -- the first occurrence
        // closes this function.
        std::size_t endIndex = returnIndex + 1;
        while (endIndex < tokens.size()
               && !(tokens[endIndex].kind == TokenKind::Identifier && tokens[endIndex].text == "end")) {
            ++endIndex;
        }
        if (endIndex >= tokens.size()) {
            result.nearMisses.push_back({ lastSeenNameContext, "unterminated match function (no matching 'end')" });
            break;   // nothing sane left to scan past an unterminated function
        }

        const std::size_t bodyStart = returnIndex + 1;
        const std::size_t bodyEnd   = endIndex;

        if (BodyContainsIdentifier(tokens, bodyStart, bodyEnd, "or")) {
            // Any `or` anywhere in the expression is an automatic near-miss -- checked before either
            // template is attempted, per Part B's own restated rule.
            result.nearMisses.push_back({ lastSeenNameContext, "expression contains 'or'" });
        } else {
            std::string failReason;
            Params::ScenarioCountCondition templateBCondition;
            std::vector<Params::ScenarioCountCondition> templateAConditions;
            const bool bMatchedTemplateB = TryMatchTemplateB(tokens, bodyStart, bodyEnd, templateBCondition, failReason);
            const bool bMatchedTemplateA = !bMatchedTemplateB
                && TryMatchTemplateA(tokens, bodyStart, bodyEnd, templateAConditions, failReason);

            if (bMatchedTemplateB || bMatchedTemplateA) {
                if (result.conditionSets.size() >= kMaxScenarioMatchConditionExtractionCount) {
                    result.bMatchConditionCountCapExceeded = true;
                    break;   // Constitution §6 cap
                }
                ScenarioMatchConditionCandidate candidate;
                candidate.identifier = lastSeenNameContext;
                candidate.conditions = bMatchedTemplateB
                    ? std::vector<Params::ScenarioCountCondition>{ templateBCondition }
                    : std::move(templateAConditions);
                result.conditionSets.push_back(std::move(candidate));
            } else {
                result.nearMisses.push_back({ lastSeenNameContext, "does not match Template A or Template B" });
            }
        }

        index = endIndex + 1;   // resume immediately after the whole match field, never re-enter it
    }
    return result;
}

} // namespace Io
} // namespace SanmapGen
