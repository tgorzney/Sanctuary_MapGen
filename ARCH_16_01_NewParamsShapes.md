[← ARCH index](ARCH.md) · [§16 ARCH_16_MarkerLayerSymmetry](ARCH_16_MarkerLayerSymmetry.md) · SanGen ARCH §16.1. **Only the ARCH Expert writes this file.**

### 16.1 New PARAMS shapes — ratified, with §16.5's naming amendment folded in
```cpp
// Symmetry_PARAMS.h — new, alongside the existing SymmetryDetection/SymmetryBlend structs
struct SymmetrySetting {
    bool bSymmetryUseGlobal = true;
    int  symmetryMask       = 0;
    int  radialSymmetryRepeatCount = 3;
};

// MarkerRule_PARAMS.h — new wrapper; MarkerRule loses its own bSymmetryUseGlobal/symmetryMask/
// radialSymmetryRepeatCount triplet (moved up a tier — see §16.6 for the migration consequence)
struct MarkerRuleLayer {
    std::string name;
    bool bEnabled = true;
    bool bHidden  = false;     // still generated (clearance/fairness) even when not shown —
                                // same semantics MarkerRule::bHidden already carries today
    SymmetrySetting symmetry;
    std::vector<MarkerRule> rules;
};

// MarkerInstance_PARAMS.h — extends the already-ratified Gap 1 shape
// (work_orders/GAP_MarkerLayerAndSymmetry_PARAMS.md) with a symmetry field
struct MarkerInstanceLayer {
    std::string name;
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float iconScale = 1.0f;
    SymmetrySetting symmetry;
};
```
`MapRecipe::markerRules` → **`markerRuleLayers`** (`std::vector<MarkerRuleLayer>`); new
`MapRecipe::markerLayers` (`std::vector<MarkerInstanceLayer>`).

- **`SymmetrySetting` vs. ARCH §13's "NOT a wrapper struct" ruling — reconciled, not
  contradicted.** §13 ruled that `radialSymmetryRepeatCount` joins `bSymmetryUseGlobal`/
  `symmetryMask` as a **flat sibling** specifically because it was landing on types that
  **already carried** the pair as flat siblings (`MarkerRule`, `PropRule`, `UnitRule`, the
  `MapRecipe` global) — matching an existing convention beats introducing a new one on an
  established type. `MarkerRuleLayer`/`MarkerInstanceLayer` are brand-new types with no such
  existing convention to match, and — unlike every prior site — **two** sibling types
  (procedural-layer and manual-layer) need the identical triplet simultaneously, so one shared
  named struct is DRYer than duplicating three fields twice, which is what the flat-sibling
  convention would otherwise force. This ratification does **not** retrofit `SymmetrySetting`
  onto `MarkerRule`/`PropRule`/`DecalRule`/`UnitRule`/`MapRecipe::globalSymmetryMask` — those
  stay exactly as §13 left them; a future full unification is a natural but non-binding
  follow-on, not decided here.
- **Why two arrays, not one unified type.** Matches the already-ratified Props/Decals split
  (§12's `PropInstanceLayer` manual-metadata array vs. the still-flat `PropsStack` procedural
  rules — Correction 7's procedural Stacks and Correction 14's manual Groups are explicitly
  different concepts). Manual markers additionally carry a hard format-mandated
  `markers[type][instanceName]` two-level dictionary (`MarkerInstanceGroup`/`MarkerTransform`,
  `ENTITY_AUTHORING_PARAMS_SPEC`) that a unified layer type cannot replace — layer membership on
  the manual side must stay a cross-cutting index-tag there, exactly the role `layerIndex`
  already plays for `PropTransform`/`DecalTransform` (§12).
- **Shape asymmetry between the two new types, worth naming explicitly so it is never assumed
  away.** `MarkerRuleLayer` is a real **container** (owns `std::vector<MarkerRule> rules`) —
  the procedural side has no pre-existing per-instance array to index into, so the layer must
  hold its rules directly. `MarkerInstanceLayer` is **flyweight metadata only** (no
  `MarkerTransform` vector) — exactly like `PropInstanceLayer`/`DecalInstanceLayer` — because the
  actual `MarkerTransform` instances already live in `MarkerInstanceGroup.transforms` and
  reference their layer by index (§16.5). A reader must not assume the two "Layer" types share a
  containment shape merely because they share a naming pattern.
- **`bHidden` on `MarkerRuleLayer`, not per-rule.** The design's R1 §1 code block carries it at
  the layer tier; `MarkerRule::bHidden` already exists per-rule today and is untouched by this
  ratification (a rule can still be individually hidden within a layer that is itself shown) —
  the two are independent, not a relocation.

