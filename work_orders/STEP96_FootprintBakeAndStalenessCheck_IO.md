# STEP96 — `FootprintBakeAndStalenessCheck_IO`: the ARCH_18_02_IngestedDataDeterminism.md §18.2 bake action, plus staleness detection

*Constitution §7. Authored jointly by the SanGen Format Expert (IO/wire-mapping half) and the SanGen
Generator Expert (PARAMS-home/scatter-consumer half), per `ARCH_18_02_IngestedDataDeterminism.md`'s
own closing paragraph naming both. Executor: SanGen Coder, once its sequencing dependencies (below)
are real.*

**Layer:** PARAMS (two new fields + one new fingerprint type) + IO (`.sanmap` wire mapping, the
staleness comparison) + UI (the bake button, the staleness warning surface). **Domain:** closes
`ARCH_18_02_IngestedDataDeterminism.md`'s explicitly-routed-but-unspecified "actual ticket that adds
the bake action," and the staleness follow-on the human raised in the same breath. **No PROC file is
touched anywhere in this ticket** — see "Explicit out-of-scope."

## Sequencing — read this before dispatching

**This ticket needs `work_orders/DESIGN_SantpFootprintIngestion_R1.md` §7 tickets 85–89 to exist as
real code first.** None of them are written yet (highest real ticket on disk today is STEP95; 85–89
are enumeration-only in the design doc). Every citation below to `Io::TemplateIngestReport`,
`Io::TemplateIngestCache_IO`, or the ingestion pipeline is marked **"per design doc, not yet real
code"** and cited by its design-doc shape, never by a `file:line` that does not exist — the same
disclosed-forward-citation practice `work_orders/STEP94_MarkerDragAndFollowSymmetry_UI.md` uses for
its own unbuilt prerequisites (STEP47/48/49/68). This ticket also needs **STEP58** (real, shipped —
`src/io/WorldFootprintSizeTable_IO.h`) for the `kDefaultPropFootprintSize`/`kDefaultUnitFootprintSize`
values this ticket's new PARAMS defaults duplicate, and **tickets 90/91** (AppSettings ingestion state
+ System-tab controls, also design-doc-only) to actually populate a live `Io::TemplateIngestReport`
the UI can hand to this ticket's bake button and staleness check. **Writing this ticket's spec is not
blocked on any of that landing — only dispatching it to the Coder is.** Landing order:
`85 → 86 → 87 → 88 → 89 → 90/91 → this ticket`.

**One shape addition this ticket makes on top of the design doc**, flagged plainly because ticket 89
is not written yet and therefore cannot conflict with it: the design doc's §3.4 file table gives
`TemplateIngest_IO`'s orchestrator an output type `Io::TemplateIngestReport` but never specifies a
per-`templateIdentifier` lookup on it — it only says the orchestrator "populate[s] STEP58's
`WorldFootprintSizeTable` as a second producer." `WorldFootprintSizeTable::Resolve()` (STEP58, real
code, `src/io/WorldFootprintSizeTable_IO.h:109-117`) is the wrong accessor for baking: it silently
folds "found in the real templates" and "fell back to a domain default" into one return value with no
way to tell which happened, and it carries no per-entry fingerprint at all. **This ticket therefore
specifies a required addition to ticket 89's `TemplateIngestReport`:**

```cpp
// Per design doc §3.4's TemplateIngest_IO shape, WITH this ticket's required addition —
// flag to ticket 89's own dispatch, not built here.
struct TemplateFootprintRecord {
    float baseFootprintWidth  = 0.0f;
    float baseFootprintDepth  = 0.0f;
    Io::SourceFingerprint sourceFingerprint;   // AssetAtlasCache_IO.h:38-48's real, shipped shape —
                                                // ticket 88's own cache is already modelled on it
                                                // per the design doc §4.2, so this is not a new shape,
                                                // just a new place it is surfaced.
};
// Returns nullptr when templateIdentifier was not found in the current ingestion (never ingested,
// projectile/marker root table, or template genuinely absent — §1.5 trap 3 of the design doc).
const TemplateFootprintRecord* TemplateIngestReport::FindByTemplateIdentifier(
    const std::string& templateIdentifier) const;
```

This is the same class of "one small addition the cited ticket's own text was always going to need"
that `STEP82_ArmySpawnMarkerValidation_IO.md`'s `Warn`/`warningCount` mirror-onto-`MapExportResult`
already models — a symmetry repair on an unbuilt shape, not an invention.

## Required reading
- `ARCH_18_02_IngestedDataDeterminism.md` — read in full; every design choice below traces to one of
  its five numbered rules, cited by number throughout.
