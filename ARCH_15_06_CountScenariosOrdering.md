[← ARCH index](ARCH.md) · [§15 ARCH_15_MapScenarioSystem](ARCH_15_MapScenarioSystem.md) · SanGen ARCH §15.6. **Only the ARCH Expert writes this file.**

### 15.6 `COUNT_SCENARIOS` ordering — array order IS the match-priority authoring action

**Binding:** `Params::Scenarios::countScenarios` is `std::vector<CountScenario>`, and its
**array order is the entire authoring mechanism for TIER 2 match priority** — first match wins
(`MAP_SCENARIO_SPEC.md` §4), exactly mirroring the live reference's own ordering discipline
(broad fallback rules kept after every more specific rule). This is not incidental container
choice; it is load-bearing semantics, so:
- The UI's authoring surface for `countScenarios` **must be a reorderable list** (the same
  `DraggableList` widget already used for `GeoLayers` and the §14.7 View-toolbar overlay stack)
  — never a set, a table with no defined iteration order, or any widget that does not make
  reordering the explicit user action for changing match priority.
- `patternScenarios` (TIER 1, exact-match) and `defaultScenario` (TIER 3, always matches) carry
  no such ordering requirement — exact-pattern matching does not depend on array position for
  correctness (duplicate authored patterns are an authoring mistake, not an ordering-law case),
  and the default is a single record, not a list.

