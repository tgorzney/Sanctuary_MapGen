[← ARCH index](ARCH.md) · [§16 ARCH_16_MarkerLayerSymmetry](ARCH_16_MarkerLayerSymmetry.md) · SanGen ARCH §16.8. **Only the ARCH Expert writes this file.**

### 16.8 Spawn/Army shrink — confirmed: no new PARAMS type or field needed
R2 §2's ruling (orphan the Army, never auto-delete it, when a Spawn-category symmetry group
shrinks) is a UI-layer behavioral decision already made by the human and the UI Expert, and this
ARCH ruling confirms — it does not need a new PARAMS type or field to be legal. Confirmed by
direct code read of `Army_PARAMS.h`: `Army` carries no back-reference to any Spawn marker at all
(no `spawnMarkerName`/similar field exists), so an `Army` with no matching
`markers["Spawn"].transforms[...]` entry is **already a legal, unremarkable state today** — there
is no flag anywhere that could go stale or need updating, because there is no linkage field in
either direction to begin with; the relationship is inferred purely by matching `Army::name`
against a `MarkerTransform`'s **`name`** (the format's dictionary key — `MapExporter_Markers_IO.cpp`
writes `transforms[markerTransform.name] = ...`) at export/runtime, an association, not a stored
reference. **Corrected wording:** an earlier draft of this paragraph said "alias/name," which is
wrong and licenses a false negative — `MarkerTransform::alias` is a SanGen-added field
(`SANMAP_FORMAT_SPEC` Correction 11) the game never reads for spawn resolution; the match key is
`MarkerTransform::name` only, byte-for-byte against `Army::name`. Verified directly against
`src/io/MapExporter_Markers_IO.cpp` (`transforms[markerTransform.name] = ...` at the JSON-key
site, `json["alias"] = markerTransform.alias` as a separate sibling field) — `STEP82_ArmySpawnMarkerValidation_IO.md`
already carries this same correction in its own "matching rule" section and independently confirms
it; this ratification aligns the ARCH text to match. `work_orders/STEP49_ManualMarkersUI.md` already
documents a missing Spawn group as a tolerated soft-degrade for the identical reason. This
ratification confirms the PARAMS-shape question only; the interaction with STEP49's still-unbuilt
export-time "warn on missing Spawn group" idea (R2 §5 item 9's own routing) was the Format Expert's
call, discharged by `STEP82_ArmySpawnMarkerValidation_IO.md`.

