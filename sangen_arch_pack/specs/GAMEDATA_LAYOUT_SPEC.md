# GAMEDATA_LAYOUT_SPEC — where everything lives in the game data

Map of the (unzipped) `Gamedata/` tree so SanGen knows where to find each asset
class. Each sanpack unzips to `Gamedata/<Name>.sanpack.unzipped/<Name>/...` (the
internal path repeats the pack name; the first path component carries the literal
`.sanpack.unzipped` suffix — see "Top level" below). Measured from the real files.

## Top level
**`Gamedata/` is NOT at the install root.** The real path is
`<install root>/engine/Sanctuary_Data/Gamedata/` — confirmed directly by the human
against the real Steam Demo install
(`...\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Gamedata\`).

`Gamedata/` holds one entry per sanpack: `Audio.sanpack.unzipped/`,
`Editor.sanpack.unzipped/`, `Environment.sanpack.unzipped/`,
`Gameplay.sanpack.unzipped/`, `Projectiles.sanpack.unzipped/`, `UI.sanpack.unzipped/`,
`VFX.sanpack.unzipped/`, plus `icons_cache.json`. This is a **naming** correction
only, not a nesting-depth one — each unzipped pack still nests one more `<Name>/`
level inside it (`Gamedata/<Name>.sanpack.unzipped/<Name>/...`), exactly as deep as
previously recorded; only the first path component's literal name (`<Name>.sanpack.unzipped`,
not a bare `<Name>`) was wrong.

**`Units.sanpack` is the one exception: it ships as a zipped file only.** There is no
`Units.sanpack.unzipped/` on disk, and consequently no loose `Units/Units/<tpId>/` tree
of the kind every other pack has — a shorthand this spec previously used and that does
not describe anything that exists. See "Units" below.

## Sprites are `.dds` + `.sansprite` pairs
Every UI image is a **`.dds`** (the pixels) + a small **`.sansprite`** (~430 B
descriptor — UV/pivot/metadata). SanGen loads the `.dds`; the `.sansprite`
describes it.

## UI — `UI.sanpack.unzipped/UI/`
- `Sprites/` = `Backgrounds, Bars, Cursors, Encyclopedia, Frames, Icons, Panels,
  Portraits`.
- `Sprites/Icons/` = `Encyclopedia, Orders, Resources, Units, UnitSymbols`:
  - **`Units/`** — per-unit **rendered thumbnails** (64² DXT5, ~4 KB compressed /
    ~16 KB decoded), keyed by tpId (`ucl3001.dds`). 231 of them — a stored render
    of the unit model on transparent bg. **Load directly; NO unit rendering needed.**
  - `Orders/` ~28 (~72 KB ea), `UnitSymbols/` 12 (~32 KB), `Resources/` 8
    (alloy/plasma variants), `Encyclopedia/` (unit encyclopedia art).
- `Sprites/Portraits/` — ~16 character portraits (100–680 KB DDS).
- `Materials/`, `Prefabs/` — UI prefabs/materials.

## Gameplay — `Gameplay.sanpack.unzipped/Gameplay/`
`StrategicIcons/` = **592 battle icons, 112² DDS ≈ 29.5 MB decoded** (air1_t1_aa_*
etc.).

## Units — no unzipped loose tree; `Units.sanpack` is zipped-only
Unlike every other pack, `Units.sanpack` is never unzipped to a loose
`Gamedata/Units.sanpack.unzipped/Units/<tpId>/` directory — that path does not exist.
Unit assets (~230 unit folders + `default`, each with `LOD0/` etc. — the 3D
assets: `<tpId>_lod0_albedo_team.dds` ~5.6 MB, `_mask.dds`, `_normal_alpha.dds`,
`.sanmaterial`, `.sanmodel` ~1.3 MB, plus `.sananimation`, `.sanskeleton`) must be
read directly out of the zipped `Units.sanpack` archive — either by SanGen reading
the zip's internal entries directly, or by SanGen unzipping it itself into its own
cache; never by assuming a pre-unzipped tree the way the other six packs provide.
The unit's UI thumbnail is NOT inside `Units.sanpack` at all — it is the stored
`UI.sanpack.unzipped/UI/Sprites/Icons/Units/<tpId>.dds`.

## Environment — `Environment.sanpack.unzipped/Environment/<Biome>/`
Biomes: `01_Highlands, 02_Evergreen, 03_Desert, 04_Baikal, 09_Industrial,
10_WhiteDesert, Winter, Common, Dev, DysonParts, Pandemonium, Skybox, Water`.
Each biome → `Decals/`, `Props/`, `Stratum/` — **except `Winter`, which has no
`Props/` directory; only biomes with a `Props/` folder can contribute to a
prop index.**
- `Props/` — prop folders. Heavy 3D assets, **no stored thumbnails**.
  **Folder names are NOT derivable from the tpId.** Three distinct conventions ship:

  | Set | Convention | Example |
  |---|---|---|
  | 01_Highlands, 02_Evergreen, 04_Baikal | `<tpId>/<tpId>.santp` | `edbm0149/edbm0149.santp` |
  | 10_WhiteDesert | `<tpId>_<description>/<tpId>.santp` | `edmm0301_chalkrock_01/edmm0301.santp` |
  | 03_Desert | Quixel asset IDs, no tpId code at all | `Nature_Rock_vd5rfiq_4K_3d_ms/` |
  | Pandemonium | flat `<Name>.sanprop`, shared `Models/` + `Materials/` siblings | `CrystCluster_B1.sanprop` |

  **Rule: always use the literal `blueprintPath` from the `.sanmap`. Never synthesize
  `<code>/<code>.santp`** — it breaks on 9 WhiteDesert props, all 15 03_Desert props,
  and all 17 Pandemonium props.

  Prop-code prefixes (`edbm/edbs/edml/edmm/edms`) remain a useful *size/class* hint
  where present, but are absent in 03_Desert and Pandemonium.
- `Stratum/` — the terrain textures (albedo/normal/mask) the `.sanmap` stratum
  layers reference.
- `Decals/` — `.sandecal` decal images.

## Projectiles / VFX / Audio
`Projectiles.sanpack.unzipped/Projectiles/` (Bomb/Plasma/EDA…), `VFX.sanpack.unzipped/`,
`Audio.sanpack.unzipped/` — referenced by units/weapons; not needed for map generation
directly.

## Implications for SanGen
- **Stored icons total ≈ 37 MB / a few hundred files** → 1–2 atlas pages, memory
  is a non-issue (see ASSET_LOADING_SPEC).
- **Prop & unit thumbnails do not exist as files** — they must be **rendered from
  the meshes on demand and cached to disk** (a thumbnail render pass + disk atlas).
- Unit build icon path: `UI.sanpack.unzipped/UI/Sprites/Icons/Units/<tpId>.dds` —
  direct tpId key.
- Stratum textures for the map's 9 layers come from
  `Environment.sanpack.unzipped/Environment/<Biome>/Stratum/`.
- **`Units.sanpack` needs its own ingestion path** (zip-entry read or self-unzip to
  cache) distinct from the loose-directory read every other pack uses — see "Units"
  above. Any asset-loading code that assumes a `Units/Units/<tpId>/` loose tree is
  reading a path that does not exist on a real install.
