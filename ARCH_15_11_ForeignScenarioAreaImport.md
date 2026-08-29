[← ARCH index](ARCH.md) · [§15 ARCH_15_MapScenarioSystem](ARCH_15_MapScenarioSystem.md) · SanGen ARCH §15.11. **Only the ARCH Expert writes this file.**

### 15.11 Narrow, permanently-bounded carve-out from §15.3 — one-shot import of AREA RECTANGLES ONLY from a FOREIGN scenario `.lua` (2026-08-29)

**Ruled: GRANTED, as a narrowing of §15.3's wording, not a reversal of its intent.** §15.3 is
amended by this section: its sentence "SanGen never calls into a Lua parser to read this file, or
any scenario content, back" continues to bind in full for every file SanGen writes and for every
kind of scenario content except the one named below.

**Why the rule bends here and nowhere else.** §15.3 rejected option (a) — literal round-trip of the
tiered scenario tables — and option (b) — the game re-reading the `.sanmap` at load. Both are about
**SanGen-owned artifacts**. The invariant §15.3 exists to protect is *scenario data has exactly one
authoritative representation* (the `.sanmap` `Scenarios` section), with the `.lua` a derived render
target that is never an input; the failure mode it forbids is a writer and a reader of the same
artifact silently diverging over time. A one-shot ingest of rectangles from a file **SanGen never
writes** creates no second authoritative representation and no writer/reader pair at all: the values
land in `PARAMS` immediately and the source file is never consulted again. That is the identical
shape §18.2 already ratified for `.santp` footprints (ingest → human-triggered bake into `PARAMS` →
never a live read), and Constitution §1 already puts foreign-map import inside `IO/BRIDGE`'s
charter ("SupCom/official-map import"). §15.4 independently supports it: the legacy
`<MapName>_Scenarios_Script.lua` and the `<MapName>_data.lua` orchestrator are **hand-authored,
never written by SanGen under any code path**, and §15.4 already contemplates a one-time *human*
migration of such a map. This carve-out mechanizes one narrow slice of that already-ratified human
action — it does not make migration automatic.

**The boundary. All eleven items bind; widening any of them requires a new ARCH ratification, and is
not a coder's, reviewer's, or other expert's call.**

1. **Source-file whitelist, mechanically enforced — this is the load-bearing guard.** The extractor
   MUST refuse any input whose first line equals `Io::kScenarioGeneratedFileBannerLine`
   (`ScenarioScript_DataLua_IO.h`'s existing single source of truth — referenced, never re-typed as
   a second literal), and MUST refuse the two SanGen-owned filenames
   `<MapName>_Scenarios_Runtime.lua` and `<MapName>_Scenarios_Data.lua` by name. Refusal is loud and
   logged, naming the path (Constitution §6). This makes "SanGen never reads back what SanGen wrote"
   a *checked property*, not a convention. **Required acceptance test:** export a
   `_Scenarios_Data.lua` and feed it to the extractor — the extractor must refuse it.
2. **Exactly one output kind: rectangles.** The result type is a list of `Params::MapArea` and
   nothing else. No scenario record, `match` predicate, `pattern`, `spawns`, `alloyMode`, alloy
   list, unit spawn, marker, army, or `COUNT_SCENARIOS`/`PATTERN_SCENARIOS` ordering may be
   extracted, returned, or inferred — **explicitly including cases where the same technique could
   trivially read them.** §15.3 remains absolute for all of those.
3. **No execution. No LuaJIT. Ever, on this path.** Not `LuaTableEvaluate_SYS`, not a variant of it,
   not a widened one. §18.1's standing constraint is extended here by name: `LuaTableEvaluate_SYS`
   captures **globals only**, these rectangles are `local`s, and widening it to reach locals is
   forbidden permanently — it would attack the safety contract of a primitive that genuinely runs
   untrusted code, for a caller it was never designed to serve. It would also not work: a real
   scenario script's first statements include `Import(...)`, which errors instantly in §18.1's
   mandatory zero-stdlib state. This path is a non-executing lexical extractor and nothing else.
4. **A closed literal-only grammar, not a Lua parser.** Ground truth, read directly from the live
   `map_scripts_backup/Pandemonium Isthmus_Scenarios_Script.lua.officialbak:61-64`, is the **named-key**
   form — `local AREA_356 = { x = 846, y = 846, width = 356, height = 356 }` — not the positional
   shorthand `MAP_SCENARIO_SPEC.md` §5.2's table uses in prose. Accept exactly: an optional `local`,
   an identifier, `=`, `{`, then **either** all four keyed pairs `x`/`y`/`width`/`height` in any
   order **or** exactly four positional values read as `x, y, width, height` in that order, `}`.
   Every value must be a single decimal numeric literal (optional sign, optional fractional part).
   **Rejected outright, never evaluated:** arithmetic, identifiers, function calls, string keys,
   nesting, a fifth key, a missing key, `nil`. Mixed keyed/positional in one table is rejected.
