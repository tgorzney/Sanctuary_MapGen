# STEP241 — Revise Link propagation to uniform master/slave (retracts STEP239's Name cascade-write)

**Layer:** PARAMS + UI. **Domain:** `src/params/MarkerLink_PARAMS.h`,
`src/ui/MarkersTab_ManualLayerHelpers_UI.h`, `src/ui/MarkersTab_Links_UI.h/.cpp`,
`src/ui/MarkersTab_LinksHeaderExtras_UI.cpp`, `src/ui/MarkersTab_ManualLayerRowBody_UI.cpp`,
`src/ui/MarkersTab_ManualLayers_UI.cpp`, `src/ui/MarkersTab_BundleHeaderExtras_UI.cpp`.
**Sequence:** revises STEP239's landed code; depends on it.

Ratifies the amended `ARCH_19_31_PropagatedPropertyMechanisms.md` (+ `ARCH_19_28`,
`ARCH_19_MarkerLayerBundle.md`). Corrects `DESIGN_MarkerLink_R1.md` §3.4. Direct human ruling: a
Link is the "master," a linked Group/Layer is the "slave" — every one of its Section-equivalent
settings is disabled locally and resolved live from the Link while linked. One mechanism, no
exceptions — Name is no longer a special case.

## Session coordination

Check `ListAgents`/message peer sessions before touching EACH file above.

## Fix

1. `MarkerLink_PARAMS.h` — add `bHidden`, `iconScale`, `bGridSnapEnabled`,
   `gridSnapSizeWorldUnits`, `bSymmetryEnabled`, `symmetry` (`Params::SymmetrySetting`, needs
   `Symmetry_PARAMS.h` include), alongside the existing
   `identifier`/`name`/`bColorOverrideEnabled`/`color[4]`.
2. `MarkersTab_ManualLayerHelpers_UI.h` — add `EffectiveManualMarkerLayerName`/`Hidden`/
   `IconScale`/`GridSnapEnabled`/`GridSnapSizeWorldUnits`/`SymmetryEnabled`/`Symmetry` resolvers,
   same read-and-resolve shape as the existing `EffectiveManualMarkerLayerColor` (`linkIdentifier
   >= 0` and resolves → Link's field; else → Layer's own field). Also add the Bundle-tier
   `EffectiveMarkerLayerBundleName` (keyed off `MarkerLayerBundle::linkIdentifier` independently —
   two-tier pattern, not shared with the Layer-tier name resolver).
3. `MarkersTab_Links_UI.h` — retire `CommitMarkerLinkRename`'s cascade-write. Renaming a Link
   commits only `link.name`; it no longer writes into any bound Bundle/Layer's `name` field (those
   now resolve it live via step 2's resolvers instead).
4. `MarkersTab_LinksHeaderExtras_UI.cpp` — update `DrawMarkerLinkHeaderExtra` for step 3. Add new
   Link-side controls for `bHidden`/`iconScale`/grid-snap/symmetry, mirroring the existing
   `DrawMarkerLinkColorOverrideHeaderControl`'s shape.
5. `MarkersTab_ManualLayerRowBody_UI.cpp` — replicate
   `DrawManualMarkerLayerColorOverrideHeaderControl`'s disable-while-linked + resolver-read template
   (currently the only one with link-awareness) onto `DrawMarkerLayerIconSizeHeaderControl`,
   `DrawMarkerLayerGridSnapHeaderControl`, `DrawMarkerLayerSymmetryToggleHeaderControl` — all
   currently read/write raw fields with zero link awareness.
6. `MarkersTab_ManualLayers_UI.cpp` — `ApplyLayerListSignal`'s `ToggleVisibility` branch and the
   row-building lambda's `row.bVisible = !bHidden` both need `EffectiveManualMarkerLayerHidden` +
   a disabled-while-linked gate instead of the raw field.
7. `MarkersTab_BundleHeaderExtras_UI.cpp` — `DrawMarkerLayerBundleNodeHeaderExtra` writes
   `bundle.name` directly with zero `linkIdentifier` awareness; add the same disable-while-linked +
   `EffectiveMarkerLayerBundleName` resolver treatment (Name now governs at the Bundle tier too).

**Explicitly out of scope, un-ruled**: `bLocked` — not named by the human's correction or the ARCH
amendment. Do not add it to `Params::MarkerLink` or treat it as governed by a Link either way.

## Verify

- Renaming a Link no longer writes to any Bundle/Layer `name` field; every bound Bundle/Layer's
  name control is disabled and displays the Link's live name instead.
- Toggling a Link's `bHidden`/icon-scale/grid-snap/symmetry updates every bound Layer's resolved
  value; each Layer's own raw-field control is disabled while `linkIdentifier >= 0`.
- Un-linking (delete-Link, STEP239's existing semantics) restores each field to independently
  editable at its last-resolved value — no data loss, matches STEP239's existing delete-link test
  coverage.
- Extend existing `MarkersTab_Links_UI_Test`, `MarkersTab_ManualLayerColorOverrideHeader_UI_Test`
  (or a new sibling covering the newly-link-aware controls), `MarkersTab_ManualLayers_UI_Test`,
  `MarkersTab_BundleHeaderExtras_UI_Test` (or equivalent) to cover all six newly-governed fields.
- Full `MarkersTab_UI_Test` suite stays green.

## Out of scope

- `bLocked` (explicitly un-ruled — flag to the human before ever touching it).
- `sangen_arch_pack/INDEX.md`'s stale changelog narrative describing the old two-mechanism ruling —
  separate ARCH-owned follow-up, not this ticket.
