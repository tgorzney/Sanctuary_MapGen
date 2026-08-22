[← ARCH index](ARCH.md) · SanGen ARCH §12. Part of the ratified v2 architecture; the Constitution (`sangen_arch_pack/CONSTITUTION.md`) and this file's preamble in `ARCH.md` bind alongside it. **Only the ARCH Expert writes this file.**

## 12. Manual-layer authoring for props/decals — `layerIndex` + `PropGroups`/`DecalGroups` (ARCH ruling, revises `ENTITY_AUTHORING_PARAMS_SPEC`)

Fills the gap `PropsTab_Manual_UI.h`'s SCOPE NOTE 1 named: v1's manual prop GROUPS (hand-placed
props kept as named groups after import) had no `_PARAMS` home — `MapRecipe` carried scatter
RULES only. **Ratifies Option B, direct field injection, over the alternative of contiguous
index-ranges in a separate key.**

- **Decisive rejection of the range-based alternative.** A contiguous index-range key (e.g.
  "layer 0 owns `transforms[0..40)`") has a real silent-corruption failure mode: if any external
  tool (hand-editing, or the real Unity map editor — both supported workflows for this format)
  reorders an entry in `transforms[]` without changing the count, the ranges stay internally
  self-consistent while silently misattributing instances to the wrong layer, undetectable by any
  validation. `layerIndex` traveling directly with the instance has no such failure mode — external
  reordering cannot desync it. Cost is trivial (~1 MB even at Forge's 63.5k prop instances,
  `SANMAP_FORMAT_SPEC`'s 23-map survey) against the file's actual bulk, which is textures, not
  JSON.
- **`PropTransform`/`DecalTransform` become real, named wrapper types**, superseding
  `ENTITY_AUTHORING_PARAMS_SPEC`'s earlier "props/decals need no wrapper type" ruling — that
  ruling's premise (zero fields beyond `InstancedTransform`) no longer holds:
  ```cpp
  struct PropTransform  { InstancedTransform transform; int layerIndex = 0; };
  struct DecalTransform { InstancedTransform transform; int layerIndex = 0; };
  struct PropInstanceGroup  { std::string blueprintPath; std::vector<PropTransform>  transforms; };
  struct DecalInstanceGroup { std::string blueprintPath; std::vector<DecalTransform> transforms; };
  ```
- **`layerIndex` does NOT go on shared `InstancedTransform`.** It would leak onto
  `MarkerTransform`'s composed member and every future consumer of the base that has no concept
  of a manual layer. JSON key `layerIndex`, lowerCamelCase, merged directly into the existing
  transform object — the same rule already governing `armyColor`/`alias` (§1.6 Correction 0),
  §1.8's format-derived-field data-KIND rule.
- **Separate layer-metadata array**, one per domain, new top-level schema-v3 PascalCase keys
  `PropGroups`/`DecalGroups` (`SANMAP_FORMAT_SPEC` Correction 14):
  ```cpp
  struct PropInstanceLayer  { std::string name; float color[4]; float iconScale = 1.0f; };
  struct DecalInstanceLayer { std::string name; float color[4]; float iconScale = 1.0f; };
  ```
  Not `PropLayers`/`DecalLayers` — `PropsStack`/`DecalsStack`'s Group→Layer(rule) procedural
  hierarchy (`SANMAP_FORMAT_SPEC` Correction 7) already uses "Layer" for something unrelated (a
  rule inside a procedural Stack); reusing the word here would collide two different concepts.
  `ManualPropGroup` is also the already-live identifier in `src/ui/PropsTab_Manual_UI.h`, so
  `PropGroups`/`DecalGroups` picks up an existing name rather than inventing one.
- **Import validation:** `layerIndex` out of range against the corresponding `PropGroups`/
  `DecalGroups` array size is a loud, logged clamp to `0` (Constitution §6), never a hard refusal
  — this is authoring-convenience metadata, not gameplay-authoritative data. A missing
  `layerIndex` key on an older/foreign file degrades for free to `0` (the field's own default).
- **General principle, binding beyond this ratification (also recorded in §1.8):** a
  format-native object gains a small SanGen field by **direct injection** when the field is
  genuinely novel information with no competing home (`armyColor`, `alias`, `layerIndex`); a
  **separate SanGen-owned array** is used only when the metadata is richer than a scalar AND no
  format-native group container already exists to hold it (`PropInstanceLayer`/
  `DecalInstanceLayer`). One consistent rule, applied per case — it does not reopen `armyColor`/
  `alias`/`MarkerTransform::alias`, all already-settled instances of the first branch.
- **Shape only, not wiring.** `MapRecipe_PARAMS.h` gaining `std::vector<PropInstanceLayer>
  propLayers;` / `std::vector<DecalInstanceLayer> decalLayers;`, the matching `IO` round-trip, and
  reconciling `Ui::ManualPropGroup` (`PropsTab_Manual_UI.h`) against this new durable
  `Params::PropInstanceLayer` home are separate coder/UI work-orders — not designed here. Full
  detail: `ENTITY_AUTHORING_PARAMS_SPEC`.
