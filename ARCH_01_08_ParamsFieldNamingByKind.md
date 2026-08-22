[← ARCH index](ARCH.md) · [§1 ARCH_01_NamingLaw](ARCH_01_NamingLaw.md) · SanGen ARCH §1.8. **Only the ARCH Expert writes this file.**

### 1.8 PARAMS field naming for format-derived types — governed by data KIND, not by key presence
Ratified alongside `ENTITY_AUTHORING_PARAMS_SPEC` (`Params::Army`/`UnitGroup`/`UnitTransform`/
`MapArea`). A PARAMS field's naming is governed by **what kind of data it is**, not by whether a
`.sanmap` format key of a similar name happens to exist.

- **Pass-through, human-authored, verbatim entity data.** No PROC stage computes or
  reinterprets the value — round-trip fidelity is the field's entire purpose. Such a field uses
  **the format's own spelling by default**, converted only for case (`camelCase`) and the §1.1
  `b`-boolean prefix. Governs `Army`, `UnitGroup`, `UnitTransform`, `MapArea`
  (`ENTITY_AUTHORING_PARAMS_SPEC`) and any future type in that same hand-authored-entity family.
- **SanGen's own generative recipe/setting.** A PROC stage computes, derives, or reshapes the
  value, so the field keeps **SanGen's own descriptive name** even where a format key of similar
  meaning exists — already the practice (`Params::Water::waterLevelMaximum` vs. the format's
  `waterLevel`; `Params::Geometry::worldUnitsPerCell`, which has no format analog at all) and does
  not change here.
- **Named exceptions inside the pass-through bucket** — verbatim would collide with an established
  SanGen quantity, or is too generic to stay AI-legible:
  - **`Area.height` → `length`.** The format's `height` here means Z-extent/depth, not
    elevation — "height" is otherwise universally elevation in this codebase.
  - **`Area.x`/`Area.y` → `originX`/`originZ`.** The format's 2D texture-space origin vs.
    SanGen's `positionX/Y/Z` world convention, where `y` reads as elevation.
  - **`Army.faction` keeps the word but becomes `enum class Faction`**, not a raw `int` —
    matches the existing `MarkerCategory`/`MarkerPriority` pattern of retyping a format-style
    category int at the JSON boundary (`MarkerRule_PARAMS.h`).
  - **`tpId`/`tpid` → `templateIdentifier`.** Already the established spelling everywhere it is
    actually used as a C++ member (`ScatterTransform_PARAMS.h`, `PlacementInstance_DATA.h`).
    This **supersedes** §1.1's naming of `tpId` itself as the verbatim exception — the literal
    spelling `tpId` has never actually shipped as a C++ member anywhere in `src/`.
- **A `Dictionary<string, X>` becomes `std::vector<X>` with the dictionary key folded in as a
  `name` field on `X`.** Not new design — the existing choice for `Area`
  (`AreasTab_List_UI.h`'s `MapAreaRectangle`); applied one level deeper for `Army.groups` and
  `UnitGroup.units`/`UnitGroup.groups` (`ENTITY_AUTHORING_PARAMS_SPEC`). A PARAMS-shape
  consequence of the naming decision above, not a separate rule.
- **A format-native object gains a small SanGen field by direct injection when the field is
  genuinely novel information with no competing home** (`armyColor`, `alias`, and — ARCH §12 —
  `PropTransform`/`DecalTransform::layerIndex`); **a separate SanGen-owned array is used only
  when the metadata is richer than a scalar AND no format-native group container already exists
  to hold it** (`PropInstanceLayer`/`DecalInstanceLayer`, ARCH §12). This is one consistent rule
  applied per-case, not two competing philosophies — it does not reopen `armyColor`/`alias`/
  `MarkerTransform::alias`, all already-settled instances of the first branch.

