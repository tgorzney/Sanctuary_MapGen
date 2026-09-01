# STEP245 — Wire round-trip for `MarkerTransform::linkIdentifier` + dangling-warn ordering fix

**Layer:** IO. **Domain:** `src/io/MapExporter_Markers_IO.cpp`, `src/io/MapImporter_Markers_IO.cpp`,
`src/io/MapImporter_MarkerLink_IO.h/.cpp`, `src/io/MapImporter_ParseDocument_IO.cpp`,
`sangen_arch_pack/specs/SANMAP_FORMAT_SPEC.md`. **Sequence:** depends on STEP244.

Ratifies `ARCH_19_33_LinkMembershipInstanceTierCorrection.md`'s wire-shape ruling, confirmed by the
Format Expert and IO Architecture Expert (both consulted directly against the live code — see their
rulings folded into this ticket's Fix list below; do not re-derive, both are settled).

## Session coordination

Check `ListAgents`/message peer sessions before touching EACH file above.

## Fix

1. `MapExporter_Markers_IO.cpp`, `BuildMarkerTransformJson` — add
   `json["LinkIdentifier"] = markerTransform.linkIdentifier;` alongside the existing
   `InstanceIdentifier`/`layerIndex`/`symmetryGroupIdentifier`/`iconNameOverride` writes on the same
   transform object. PascalCase key, confirmed against the identically-shaped shipped
   `InstanceIdentifier`/Bundle-Layer-tier `LinkIdentifier` precedent — do not use lowerCamelCase.
2. `MapImporter_Markers_IO.cpp`, `ReadMarkerTransformJson` — add
   `ReadJsonInteger(json, "LinkIdentifier", markerTransform.linkIdentifier);`, bare/unvalidated, same
   posture as every other `linkIdentifier` read in this codebase (absent key → struct default `-1`, no
   clamp, no legacy-backfill counter — this is NOT `instanceIdentifier`'s uniqueness-key posture).
3. **Dangling-warn ordering fix (real bug found during design, not cosmetic):**
   `WarnDanglingMarkerLinkIdentifiers` (`MapImporter_MarkerLink_IO.cpp`) is currently called INSIDE
   `ReadMarkerLinksJson`, which itself runs in `MapImporter_ParseDocument_IO.cpp` BEFORE
   `ReadMarkersJson` — so `recipe.markers` is still empty at that call site today. Adding a third
   transform-tier loop to the existing function without fixing this would silently never fire (false
   negative, not a crash). Required restructuring:
   - `ReadMarkerLinksJson` — remove its internal call to `WarnDanglingMarkerLinkIdentifiers`; it
     becomes pure population only.
   - `MapImporter_ParseDocument_IO.cpp` (`ParseEntityDomainsJson` or equivalent) — call
     `WarnDanglingMarkerLinkIdentifiers(outRecipe, result)` explicitly, once, immediately AFTER
     `ReadMarkersJson` runs (so bundles, layers, links, AND markers are all populated).
   - Update `WarnDanglingMarkerLinkIdentifiers` itself: add a third loop over
     `recipe.markers[*].transforms[*].linkIdentifier`, identical shape/soft-warn posture to the
     existing two (Bundle-tier, Layer-tier) loops, using the same `MarkerLinkExists` helper.
   - Update the header comment on `ReadMarkerLinksJson` (`MapImporter_MarkerLink_IO.h`) — it no longer
     claims to run the warn pass. Update the ordering-rationale comment in
     `MapImporter_ParseDocument_IO.cpp` to describe the new call site and why it must run after
     `ReadMarkersJson`.
4. `sangen_arch_pack/specs/SANMAP_FORMAT_SPEC.md`, the `markers[type].transforms[name]` merged-fields
   section (~line 938-967) — add a fourth bullet for `LinkIdentifier`, explicitly noting it is
   PascalCase on the wire (an exception to that section's stated "all lowerCamelCase" framing, same as
   its sibling `InstanceIdentifier`). **Also backfill the missing `InstanceIdentifier` bullet in the
   same section while here** — it was never documented despite already shipping (Format Expert finding,
   not scope creep: same section, same fix, same PascalCase-exception note applies to both).

**No migration unit, no `SanGenVersion` bump** — confirmed by IO Architecture Expert: pure additive
scalar, identical precedent class as `instanceIdentifier`/`AssemblyIdentifier`/the two existing
Bundle/Layer-tier `LinkIdentifier` fields, none of which needed one either.

## Verify

- Round-trip test: export a recipe with a `MarkerTransform::linkIdentifier` set, re-import, confirm
  the value survives. Extend the existing `MapExporter_Markers_IO`/`MapImporter_Markers_IO` test
  files, or `MapImporter_IO_Test.cpp` if that's where transform-field round-trip coverage lives.
- Import a `.sanmap` with NO `LinkIdentifier` key on a transform — confirm it defaults to `-1`, no
  warning, no crash (absent-safe).
- Import a `.sanmap` with a `LinkIdentifier` on a transform that does NOT resolve to any
  `Params::MarkerLink` — confirm `WarnDanglingMarkerLinkIdentifiers` now fires for it (this is the
  ordering-fix's own regression check — write a test that would have silently passed before this fix).
- Confirm the ordering fix doesn't double-warn or skip-warn any pre-existing Bundle/Layer-tier
  dangling case — extend whatever test currently covers `WarnDanglingMarkerLinkIdentifiers`, or add one
  if none exists.
- Full `MapImporter_IO_Test`/`MapExporter_*_IO_Test` suites stay green.

## Out of scope

- Any resolver, UI, or PARAMS change beyond what STEP244 already landed.
- `sangen_arch_pack/INDEX.md`'s broader staleness (flagged separately by the ARCH Expert, not this
  ticket's concern beyond the one `SANMAP_FORMAT_SPEC.md` addendum above).