- `work_orders/DESIGN_SantpFootprintIngestion_R1.md` — §3.4 (file set), §4.1/§4.2 (flow + the
  `AssetAtlasCache`-modelled fingerprinted disk cache this ticket reuses, not reinvents), §5
  (STEP58 supersession — unaffected by this ticket), §7 (ticket 85–91 shapes).
- `work_orders/STEP73_ScenarioAlloyRosterRender_IO.md` §0 and
  `work_orders/STEP82_ArmySpawnMarkerValidation_IO.md` — the house warning shape: loud, aggregate
  (one warning naming every offending entity, not one per entity), non-blocking, never auto-fixes.
  Reused verbatim below, not reinvented.
- `work_orders/STEP58_WorldFootprintSizeTable_IO.md` — the placeholder table this ticket's bake value
  source (`TemplateIngestReport`, once real) eventually supersedes, without STEP58's own file being
  edited by this ticket at all. `kDefaultPropFootprintSize{4.0f,4.0f}` /
  `kDefaultUnitFootprintSize{2.0f,2.0f}` are duplicated (as literals, not an include — see "Why the
  defaults are duplicated, not shared") into this ticket's new PARAMS defaults.
- `src/params/ScatterRule_PARAMS.h`, `src/params/ScatterTransform_PARAMS.h` — read in full; this is
  where scatter spacing is authored today (see next section).
- `src/io/AssetAtlasCache_IO.h:38-48` — the real, shipped `Io::SourceFingerprint{sourcePath, byteSize,
  modifiedTime, contentHash}` shape this ticket's PARAMS-side fingerprint type mirrors field-for-field.

## Why this ticket exists
`ARCH_18_02_IngestedDataDeterminism.md` §18.2 ruled that ingested `.santp` footprint data may only
ever reach generation through a one-shot, human-triggered bake into an ordinary `PARAMS` field —
never a live read from `Io::WorldFootprintSizeTable` or any ingestion type. Its closing paragraph
explicitly leaves "the actual ticket that adds the bake action (the `PARAMS` field, its wire mapping,
and the UI trigger)" unspecified, routing it to the Format Expert and Generator Expert jointly. The
human's concrete motivating case is real: scatter needs a template's true ground-plane extent to space
adjacent props/units without overlap, and today `PropRule`/`UnitRule` only carry a hand-typed
`spacingMinimum` (Poisson-disk radius) with no connection to any real object size.

Separately, the human immediately raised a follow-on concern §18.2 permits but does not solve: once a
value is baked, a later game patch can change the source template's real footprint with nothing
telling the designer their baked value has gone stale — generation silently keeps using an outdated
number. §18.2 rule 3 forbids ever auto-re-baking (the value must stay an ordinary, freely-overridable
tweakable), so staleness must be *detected and surfaced*, never *silently corrected*. This ticket
specifies both halves together, per the human's own framing — staleness detection is meaningless
without the bake action underneath it, and a bake action with no staleness signal is a half-finished
mechanism.

## 1. The `PARAMS` home — `Params::PropRule` and `Params::UnitRule`, not `DecalRule`/`MarkerRule`

`src/params/ScatterRule_PARAMS.h` already shows exactly how scatter spacing is authored today: `PropRule`,
`DecalRule`, and `UnitRule` each carry a hand-typed `spacingMinimum` (Poisson-disk radius, "in cells" —
i.e. already a spacing *value*, not a footprint size), and `PropRule` additionally carries
`obstacleDistanceMinimum` (Jump-Flood exclusion from terrain obstacles — a different concept, distance
to cliffs/water, not to other placed instances). None of the three existing spacing fields is
footprint-derived; a designer picks `spacingMinimum` by feel with no connection to the actual size of
whatever `ScatterTransform::templateIdentifier` (`src/params/ScatterTransform_PARAMS.h:23`) names.