5. **Comments and strings must be skipped before matching** — `--` line comments, `--[[ ]]` long
   comments, and quoted strings. The live file contains prose mentioning `AREA_356` inside comments;
   a naive scan that matched them would import phantom areas.
6. **Non-matching text is not an error; a near-miss is logged, never guessed.** A scenario script is
   mostly code, and ignoring it is normal operation. A candidate that enters the grammar and fails
   it is skipped with a one-line log naming the identifier and reason — never partially filled,
   never defaulted into a rectangle (Constitution §6's validate-then-default-then-log, where the
   "default" for an un-parseable rectangle is *no rectangle*).
7. **Field mapping is fixed.** Lua `y` → `MapArea::originZ`; Lua `height` → `MapArea::length` (the
   existing format-dictated rename already recorded in `MapArea_PARAMS.h`, ARCH §1.8's family);
   `x`/`width` map verbatim. The Lua identifier becomes `MapArea::name` **verbatim** (`AREA_356`
   stays `AREA_356`) — it is a load-bearing gameplay identifier (`GameUtils.GetArea(name)`), so the
   extractor must never prettify, de-prefix, or invent a name. Values are parsed straight to `float`
   with no arithmetic performed on them. Repeated assignment of the same identifier within one file
   resolves last-wins (Lua's own semantics) with the collision logged.
8. **Human-triggered, one-shot, no live binding — §18.2 applied verbatim.** Invocation is an explicit
   authoring action. It is forbidden to run implicitly on map open, on export, on generate, or on any
   dirty-hash recompute. The imported rectangles become ordinary authored `MapRecipe::areas` entries,
   fully hand-editable afterward, serialized to the `.sanmap` like any other area, with **no
   provenance field, no re-sync action, and no ongoing link to the source file**. No `PROC` or
   `PIPELINE` stage may ever read the extractor's output or the `.lua`.
9. **Additive, never destructive.** An incoming name that collides with an existing `MapArea` is a
   conflict surfaced to the human (default: skip and report), never a silent overwrite — area names
   are gameplay-load-bearing and `"PlayableArea"` in particular is pinned by §14.17.
10. **Constitution §6 caps apply literally:** a byte cap on the source text before scanning, a cap on
    the number of extracted rectangles, and rejection of non-finite or absurd coordinates rather than
    importing them.
11. **No round-trip framing, ever.** This extractor must never be documented, named, or refactored as
    "the reader half" of `ScenarioScript_DataLua_IO`. They read and write disjoint file sets by
    construction (item 1), and that disjointness IS the carve-out.

**Layer and placement.** `IO`. It is **not** a `MapImporter_*` unit — §15.2 already ruled that
scenario `.lua` companion artifacts do not extend the `MapImporter_<Domain>_IO` convention, which is
scoped to JSON fragments of the one `.sanmap` document; this file is not part of that document and
does not live in the map asset folder. It belongs to the `ScenarioScript_*_IO` family as a **new,
physically separate translation unit** (working name `ScenarioScript_AreaRectangleExtract_IO`), so
that `ScenarioScript_DataLua_IO.h`'s standing promise — "there is no matching 'read this back'
function anywhere in this file or its .cpp, and there never will be" — remains literally true and
needs no edit. **Exact file split, naming, and the pure-text-in/values-out vs. filesystem-touching
division are the SanGen IO Architecture Expert's call** (§15.2's ownership ruling), and should
mirror the family's existing pure/disk split (`ScenarioScript_DataLua_IO` pure ↔
`ScenarioScript_Export_IO` touches disk). No code until a ratified work-order exists.

**Export direction, confirmed unblocked (answers the "eventually export areas too" question).**
Rendering area rectangles into the SanGen-owned `<MapName>_Scenarios_Data.lua` is already permitted
and already partly built — `ScenarioScript_DataLua_IO.cpp`'s `AppendScenarioBodyFields` reads
`body.area.originX/originZ/width/length` today. Export was never what §15.3 restricted. **But
writing areas into `<MapName>_data.lua` or the legacy `<MapName>_Scenarios_Script.lua` collides
head-on with §15.4 point 1 ("never written by SanGen, under any code path") and §15.4's
filename-disjointness overwrite guard — that is refused today and would need its own ratification.**
Separately, worth knowing before that work is scoped: `areas` is already in `LoadMapData`'s
whitelist as quoted in §15.3/§15.9 (`props, decals, areas, armies, markers, chains, groups`), so a
scenario script can read `GameInfo.MapData.areas` by name from the `.sanmap` **with no engine change
at all** — unlike `scenarios`, which needs §15.9's not-yet-real whitelist addition. The likely
correct long-term shape for areas is therefore "author in SanGen → ships in the `.sanmap` → the
script looks the area up by name," which would make rendering areas into Lua largely unnecessary.
(Basis: the whitelist as recorded live-verified in §15.3, not independently re-verified in this
session.)
