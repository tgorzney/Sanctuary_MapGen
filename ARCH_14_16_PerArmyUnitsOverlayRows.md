[← ARCH index](ARCH.md) · [§14 ARCH_14_PreviewOverlayLayering](ARCH_14_PreviewOverlayLayering.md) · SanGen ARCH §14.16. **Only the ARCH Expert writes this file.**

### 14.16 Per-army Units overlay rows — dynamic row-per-army, real `armyIndex` plumbing for procedural units, default color palette (ARCH ruling, responds to the human's "one overlay layer per army" ask)

**A. Row cardinality: N `OverlayLayer_UI` rows for `domainKind == Units`, not one.** `overlayLayers`
is already `std::vector<OverlayLayer_UI>` with no cardinality constraint tying one `domainKind`
value to exactly one row (§14.2) — today's single "Units" row is a launch-time seeding convention
in `ConfigureDefaultOverlayLayers` (`Application_OverlaySetup_UI.cpp:30-33,46-47`), not a binding
shape. Ruled: that function seeds one `OverlayLayer_UI{domainKind = Units}` per
`recipe.armies[i]`, in roster order, instead of one shared row. No change to `OverlayDomainKind_UI`,
`OverlayLayer_UI`'s fields, or `DraggableList` — `Flat` mode (STEP200) already renders whatever
count `overlayLayers` holds (`Application_ViewLayersPopup_UI.cpp:90-105`), and the icon-culling gate
already switches per-row on `layer.domainKind` (`MapCanvas_IconLayer_CullHelpers_UI.cpp:29`,
`MapCanvas_IconLayer_CullManual_UI.cpp:217-221`), never on an assumed single Units row — both are
confirmed correct for N rows with zero code change.

**Row naming.** Reuse `ArmyRowLabel(army)` (`ArmiesTab_UI.h:82-85`) verbatim — the exact same
displayName-falls-back-to-`name`-falls-back-to-`"Army"` rule the Armies tab, Scenarios tabs, and
canvas chrome already use. No new naming logic.

**Sub-layer partition, not a resolution-formula change.** `ResolveUnitsManualSubLayer`
(`Application_OverlaySetup_UI.cpp:50-66`) keeps its existing global flat-index formula over
`recipe.armies[*].groups` completely unchanged — zero risk of an indexing regression. Only the
*seeding* step changes: `SeedUnitsManualSubLayers` (`Application_OverlaySetup_Seed_UI.cpp:66-72`)
resolves each flat index's `(armyIndex, groupIndex)` via that same function and pushes the
`OverlaySubLayerRef_UI{Manual, flatIndex}` into the one row whose army matches — the identical
"push into whichever row owns it" pattern §14.14 (`SeedMarkerDomains`) and `SeedPropReclaimDomains`
already use. §14.4 ("nested `UnitGroup` addressing is flat") is unchanged; it now composes as "flat
within one army's row" instead of "flat within one shared row."

**B. Procedural units DO carry real per-army identity — the "Faction-only, unsafe to fold" premise
relayed from the Format Expert is contradicted by live code and is withdrawn for `armyIndex`
specifically.** Direct read of the codebase, not `ScatterRule_PARAMS.h:89`'s comment alone:
- `ArmiesTab_UI.h:6-7`'s own module header states `Params::UnitRule`'s `armyIndex` "is a POSITIONAL
  index into `recipe.armies`" — not a Faction value.
- `DropUnitRulesForRemovedArmy` and `RenumberUnitRuleArmyIndicesForReorder`
  (`ArmiesTab_UI.h:97-134`) actively renumber `UnitRule::armyIndex` whenever `recipe.armies` is
  reordered or an army is deleted, exactly the maintenance a real positional index requires and a
  Faction value never would.
- `ArmiesTab_UI_Test.cpp:122-128` asserts this directly:
  `armies[unitRules[0].armyIndex].displayName == "Alpha"` after a reorder — proof the field
  resolves through `recipe.armies[]`, not a 3-value Faction enum.
