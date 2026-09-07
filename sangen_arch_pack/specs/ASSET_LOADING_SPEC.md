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

## LOD, culling and impostor schema (props)
Source of truth: the engine's own generated docs,
`engine/LJ/lua/client/generated/doc/engineClasses.lua` (verbatim below) — ranked
**above** inference from shipped `.santp` files (`GAMEDATA_LAYOUT_SPEC.md` names
this directory as the authoritative schema source). The consuming loader is
`propTemplateLoader.lua` in `engine/LJ/lua/common/loading/`.

```
---@class LODTemplate
---@field entityName        string
---@field lodLevelTemplates LODLevelTemplate[]  ordered by detail, highest to lowest
---@field skeletonPath      string?  REQUIRED for LOD on Skinned Mesh Renderers;
---                                  null means the levels are treated as Mesh Renderer

---@class LODLevelTemplate
---@field meshPath         string
---@field materialPath     string
---@field renderDistance   number   Maximum camera distance at which this LOD level is
---                                 used. The engine selects the FIRST level whose
---                                 render distance exceeds the camera distance.
---@field shadowCastingMode ShadowCastingMode
---@field horizontalFrames integer  impostor atlas
---@field verticalFrames   integer  impostor atlas
---@field textureSize      integer  impostor atlas resolution, px
---@field padding          integer  impostor atlas frame padding, px
---@field maxVertices      integer  max vertices for the impostor mesh
---@field hemiOctahedron   boolean  upper hemisphere only when true
---@field isImpostor       boolean  render a billboard from the atlas instead of the mesh

---@class CullingTemplate
---@field entityName string
---@field radius     number  The culling sphere radius used for visibility testing.
---                          Entities outside the camera frustum by this radius are
---                          not rendered.
```

**Culling is a frustum-sphere test, not a distance cutoff.** `CullingTemplate.radius`
is a bounding-sphere radius for frustum visibility testing — it governs whether an
entity counts as on-screen, not how far away it stops drawing. Distance behaviour
comes entirely from `LODLevelTemplate.renderDistance`, per the field description
above. `Engine.SetCullingRadius` does **not** exist — searched all 422 documented
`Engine.*` functions; every `cull` hit is audio-category culling, unrelated to meshes.

**Presence-gated, not value-gated (confirmed in-game).** Whether a prop's LOD/culling
behaviour applies at all is gated on whether the `LODTemplate`/`CullingTemplate`
attachment exists on the entity, not on the values inside it. Raising `renderDistance`
or `radius` to arbitrarily large values does nothing — tested in-game across 5
attempts and confirmed inert on both axes (distance and frustum) at once, which is
the tell that the values were being ignored rather than merely set too low. The only
way to make a prop always render is to omit both attachments (`nil`, not an empty
table — an empty table may still register the attachment) while still emitting the
`MeshTemplate`. See `UNIT_PROP_MARKER_DATA_SPEC.md` for the prop-authoring statement
of this rule.

**Mesh/LOD coupling.** `propTemplateLoader.lua` builds `MeshTemplate` from
`tp.visuals.lods[1]` only inside the branch gated on `tp.visuals.lods` being
non-empty — the same branch that builds the LOD level templates. A `.santp` with
`lods` removed produces **no mesh**, not an always-rendering prop. Always-render
therefore cannot be expressed by blueprint data alone; it requires a loader-side
change (out of SanGen's control — `propTemplateLoader.lua` lives in the user's game
install, not in SanGen). Any SanGen feature offering an always-render toggle must be
opt-in per prop, never a blanket default (some maps carry 20k+ prop instances).

**Culling radius is hardcoded to 10 for every prop.** `propTemplateLoader.lua` builds
`CullingTemplate(entityName, 10)` unconditionally; a `TODO` in the loader notes the
intended formula (`math.max(math.max(math.cmax(unit.footprint),
math.cmax(unit.collisionInfo.collisionSize)) / 1.5, 1.0)`) was never wired up. Every
prop gets the same visibility sphere regardless of mesh or instance scale — a known
unfinished defect in the engine's own loader, outside SanGen's control.

**`impostor.cullDistance` is dead when impostors are disabled.** The loader only
appends the impostor LOD level inside `if tp.visuals.impostor and
tp.visuals.impostor.enabled then`. When `enabled = false` (true for most Pandemonium
props), editing `cullDistance` has no effect whatsoever — only
`visuals.lods[].distance` (→ `LODLevelTemplate.renderDistance`) does anything. Any
SanGen tooling that presents `cullDistance` as a visibility knob must say this.

**`shadowCastingMode` written in `.santp` is ignored.** The loader hardcodes it per
LOD index (`ShadowCastingMode.On` for LOD index 1, `.Off` otherwise, `.Off` for the
impostor level) — the per-LOD value written in every shipped blueprint is never
consulted. Do not expose it as a tunable.

## Async & safety
- Run ingestion + atlas build on a **background thread** (async I/O — pillar);
  the UI shows placeholders until the atlas is ready, then swaps in.
- All decoding is validated (pre-alpha files unreliable — Constitution §6). A bad
  icon never crashes or bloats; it becomes a placeholder.

## Ties
- IO/SYS layer owns this; UI only samples the finished atlas.
- Pillars: single-pass/async I/O, texture-array packing, arena buffers, validation.
- Feeds UI_FRAMEWORK_SPEC (the 100k lists) and respects the accuracy/asset rules.

## Decisions (owner) & measured footprint
- **Cache location:** user-selectable via a **"Cache folder" picker button** in the
  UI (not auto-placed). Rebuild-vs-load decided by a sanpack fingerprint
  (path + size + mtime; optional content hash).
- **Ingestion scope:** extract **everything** up-front in the single pass.
- **Atlas budget:** tuned for performance, with a **configurable max VRAM / atlas
  cap**; if the icon set exceeds the cap, icons are **adaptively downscaled** (or
  spilled to more pages) rather than failing to load.
- **Measured (real files, now exact):** stored icons total **≈ 37 MB across a few
  hundred files** — 231 unit thumbnails (64² DXT5, ~4 KB compressed ea, keyed by
  tpId), 592 strategic icons (112² ≈ 29.5 MB), ~28 orders + 12 symbols + 8
  resources + ~16 portraits (~6 MB). **→ 1–2 atlas pages; memory is a non-issue.**
  The cap is only a low-VRAM safety valve. (Full layout: `GAMEDATA_LAYOUT_SPEC.md`.)
- **Unit thumbnails ARE stored** — `UI/Sprites/Icons/Units/<tpId>.dds` holds a
  pre-rendered 64² DXT5 preview of every unit (model on transparent bg). **Load
  directly into the atlas; NO unit rendering pass needed.**
- **Prop thumbnails are NOT stored** — prop folders are heavy 3D assets
  (`.sanmodel` + multi-MB `.dds`) with no stored preview. SanGen must **render prop
  thumbnails on demand and cache them to disk** (a thumbnail render pass writing
  into the same disk atlas). This — not memory, and only for props — is the real work.
- Sprites are **`.dds` + `.sansprite`** pairs; load the `.dds`, the `.sansprite`
  is a small descriptor.
