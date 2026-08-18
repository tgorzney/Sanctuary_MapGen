# Work-Order — Step 7: give `DecalRule` the symmetry override every other rule type already has

*Constitution §7. Executor: SanGen Coder. Closes the standing recorded defect from
`sangen_arch_pack/INDEX.md`: "`DecalRule` (`src/params/ScatterRule_PARAMS.h`) has no
`bSymmetryUseGlobal`/`symmetryMask` pair, and `AppendDecalRules` never resolves a symmetry mask
for decals at all — see `SANMAP_FORMAT_SPEC.md` Correction 4 and `PLACEMENT_SCATTER_SPEC.md`."
Pure precedent-replication: `MarkerRule`/`PropRule`/`UnitRule` already have this exact
field/PROC/IO/UI quadruplet; `DecalRule` is missing all four pieces of it. No new design.*

## Root problem
Confirmed by reading the real code (not just the INDEX.md note): `Params::DecalRule`
(`ScatterRule_PARAMS.h:35-49`) has no `bSymmetryUseGlobal`/`symmetryMask` fields, while
`PropRule`/`UnitRule` in the same file do (lines 29-30, 67-68). `AppendDecalRules`
(`Placement_Rules_PROC.cpp:92-106`) never calls `ResolveSymmetryMask` — every other `Append*Rules`
function does (lines 29-30, 60-61, 82-83). `ScatterRuleConfiguration`'s `symmetryMask` is left at
its default-constructed value for every decal, so **decals get zero symmetry in the running
placement code today, unconditionally**, regardless of any global or per-rule symmetry setting.
`MapExporter_Rules_IO.cpp`/`MapImporter_Rules_IO.cpp` never write/read `SymmetryUseGlobal`/
`SymmetryMask` for decals (confirmed: only 3 of the expected 4 `SymmetryUseGlobal`/`SymmetryMask`
write pairs exist, for Marker/Prop/Unit). `PropsTab_Decals_UI.cpp`'s `DrawDecalRuleStack` never
calls `DrawPlacementSymmetryAxes`, while `PropsTab_UI.cpp`/`MarkersTab_UI.cpp`/
`ArmiesTab_Units_UI.cpp` all do, identically.

## Target files
- `src/params/ScatterRule_PARAMS.h` — add `bool bSymmetryUseGlobal = true; int symmetryMask = 0;`
  to `DecalRule`, in the same position (immediately before `ScatterTransform transform;`) as
  `PropRule`/`UnitRule` already have it.
- `src/proc/Placement_Rules_PROC.cpp` — `AppendDecalRules`: add
  `configuration.symmetryMask = ResolveSymmetryMask(rule.bSymmetryUseGlobal, rule.symmetryMask, recipe.globalSymmetryMask);`
  in the same position (right after `MakeCommonConfiguration`, before the density/spacing fields)
  the other three `Append*Rules` functions already use.
- `src/io/MapExporter_Rules_IO.cpp` — `BuildDecalRuleJson`: add
  `json["SymmetryUseGlobal"] = rule.bSymmetryUseGlobal; json["SymmetryMask"] = rule.symmetryMask;`
  matching `BuildPropRuleJson`'s exact two lines and position.
- `src/io/MapImporter_Rules_IO.cpp` — `ReadDecalRuleJson`: add
  `ReadJsonBoolean(json, "SymmetryUseGlobal", rule.bSymmetryUseGlobal); ReadJsonInteger(json, "SymmetryMask", rule.symmetryMask);`
  matching `ReadPropRuleJson`'s exact two lines and position.
- `src/ui/PropsTab_Decals_UI.cpp` — `DrawDecalRuleStack`: add a
  `DrawPlacementSymmetryAxes("decalSymmetry", rule->bSymmetryUseGlobal, rule->symmetryMask, ...)`
  call, matching `PropsTab_UI.cpp:86`'s call exactly (same call site position: after
  `DrawPlacementGateSection`, before or after `DrawPlacementTransformSection` — match whichever
  position `PropsTab_UI.cpp` uses for props, since decals share the same tab family and should
  read consistently). Check `PropsTab_UI.cpp`'s and `MarkersTab_UI.cpp`'s exact call signature
  (including the trailing `previewDriver`/section-state argument) and replicate it verbatim —
  do not invent a new state field if `DecalRuleStackState` already has room for one, or add the
  minimal state field needed if it doesn't (check whether `PropRuleStackState`/the marker
  equivalent needed a new `SectionState`-typed field for their symmetry section, and mirror that).
