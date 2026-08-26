[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.11. **Only the ARCH Expert writes this file.**

### 19.11 `SANMAP_FORMAT_SPEC.md` staleness correction bundle — landed in this ratification
`DESIGN_MarkerGroupLayerRestructure_R1.md` §0/§7 item 12 flagged `SANMAP_FORMAT_SPEC.md` as stale
relative to the live exporter/importer, and asked for this correction pass to be bundled into this
ratification "if practical." Practical, and done — `sangen_arch_pack/specs/SANMAP_FORMAT_SPEC.md`
was edited directly (I own that file, §19's own charter). Summary, with citations into that file:

1. **Correction 16's `MarkerGroups` field list was missing four already-shipped fields.**
   `Locked`/`GridSnapEnabled`/`GridSnapSizeWorldUnits`/`ColorOverrideEnabled` (STEP106/STEP116) are
   real, live wire keys (`MapExporter_Markers_IO.cpp:77-80`, `MapImporter_Markers_IO.cpp:138-141`)
   that this file's own documented shape never listed. Added to Correction 16's shape block —
   documentation-only, no wire-format change.
2. **The `markers[type].transforms[name]` merged-field list was missing `iconNameOverride`
   (STEP114).** Added as a third bullet alongside `layerIndex`/`symmetryGroupIdentifier`; that
   section's heading is corrected from "two new merged fields" to "three."
3. **The `layerId`/"Id" naming defect is now RULED, not merely flagged.** Correction 16's prior text
   assumed STEP56/STEP60 were still undispatched, so no shipped code needed migrating if ARCH acted
   first — confirmed false by direct code read (§1.9). Correction 16's naming-note paragraph is
   rewritten to state the ruling (`layerId` → `layerIdentifier`, `"Id"` → `"Identifier"` with a
   legacy-fallback import path) and point at §1.9 as the binding citation.
4. **The confirmed-still-live `layerIndex` export bug is now recorded in the spec, not only in the
   design doc.** `BuildMarkerTransformJson` (`MapExporter_Markers_IO.cpp:17-39`) never writes
   `layerIndex` — confirmed by direct read, not fixed by this pass (ARCH does not write code). Noted
   in both the "Conversion / import-export logic" section (a new bullet) and the
   `markers[type].transforms[name]` section (inline on the `layerIndex` bullet), each pointing at the
   other so a future reader doesn't need to already know both exist.
5. **New Correction 19 — `MarkerLayerBundles`**, the wire shape for the new type this ratification
   adds (§19.3/§19.4), including the two merged `ParentBundleIdentifier` back-reference keys on
   `MarkersStack`/`MarkerGroups` entries (also added inline to Corrections 15/16's own shape blocks,
   so a reader of either section sees the full current shape without cross-referencing Correction 19
   separately).
6. **Correction 7's "Not the same 'Group'" paragraph gains one sentence** naming the new, third
   container-above-Layer concept (`MarkerLayerBundles`) and cross-referencing why it was deliberately
   named "Bundle" rather than "Group" (§19.1) — so a future reader landing on Correction 7's existing
   two-meanings disambiguation sees the third meaning called out in the same place.

**Not touched by this pass, left exactly as flagged, not silently dropped:** the pre-existing,
unrelated `Correction 17`-numbering gap this file already exhibits (several other files/work-orders
cite a "Correction 17" for the `Scenarios` section that has no matching `### ... Correction 17`
heading in this spec file — confirmed by grep, out of scope for this ratification, not investigated
or fixed here).
