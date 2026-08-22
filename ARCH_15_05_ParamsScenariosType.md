[← ARCH index](ARCH.md) · [§15 ARCH_15_MapScenarioSystem](ARCH_15_MapScenarioSystem.md) · SanGen ARCH §15.5. **Only the ARCH Expert writes this file.**

### 15.5 `Params::Scenarios` — the new PARAMS type (shape ruling)

**Binding shape** (`src/params/Scenario_PARAMS.h`, `Params::` namespace; naming per §1.1/§1.8 —
pass-through, human-authored round-trip data, so field names default to the format's own
spelling per §1.8's first bucket, converted for case only):

```cpp
enum class ScenarioAlloyMode { Explicit, Occupancy, KeepAll, Delta };  // §5, MAP_SCENARIO_SPEC

struct ScenarioSpawn         { std::string armyName; float positionX, positionY, positionZ; };
struct ScenarioAlloyOverride { std::string armyName; std::string markerName; float positionX, positionY, positionZ; };
struct ScenarioAlloyRemoval  { std::string armyName; std::string markerName; };

// Naval fleet composition, shaped 2026-08-21 from a live read of the reference
// `SpawnNavalFleets` Lua body (`Pandemonium Isthmus_Scenarios_Script.lua`) — see the
// ratification bullet below for the per-scenario ruling and the excluded tuning constants.
struct ScenarioNavalFleetEntry { std::string templateIdentifier; int count = 0; };  // ordered
                                                        // spawn-batch list, NAVAL_FLEET —
                                                        // §1.8 tpId->templateIdentifier exception
enum class ScenarioNavalPondSide : int8_t { West = -1, East = 1 };    // matches the live
                                                        // reference's own signed convention
struct ScenarioNavalPondAssignment { std::string armyName; ScenarioNavalPondSide side = ScenarioNavalPondSide::East; };
                                                        // sparse — an army absent from this list
                                                        // defaults to East, mirroring the live
                                                        // reference's own
                                                        // `NAVAL_POND_SIDE_BY_ARMY[army.name]
                                                        // or 1` fallback
struct ScenarioNavalFleet {
    std::vector<ScenarioNavalFleetEntry>     fleet;              // NAVAL_FLEET
    std::vector<ScenarioNavalPondAssignment> pondSideByArmy;     // NAVAL_POND_SIDE_BY_ARMY
    float                                    sideBiasDistance = 90.0f;  // world units,
                                                                          // NAVAL_SIDE_BIAS_DISTANCE
};

struct ScenarioBody {
    std::string name;                                    // log/debug identifier, §5
    Params::MapArea area;                                 // reuse the existing §9 type — same
                                                            // world-space rect shape MAP_SCENARIO_SPEC
                                                            // §5 already describes for `area`
    bool navy = false;
    ScenarioAlloyMode alloyMode = ScenarioAlloyMode::Occupancy;   // RATIFIED default, see below
    std::vector<ScenarioSpawn> spawns;                     // §6 HARD REQUIREMENT — empty is only
                                                            // legal with documented intent
    std::vector<ScenarioAlloyOverride> alloys;             // meaningful for `Explicit` only
    std::vector<ScenarioAlloyOverride> alloysToAdd;         // meaningful for `Delta` only
    std::vector<ScenarioAlloyRemoval>  alloysToRemove;      // meaningful for `Delta` only
    std::string authoringNote;                             // carries forward §6's "document that
                                                            // intent in the entry's own comment" as
                                                            // real data now that this is no longer
                                                            // hand-authored Lua text
    ScenarioNavalFleet navalFleet;                          // meaningful only when navy == true;
                                                            // per-scenario, not per-map — see the
                                                            // ratification bullet below
};

struct PatternScenario { ScenarioBody body; std::string slotPattern; };            // TIER 1

enum class ScenarioComparator { Equal, NotEqual, GreaterThan, GreaterOrEqual, LessThan, LessOrEqual };
enum class ScenarioCountField { Total, HumanCount, AiCount };
struct ScenarioCountCondition { ScenarioCountField field; ScenarioComparator comparator; int value = 0; };
struct CountScenario { ScenarioBody body; std::vector<ScenarioCountCondition> conditions; };  // TIER 2,
                                                            // conditions are AND'd (conjunction)

struct Scenarios {
    std::vector<PatternScenario> patternScenarios;   // TIER 1 — pattern, §4
    std::vector<CountScenario>   countScenarios;      // TIER 2 — ORDER IS LOAD-BEARING, §15.6
    ScenarioBody                 defaultScenario;     // TIER 3 — always matches, exactly one
};
```
`MapRecipe::scenarios` — a flat sibling, same pattern as `armies`/`areas` (§9).

- **Why a flat sibling field set, not a tagged union/variant.** `alloys`/`alloysToAdd`/
  `alloysToRemove` sit side by side on `ScenarioBody`; only the field(s) matching the record's
  own `alloyMode` are meaningful — mirrors how Lua's untyped tables leave the non-selected fields
  simply absent, and matches this codebase's existing style (no `std::variant` precedent anywhere
  in `src/params/`); a variant would be more type-precise but is a new pattern this ratification
  does not introduce without cause.
- **Comparator vocabulary is deliberately small** (`Equal`/`NotEqual`/`GreaterThan`/
  `GreaterOrEqual`/`LessThan`/`LessOrEqual` × `Total`/`HumanCount`/`AiCount`, conjunction-only) —
  per the human's own settled framing: "every live predicate is a simple conjunction over
  total/human/AI counts, so a small vocabulary suffices." This is taken as given (the human's
  design ruling), not independently re-derived from the live Lua `match` closures, which this
  pack cannot read (external, game-install-only, `MAP_SCENARIO_SPEC.md`'s own sourcing caveat).
- ✅ **RATIFIED: `alloyMode`'s default is `Occupancy`.** Confirmed by the human (2026-08-21) —
  promoted from the structurally-safe placeholder this ruling originally used (it neither invents
  alloy data nor blanks it) to settled law; the field's own default-initializer above
  (`ScenarioAlloyMode::Occupancy`) already reflected it correctly and needs no code change.
- ✅ **RATIFIED (2026-08-21): naval-fleet composition, shaped from a live read of the reference
  `SpawnNavalFleets` Lua body.** The prior "no documented shape anywhere accessible to this pack"
  flag is retracted — not because the data didn't exist, but because it hadn't yet been handed to
  this pack; it has now been read directly at
  `LJ/lua/maps/Pandemonium Isthmus/Pandemonium Isthmus_Scenarios_Script.lua`. Shape: the code
  block above (`ScenarioNavalFleetEntry`/`ScenarioNavalPondSide`/`ScenarioNavalPondAssignment`/
  `ScenarioNavalFleet`, and `ScenarioBody::navalFleet`).
  - **Per-scenario, not per-map — ruled, correcting the live file's own shape.** The live
    reference holds `NAVAL_FLEET`/`NAVAL_POND_SIDE_BY_ARMY`/`NAVAL_SIDE_BIAS_DISTANCE`
    file-scoped (shared across every scenario in the file), while `navy` itself is already
    per-scenario (`ScenarioBody::navy`). Ruling: SanGen's `navalFleet` is **per-scenario** (a
    member of `ScenarioBody`, alongside `navy`), not a single map-level field. The live file's
    sharing is an artifact of only one scenario (`4human`) ever having used `navy = true` —
    nothing in the reference constrains different navy-enabled scenarios to the same
    composition, and a small-lobby pond skirmish and a large-lobby pond map plausibly want
    different fleet sizes/compositions. Per-scenario placement is strictly more general (an
    author can still repeat identical values across every scenario, which reproduces the live
    file's behavior exactly) while per-map-only placement would foreclose a real, plausible
    authoring need with no offsetting benefit.
  - **Algorithm tuning constants — NOT PARAMS, ruled out of `Params::Scenarios` entirely.**
    `NAVAL_BATCH_SIZE`/`NAVAL_SPIRAL_STEP`/`NAVAL_SPIRAL_MAX_TRIES`/`NAVAL_GAP`/
    `NAVAL_DEFAULT_FOOTPRINT`/`NAVAL_GRID_CELL`/`NAVAL_GIVE_UP_AFTER_MISSES` never vary per map
    (the live file's own header comment already marks them "algorithm tuning constants, NOT
    per-map authored data") and the runtime consumes them purely internally (spiral search,
    spatial-hash bucketing, batch-yield cadence) — they belong to
    `<MapName>_Scenarios_Runtime.lua` (`MAP_SCENARIO_SPEC.md` §2), the bundled,
    identical-across-every-map runtime file, not to any per-map `Params::Scenarios` data. A
    coder must not add them to `ScenarioNavalFleet` or any other PARAMS type.

