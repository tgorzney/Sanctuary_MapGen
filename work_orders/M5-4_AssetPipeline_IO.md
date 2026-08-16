# Work-Order M5-4 — asset pipeline (sanpack → atlas → disk cache)

*Constitution §7. Milestone M5. **BATCH 1 (parallel).** Own files (IO + SYS + a UI-facing
atlas handle). Executor: SanGen Coder.*

## Root problem
`ASSET_LOADING_SPEC`: open a `.sanpack` (zip, ~2 GB, ~20k entries) **once**, extract what
the app needs in a single pass, build texture atlases, cache to disk, be done. Today the
UI re-scans the zip; that is the layer violation to remove.

## Target files
- `src/io/SanpackReader_IO.*` (mmap + single central-dir read + offset-sorted extraction),
  `src/io/AssetAtlasCache_IO.*` (atlas build + disk cache + fingerprint), and the resident
  atlas handle the UI samples (via `GpuResource_SYS` textures, M5-0b).

## Layer & accuracy
`IO` owns ingestion (loads/saves, never simulates); `SYS` owns the resident GPU atlas;
`UI` only samples. Validate every entry (Constitution §6) → placeholder on failure.

## Solution
1. Memory-map the sanpack; read the central directory once; filter to needed entries by
   path/extension; sort by offset; extract + inflate in one sequential pass; validate each.
2. Decode + pack into large atlas pages (e.g. 4096²) with a `name → {page, uvRect}`
   manifest. Unit thumbnails load direct (64² DXT5); **prop thumbnails render on demand**
   and cache into the same disk atlas.
3. Write atlas blob + manifest + source fingerprint (path/size/mtime/hash) to a
   user-selectable cache dir; on next launch, fingerprint-match → skip extraction, upload
   the blob. Run ingestion on a background thread (`ThreadPool_SYS`).

## Acceptance
Single-pass extraction (assert the central dir is read once, entries extracted in offset
order); atlas manifest resolves a known icon to the right page+UV; disk cache round-trips
(second load with matching fingerprint skips extraction); a corrupt entry becomes a
placeholder, no crash. Builds clean. (Use a small synthetic sanpack for the test.)

## Out of scope
The IconGrid widget that displays it (M5-3); prop thumbnail render quality tuning.
