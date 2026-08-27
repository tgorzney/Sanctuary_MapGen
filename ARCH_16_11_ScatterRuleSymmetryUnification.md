[← ARCH index](ARCH.md) · [§16 ARCH_16_MarkerLayerSymmetry](ARCH_16_MarkerLayerSymmetry.md) · SanGen ARCH §16.11. **Only the ARCH Expert writes this file.**

### 16.11 `SymmetrySetting` retrofit onto `PropRule`/`DecalRule`/`UnitRule` — the §16.1 non-binding follow-on, now taken

§16.1 explicitly left this open ("a future full unification is a natural but non-binding
follow-on, not decided here") and did not retrofit `SymmetrySetting` onto `PropRule`/`DecalRule`/
`UnitRule`/`MapRecipe::globalSymmetryMask`. This ruling takes exactly that follow-on for the three
per-rule types — **not** `MapRecipe`'s global triplet, which stays out of scope here (it is the
top-level default, not a per-rule override, and was never part of this request).

**Ruled: GO.** `PropRule`/`DecalRule`/`UnitRule` (`src/params/ScatterRule_PARAMS.h`) each replace
their inline `bSymmetryUseGlobal`/`symmetryMask`/`radialSymmetryRepeatCount` triplet with:
```cpp
SymmetrySetting symmetry;
```
(field name `symmetry`, type `Params::SymmetrySetting`, matching `MarkerRuleLayer`/
`MarkerInstanceLayer` verbatim — no new field name invented). `ScatterRule_PARAMS.h` gains
`#include "Symmetry_PARAMS.h"` (not currently included — it only pulls
`FootprintBakeFingerprint_PARAMS.h`/`ScatterTransform_PARAMS.h` today).

**Why the §16.1-era objection no longer applies.** §16.1's reasoning for NOT composing at that
time was "match an existing convention beats introducing a new one on an established type" — true
in isolation, but that convention was only ever "these three types + `MarkerRule` all happen to
duplicate the same three flat fields," not a load-bearing wire-format constraint. `MarkerRuleLayer`
already broke that convention for markers; `PropRule`/`DecalRule`/`UnitRule` duplicating it three
more times afterward is a pure DRY liability with a real fourth-and-fifth-site copy-paste risk
(exactly the shape of bug `SANMAP_FORMAT_SPEC` Correction 4's Defect 1 already produced once, when
`DecalRule` silently diverged from `PropRule`/`UnitRule`'s copy). Nothing else in §16.1's reasoning
(the two-simultaneous-consumer DRY argument, the JSON-flattening precedent) was type-specific to
markers — it applies identically here.

**JSON/wire shape: unchanged, confirmed by direct read of all three exporters/importers before this
ruling.** `MapExporter_{Props,Decals,Units}Stack_IO.cpp` all currently write
`json["SymmetryUseGlobal"] = rule.bSymmetryUseGlobal` (etc.) as flat sibling keys on the rule
object; composing the C++ struct changes those three lines to
`json["SymmetryUseGlobal"] = rule.symmetry.bSymmetryUseGlobal`, **writing the identical key**. This
is exactly the already-established "`SymmetrySetting` flattens to sibling keys, not a nested
`\"Symmetry\"` sub-object" convention `SANMAP_FORMAT_SPEC`'s Correction 15 already documents for
`MarkerRuleLayer`/`MarkerInstanceLayer` — the same struct, same flattening rule, now applied at a
third and fourth site. **No `SanGenVersion` bump, no migration, no `IO_MIGRATION_SPEC` entry.** This
is a pure C++-internal field-grouping refactor with a byte-identical `.sanmap` output — the
opposite of §16.6's marker migration, which WAS a genuine breaking schema change (per-rule tier
removed entirely, not just regrouped in C++). Do not conflate the two.