- `Placement_Rules_PROC.cpp:56` (`configuration.armyIndex = rule.armyIndex;`) →
  `Placement_Emit_PROC.cpp:72` → `Data::PlacementInstance::armyIndex` (`PlacementInstance_DATA.h:50`,
  "units only; -1 for markers/props/decals") carries this same positional value through PROC
  unmodified.
- `ArmiesTab_Units_UI.cpp:45` (`rule.armyIndex = armyIndex;`, the Add-Units-to-this-army-row call
  site) mints new rules with the real row index directly, not a Faction.

`ScatterRule_PARAMS.h:89`'s inline comment ("Chosen = 0, Guard = 1, EDA = 2") is stale — predating
STEP20's retyping of this system onto real `Params::Army` rosters (`ArmiesTab_UI.h:3`) — and is
corrected in prose here; the comment text itself is a coder housekeeping fix, not an ARCH shape
change.

**Ruled: procedurally-scattered units route into the SAME per-army rows as manual units, as
`ProceduralRule` sub-layers, exactly like every other domain's rule family.** No separate "Units
(Procedural)" bucket. Seeding: for each `recipe.unitRules[i]` where `bounds-checked
0 <= armyIndex < recipe.armies.size()`, push `OverlaySubLayerRef_UI{ProceduralRule, i}` into
`overlayLayers[armyIndex]`'s row. An out-of-range `armyIndex` (corrupt/hand-edited data — never
produced by the shipped UI, which always mints a valid row index) drops the ref silently rather
than crashing or attaching to the wrong army — the same defensive floor `ResolveUnitsManualSubLayer`
already applies to Manual refs.

**C. Per-army color reads `Params::Army::armyColor` directly — retires `unitsAppearance.color` for
this purpose, no shadow copy.** `Army::armyColor[4]` (`Army_PARAMS.h:65`) is real, already-shipped,
already-round-tripped recipe data — round-trips import→export (`MapImporter_Armies_IO.cpp:87-94,106`,
`MapExporter_Armies_IO.cpp:74-75`), already consumed for tinting elsewhere
(`MapCanvas_MarkerDrag_UI.cpp:45-46`, `MapCanvas_ScenarioEditMode_DrawMarkers_UI.cpp:27-30`,
`ScenariosTab_MatchRules_UI.cpp:67-68`). Per §14.5's own rule (a domain with a real recipe-serialized
record reads/writes it directly, no shadow copy — the same posture §16 already gave
Alloy/SpawnsArmies via `Params::MarkerInstanceLayer`), each per-army row's icon tint reads
`recipe.armies[i].armyColor` at draw time. **This amends §14.5's closing line** ("Units remains
UI-session-only, unaffected") — no longer accurate for color; Units now has a real PARAMS color
source exactly as Alloy/SpawnsArmies gained one in §16. `OverlaySessionAppearance::unitsAppearance`
(`OverlayLayer_Settings_UI.h:53,62`) stops being read for Units tint; a coder work-order may drop
its now-dead `color` sub-field or leave it unused — either is a small cleanup, not a shape
decision. `unitsAppearance.iconScale` is **not** retired: the human's ask was specifically about
color; icon scale stays one shared UI-session default applied uniformly across every per-army Units
row (forking it per-army is a legitimate future ask, not designed in now — same posture §14.5
already takes for undesigned per-domain fields).

**D. Default per-army color — porting v1's rotating palette is IN SCOPE here, as a named
prerequisite, because the feature is inert without it.** Confirmed (direct read, not
inference): `Army::armyColor` defaults to plain white (`Army_PARAMS.h:65`); no call site —
`ArmiesTab_UI.cpp:71-77`'s Add-Army button, `MapImporter_Armies_IO.cpp`'s importer, nor
`MapExporter_Armies_IO.cpp`'s exporter — ever assigns a distinct default. v1 had an 8-color rotating
palette assigned at `armyIndex % 8`; migrated v1 files already carry real colors via
`EntityCollections_Migrate_V2_IO.cpp`'s `Color` → `armyColor` move, so the live gap is narrower than
"every map" — it is: (1) armies added fresh in v2/v3 via "Add Army," and (2) hand-authored/partial
`.sanmap` JSON with no `armyColor` key at all. Both resolve to the same fix.

**Single source of truth — one palette constant, `PARAMS`-homed, the `kSpawnMarkerGroupName`
precedent (§14.14).** `Params::kDefaultArmyColors` — a plain ordered `inline constexpr float
kDefaultArmyColors[8][4]` (rotation-by-index consumption pattern; a dictionary would only suit a
name-keyed lookup, which this is not) — declared in `src/params/Army_PARAMS.h` beside `Army`, so
both `UI` (Add Army) and `IO` (import backfill) — which already legally depend on `PARAMS` — share
one definition instead of v1's five copy-pasted inline arrays (the human's actual stated
requirement: one definition, not a mandated container shape). Canonical values, index = roster
position mod 8:

```cpp
inline constexpr float kDefaultArmyColors[8][4] = {
    { 1.0f, 0.0f, 0.0f, 1.0f },   // 0 Red
    { 1.0f, 0.4f, 0.7f, 1.0f },   // 1 Pink
    { 1.0f, 0.5f, 0.0f, 1.0f },   // 2 Orange
    { 0.5f, 0.0f, 0.5f, 1.0f },   // 3 Purple
    { 0.0f, 0.0f, 1.0f, 1.0f },   // 4 Blue
    { 0.0f, 0.5f, 0.5f, 1.0f },   // 5 Teal
    { 0.0f, 0.5f, 0.0f, 1.0f },   // 6 Green
    { 0.2f, 0.8f, 0.2f, 1.0f },   // 7 LimeGreen
};
```

**Two mint sites, both bounds-safe against reorder because color is a field on the `Army` struct
itself (moves with it on any vector reorder, never a parallel array):**
- **Add Army** (`ArmiesTab_UI.cpp:71-77`): before `push_back`, roster position is `armies.size()`
  (0-based, unambiguous single call site) — assign
  `army.armyColor = kDefaultArmyColors[armies.size() % 8]` alongside the existing `displayName` seed.
- **Import backfill** (`MapImporter_Armies_IO.cpp`): the discriminator is **explicit-key presence**,
  never a value comparison against white (a value check would silently overwrite a genuinely-authored
  white army). `ReadArmyColorJson` already early-returns when `"armyColor"` is absent
  (`MapImporter_Armies_IO.cpp:88`) — ruled: it reports whether it found the key (return `bool`, the
  same pattern `ReadJsonFloat` already uses elsewhere in this file), and when absent, the caller
  backfills `kDefaultArmyColors[rosterPosition % 8]`. Roster position = this army's final position in
  `recipe.armies` — confirmed stable and safe to use at parse time: `NormalizeArmyIdentities`
  (`MapImporter_ArmyIdentityNormalize_IO.cpp:28-49`) renames `army.name` in place, per-index; it never
  reorders the `armies` vector itself, so parse-order position already equals final roster position.
  Exact mechanism (thread the index through `ReadNameKeyedObject`'s generic lambda vs. a small
  second pass over the same JSON object re-checking key presence per entry) is a coder's
  implementation choice — both are cheap at this cardinality (tens of armies) and neither changes
  this ruling's policy.

**Not this ruling's concern.** `MapExporter_Armies_IO.cpp` already writes `armyColor` unconditionally
regardless of source (default-struct, backfilled, or hand-set) — no export-side change.

**Dispatchable as four small, independently shippable pieces** (a coder work-order may split or
combine them): (A) per-army row seeding, (B) procedural-unit routing via the now-confirmed-real
`armyIndex`, (C) per-army tint reading `Army::armyColor` directly, (D) the default-palette
prerequisite. (D) should land no later than (C) — (C) is visually inert without it.
