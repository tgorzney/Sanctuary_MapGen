# STEP79 — PROC consumer update for `markerRuleLayers` (`Placement_Rules_PROC.cpp` + `Placement_Hash_PROC.cpp`)

**Layer:** PROC (+ mechanical PROC/PIPELINE test-fixture updates). **Domain:** `AppendMarkerRules`'s
two-level walk and its flat seed counter; `PlacementStage::ComputeParameterHash`'s marker-rule
hashing. **Sequence:** the PROC-consumer counterpart to `STEP66_MarkerRuleLayer_PARAMS.md`,
transcribed from `HANDOFF_TRACK_MarkerLayerSymmetry.md` §G (the only place this scope existed) and
re-verified line by line against the real code before writing.

---

## ⚠️ SINGLE DISPATCH UNIT — STEP66 + STEP79 LAND TOGETHER, NEVER SEPARATELY

**Ruled by the human.** This ticket and `STEP66_MarkerRuleLayer_PARAMS.md` are **one dispatch**, not
a sequence of two. Neither is independently landable:

- STEP66 removes `Params::MapRecipe::markerRules` (`MapRecipe_PARAMS.h:56`) and the three symmetry
  fields from `MarkerRule` (`MarkerRule_PARAMS.h:57-64`). The instant it lands alone,
  `Placement_Rules_PROC.cpp:20-21,30-33` and `Placement_Hash_PROC.cpp:52,57,113` stop compiling.
  STEP66 states this in its own Explicit-out-of-scope section and in its Acceptance test.
- STEP79 alone is meaningless: the types it consumes (`MarkerRuleLayer`, `SymmetrySetting`,
  `MapRecipe::markerRuleLayers`) do not exist until STEP66 creates them.

So: **one branch, one commit/PR.** A red intermediate build is expected *within* the unit and is not
a regression; a red build *at the unit's boundary* is. The coder must not report either ticket
"done" until both are in.

