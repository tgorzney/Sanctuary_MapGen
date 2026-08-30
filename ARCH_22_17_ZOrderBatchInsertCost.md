[← ARCH index](ARCH.md) · [§22 ARCH_22_NavmapModifierBlockers](ARCH_22_NavmapModifierBlockers.md) · SanGen ARCH §22.17. **Only the ARCH Expert writes this file.**

### 22.17 Z-order convention adopted for Navmesh; batch-insert cost ruled NOT to need a bulk-sort path (rough estimate)

**RULED: Navmesh adopts the same "index 0 is topmost, continuously size-sorted" convention**
`ARCH_14_19_AreaZOrderInversionAndImportSizeSort.md` established for `recipe.areas`, via its **own
independent, parallel** `InsertMapAreaSortedBySize`-shaped function scoped to whichever vector(s)
hold `NavmeshBlockerRectangle` (per §3.5, a pure PARAMS-resident function) — never touching
`recipe.areas` itself, a wholly separate array and convention instance, not a shared function across
domains. This is consistent with, not an exception to, §19.2's "genericity lives in the mechanism,
never the data shape": here even the *mechanism* stays per-domain, because §14.19 point 3's "one
insertion function, used everywhere this array grows" rule was always scoped to `recipe.areas`
specifically, never written as cross-domain-reusable.

**Batch-insert cost — ruled NOT to require a bulk-sort-then-batch-insert path.** This is a
**rough-estimate basis** (Constitution §7's basis-tag discipline — not benchmarked): the per-item
`InsertMapAreaSortedBySize`-shaped insert is `O(N)` per call; a mesh-ingestion bake producing the
spec's own observed range (88 to 815 rectangles per `NAVMAP_MODIFIER_BLOCKER_SPEC.md` §7's real
numbers) means at most on the order of `815² ≈ 6.6×10^5` simple float comparisons for the whole
batch — a sub-millisecond cost on any realistic target hardware by any reasonable estimate, for a
human-triggered, author-time, rare action (never per-frame, never inside a regeneration DAG per
`ARCH_22_13_BakedArtifactStorageAndDeterminism.md`).

**This also matches existing precedent exactly, rather than diverging from it.**
`MapImporter_Areas_IO.cpp::ReadAreasJson` already uses the identical per-item incremental-insert
pattern for a full from-scratch batch load (§14.19 point 3) — the structurally identical situation
(fresh array, many items, one pass) — and that ruling did not need a bulk-sort special case.
Introducing a second insertion pattern for Navmesh specifically, absent any measured cost, would be
exactly the kind of unbacked-performance-complexity Constitution §7 warns against, mirroring
`ARCH_22_11_MeshIngestionShape.md` point 4's "argues against pre-building infrastructure for a cost
that has not been shown to be real yet" reasoning applied to a different mechanism.

**If a real profiling run at actual ingestion scale ever shows a measurable stall, that is a
benchmark-backed follow-up ticket** — replacing the per-item insert with a bulk-append-plus-single-
`std::sort` for the batch-ingestion path only, never for the interactive single-rectangle
create-by-drag path (which mirrors `CreateAreaFromDrag` and only ever adds one at a time) — not a
default design decision made now on no measurement.