**The new fields go on `PropRule` and `UnitRule` only** — the human's stated use case is "spacing
adjacent props/units," and both types already carry exactly one `templateIdentifier` per rule (via
their shared `ScatterTransform transform` member), which is the natural bake key: one rule scatters
instances of one template, so one bake writes one footprint value onto that rule.
- **`DecalRule` is excluded.** Decals are cosmetic-only (`PLACEMENT_SCATTER_SPEC.md`: "decals aren't
  previewed," and nowhere in the codebase are decals collidable or pathing-relevant) — there is no
  overlap-to-avoid for a decal, so a footprint field would be dead weight, not total-tweakability.
- **`MarkerRule` is excluded.** It already has its own `clearanceSpacing` concept
  (`MarkerRule_PARAMS.h:36`) for a different reason (marker-to-marker clearance for gameplay
  resource/spawn placement, not template-footprint overlap), and markers are not the human's stated
  case. Extending this ticket to markers would be scope creep against an unstated need.

**New fields, added to both `PropRule` and `UnitRule`:**
```cpp
// The real, ingested ground-plane extent for transform.templateIdentifier, UNSCALED — this is the
// per-template "base" size before ScatterTransform::scaleMinimum/scaleMaximum's per-instance scale
// multiplier is applied (ARCH_18_02_IngestedDataDeterminism.md §18.2 doesn't rule on this multiplication; a future PROC ticket that
// consumes this field must apply the instance's chosen scale itself, not assume this value already
// includes it). "base" mirrors STEP58's own baseFootprintWidth/baseFootprintDepth naming
// (WorldFootprintSizeTable_IO.h) for exactly the same reason STEP58 chose it.
// Ordinary, hand-editable PARAMS value (ARCH_18_02_IngestedDataDeterminism.md §18.2 rule 3) — the
// bake action below only ever fills in a STARTING value; nothing locks it afterward.
float baseFootprintWidth = 4.0f;   // PropRule: kDefaultPropFootprintSize.x (WorldFootprintSizeTable_IO.h:90)
float baseFootprintDepth = 4.0f;   // PropRule: kDefaultPropFootprintSize.y
// (UnitRule's defaults are 2.0f/2.0f — kDefaultUnitFootprintSize, WorldFootprintSizeTable_IO.h:89)

// Empty/zeroed (FootprintBakeFingerprint::IsValid() == false) means "never baked" — an ordinary,
// non-error state (ARCH_18_02_IngestedDataDeterminism.md §18.2 rule 4). Populated only by the bake action below; compared, never
// auto-rewritten, by the staleness check below.
FootprintBakeFingerprint footprintBakeFingerprint;
```

**Why the defaults duplicate STEP58's constants as literals instead of including them:** Constitution
§1 layering is `IO → {DATA, PARAMS}`, never the reverse — `src/params/*.h` must not `#include` any
`src/io/*.h`. `kDefaultPropFootprintSize{4.0f,4.0f}`/`kDefaultUnitFootprintSize{2.0f,2.0f}` are IO-layer
constants (`WorldFootprintSizeTable_IO.h:89-91`); this ticket's PARAMS defaults are the same numeric
values, spelled as literals, with a comment citing their IO-layer origin for anyone auditing drift —
this is exactly what ARCH_18_02_IngestedDataDeterminism.md §18.2 rule 4 means by "keeps whatever default it already has today (STEP58's
`kDefault*FootprintSize` class of constants)": the *values* carry over, not a dependency.

### 1.1 The fingerprint type — `Params::FootprintBakeFingerprint`, a deliberate mirror, not a shared type

NEW `src/params/FootprintBakeFingerprint_PARAMS.h` (new file — keeps `ScatterRule_PARAMS.h`'s own
growth minimal, one small tightly-coupled type, well under the 100-line soft ceiling):
```cpp
// FootprintBakeFingerprint_PARAMS.h — a source snapshot captured at bake time, compared later to
// detect staleness (ARCH_18_02_IngestedDataDeterminism.md; work_orders/STEP96...).
// Layer: PARAMS. Deliberately field-for-field IDENTICAL to Io::SourceFingerprint
// (src/io/AssetAtlasCache_IO.h:38-48, real shipped code) but NOT the same type and NOT shared via
// include -- Constitution §1 layering is IO -> {DATA, PARAMS}, never the reverse, so PARAMS cannot
// depend on an IO header. This is intentional duplication of a SHAPE, not of a TYPE.
#pragma once
#include <cstdint>
#include <string>

namespace SanmapGen {
namespace Params {

struct FootprintBakeFingerprint {
    std::string   sourcePath;
    std::uint64_t byteSize     = 0;
    std::uint64_t modifiedTime = 0;
    std::uint64_t contentHash  = 0;

    // Empty sourcePath == never baked (ARCH_18_02_IngestedDataDeterminism.md §18.2 rule 4's "not an error state").
    bool IsValid() const { return !sourcePath.empty() && byteSize > 0; }
};

} // namespace Params
} // namespace SanmapGen
```
No `Matches()` member here on purpose — the comparison this ticket needs is cross-type (a
`Params::FootprintBakeFingerprint` against an `Io::SourceFingerprint`, once ticket 89 exists), so it
lives as a free function in IO (§3 below), the legal dependency direction, not duplicated again here.

### 1.2 `EDIT src/params/ScatterRule_PARAMS.h`
Add `#include "FootprintBakeFingerprint_PARAMS.h"`; add the four members from §1 to `PropRule` (after
`nearCliffDistanceMaximum`, before the symmetry block) and to `UnitRule` (after `maskWeightMinimum`,
before the symmetry block) with their type-specific defaults. `DecalRule` is untouched.