**Favorable difference from the §16.10 marker-migration hazard: this retrofit is compile-fail-safe,
not silent-failure-prone.** §16.10 flagged a silent-hash risk for the marker migration because a
consumer could keep compiling against a field that still existed elsewhere. Here, removing the
three flat members outright means every stale reference (`rule.bSymmetryUseGlobal`,
`rule.symmetryMask`, `rule.radialSymmetryRepeatCount` on a `PropRule`/`DecalRule`/`UnitRule`
instance) fails to compile — a full rebuild after the `ScatterRule_PARAMS.h` edit is itself the
completeness check; no separate grep sweep is load-bearing, though one is still good practice.

**Full touch list, confirmed by direct read (mechanical field-path insertion, `X` → `X.symmetry.` on
each named struct's own three fields only):**
- `src/params/ScatterRule_PARAMS.h` — the three struct definitions themselves; add the new include.
- `src/proc/Placement_Rules_PROC.cpp` — three call-site blocks (`AppendPropRules`, `AppendDecalRules`,
  `AppendUnitRules`), each reading `rule.bSymmetryUseGlobal`/`rule.symmetryMask`/
  `rule.radialSymmetryRepeatCount` into a `ScatterRuleConfiguration` via `ResolveSymmetryMask`/
  `ResolveRadialSymmetryRepeatCount`.
- `src/proc/Placement_Hash_PROC.cpp` — `HashPropRule` and `HashUnitRule` read
  `rule.bSymmetryUseGlobal`/`rule.symmetryMask` directly into the dirty-hash. **Note, not part of
  this ticket:** `HashDecalRule` does not hash the symmetry triplet at all today (a separate,
  pre-existing gap — do not conflate fixing it with this mechanical retrofit; if it's fixed in the
  same diff, say so explicitly rather than let it hide inside an unrelated rename).
- `src/ui/PropsTab_UI.cpp` (`DrawPlacementSymmetryAxes("propSymmetry", rule.bSymmetryUseGlobal,
  rule.symmetryMask, ...)`), `src/ui/PropsTab_Decals_UI.cpp` (same, `"decalSymmetry"`),
  `src/ui/ArmiesTab_Units_UI.cpp` (same, `"unitSymmetry"`, pointer receiver `rule->`) — three
  trivial argument-path edits; `DrawPlacementSymmetryAxes` itself (`PlacementRuleSections_UI.h/.cpp`)
  takes bare `bool&`/`int&` references and needs no change, confirmed by direct read.
- `src/io/MapExporter_{Props,Decals,Units}Stack_IO.cpp` and
  `src/io/MapImporter_{Props,Decals,Units}Stack_IO.cpp` — six files, one `rule.` → `rule.symmetry.`
  edit each on their existing `SymmetryUseGlobal`/`SymmetryMask`/`RadialSymmetryRepeatCount` lines;
  JSON key strings themselves do not change.

No `ENTITY_AUTHORING_PARAMS_SPEC.md` update needed — that spec covers the pass-through/baked
instance-data family (`PropInstanceLayer`/`DecalInstanceLayer`/`MarkerInstanceLayer`, etc.), not
`ScatterRule_PARAMS.h`'s procedural rule types, confirmed by grep (zero matches for `PropRule`/
`DecalRule`/`UnitRule` in that spec).

**Spec staleness, only partly closed this session.** `PLACEMENT_SCATTER_SPEC.md`'s "Layer-scoped
marker symmetry" section carried a now-stale sentence ("`PropRule`/`DecalRule`/`UnitRule` are
unaffected") — corrected in place this session, pointing here. `SANMAP_FORMAT_SPEC.md` Correction 4's
"confirmed live on `MarkerRule`/`PropRule`/`UnitRule`" paragraph is still accurate (those types still
carry the same fields, just regrouped in C++), but its neighboring Correction 15 closing paragraph
("`PropRule`/`DecalRule`/`UnitRule` keep the triplet exactly where it is; only `MarkerRule` loses
it") is now stale too and was **not** edited this session — that file is over 1,100 lines and a
full-file rewrite was judged too high-risk for this pass's tool constraints. Flagged here as a
standing, low-priority documentation-only follow-up for whichever ARCH session next touches
`SANMAP_FORMAT_SPEC.md`; it blocks nothing (the wire format itself did not change).
