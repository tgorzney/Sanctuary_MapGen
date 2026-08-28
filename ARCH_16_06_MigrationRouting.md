[← ARCH index](ARCH.md) · [§16 ARCH_16_MarkerLayerSymmetry](ARCH_16_MarkerLayerSymmetry.md) · SanGen ARCH §16.6. **Only the ARCH Expert writes this file.**

### 16.6 Migration — a real breaking schema change; routed to the IO Architecture Expert, not ruled on here
`MarkerRule`'s existing per-rule `bSymmetryUseGlobal`/`symmetryMask`/`radialSymmetryRepeatCount`
triplet is **live and already exported today** (`MapExporter_MarkersStack_IO.cpp`, per the
design's own §0 premise correction). §16.1 moves this triplet up a tier onto the new
`MarkerRuleLayer` wrapper and removes it from `MarkerRule` — this is confirmed to be a genuine,
breaking `.sanmap` schema change on an already-shipped v2 field family, not a purely additive
change, and it needs a real `IO_MIGRATION_SPEC` version-step (ARCH §1.7:
`MarkersStack_Migrate_V<N>_IO.h/.cpp`, paired test, one manifest line) — **confirmed as ARCH's
call to make (yes, a migration is required), but the migration's actual mechanics are the IO
Architecture Expert's domain, not ruled on here.** One concrete sub-problem is flagged for that
expert's attention, not resolved by this ruling: today's flat `MarkersStack` array can legally
contain `MarkerRule` entries with **differing** per-rule symmetry settings; migrating to one
layer-level setting forces a decision (e.g., wrap the whole flat array as a single new
default-named layer, adopt one rule's settings as the layer's, and loudly log/report every
discarded per-rule divergence per Constitution §6 — or split into multiple single-rule layers to
preserve every distinct combination). This is real, potentially-lossy migration design, squarely
the IO Architecture Expert's charter, not asserted as decided here.

**Shipped — §16.6 fully discharged (confirmed 2026-08-27):** the IO Architecture Expert's
consult landed as `work_orders/STEP67_MarkersStackSymmetryMigration_IO.md`
(`HANDOFF_TRACK_MarkerLayerSymmetry.md`) and the mechanics are built and merged as real code, not
just designed: `src/io/MarkersStack_Migrate_V3_IO.h`/`.cpp` (the `MarkersStack_Migrate_V3`
grouping transform, `sourceVersion = 3`), registered correctly in
`src/io/Sanmap_MigrationManifest_IO.cpp`, with `src/io/MarkersStack_Migrate_V3_IO_Test.cpp`
covering all of STEP67's acceptance items, including the required
`bIndependentlySelectable`-isolation test. There is no remaining open item under §16.6 — do not
describe this migration as "still open" or "unbuilt" elsewhere in the pack; any such reference is
stale and should be corrected to point here.
