[← ARCH index](ARCH.md) · [§16 ARCH_16_MarkerLayerSymmetry](ARCH_16_MarkerLayerSymmetry.md) · SanGen ARCH §16.5. **Only the ARCH Expert writes this file.**

### 16.5 `MarkerTransform` — `symmetryGroupIdentifier` (not `symmetryGroupId`) + already-ratified `layerIndex`
```cpp
struct MarkerTransform {
    std::string name;
    InstancedTransform transform;
    std::string alias;
    int layerIndex = 0;               // Gap 1 (GAP_MarkerLayerAndSymmetry_PARAMS.md), unchanged —
                                       // indexes recipe.markerLayers
    int symmetryGroupIdentifier = 0;  // NEW — 0 = ungrouped (both "never grouped" and "Break
                                       // Symmetry Link" use this same sentinel; real group ids
                                       // start at 1)
};
```
- **Naming amendment: `symmetryGroupId` → `symmetryGroupIdentifier`.** The design's own proposed
  field name uses the abbreviation "Id." ARCH §1.1 permits no abbreviations beyond a short named
  exception list (file extensions, format-dictated identifiers, `Cpu`/`Gpu`) — "Id" is not on it.
  ARCH §1.8 already establishes live precedent for exactly this class of field: the format's own
  `tpId`/`tpid` is spelled `templateIdentifier` everywhere it actually ships as a C++ member,
  specifically because "the literal spelling `tpId` has never actually shipped as a C++ member
  anywhere in `src/`." Confirmed by a fresh grep this session: zero `Id`-suffixed integer field
  exists anywhere under `src/params/` today; `templateIdentifier` is the only precedent, and it is
  spelled in full. `symmetryGroupIdentifier` is not format-dictated (no `.sanmap` key forces this
  spelling — it is a new SanGen-invented linkage field), so §1.1's general rule applies at full
  strength with no format-derived exception available. Every other reference to this field in the
  consolidated list (items 2 and 6) is understood to mean `symmetryGroupIdentifier`.
- **R1's originally-proposed `bSymmetryAnchor`/`symmetryOrbitIndex` are correctly dropped, not
  ratified.** R2 §1 resolved the any-member-drag identity problem without needing either field
  (the orbit's own slot-0 seed-point convention plus exact-position matching at gesture-start is
  sufficient, and the {slot → instance} correspondence is fully re-derivable on demand, never
  needing to persist) — this ARCH ruling agrees with R2's own retraction and does not reopen it.
- **Merged field on `markers[type].transforms[name]`: `symmetryGroupIdentifier` only** (renamed
  per the amendment above) **+ the already-carried `layerIndex`.** Both are genuinely novel
  scalar information with no competing home, so both use **direct field injection** on the
  existing format-native `MarkerTransform` object — the same §1.8/§12 "direct injection when the
  field is a novel scalar with no competing home" rule already governs `armyColor`/`alias`/
  `layerIndex`, applied consistently rather than reopened.

