[← ARCH index](ARCH.md) · [§1 ARCH_01_NamingLaw](ARCH_01_NamingLaw.md) · SanGen ARCH §1.9. **Only the ARCH Expert writes this file.**

### 1.9 "Id" is banned — resolved once, binding on every current and future field (closes the question raised twice: `DESIGN_Assembly_R1.md` §7, `DESIGN_MarkerGroupLayerRestructure_R1.md` §7 item 3)
§16.5 already ruled "Id" out for one field (`symmetryGroupIdentifier`, not `symmetryGroupId`).
That was a single-field ruling; two later design rounds independently re-raised the identical
question for their own new fields rather than treating §16.5 as settled law. **Ruled here,
generally, so it never needs asking a third time:**

- **"Id" is not on §1.1's permitted-abbreviation list (file extensions, format-dictated
  identifiers, `Cpu`/`Gpu`) and never will be.** Every stable-identity integer field, in every
  PARAMS type, present or future, is spelled `identifier` (bare) or `<noun>Identifier` (qualified),
  never `id` or `<noun>Id`. This is not a new rule — it is §1.1/§16.5 applied without a per-feature
  re-litigation.
- **New types ratified by this session use the correct spelling from day one:**
  `Params::Assembly::identifier`/`parentIdentifier` (`DESIGN_Assembly_R1.md` §5 already, correctly,
  used this spelling — confirmed, not corrected), the new per-instance
  `assemblyIdentifier` field on `PropTransform`/`DecalTransform`/`MarkerTransform` (**not**
  `AssemblyId`, the brief's working spelling — corrected here), and the new
  `MarkerLayerBundle::identifier`/`parentBundleIdentifier`/`assemblyIdentifier` fields (§19.3).
- **Retroactive defect, confirmed real and now shipped — not a "before it ships" freebie.**
  `SANMAP_FORMAT_SPEC.md` Correction 16's own text (and `DESIGN_MarkerGroupLayerRestructure_R1.md`
  §0/§7 item 12, relaying it) states `MarkerInstanceLayer::layerId` "predates [§16.5], STEP60/STEP56
  are still undispatched work-orders, so no shipped code needs migrating if ARCH acts before either
  lands." **That premise is false as of this ratification** — direct read of the live tree
  confirms STEP56/STEP60/STEP111/STEP116 have all shipped:
  `src/params/PropInstance_PARAMS.h` (`PropInstanceLayer::layerId`, `DecalInstanceLayer::layerId`,
  `ResolvePropInstanceLayerId`/`ResolveDecalInstanceLayerId`) and
  `src/params/MarkerInstance_PARAMS.h` (`MarkerInstanceLayer::layerId`) are real, live code, and
  the wire keys are real and live too (`MapExporter_Markers_IO.cpp:73` writes `layerJson["Id"]`,
  `MapImporter_Markers_IO.cpp:132` reads `"Id"`; the identical shape ships for `PropGroups`/
  `DecalGroups`). **This is therefore a real, standing, already-shipped naming-law violation, not
  a pre-ship correction.**
- **The fix (C++ + wire) is ruled, not built — routed as a coder work-order, IO Architecture
  Expert territory for the migration mechanics, not blocking this ratification's new Group/Bundle
  ticket.** The target shape:
  - `PropInstanceLayer::layerId` → `layerIdentifier`; `DecalInstanceLayer::layerId` → `layerIdentifier`;
    `MarkerInstanceLayer::layerId` → `layerIdentifier`.
  - `ResolvePropInstanceLayerId`/`ResolveDecalInstanceLayerId` → `ResolvePropInstanceLayerIdentifier`/
    `ResolveDecalInstanceLayerIdentifier` (same signature, same bounds-check body — pure rename).
  - Wire key `"Id"` → `"Identifier"` on `PropGroups`/`DecalGroups`/`MarkerGroups` entries. **This is
    a real breaking rename on an already-shipped field**, unlike every other addition this pack has
    recorded as additive/no-bump — the importer must keep accepting the legacy `"Id"` key as a
    fallback for any `.sanmap` written by a build before the rename ships (read `"Identifier"` if
    present, else `"Id"`, else legacy-backfill by array index as today), so old files keep
    degrading loudly rather than losing their stable ids outright (Constitution §6). Exporters
    always write `"Identifier"` going forward. Whether this needs a `SanGenVersion` bump or is
    handled as an unconditional dual-read is the IO Architecture Expert's call, not ruled here.
  - Every call site (`Placement_Manual_PROC.cpp`, `MapCanvas_IconLayer_CullManual_UI.cpp`, both
    exporters, both importers) updates its field/function references mechanically.
  - Logged as a **standing recorded defect** alongside the pack's existing ones (`sangen_arch_pack/INDEX.md`).
- **New code never repeats this defect.** Any coder work-order touching a new stable-identity field
  is rejected on sight by this rule if it spells it `Id`/`<noun>Id` — cite §1.9, not §16.5, going
  forward (§16.5 is superseded by this section as the canonical citation for the "Id" ban; it is
  not deleted, since it still correctly documents the first instance of the ruling).
