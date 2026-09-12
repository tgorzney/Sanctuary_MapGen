# STEP262 — Shared `ScenarioScript_LuaLiteralLexer_IO`; migrate the area extractor onto it

**Layer:** IO/BRIDGE. **Domain:** new `src/io/ScenarioScript_LuaLiteralLexer_IO.h/.cpp`, refactor
`src/io/ScenarioScript_AreaRectangleExtract_IO.cpp` onto it. **Executor:** SanGen Coder. **Sequence:**
independent of `STEP260`/`STEP261`; must land before `STEP263`/`STEP264` (both new extractors consume
this lexer). Pure internal refactor of the area extractor — no behavior/contract change to its public
header (`ScenarioScript_AreaRectangleExtract_IO.h`, untouched).

## 0. Why

`ScenarioScript_AreaRectangleExtract_IO.cpp` (`Tokenize`, `SkipWhitespaceAndComments`,
`SkipQuotedString`, `TryReadSignedNumberSpanningRange`, lines 22-124) is currently a private,
single-caller tokenizer — its own file's design rationale for keeping it private was explicitly tied
to "this grammar has exactly one caller today." `ARCH_15_14_ForeignScenarioFullDataImportAndUnitPlacement.md`
adds three more extractors (`STEP263`/`STEP264`) that all need the same comment/string-skipping and
numeric/identifier tokenization — per this codebase's own established law (a helper recurring across
two or more call sites becomes a shared primitive, not copy-paste, same rule already binding on
migration units), promote it now rather than let a fourth divergent copy accumulate.

## 1. New file — `src/io/ScenarioScript_LuaLiteralLexer_IO.h/.cpp`

Generalize the existing private helpers (`ScenarioScript_AreaRectangleExtract_IO.cpp:22-124`) into a
pure, disk-free, non-executing primitive — same "NO Lua execution of any kind" contract as the file it
comes from (`ARCH_15_11` item 3), stated explicitly in the new header's own doc comment so nothing
downstream mistakes it for a general Lua parser.

Widen scope beyond the area extractor's current needs, since the new extractors need more token kinds:
- Keep `IsIdentifierStartChar`/`IsIdentifierChar`/`IsDigitChar`, `SkipWhitespaceAndComments`,
  `SkipQuotedString`, `Tokenize`, `TryReadSignedNumberSpanningRange` — same signatures, same behavior,
  verbatim (this is a promotion, not a rewrite; the area extractor's existing tests must still pass
  unchanged against the promoted code).
- **New**: emit actual `String` tokens carrying the quoted content (the area extractor's own
  `SkipQuotedString` currently just skips past a string, discarding it — `STEP263`'s slot-pattern
  shape needs the literal string content, e.g. `"----hhhh"`).
- **New**: comparator operator tokens (`==`, `~=`, `>`, `>=`, `<`, `<=`) — needed by `STEP263`'s
  match-condition shape (`t == 4`, `h >= 2`, etc.), not needed by the area extractor's grammar today.

## 2. Migrate `ScenarioScript_AreaRectangleExtract_IO.cpp` onto the shared lexer

Replace its private `Tokenize`/`SkipWhitespaceAndComments`/`SkipQuotedString`/
`TryReadSignedNumberSpanningRange` (lines 22-124) with calls into the new shared header. This is a
pure internal refactor: `ExtractAreaRectanglesFromScenarioScriptText`'s public contract
(`ScenarioScript_AreaRectangleExtract_IO.h`) does not change in any way — same inputs, same outputs,
same caps (`kMaxScenarioAreaExtractionRectangleCount = 512`,
`kMaxScenarioAreaCoordinateMagnitude = 1.0e6f`, both stay defined in this file, not moved to the
lexer — they are grammar-specific caps, not lexer-level concerns).

**Re-run the area extractor's full existing test suite unchanged after the refactor** — every existing
assertion must still pass byte-for-byte; this ticket adds zero new area-extraction behavior.

## 3. Tests

1. **Area extractor regression**: every pre-existing test in
   `ScenarioScript_AreaRectangleExtract_IO_Test.cpp` (or wherever its tests live — confirm the exact
   file before starting) passes unchanged after the migration.
2. **New lexer unit tests** (first coverage, since the lexer is new as an independently-addressable
   unit): `Tokenize` on a string literal (`"foo"`, `'bar'`) emits a `String` token with the unescaped
   content, not just a skip; `Tokenize` on `==`, `~=`, `>=`, `<=`, `>`, `<` emits the correct
   comparator token kind for each, distinguishing `=` (assignment, unrelated) from `==`.
3. **Comment/string-skipping regression**: `--` line comments, `--[[ ]]` long comments, and quoted
   strings containing `--` or braces inside them are still skipped/parsed correctly (this behavior
   already exists in the area extractor; confirm it survives the promotion verbatim).

## 4. Out of scope

- Any new extractor consuming this lexer (`STEP263`, `STEP264`) — this ticket only promotes the
  primitive and re-points the existing area extractor at it.
- Any change to `ScenarioScript_AreaImport_IO.h`'s disk-touching entry point or its banner/filename
  refusal guard — untouched.
- Any widening of what the area extractor's own grammar accepts — its accepted shape
  (`[local] IDENTIFIER = {x,y,width,height}`, keyed or positional) is unchanged.

## 5. Files touched

**New:** `src/io/ScenarioScript_LuaLiteralLexer_IO.h`, `src/io/ScenarioScript_LuaLiteralLexer_IO.cpp`,
plus a new test file for the lexer's own new coverage (item 2 above).

**Modified:** `src/io/ScenarioScript_AreaRectangleExtract_IO.cpp` (internal refactor only — its `.h`
does not change).
