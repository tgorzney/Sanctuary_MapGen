[← ARCH index](ARCH.md) · [§18 ARCH_18_SantpFootprintIngestion](ARCH_18_SantpFootprintIngestion.md) · SanGen ARCH §18.3. **Only the ARCH Expert writes this file.**

### 18.3 Q3 ruled: richer catalog data (footprint + tags now) stays IO-owned, asset-derived — not a new DATA-layer catalog type

**Ruled as the design recommended: option (a) for footprint + tags, option (c) for the rest.**
`DESIGN_SantpFootprintIngestion_R1.md` §7 Q3 asks where `tags`/`economy.harvest`/
`collisionInfo`/`collider`/`general.displayName` live, once the sandboxed reader (§18.1) surfaces
more than footprint. Scoped to what tickets 89 and 92 actually need — nothing more:

- **Footprint (ticket 89): unchanged, already correctly homed.** `Io::WorldFootprintSizeTable`
  (`STEP58_WorldFootprintSizeTable_IO.md`) stays exactly where it is — an IO-owned,
  templateIdentifier-keyed lookup table, the same asset-derived-cache category `AssetAtlasCache_*`
  already establishes. Ticket 89 becomes its second producer (real ingested data layered over the
  existing placeholder seed); no shape change.
- **Tags — specifically the `HARVESTABLE`-style signal ticket 92 needs: same IO-owned category, a
  sibling table, not a new field bolted onto `WorldFootprintSizeTable_IO.h`.** A
  templateIdentifier-keyed tags lookup lives in `IO`, matching `AssetAtlasCache_*`'s
  file-per-concern split (`ARCH_02_LayerDirectoryMap.md`). The exact type name/shape is the Format
  Expert's ticket-92 call, not asserted here — the same deferral precedent §16.4 already used for
  wire-key spelling. Ticket 92 then reads it exactly once, at the same human-triggered bake action
  §18.2 already mandates for footprint (never a live `PROC`/scatter read) —
  `STEP62_ReclaimPropFilter_PARAMS.md`'s own already-shipped deferred-scope note ("Auto-populating
  `bReclaimable` from a future blueprint `tags`/`HARVESTABLE` import — whether the auto-populated
  value stays overridable or becomes a live derived field is a question for whoever builds that
  importer") already anticipated exactly this mechanism; §18.2 is what answers it: the baked
  `bReclaimable` stays an ordinary, hand-overridable `PropRule`/`PropInstanceGroup` field after the
  bake (§18.2 point 3), and ingestion never becomes a mandatory dependency for the reclaim flag to
  exist (§18.2 point 4).
- **`economy.harvest`, `collisionInfo`/`collider`, `general.displayName`: explicitly deferred, not
  ruled on now.** Nothing in tickets 89/92 needs them. They feed the not-yet-scoped texture/asset
  importer effort — when that effort is actually scoped, its own catalog-shape question gets its
  own ARCH consult rather than being pre-decided here against work that does not exist yet.

**Option (b) — a new DATA-layer catalog type — is rejected, on the same grounds §18.2 already
established for footprint.** Constitution §1 defines `DATA` as generation's *computed output*,
regenerated from `PARAMS`+seed, every field owned by exactly one writing `PROC` stage. Ingested
template metadata (footprint, tags, or any of the deferred fields) is none of that — it is
external, asset-derived, ingestion-time input, not something any `PROC` stage computes. Putting it
in `DATA` would either force a `PROC` stage to "write" a value it does not compute, or force `DATA`
to persist independent of `PARAMS` — both already forbidden by §18.2's reasoning for footprint, and
equally forbidding for tags or any other ingested catalog field. `PARAMS` is not the right home
either for the *raw ingested* form specifically — `PARAMS` is reserved for the human-baked,
`.sanmap`-serialized result (§18.2's whole mechanism), not the live-from-install catalog itself;
conflating the two would reintroduce the exact non-determinism §18.2 closes. `IO` — asset-derived,
neither authored nor computed, exactly the category `AssetAtlasCache_*`/`WorldFootprintSizeTable`
already occupy — is confirmed the correct and only home for both artifacts this ruling is scoped
to.