**⚠️ ARCH_01_05_FileSizeCeilings.md §1.5 soft-ceiling note, documented per Constitution §7:** the file is 92 lines today; this
addition (2 structs × ~6 lines each, plus the new include) lands it at roughly 105–108 lines —
**over the 100-line soft ceiling, comfortably under the 150-line hard ceiling.** No exception request
needed at the hard-ceiling level (§1.5 requires one only for exceeding a ceiling, and only the soft
one is touched); the Coder should report the real final count and split `FootprintBakeFingerprint`
out (already done, §1.1) is the mitigation already taken — no further split is prescribed here.

## 2. The bake action — a discrete, per-rule UI button, never automatic

**Where:** `src/ui/PropsTab_Rules_UI.cpp` (which already draws `PropRule`'s fields, including
`spacingMinimum` at line 25) and `src/ui/ArmiesTab_Units_UI.cpp` (same, `UnitRule`, line 72). Both
files already invoke the shared `templateIdentifier` text box indirectly through
`PlacementRuleSections_UI.cpp:90`'s `ImGui::InputText("Template Id (tpId)", transform.templateIdentifier, ...)`
— **the new button is NOT added inside that shared function**, because `baseFootprintWidth`/
`baseFootprintDepth`/`footprintBakeFingerprint` live on `PropRule`/`UnitRule`, not on the shared
`ScatterTransform` that function edits, and `DecalRule`/`MarkerRule` also call through that same
shared function without gaining a bake button (§1's exclusion). The button is added at each of the
two type-specific call sites instead, immediately after that call, so it sits visually beside the
Template Id field it reads without widening the shared widget's contract.

**Behavior, exactly:**
```cpp
// PropsTab_Rules_UI.cpp / ArmiesTab_Units_UI.cpp — pseudocode, exact ImGui layout is a coder call.
if (ImGui::Button("Resolve Footprint")) {
    const Io::TemplateFootprintRecord* record =
        templateIngestReport.FindByTemplateIdentifier(rule.transform.templateIdentifier);  // §0 addition
    if (record != nullptr) {
        rule.baseFootprintWidth  = record->baseFootprintWidth;
        rule.baseFootprintDepth  = record->baseFootprintDepth;
        rule.footprintBakeFingerprint.sourcePath   = record->sourceFingerprint.sourcePath;
        rule.footprintBakeFingerprint.byteSize     = record->sourceFingerprint.byteSize;
        rule.footprintBakeFingerprint.modifiedTime = record->sourceFingerprint.modifiedTime;
        rule.footprintBakeFingerprint.contentHash  = record->sourceFingerprint.contentHash;
    } else {
        // inline text, no popup, no modal (Constitution §6, STEP82's "never pop a modal" posture):
        // "No ingested data for tpId '<id>'. Ingest game templates in the System tab, or enter a
        // value by hand."
    }
}
```
- Disabled (or hidden) when `rule.transform.templateIdentifier[0] == '\0'` — nothing to resolve.
- Fires **only** on click. Never runs automatically, never runs inside dirty-hash recompute, never
  runs inside `RequestRegeneration()` (§4 below) — ARCH_18_02_IngestedDataDeterminism.md §18.2 rule 1's "discrete, human-triggered
  authoring action — never an implicit step inside generation itself," applied literally.
- Overwrites `baseFootprintWidth`/`baseFootprintDepth`/`footprintBakeFingerprint` **only** — never
  touches `spacingMinimum`, `obstacleDistanceMinimum`, or anything else on the rule. The designer's
  hand-tuned spacing stays exactly as authored; this ticket adds a second, independent value beside
  it, not a replacement for it (a future PROC ticket, out of scope here, decides how the two combine).
- After baking, the two float fields are ordinary sliders/drags like every other field on the rule —
  nothing in this ticket makes them read-only (ARCH_18_02_IngestedDataDeterminism.md §18.2 rule 3).
- **Depends on tickets 90/91** having wired a live `Io::TemplateIngestReport` (or equivalent cached
  ingestion result) into `ApplicationAssetBridge`/`Application_UI.h` — not built by this ticket, same
  deferred-wiring posture STEP58 itself used ("adding a field to `Application_AssetBridge_UI.h` ... is
  STEP51's or STEP52's job at their own dispatch time, not invented here"). Confirmed by reading
  `Application_AssetBridge_UI.h` this session: it carries no footprint/ingestion field today.

## 3. The staleness check — detect, never auto-correct

NEW `src/io/FootprintBakeFingerprint_IO.h` / `.cpp` (shared by the wire-mapping in §5 and the
comparison here):
```cpp
// FootprintBakeFingerprint_IO.h — Build/Read for the nested wire object, plus the cross-type compare
// this ticket's staleness check needs. Layer: IO (legally depends on PARAMS, never the reverse).
nlohmann::ordered_json BuildFootprintBakeFingerprintJson(const Params::FootprintBakeFingerprint&);
void ReadFootprintBakeFingerprintJson(const nlohmann::json&, const char* key,
                                       Params::FootprintBakeFingerprint& out);

// True when the two disagree on ANY field -- reuses the design doc's fingerprint mechanism
// (path+size+mtime+contentHash, modelled on the real Io::SourceFingerprint, AssetAtlasCache_IO.h:38)
// rather than inventing a second comparison scheme.
bool FootprintBakeFingerprintIsStale(const Params::FootprintBakeFingerprint& baked,
                                      const Io::SourceFingerprint& current);
```

NEW `src/io/FootprintBakeStaleness_IO.h` / `.cpp`:
```cpp
struct StaleFootprintEntry {
    std::string ruleKind;             // "Prop" or "Unit" -- which array it came from
    std::string ruleName;             // PLACEMENT_SCATTER_SPEC's per-rule `name` field, once it lands;
                                       // falls back to "rule #<index>" if the rule has no name yet
    std::string templateIdentifier;
    float oldBaseFootprintWidth = 0.0f, oldBaseFootprintDepth = 0.0f;
    float newBaseFootprintWidth = 0.0f, newBaseFootprintDepth = 0.0f;  // 0/0 when bNoLongerIngestible
    bool  bNoLongerIngestible = false;  // FindByTemplateIdentifier returned nullptr THIS time
};

struct FootprintBakeStalenessReport {
    std::vector<StaleFootprintEntry> staleEntries;
    bool AllFresh() const { return staleEntries.empty(); }
    std::string SummaryText() const;   // ONE wording, house shape (STEP73 §0 / STEP82)
};

// Pure, read-only, touches no disk. Skips any rule whose footprintBakeFingerprint.IsValid() == false
// (never baked -- ARCH_18_02_IngestedDataDeterminism.md §18.2 rule 4, not an error, not reported). If currentReport itself is empty/
// absent (no install, or never ingested this session), returns an empty report immediately -- this
// check must never require a game install to run (ARCH_18_02_IngestedDataDeterminism.md §18.2 rule 4 / design doc §6.1).
FootprintBakeStalenessReport CheckFootprintBakeStaleness(const Params::MapRecipe& recipe,
                                                          const Io::TemplateIngestReport& currentReport);
```

**`SummaryText()` — reuses the STEP73/STEP82 house shape exactly** (loud, non-blocking, names every
offender in one aggregate block, never auto-fixes):
```
2 baked footprint value(s) are stale (the game's template data has changed since baking):
  Prop rule "Rock Scatter" (tpId "edbm0101"): baked 0.70x0.69, now 0.82x0.79
  Unit rule "Base Guards" (tpId "uca1001"): baked 1.20x1.20, now 1.35x1.35
Nothing was changed: SanGen never re-bakes a footprint value for you. Click "Resolve Footprint" on
the affected rule to update it, or leave it as authored if the old value was intentional.
```
A `bNoLongerIngestible` entry gets its own line: `Prop rule "X" (tpId "y"): baked 4.0x4.0, template no
longer found in the current game install (renamed, removed, or install changed)`.

### 3.1 Where the check runs

**Call site 1 — on `.sanmap` import ("whenever the recipe is opened").** After the recipe is fully
populated (same tier STEP82 inserts its own check at — a sibling pre-flight, not inside a pure
JSON-building function), if a cached `Io::TemplateIngestReport` is available (sourced from whatever
ticket 90 durably remembers as "last ingest," or skipped entirely if none exists — never a forced
re-ingest, per the design doc §4.3's "explicit user action, cached thereafter"), call
`CheckFootprintBakeStaleness` and, if not `AllFresh()`, `result.Warn(report.SummaryText())` — reusing
`Io::MapImportResult::Warn`/`warningCount`, **already real, shipped code**
(`src/io/MapImporter_IO.h:68,74`, confirmed by STEP82's own citation). One aggregate `Warn()` call, not
one per stale entry (mirrors STEP82 acceptance test 9's "one aggregate warning, not one per army").

**Call site 2 — before the discrete Regenerate action ("before Generate runs").** Confirmed by reading
`src/ui/Application_Draw_UI.cpp:47`: `if (ImGui::Button("Regenerate")) canvas.RequestRegeneration();`
is the one discrete, human-clicked generation trigger in the current UI (the dirty-hash/idle-refinement
machinery in `Pipeline::GenerationAssembler`/`PreviewDriver_PIPELINE` recomputes continuously off dirty
flags, but this button is the explicit "the designer asked for this now" moment ARCH_18_02_IngestedDataDeterminism.md §18.2 rule 1's
bake-vs-generate distinction is really drawing). Insert the same non-blocking check immediately before
`canvas.RequestRegeneration()` fires, using whatever `Io::TemplateIngestReport` is already resident
(never triggers a fresh ingest — that stays a System-tab-only action per ticket 91). **The check never
gates the click** — `RequestRegeneration()` fires unconditionally either way; the check only decides
whether a warning also gets surfaced. Exact warning surface (a status line near the button, the Files
tab's existing log panel, a toast) is a coder-level UI call, flagged as an open question below, same
posture as STEP73 §0's own unresolved warning-placement question.

**Never a third call site.** The staleness check is explicitly NOT run every frame, not run inside
`PreviewDriver_PIPELINE`'s continuous recompute, and not run from any `Proc::` file — ARCH_18_02_IngestedDataDeterminism.md §18.2 rule 2
forbids PROC from touching the ingestion result in any form, and this check's own second argument
(`Io::TemplateIngestReport`) is exactly the type rule 2 says PROC must never see.

## 4. Un-baked-field behavior, confirmed explicitly (ARCH_18_02_IngestedDataDeterminism.md §18.2 rule 4)

- A `PropRule`/`UnitRule` whose `footprintBakeFingerprint.IsValid() == false` (the default,
  compile-time state) keeps `baseFootprintWidth`/`baseFootprintDepth` at STEP58's own default values
  (4.0/4.0 or 2.0/2.0) forever, until a human clicks "Resolve Footprint" — an ordinary tunable a
  designer can also hand-edit directly, not a "missing data" condition.
- The staleness check (§3) silently skips any such rule — no warning, no log line, nothing.
- Neither the bake button nor the staleness check ever requires `AppSettings::gameInstallRoot` to be
  set. With no install configured, `Io::TemplateIngestReport` is simply empty/absent everywhere it is
  consulted, and every code path above already treats that as "skip, don't error" (bake button: shows
  the "no ingested data" inline message; staleness check: empty report, `AllFresh() == true`).
- **Generate is never blocked by any of this**, at any point, on any machine, with or without a game
  install — the Regenerate button (§3.1 call site 2) fires unconditionally regardless of what the
  staleness check finds.

## 5. `.sanmap` wire mapping

Casing per `ARCH_01_06_SanmapKeyCasing.md`: `PropsStack`/`UnitsStack` are already SanGen-owned
PascalCase sections; per-field keys inside each rule object follow the existing flat-scalar-plus-one-
nested-object convention already live in both files (`"SpacingMinimum"` flat, `"Transform"` nested).
This ticket adds two new flat scalars and one new nested object, identically shaped in both stacks:

**`EDIT src/io/MapExporter_PropsStack_IO.cpp`** (after the existing `"ObstacleDistanceMinimum"` line):
```cpp
json["BaseFootprintWidth"] = rule.baseFootprintWidth;
json["BaseFootprintDepth"] = rule.baseFootprintDepth;
json["FootprintBakeFingerprint"] = BuildFootprintBakeFingerprintJson(rule.footprintBakeFingerprint);
```
**`EDIT src/io/MapImporter_PropsStack_IO.cpp`** (after the existing `"ObstacleDistanceMinimum"` line):
```cpp
ReadJsonFloat(json, "BaseFootprintWidth", rule.baseFootprintWidth);
ReadJsonFloat(json, "BaseFootprintDepth", rule.baseFootprintDepth);
ReadFootprintBakeFingerprintJson(json, "FootprintBakeFingerprint", rule.footprintBakeFingerprint);
```
**`EDIT src/io/MapExporter_UnitsStack_IO.cpp` / `MapImporter_UnitsStack_IO.cpp`** — identical three
lines each, after the existing `"SpacingMinimum"` line (UnitRule has no `ObstacleDistanceMinimum` to
anchor after).

`#include "FootprintBakeFingerprint_IO.h"` added to all four files. `BuildFootprintBakeFingerprintJson`
writes a plain sub-object (`{"SourcePath":..., "ByteSize":..., "ModifiedTime":..., "ContentHash":...}`,
PascalCase members matching the top-level convention); a missing/absent key on read leaves the PARAMS
default (`IsValid() == false`) untouched, per `IO_MIGRATION_SPEC`'s never-refuse-on-absence posture,
consistent with every other `ReadJson*` call in these files already degrading the same way on an
older `.sanmap` that predates this ticket.

## Backend policy
N/A for the whole ticket. The bake action is a single-click O(1) hash lookup on the UI thread; the
staleness check is a linear scan over at most a few dozen scatter rules, run at most once per import
and once per Regenerate click. No compute dispatch, no SIMD, no GPU handle, no `Dispatch_SYS`
involvement anywhere in this ticket (same posture STEP82's identical-scale IO diagnostic already
established).

## Layer & accuracy class
PARAMS fields carry no accuracy class of their own (Constitution §4 classes computations, not data).
The IO-side ingestion source (`Io::TemplateIngestReport`, once real) stays **Visual**-class exactly as
`ARCH_18_02_IngestedDataDeterminism.md` rule 5 requires — nothing in this ticket changes that
assignment. **This ticket does not assign an accuracy class to any future scatter-spacing
calculation** that might one day consume `baseFootprintWidth`/`baseFootprintDepth` from PROC — per
§18.2 rule 2's own text, that is "a Generator/Compute Optimization Expert call, out of scope here."

## ARCH rules invoked
- `ARCH_18_02_IngestedDataDeterminism.md` §18.2, all five rules — cited by number throughout; this
  ticket is the ticket its closing paragraph names.
- `ARCH_01_08_ParamsFieldNamingByKind.md` — `baseFootprintWidth`/`baseFootprintDepth`/
  `footprintBakeFingerprint` are SanGen's-own-recipe-field naming (not a format-key pass-through);
  fully spelled, no abbreviation.
- `ARCH_01_06_SanmapKeyCasing.md` — the new wire keys are single-token PascalCase members of the
  already-PascalCase `PropsStack`/`UnitsStack` sections.
- `ARCH_01_05_FileSizeCeilings.md` — the soft-ceiling note on `ScatterRule_PARAMS.h` (§1.2); every
  other new file is a small, single-primary-type file comfortably under 100 lines.
- Constitution §1 layering — `PARAMS` never includes `IO`; `Params::FootprintBakeFingerprint` is a
  deliberate field-for-field duplicate of `Io::SourceFingerprint`, not a shared include (§1.1).
- Constitution §6 — validate-then-default-then-log: an absent/never-baked field is never an error; a
  stale field is a loud, logged, non-blocking finding, never a silent correction and never a refusal.
- Constitution §7 — this document's own schema; `Warn`/`warningCount` reuse (not reinvention) mirrors
  STEP82's own justification for the identical reuse.
- Constitution §8 — total tweakability: the baked fields are ordinary sliders after baking, per §18.2
  rule 3; nothing in this ticket introduces a read-only or provenance-gated value.

## Solution + performance estimate (basis)
Bake: one hash-map lookup (`FindByTemplateIdentifier`) per click, at most a few hundred entries in the
ingested corpus (design doc §1.1: 546 files total). Staleness: one linear scan over `recipe.propRules`
+ `recipe.unitRules` (tens of rules, same sizing basis `STEP49`'s roster note already establishes for
hand-authored counts), each doing one more hash-map lookup. Both are sub-millisecond, human-triggered,
at most a few times per session. Constitution §7 basis tag: **direct algorithmic inspection** — the
same basis STEP52/STEP58 already used for their own O(1)/small-N, non-per-frame lookups; no
microbenchmark is warranted at this scale.

## Explicit out-of-scope
- **Any PROC/scatter code consuming `baseFootprintWidth`/`baseFootprintDepth`.** ARCH_18_02_IngestedDataDeterminism.md §18.2 rule 2
  forbids PROC from reading the ingestion result directly, but says nothing about a *future* ticket
  reading the now-baked `PARAMS` field from PROC — that ticket's accuracy-class review and its
  spacing-math design (how `baseFootprintWidth`/`Depth` combines with `spacingMinimum`/
  `obstacleDistanceMinimum`/the instance's own scale range) is explicitly not decided here.
- **Tickets 85–91 themselves.** This ticket specifies against their design-doc shapes (plus the one
  `TemplateFootprintRecord`/`FindByTemplateIdentifier` addition flagged in "Sequencing") and does not
  write any of their code.
- **`DecalRule` and `MarkerRule`.** No footprint field added to either — see §1's reasoning.
- **Overwriting `spacingMinimum`/`obstacleDistanceMinimum`.** The bake action never touches either
  existing field; they remain fully independent, hand-authored values.
- **Auto-re-baking on staleness**, in any form, at any trigger point (import, Regenerate, or a timer).
  ARCH_18_02_IngestedDataDeterminism.md §18.2 rule 3 forbids it outright.
- **A confirmation dialog / modal for the staleness warning.** Log/status-line only, same "no modal
  for a tolerated, non-blocking state" posture `STEP82`'s Open Question 2 already settles for its own
  analogous case.
- **Height / 3D collision-box data.** Ground-plane `baseFootprintWidth`/`Depth` only, matching
  STEP58's own scope boundary verbatim.
- **A "resolve all" batch-bake button** across every rule at once. One rule, one click, per ARCH_18_02_IngestedDataDeterminism.md §18.2
  rule 1's "discrete... action" — a batch variant is a plausible future UI convenience, not specified
  or precluded here.

## Acceptance test
Deferred to implementation time (this ticket cannot be dispatched until 85–91 are real, per
"Sequencing"), but the shape is specified now so the Coder does not have to invent it:
1. **Bake, found.** A `PropRule` with `templateIdentifier = "edbm0101"` and a `TemplateIngestReport`
   containing a matching record → clicking Resolve Footprint sets `baseFootprintWidth`/`Depth` to the
   record's values and `footprintBakeFingerprint` to the record's fingerprint (`IsValid() == true`
   afterward).
2. **Bake, not found.** `templateIdentifier` present but absent from the report → fields unchanged, an
   inline "No ingested data" message shown, no crash, no exception.
3. **Bake, empty tpId.** Button disabled/hidden; no lookup attempted.
4. **Round-trip.** Export a recipe with a baked `PropRule`, re-import → `baseFootprintWidth`/`Depth`/
   `footprintBakeFingerprint` all match byte-for-byte (floats via `NearlyEqual`, fingerprint fields
   exact).
5. **Never-baked round-trip.** A rule that was never baked exports/imports with
   `footprintBakeFingerprint.IsValid() == false` on both ends — no crash on the absent nested key.
6. **Staleness, fresh.** Baked fingerprint matches the current report's fingerprint for that tpId →
   `AllFresh() == true`, empty `SummaryText()`.
7. **Staleness, changed.** Current report's fingerprint differs (any one of the four fields) →
   reported, with correct old/new width/depth values in `SummaryText()`.
8. **Staleness, no longer ingestible.** `FindByTemplateIdentifier` now returns `nullptr` for a
   previously-baked tpId → `bNoLongerIngestible == true`, reported with the specific wording above.
9. **Staleness, never baked → never checked.** A rule with `IsValid() == false` never appears in
   `staleEntries` regardless of what the current report contains.
10. **Staleness, no install / empty report.** `CheckFootprintBakeStaleness` with a default-constructed
    `TemplateIngestReport` returns an empty report immediately — proves it never requires an install.
11. **One aggregate warning.** Three stale rules at once → exactly one `Warn()` call on import (mirrors
    STEP82 acceptance test 9).
12. **Regenerate never blocked.** With a stale report present, clicking Regenerate still calls
    `RequestRegeneration()` — the staleness check's presence has zero effect on control flow.
13. Full solo rebuild + `ctest -C Debug`: previously-passing suite stays green, zero unrelated test
    files edited.

## Verify
- Grep every new `.cpp` for a write through a non-`const` reference to `Io::TemplateIngestReport` —
  there must be none; this ticket only ever reads it.
- Grep `src/proc/` for any new reference to `TemplateIngestReport`/`WorldFootprintSizeTable`/
  `FootprintBakeFingerprint` introduced by this ticket — there must be none (ARCH_18_02_IngestedDataDeterminism.md §18.2 rule 2).
- Confirm `src/params/*.h` (the whole directory, not just the two files this ticket touches) still has
  zero `#include "../io/` lines after this ticket lands.

## ❓ Open questions
1. **Exact UI surface for the Regenerate-time staleness warning** (§3.1 call site 2). A status line
   near the Regenerate button, the Files tab's existing log panel (reused cross-tab), or a lightweight
   toast are all plausible; this ticket does not pick one, same posture STEP73 §0 leaves its own
   warning-placement question open for the human/UI Expert to settle at implementation time.
2. **`ruleName` fallback when `PropRule`/`UnitRule` gains no `name` field before this ticket lands.**
   `PLACEMENT_SCATTER_SPEC.md` says each rule type "gains a `name` field" as part of a still-pending
   Group-container correction; if that has not landed by the time this ticket is implemented, use
   `"rule #<index>"` as specified in `StaleFootprintEntry`'s comment — revisit once the real `name`
   field exists.
3. **Should the bake button also appear on a per-instance basis** (e.g. STEP49's manual-marker-style
   roster row) rather than only on the rule itself? Not raised by the human's stated case (rule-level
   scatter spacing), so not specified here; flagged in case a future ticket's real use turns out to
   need per-instance resolution instead of per-rule.
