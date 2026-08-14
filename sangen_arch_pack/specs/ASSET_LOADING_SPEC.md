# ASSET_LOADING_SPEC — sanpack ingestion, icon atlases, disk cache

The rule: **open a sanpack once, extract everything the app needs in a single
pass, build atlases, cache to disk, and be done.** No per-icon re-opens, never the
whole 2 GB in RAM. Owned by the **IO / SYS** layers — NOT the UI/tabs (the current
`MaterialTabs`/`main.cpp` zip-scan is a layer violation to remove).

## The problem
A `.sanpack` is a zip (miniz) up to ~2 GB with ~20k entries; the app needs
thousands of small icons/thumbnails (unit strategic icons, prop previews) to fill
the 100k-scroll lists. Naive per-icon extraction re-scans the central directory
and re-inflates repeatedly — slow and stally.

## Single-pass ingestion
1. **Memory-map** the sanpack; never copy 2 GB into RAM.
2. Read the **central directory once**; filter to just the entries the app needs
   (icons/thumbnails by path prefix/extension).
3. Sort the needed entries by **file offset** and extract them in **one sequential
   pass** (minimize seeks); inflate only those small entries.
4. **Validate each** on the way (Constitution §6): dimensions/format/size sanity;
   bad/corrupt entries get a placeholder, logged — done once, so runtime is safe.
5. Close the sanpack. It is never reopened for icons.

## Atlas build
- Decode each icon and **pack into large GPU texture-atlas pages** (e.g. 4096²),
  recording a manifest: `name → { page, uv-rect }`. Thousands of small icons
  collapse into a handful of atlas pages (evolves the existing
  `UnitAtlasTexture`/`UnitAtlasUVs`/`IconCache`).
- Runtime: virtualized lists (clipper) and the preview sample the **resident
  atlas** by UV — 100k-scroll shows thumbnails with **zero per-item file I/O**.

## Disk cache (the "be done with it")
- Write the built result to disk: the **packed atlas image(s)** (or a raw
  GPU-ready blob) + the **manifest** + a **source fingerprint** (sanpack
  path + size + mtime + content hash). Evolve `icons_cache.json` into this manifest.
- **On next launch:** if the fingerprint matches, **skip all extraction/decoding** —
  memory-map the atlas blob straight to the GPU and load the manifest. Cold start
  becomes a couple of texture uploads.
- Rebuild only when the fingerprint changes (sanpack updated). Store in a SanGen
  cache dir (or beside the map/sanpack — decision below).

## Async & safety
- Run ingestion + atlas build on a **background thread** (async I/O — pillar);
  the UI shows placeholders until the atlas is ready, then swaps in.
- All decoding is validated (pre-alpha files unreliable — Constitution §6). A bad
  icon never crashes or bloats; it becomes a placeholder.

## Ties
- IO/SYS layer owns this; UI only samples the finished atlas.
- Pillars: single-pass/async I/O, texture-array packing, arena buffers, validation.
- Feeds UI_FRAMEWORK_SPEC (the 100k lists) and respects the accuracy/asset rules.

## Open decisions
1. **Cache location & invalidation:** SanGen cache dir vs beside the sanpack;
   fingerprint = path+size+mtime(+hash)? (hash is safest but costs a full read once).
2. **Atlas budget:** icon resolution (e.g. 64²/128²) and max pages / VRAM cap.
3. **Ingestion scope:** extract *all* icons up-front (true single pass) vs lazy
   per-category. Owner leans all-up-front ("get what the app needs, be done").