- `src/io/MapImporter_IO_Test.cpp`/`MapFormat_TestSupport_IO.h` — extend `FillFixturePlacementRules`/
  `CheckLayerStackAndRules` (the existing round-trip fixture, already covers marker/prop/decal/unit
  rules) with a non-default `bSymmetryUseGlobal`/`symmetryMask` on the fixture's `DecalRule`, and
  assert it survives, matching how the fixture presumably already does this for `PropRule`/
  `UnitRule` (check the existing fixture first — if it doesn't yet assert symmetry fields for
  Prop/Unit either, add coverage for all three consistently rather than only Decal).

## Layer & accuracy class
PARAMS + PROC + IO + UI. Accuracy class: Exact (a settings field either round-trips and resolves
correctly, or it doesn't).

## Backend policy
CPU/GPU parity unaffected — `ResolveSymmetryMask` is a pure settings-resolution function already
shared by every other rule family; adding a fourth caller changes no dispatch behavior.

## ARCH rules invoked
- `SANMAP_FORMAT_SPEC.md` Correction 4 / `PLACEMENT_SCATTER_SPEC.md` — the already-ratified
  per-rule symmetry override pattern (`bSymmetryUseGlobal`/`symmetryMask`), being extended to the
  one rule family that was missed, not redesigned.
- Constitution §8 (total tweakability) — every rule family gets the same override capability;
  decals silently not having one is exactly the kind of inconsistency this principle forbids.
- ARCH §1.1 (`b`-prefix boolean naming) — `bSymmetryUseGlobal`, matching the existing three.

## Solution
Four mechanical, precedent-matched edits (PARAMS field, PROC resolution call, IO read/write pair,
UI widget call) plus one test-fixture extension, exactly as enumerated in "Target files" above.
Do not deviate from the existing three rule types' field order, naming, or call-site position —
the entire point of this fix is that decals become indistinguishable in shape from
markers/props/units for this one concern. If anything about the existing precedent is ambiguous
(e.g. the exact `DrawPlacementSymmetryAxes` call-site position relative to the transform/gate
sections), read `PropsTab_UI.cpp` and `MarkersTab_UI.cpp` side by side and match whichever
convention the two agree on; if they disagree with each other, stop and report rather than
guessing which one decals should follow.

## Explicit out-of-scope
- **The 16-slot symmetry-orbit buffer overflow risk** (`Params::symmetryOrbitMaximum = 16`,
  `INDEX.md`'s second standing defect) — a separate, unrelated defect; not fixed by this ticket.
- **The Global Symmetry tab UI's exclusive-choice bug** (can't combine axes at the global level) —
  separate UI defect, not this ticket's concern; this ticket only gives decals the same PER-RULE
  override capability the other three families already have, which already correctly supports
  combining axes (the bug is in the *global* tab's checkbox-group, not the per-rule mechanism this
  ticket touches).
- **Retroactively resolving existing saved `.sanmap` files' decal symmetry** — this is a new field
  with a sensible default (`bSymmetryUseGlobal = true`), so an old file simply inherits the global
  mask on import, same degrade-gracefully behavior every other new-field addition in this project
  has had. No migration needed (matches Step 6's "content-shape-only, no version bump" reasoning
  for a field addition with a safe default).

## Acceptance test
Extend the existing placement-rule round-trip test coverage (wherever `PropRule`/`UnitRule`'s
symmetry fields are currently asserted, if they are — add decal coverage alongside): a `DecalRule`
with non-default `bSymmetryUseGlobal`/`symmetryMask` survives export→import unchanged. If a PROC-
level test exists for `AppendPropRules`/`AppendMarkerRules` resolving symmetry correctly, add the
equivalent for `AppendDecalRules` (a decal rule with `bSymmetryUseGlobal = false` and a specific
`symmetryMask` produces a `ScatterRuleConfiguration.symmetryMask` equal to the rule's own mask, not
the global one — and the inverse when `bSymmetryUseGlobal = true`). Full `SanGenV2` build stays
clean; all existing tests continue to pass.