**⚠️ `STEP80_MarkersTabRulesLayerSymmetry_UI.md` must be in the same working tree for this unit to
be verifiable at all — a build-system fact, not a scope claim.** `CMakeLists.txt:142-151` GLOBs
`src/ui/*.cpp` into the **same** `SanGenV2` library as `src/proc`, and every acceptance-test
executable links that library (`CMakeLists.txt:240-241`). STEP66 breaks `src/ui`
(`MarkersTab_UI.cpp:100-101` and the `SelectedMarkerRule` call sites — STEP80's Root problem
enumerates them), so until STEP80 lands **no test binary can even be built**, including this
ticket's own. STEP80's own header sets the dispatch order STEP66 → STEP79 → **STEP80**, which is
correct as an *authoring/review* order; it is not a claim that the first two can be built and tested
without the third. Recommended handling, stated as an assumption rather than a new ruling (the
human's single-dispatch-unit ruling covers STEP66+STEP79 and is not being widened here): land all
three on one branch, keeping STEP66+STEP79 inseparable within it, and treat STEP80 as the third
commit that closes the build. If the human prefers STEP80 as a genuinely separate landing, then the
gate in Acceptance test 7 cannot be run at that boundary and must be deferred to STEP80's landing —
say so explicitly in the PR rather than reporting a green run that did not happen.

`STEP67_MarkersStackSymmetryMigration_IO.md` stays a separate, **later** landing (it depends on this
unit, not the reverse).

---

## Root problem

`Placement_Rules_PROC.cpp`'s `AppendMarkerRules` (lines 16-54) is a **flat single loop** over
`recipe.markerRules`:

```cpp
for (std::size_t index = 0; index < recipe.markerRules.size(); ++index) {        // :20
    const Params::MarkerRule& rule = recipe.markerRules[index];                  // :21
    if (!rule.bEnabled && !rule.bHidden) continue;                               // :24
    ScatterRuleConfiguration configuration = MakeCommonConfiguration(
        constants, recipe.geometry, recipe.water, rule, static_cast<int>(index), 0);  // :25-26
    ...
    configuration.symmetryMask = ResolveSymmetryMask(rule.bSymmetryUseGlobal, rule.symmetryMask,
                                                     recipe.globalSymmetryMask);      // :30-31
    const int radialSymmetryRepeatCount = ResolveRadialSymmetryRepeatCount(
        rule.bSymmetryUseGlobal, rule.radialSymmetryRepeatCount, recipe.radialSymmetryRepeatCount); // :32-33
```

Two things must change and one thing must **not**:

1. The array being walked becomes two-level (`recipe.markerRuleLayers` → `layer.rules`).
2. The symmetry triplet is sourced from `layer.symmetry.*`, not `rule.*`.
3. The value passed as `ruleIndex` (`:26`) — which is both the array position **and** the
   seed-decorrelation input — must keep the **exact same numbering it has today**. See the next
   section; this is the whole risk of this ticket.

A second consumer, easy to miss because it fails silently rather than at compile time, is
`Placement_Hash_PROC.cpp`'s `ComputeParameterHash` (`:98-118`), whose `HashMarkerRule` (`:47-67`)
reads `rule.bSymmetryUseGlobal` (`:52`) and `rule.symmetryMask` (`:57`) through its own independent
top-level loop (`:113`).

## Target files

- `src/proc/Placement_MarkerRules_PROC.cpp` — **new.** Receives `AppendMarkerRules` (moved out of
  `Placement_Rules_PROC.cpp`) plus its new per-rule helper. Required by ARCH_01_05_FileSizeCeilings.md §1.5 — see
  "File- and function-size ceilings" below; this is not a stylistic preference.
- `src/proc/Placement_RuleAppend_PROC.h` — **new, tiny.** Declares exactly one function
  (`AppendMarkerRules`) so the moved definition is reachable from `Placement_Rules_PROC.cpp`.
- `src/proc/Placement_Rules_PROC.cpp` — `AppendMarkerRules` removed; `BuildRuleConfigurations`
  (`:128-140`) now calls it through the new header. Props/units/decals untouched.
- `src/proc/Placement_Hash_PROC.cpp` — new `HashMarkerRuleLayer`; `HashMarkerRule` trimmed;
  `ComputeParameterHash`'s marker loop nested.
- `src/proc/Placement_Symmetry_PROC_Test.cpp` — fixture rewrite (**careful, not find-replace**).
- `src/proc/Placement_PROC_Test.cpp`, `src/proc/Placement_Gpu_PROC_Test.cpp` — mechanical.
- `src/pipeline/GenerationAssembler_TestScene_PIPELINE.h` — mechanical + comment correction.

**Not a target file, verified:** `src/proc/Placement_RuleBuild_PROC.h` — see "Zero-change
verification" below. **No `CMakeLists.txt` edit:** `src/proc/*.cpp` is picked up by
`file(GLOB_RECURSE ... CONFIGURE_DEPENDS)` at `CMakeLists.txt:142-146`, and every Placement test
target links the `SanGenV2` library rather than naming sources (`CMakeLists.txt:240-241`), so the
new translation unit reaches the tests automatically.

## Layer & accuracy class

PROC. Accuracy class: **Exact**, and specifically **Deterministic** (Constitution §4) — the rule
seed feeds `MakeRuleSeed` → `AvalancheHash`, and marker positions are gameplay-authoritative
(spawns/alloys). This ticket must be output-bit-identical to today's behavior for any recipe whose
flattened rule sequence matches today's flat array.

## Backend policy

CPU only. **The GPU twin needs no change and must not be touched.** `Placement_Rules_PROC.cpp`'s own
header comment states the contract — the four rule families are flattened into one
`ScatterRuleConfiguration` record and "from here on the scatter code knows nothing about rule
families." That record's layout is unchanged by this ticket, so `Placement_PROC.glsl`,
`Placement_Kernel_PROC.h`, and `Placement_Gpu_PROC.cpp` are all unaffected (verified: none of them
reads `recipe.markerRules` or any `MarkerRule` field).

## ARCH rules invoked

- `ARCH_16_01_NewParamsShapes.md` §16.1 — the ratified `MarkerRuleLayer`/`SymmetrySetting` shape
  this ticket consumes.
- `ARCH_16_10_ConsultRoutingSummary.md` §16.10 item 3 — the Generator Expert consult this ticket
  discharges. **That list is incomplete — see "Correction to ARCH_16_10_ConsultRoutingSummary.md §16.10" below.**
- `ARCH_01_05_FileSizeCeilings.md` §1.5 — soft 100 / hard 150 lines per file, functions ≤ 40 lines.
  **Binding on the shape below, and the reason a file split is mandated rather than suggested.**
- Constitution §2 — no abbreviations in the new helper/file names.
- Constitution §4 — Deterministic sub-mode of the CPU Exact path; the seed-numbering requirement
  below is this rule applied.

---

## Solution — shape

### 1. ⚠️ THE FLAT SEED COUNTER — A DETERMINISM REQUIREMENT, NOT A STYLE CHOICE

**Read this before writing a line of the loop.**

Today's `index` (`Placement_Rules_PROC.cpp:20`) is a plain `for` counter over the whole array. It is
passed at `:26` as `MakeCommonConfiguration`'s `ruleIndex`, which lands in two places
(`Placement_RuleBuild_PROC.h:75-77`):

```cpp
configuration.ruleIndex = ruleIndex;
configuration.ruleSeed  = MakeRuleSeed(constants, geometry.seed, collectionIndex, ruleIndex);
```

and `MakeRuleSeed` (`Placement_RuleBuild_PROC.h:34-40`) mixes `ruleIndex + 1` into the per-rule PRNG
seed. **Because the counter lives in the `for` header and the suppression gate at `:24` is a
`continue`, the counter advances for suppressed rules too.** A rule at flat position 5 is seeded as
rule 5 whether or not rules 0-4 were skipped.

**Required after-shape: ONE flat counter threaded through BOTH loops, incremented once per rule
encountered — including rules that are skipped, and including every rule inside a fully suppressed
layer.**

```cpp
int ruleIndex = 0;                                    // ONE counter, outside both loops
for (const Params::MarkerRuleLayer& layer : recipe.markerRuleLayers) {
    const bool bLayerSuppressed = !layer.bEnabled && !layer.bHidden;
    for (const Params::MarkerRule& rule : layer.rules) {
        const int flatRuleIndex = ruleIndex++;        // ADVANCES UNCONDITIONALLY — see above
        if (bLayerSuppressed || (!rule.bEnabled && !rule.bHidden)) continue;
        ...
    }
}
```

**What breaks if you get this wrong.** A per-layer counter that resets, or a counter incremented
only for rules that survive the gate, silently reseeds **every marker rule following a skipped
one**. Nothing fails to compile. No assertion fires. The map simply scatters differently than it did
before — for every user, on every existing `.sanmap`, permanently. This is precisely the class of
regression Constitution §4's Deterministic guarantee exists to prevent, and it is invisible to any
test that only checks marker *counts*. The acceptance test below pins it explicitly.

**Corroboration that flat order is the intended invariant, not just today's accident:**
`STEP67_MarkersStackSymmetryMigration_IO.md` migrates old flat files by **contiguous-run** grouping,
which preserves the original flat rule order exactly through the concatenation — so a migrated file
produces byte-identical flat indices. And `HANDOFF_TRACK_MarkerLayerSymmetry.md` §D records that
STEP67's optional "non-adjacent same-triplet cosmetic merge" is explicitly **not to be built**,
precisely because it would change flattened rule-execution order. Two independent tickets already
treat the flat sequence as load-bearing.

### 2. Suppression — the OR-combination is confirmed, not inferred

```cpp
(!layer.bEnabled && !layer.bHidden) || (!rule.bEnabled && !rule.bHidden)
```

`STEP66`'s header carries the human's binding ruling: **`bEnabled` on a generation layer is a real
generation gate, not a UI-only display toggle** — a disabled layer is not calculated at all, though
it stays stored. `bHidden` keeps its existing meaning (still generates, for clearance/fairness; just
not rendered) — the semantic today's `:22-23` comment already documents. So the layer tier's gate is
the identical predicate the rule tier already uses, and the two compose with OR: suppressed at
either tier ⇒ the rule does not generate.

Note the asymmetry that must be preserved: `bHidden` at the **rule** tier still sets
`ScatterSelectionFlag::Hidden` on the configuration (`:49`). This ticket does **not** propagate
`layer.bHidden` into that flag — a layer-level hidden-but-generating flag is a preview/overlay
concern (STEP51's domain), and inventing a propagation here would change emitted instance flags,
which is out of scope. `layer.bHidden`'s only role in this ticket is as the second half of the
layer-tier suppression predicate above, exactly mirroring the rule tier.

### 3. Symmetry sourcing

```cpp
configuration.symmetryMask = ResolveSymmetryMask(
    layer.symmetry.bSymmetryUseGlobal, layer.symmetry.symmetryMask, recipe.globalSymmetryMask);
const int radialSymmetryRepeatCount = ResolveRadialSymmetryRepeatCount(
    layer.symmetry.bSymmetryUseGlobal, layer.symmetry.radialSymmetryRepeatCount,
    recipe.radialSymmetryRepeatCount);
```

Field-for-field the same call, same argument order, same resolution rule — only the *source object*
moves from `rule` to `layer.symmetry`. The `bSymmetryUseGlobal` argument must be the **same** value
in both calls (`Placement_RuleBuild_PROC.h:58-61` documents why: STEP23 ruling #5 / STEP16 ruling
#2 — a local override with `bSymmetryUseGlobal == false` must not silently inherit the global N).
Do not "optimize" by hoisting only one of the two.

Everything else in the loop body (`:27-29`, `:34-52`) is untouched: category, priority mode, focus
gradient, counts, spacings, clearance radii, the reciprocal, the selection flags, and the three
`push_back`s all keep reading `rule.*`.

### 4. Zero-change verification — `Placement_RuleBuild_PROC.h`

**Verified by reading the file, per §G's claim: the claim holds.** All three functions the caller
uses are already generic and pure over their arguments —

- `ResolveSymmetryMask(bool, int, int)` (`:54-56`) — plain ternary over three scalars; no rule type.
- `ResolveRadialSymmetryRepeatCount(bool, int, int)` (`:62-64`) — likewise.
- `MakeCommonConfiguration` (`:68-91`) — a `template <typename RuleType>` reading only the gate core
  (`minHeight`/`maxHeight`/`minSlope`/`maxSlope`/`mapEdgePadding`/`maskStratumIndex`/
  `maskWeightMinimum`/`transform`), **none of which moves to the layer tier**, plus `ruleIndex` and
  `collectionIndex` as explicit `int` parameters.

`MarkerRule` still satisfies the template's requirements after STEP66 removes the symmetry triplet,
because the template never touched that triplet. The file is **not** edited by this ticket, and the
acceptance test below requires proving it byte-identical.

### 5. ⚠️ File- and function-size ceilings — where §G's spec does not survive contact with the code

**This is the one place the handoff's specification is under-specified against ARCH_01_05_FileSizeCeilings.md §1.5, and the
coder must not discover it mid-edit.**

Measured on disk today:

| | today | after a naive in-place edit | §1.5 ceiling |
|---|---|---|---|
| `AppendMarkerRules` (`:16-54`) | **39 lines** | ~42-45 | **≤ 40 (hard)** |
| `Placement_Rules_PROC.cpp` | **143 lines** | ~154 | **150 (hard)** |

Adding a second loop level, the layer-suppression local, and the counter to a function already
sitting **one line** under the function ceiling, in a file sitting **seven lines** under the file
ceiling, breaches both. §1.5 permits exceeding a ceiling only via a *documented work-order
exception* — "a deliberate ratchet, never silent drift." **This ticket does not grant one.** It
mandates the split instead, which §1.5 itself prescribes ("a large class splits its method
definitions across multiple `.cpp` files … behind one small header"):

**`src/proc/Placement_RuleAppend_PROC.h`** (new, ~20 lines) — declares one function:

```cpp
// Placement_RuleAppend_PROC.h — the marker family's rule-flattening entry point, split out of
// Placement_Rules_PROC.cpp so the two-level markerRuleLayers walk fits the §1.5 ceilings.
#pragma once
#include "Placement_PROC.h"

namespace SanmapGen { namespace Proc {

void AppendMarkerRules(const PlacementConstants& constants, const Params::MapRecipe& recipe,
                       std::vector<ScatterRuleConfiguration>& configurations,
                       std::vector<Data::TemplateIdentifier>& identifiers,
                       std::vector<int>& radialSymmetryRepeatCounts);

} }
```

**`src/proc/Placement_MarkerRules_PROC.cpp`** (new, ~68 lines — comfortably under the *soft* 100)
holds two functions, both under 40 lines:

- `AppendMarkerRuleConfiguration(...)` (file-local, in the anonymous namespace, ~33 lines) — today's
  loop **body** verbatim (`:25-52`) with the two symmetry calls re-sourced per §3, taking
  `const Params::MarkerRule& rule`, `const Params::SymmetrySetting& symmetry`, and
  `int flatRuleIndex` as parameters.
- `AppendMarkerRules(...)` (~17 lines) — the two-level walk of §1 plus the gate of §2, calling the
  helper. `AppendMarkerRules` **loses** its anonymous-namespace internal linkage (it is now declared
  in the header above); the helper keeps its.

**`src/proc/Placement_Rules_PROC.cpp`** drops from 143 to ~104 lines (still above the soft ceiling,
but a net improvement and well clear of the hard one), keeps `AppendPropRules`/`AppendUnitRules`/
`AppendDecalRules` in its anonymous namespace **byte-identical**, adds
`#include "Placement_RuleAppend_PROC.h"`, and leaves `BuildRuleConfigurations` (`:128-140`)
textually unchanged — its `AppendMarkerRules(...)` call at `:132-133` resolves through the new
header instead of the local anonymous namespace.

**Rejected alternative, recorded so it is not re-proposed:** keep everything in one file and take a
documented §1.5 exception for a ~42-line function in a ~154-line file. Rejected — §1.5's stated
rationale is per-edit token cost and mis-match risk, and this is the *most* determinism-sensitive
function in the placement stage; it is the last file that should be allowed to drift past the
ceiling. The split also isolates the marker family's now-unique two-level shape from the three
families that stay flat, which is honest about the divergence rather than hiding it.

**Naming flagged for one-line ARCH confirmation before dispatch** (same posture STEP68 uses for its
connector filename, and for the same reason — file naming is ARCH's call, not the Generator
Expert's): `Placement_MarkerRules_PROC.cpp` / `Placement_RuleAppend_PROC.h` /
`AppendMarkerRuleConfiguration`. The direction (split, don't except) is settled; only the exact
spellings are proposals. Existing siblings the names were matched against: `Placement_Rules_PROC.cpp`,
`Placement_RuleBuild_PROC.h`, `Placement_Accept_PROC.cpp`, `Placement_Emit_PROC.cpp`.

---

### 6. `Placement_Hash_PROC.cpp` — the second consumer, and the one that fails SILENTLY

`PlacementStage::ComputeParameterHash()` (`:98-118`) is the dirty-hash key PIPELINE reads. Its
marker path is completely independent of `Placement_Rules_PROC.cpp` — its own loop at `:113`, its
own field reads. **If it is not migrated in lockstep, the build still goes green and the dirty hash
silently stops reacting to marker-symmetry-layer edits**: the user changes a layer's symmetry and the
preview does not regenerate. A stale-preview bug, not a compile error. This is the single easiest
thing in the unit to miss.

**After-shape:**

```cpp
// trimmed — the two symmetry reads move up to HashMarkerRuleLayer
std::size_t HashMarkerRule(std::size_t seed, const Params::MarkerRule& rule) {
    seed = HashGateCore(seed, rule);
    seed = HashInteger(seed, (rule.bEnabled ? 1 : 0) | (rule.bHidden ? 2 : 0)
                           | (rule.bUseDensity ? 4 : 0) | (rule.bUseAllPositions ? 8 : 0)
                           | (rule.bRandomSelection ? 16 : 0) | (rule.bCheckMaximumRadius ? 32 : 0));
    //                       ^ the `| (rule.bSymmetryUseGlobal ? 64 : 0)` term at :52 is REMOVED
    ...                   // :53-56 unchanged
    // the `seed = HashInteger(seed, rule.symmetryMask);` line at :57 is REMOVED
    ...                   // :58-66 unchanged
}

std::size_t HashMarkerRuleLayer(std::size_t seed, const Params::MarkerRuleLayer& layer) {
    seed = HashInteger(seed, (layer.bEnabled ? 1 : 0) | (layer.bHidden ? 2 : 0)
                           | (layer.symmetry.bSymmetryUseGlobal ? 4 : 0));
    seed = HashInteger(seed, layer.symmetry.symmetryMask);
    seed = HashInteger(seed, layer.symmetry.radialSymmetryRepeatCount);
    for (const Params::MarkerRule& rule : layer.rules) seed = HashMarkerRule(seed, rule);
    return seed;
}
```

and the top-level loop at `:113` becomes:

```cpp
for (const Params::MarkerRuleLayer& layer : recipe.markerRuleLayers)
    hash = HashMarkerRuleLayer(hash, layer);
```

**Three notes the coder must not deviate from:**

- **`layer.symmetry.radialSymmetryRepeatCount` MUST be hashed.** Today's `HashMarkerRule` hashes
  `bSymmetryUseGlobal` and `symmetryMask` but **not** `radialSymmetryRepeatCount` — a pre-existing
  gap (a rule-local N change does not currently dirty the hash). Since this ticket rewrites the
  marker hash path anyway, closing that gap here costs one line and removes a real stale-preview
  case. This is a deliberate, small behavior improvement, called out rather than smuggled in; the
  hash value is not an API and changes wholesale in this ticket regardless.
- **`HashGateCore` (`:35-45`) is untouched.** It reads only gate-core fields
  (`minSlope`/`maxSlope`/`minHeight`/`maxHeight`/`mapEdgePadding`/`maskStratumIndex`/
  `maskWeightMinimum`/`transform`) — none of which moved tiers. Verified by reading it.
- **`HashPropRule` (`:69-78`), `HashUnitRule` (`:80-87`), `HashDecalRule` (`:89-94`) are
  untouched**, along with their three loops at `:114-116`. Those domains keep the flat per-rule
  shape and their own per-rule symmetry triplets. Do not "unify" them in this ticket.

Size check: `Placement_Hash_PROC.cpp` goes 121 → ~130 lines (under the 150 hard ceiling);
`ComputeParameterHash` 21 → ~23 lines and `HashMarkerRule` 21 → 19 lines (both well under 40). No
split needed here.

---

### 7. Test fixtures that break mechanically

**The grep was re-run for this ticket. §G's PROC/PIPELINE set is still complete and correct** — no
file under `src/proc/` or `src/pipeline/` other than the two consumers above and the four fixtures
below touches `recipe.markerRules` or `MarkerRule`'s symmetry triplet. **§G's list is, however,
incomplete on the PARAMS side by one file** — see the note at the end of this section.

- **`src/proc/Placement_Symmetry_PROC_Test.cpp` — CAREFUL REWRITE, NOT FIND-REPLACE.** This is the
  file that exercises the exact local-override-vs-global bug STEP16/STEP23 guard against, and it
  asserts *specific resolved values*, not just "it ran":
  - `MakeSymmetricRecipe` (`:24-45`) sets `spawnRule.bSymmetryUseGlobal = true` (`:39`) and pushes
    at `:41` → becomes one `MarkerRuleLayer` with `layer.symmetry.bSymmetryUseGlobal = true`
    holding the one rule.
  - **Acceptance test 8 (`:321-361`) is the load-bearing one.** It sets a **global**
    `radialSymmetryRepeatCount = 4` (`:334`) and a **rule-local** override —
    `bSymmetryUseGlobal = false` (`:344`), `symmetryMask = Radial` (`:345`),
    `radialSymmetryRepeatCount = 9` (`:346`) — then asserts the placed orbit count divides evenly
    by **9**, not 4 (`:355-361`). Those three lines must move onto the *layer's*
    `SymmetrySetting`, with the rule pushed into that layer's `rules`. The assertion, the counts,
    and the "% 9 == 0" reasoning stay exactly as they are — **the test's meaning must survive the
    move verbatim.** If it is rewritten in a way that no longer distinguishes 9 from 4, the STEP16
    regression guard is gone and nobody will notice.
  - Test 9's throughput fixture mutates `markerRules[0]` in place at `:370-371` and `:375-376` →
    becomes `markerRuleLayers[0].rules[0]`.
- **`src/proc/Placement_PROC_Test.cpp`** — one spawn rule (`:31-40`), pushed at `:40`. Mechanical.
  Its comment at `:24-28` explains that the fixture pins `globalSymmetryMask = None` because the
  spawn rule relies on `bSymmetryUseGlobal == true` (its default); **update that comment** to name
  the layer's `symmetry.bSymmetryUseGlobal` default instead — a stale comment here is what would
  mislead the next person debugging a marker-count assertion.
- **`src/proc/Placement_Gpu_PROC_Test.cpp`** — one Alloys rule (`:62-71`), pushed at `:71`.
  Mechanical; same comment correction at `:54-60`.
- **`src/pipeline/GenerationAssembler_TestScene_PIPELINE.h`** — the M3-8 end-to-end fixture. Builds
  a spawn rule at `:78-85`, pushed at `:85`. Its comment at `:70-74` explicitly ties
  `bSymmetryUseGlobal == true` (the default) to **`GenerationAssembler_Outputs_PIPELINE_Test.cpp`'s
  exact-count assertion**. The default carries over unchanged (`SymmetrySetting::bSymmetryUseGlobal`
  defaults to `true`, per ARCH_16_01_NewParamsShapes.md §16.1 and STEP66), so the exact count is preserved — but the comment
  must be updated to point at the layer, and the coder must **confirm that exact-count assertion
  still passes** rather than assume it.

**Owned by STEP66, not this ticket, but breaking in the same landing** (cite in the PR so nobody
hunts for an untracked break):

- `src/params/MapRecipe_PARAMS_Test.cpp:16,22` — `recipe.markerRules.resize(3)` and the
  `markerRules.size() != 3` check.
- **`src/params/PlacementRules_PARAMS_Test.cpp:13,17` — NOT named in handoff §G; found by this
  ticket's own grep.** It constructs a bare `MarkerRule marker;` and asserts
  `!marker.bSymmetryUseGlobal` at `:17` — a direct read of a field STEP66 deletes. Hard compile
  break, PARAMS territory. **STEP66's target-file list does not currently mention it either**, so
  without this note it is an untracked break in the joint landing.

UI-layer consumers (`MarkersTab_UI.cpp`, `MarkersTab_Rules_UI.*`, `Application_Recipe_UI.cpp`,
`Application_AssetPanel_UI.cpp`, and the UI tests) also break; STEP66's Explicit-out-of-scope
section already flags that as a known gap with no ticket yet, and this ticket does not claim it.

---

## ⭐ Downstream authority ruling — the flat-`ruleIndex` assumption is **CONFIRMED**

`STEP50_ProceduralSubLayerCsrBucketIndex_UI.md` and `STEP51_OverlayLayerDataModel_UI.md` both carry
a ⚠️ UNCONFIRMED flag naming *this* ticket as the authority. **This section is that authority's
answer. Both flags may be cleared. Neither ticket's code needs to change.** (Those files are not
edited by this ticket — their owners clear their own flags.)

**Verdict: CONFIRMED.** `ruleIndex` remains a **flat, running index over the layer-concatenated rule
sequence**, exactly as both tickets assumed. It is *not* per-layer-local, and it does *not* reset at
a layer boundary. §1 above makes preserving that numbering a hard determinism requirement, so it is
now the strongest-guaranteed property in the marker path, not merely the incidental status quo.

Three consequences, each checked against the two tickets' actual code:

1. **`STEP50`'s `BuildRuleBucketIndex` (its lines 194-201) is correct as written.** It computes
   `markerRuleCount` by summing `layer.rules.size()` across all layers, including disabled ones.
   That is exactly the right `bucketTotal`: this ticket's counter advances for suppressed rules, so
   the maximum `ruleIndex` any emitted instance can carry is `(total rules across all layers) - 1`.
   Summing only *enabled* rules would have under-sized the bucket array and produced
   out-of-range keys.
2. **`STEP51`'s `SeedMarkerDomains` (its lines 198-209) is correct as written.** Its `flatIndex`
   increments once per rule across the nested walk with **no enable/hide filtering** — the same
   index space this ticket produces. A sub-layer ref built there will match the `ruleIndex` column
   on emitted instances (`Placement_Emit_PROC.cpp:68` writes `instance.ruleIndex =
   configuration.ruleIndex` into `PlacementInstance_DATA.h:46`).
3. **`ruleIndex` remains per-*family*, which STEP50 already documents correctly** (its line 39,
   "`ruleIndex` is per-family, not a global index"). This ticket changes nothing there: markers
   still start at 0 with `collectionIndex = 0`, and props/units/decals keep their own independent
   0-based sequences with `collectionIndex` 1/2/3 (`Placement_Rules_PROC.cpp:64,90,112`). "Flat" in
   this ruling means *flat across layers within the marker family*, never *global across families*.

**Standing constraint this creates for future work, stated so it is not silently violated:** any
later ticket that changes marker rule ordering, introduces per-layer-local rule indices, or filters
rules out of the flat counting sequence must treat this ruling — and STEP50/STEP51's code — as
consumers to update, and must re-derive the determinism argument in §1.

---

## Correction to `ARCH_16_10_ConsultRoutingSummary.md` §16.10 — the routing list is incomplete

§16.10 item 3 routes to the Generator Expert: "confirm **`Placement_Rules_PROC.cpp`**'s marker
symmetry-resolution call site can walk the new `MarkerRuleLayer` tier … (mechanical once §16.1/§16.6
are wired)."

Two corrections, offered as a **finding for the ARCH Expert** (only the ARCH Expert writes ARCH
files; this ticket does not amend §16.10):

1. **It names one PROC consumer; there are two.** `Placement_Hash_PROC.cpp` is a fully independent
   consumer with its own loop and its own field reads (§6 above). It is the more dangerous of the
   two precisely because it does not appear as a compile error. Anyone scoping this work from
   §16.10's list alone would ship the stale-preview bug.
2. **"Mechanical" understates it.** The seed-numbering invariant of §1 is a silent-determinism
   hazard, and the §1.5 ceiling breach of §5 forces a file split. Neither is mechanical, and both
   are invisible from the ARCH's vantage point without reading the two functions.

---

## Solution — estimate

No benchmark required (Constitution §7 permits this for a change with no algorithmic delta). The
work per marker rule is identical to today's — same `MakeCommonConfiguration` call, same two resolve
calls, same three `push_back`s. The only added cost is one extra loop level over
`recipe.markerRuleLayers` (typically single-digit) and one boolean per layer, against a stage whose
real cost is the Poisson/clearance scatter downstream, not rule flattening. The hash path adds three
`HashInteger` calls per layer and removes two per rule — a wash at any realistic rule count.
Determinism posture is unchanged: same `AvalancheHash`, same `MakeRuleSeed`, same inputs.

## Explicit out-of-scope

- **`src/params/**` and `src/io/**`** — every PARAMS type change (`SymmetrySetting`,
  `MarkerRuleLayer`, `MapRecipe::markerRuleLayers`) and the `MarkersStack` exporter/importer rewrite
  belong to **STEP66**, landing in the same dispatch unit. This ticket consumes those types; it does
  not define them. `MapRecipe_PARAMS_Test.cpp` and `PlacementRules_PARAMS_Test.cpp` are STEP66's to
  fix.
- **`MarkersStack_Migrate_V<N>_IO` (old-file backward compatibility)** — `STEP67`, lands **after**
  this unit. Nothing here reads or writes a `.sanmap`.
- **`Placement_RuleBuild_PROC.h`** — not modified. See §4; proving it unmodified is an acceptance
  criterion.
- **`HashPropRule`/`HashUnitRule`/`HashDecalRule` and `AppendPropRules`/`AppendUnitRules`/
  `AppendDecalRules`** — props, units and decals keep their flat arrays and per-rule symmetry
  triplets. No unification, no layer tier, not even a "while we're here" tidy.
- **The GPU path** — `Placement_PROC.glsl`, `Placement_Kernel_PROC.h`, `Placement_Gpu_PROC.cpp`
  unmodified; the `ScatterRuleConfiguration` contract they consume is unchanged.
- **Propagating `layer.bHidden` into `ScatterSelectionFlag::Hidden`** — see §2. A layer-level
  hidden-but-generating flag is STEP51/overlay work.
- **Any UI tab change** — `MarkersTab_UI.cpp:100-101`'s per-rule `DrawPlacementSymmetryAxes` call and
  the `SelectedMarkerRule` call sites in `Application_AssetPanel_UI.cpp:47` /
  `Application_Recipe_UI.cpp:62` all break under STEP66. **That work is owned by
  `STEP80_MarkersTabRulesLayerSymmetry_UI.md`, which now exists** (it was the untracked gap STEP66's
  out-of-scope bullet 4 and handoff §C.3 flagged). This ticket does not touch `src/ui/**` — but see
  the dispatch-unit banner: STEP80 must be in the same working tree for anything here to build.
- **Editing `STEP50`/`STEP51`** — this ticket rules on their assumption; their owners clear their
  own ⚠️ flags.
- **Editing `ARCH_16_10`** — ARCH Expert's file. The correction above is a finding, not an edit.

## Acceptance test

1. **⭐ Seed-numbering equivalence (the determinism guard — write this test first).** Build two
   recipes with identical geometry/seed: (a) today's flat shape with **five** marker rules where
   rules 1 and 3 are suppressed (`bEnabled = false`, `bHidden = false`); (b) the new shape with the
   same five rules distributed across **three** layers (e.g. 2 / 1 / 2), all layers enabled, same
   order. Assert the surviving configurations carry `ruleIndex` **0, 2, 4** — not 0, 1, 2 — and that
   their `ruleSeed` values match `MakeRuleSeed(constants, seed, 0, {0,2,4})` exactly. Then add a
   fourth, fully disabled layer holding 2 rules **in the middle** of the sequence and assert the
   rules after it are seeded 6 and 7 (the counter advanced through the suppressed layer), not 4
   and 5. **A test that only checks marker counts does not catch this and does not satisfy this
   item.**
2. **Suppression matrix.** Assert no configuration is emitted for: a rule in a disabled+visible
   layer; a disabled+visible rule in an enabled layer; and a disabled-but-`bHidden` rule in an
   enabled layer **is** emitted (with `ScatterSelectionFlag::Hidden` set) — the existing
   hidden-still-generates semantic must survive at both tiers.
3. **Symmetry resolution at the layer tier.** Port `Placement_Symmetry_PROC_Test.cpp`'s acceptance
   test 8 to the layer shape and confirm it still asserts the **local** `radialSymmetryRepeatCount`
   (9) wins over the **global** (4) — the orbit count must still divide by 9. Add the mirror case:
   `layer.symmetry.bSymmetryUseGlobal = true` resolves to the global mask and global N.
4. **Dirty-hash reactivity (the silent-failure guard).** Take one recipe; flip **only**
   `markerRuleLayers[0].symmetry.symmetryMask`, then only `.bSymmetryUseGlobal`, then only
   `.radialSymmetryRepeatCount`, then only `.bEnabled`, then only `.bHidden`. Assert
   `ComputeParameterHash()` differs from the baseline in **all five** cases. Also assert two
   recipes differing only in how the same rules are *distributed* across layers (2/1 vs 1/2, same
   symmetry settings on every layer) produce **different** hashes — layer structure is a real input.
5. **`Placement_RuleBuild_PROC.h` byte-identical** before/after (`git diff --stat` shows it
   untouched, not merely "tests pass"). Same for `Placement_PROC.glsl`, `Placement_Kernel_PROC.h`,
   `Placement_Gpu_PROC.cpp`, and the prop/unit/decal `Append*`/`Hash*` functions.
6. **§1.5 conformance, measured not assumed.** `wc -l` every file this ticket touches or creates;
   assert no file exceeds 150 lines and no function exceeds 40 lines. Specifically:
   `Placement_MarkerRules_PROC.cpp` ≤ 100, `Placement_Rules_PROC.cpp` ≤ 150 (expected ~104),
   `Placement_Hash_PROC.cpp` ≤ 150 (expected ~130), `Placement_RuleAppend_PROC.h` ≤ 100.
7. **The joint-landing gate.** With STEP66 + STEP79 **and STEP80 in the working tree** (see the
   dispatch-unit banner — without STEP80 the `SanGenV2` library does not build, so items 1-6 cannot
   be executed either): full `SanGenV2` build clean, **all** existing tests pass — including
   `GenerationAssembler_Outputs_PIPELINE_Test`'s exact marker-count assertion (§7's TestScene note)
   and both `Placement_PROC_Test` and `Placement_Gpu_PROC_Test`. Neither STEP66 nor STEP79 may be
   reported complete on its own.
