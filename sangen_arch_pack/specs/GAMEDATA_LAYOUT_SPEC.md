# GAMEDATA_LAYOUT_SPEC — where everything lives in the game data

Map of the (unzipped) `Gamedata/` tree so SanGen knows where to find each asset
class. Each sanpack unzips to `Gamedata/<Name>/<Name>/...` (the internal path
repeats the pack name). Measured from the real files.

## Top level
`Gamedata/` = `Audio/`, `Editor/`, `Environment/`, `Gameplay/`, `Projectiles/`,
`UI/`, `Units/`, `VFX/`, plus `icons_cache.json`.

## Sprites are `.dds` + `.sansprite` pairs
Every UI image is a **`.dds`** (the pixels) + a small **`.sansprite`** (~430 B
descriptor — UV/pivot/metadata). SanGen loads the `.dds`; the `.sansprite`
describes it.

## UI — `UI/UI/`
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

## Gameplay — `Gameplay/`
`StrategicIcons/` = **592 battle icons, 112² DDS ≈ 29.5 MB decoded** (air1_t1_aa_*
etc.).

## Units — `Units/Units/<tpId>/`
~230 unit folders (+`default`), each with `LOD0/` (and further LODs): the 3D
assets — `<tpId>_lod0_albedo_team.dds` (~5.6 MB), `_mask.dds`, `_normal_alpha.dds`,
`.sanmaterial`, `.sanmodel` (~1.3 MB), plus `.sananimation`, `.sanskeleton`. The
unit's UI thumbnail is NOT here — it is the stored
`UI/Sprites/Icons/Units/<tpId>.dds`.

## Environment — `Environment/Environment/<Biome>/`
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
`Projectiles/Projectiles/` (Bomb/Plasma/EDA…), `VFX/`, `Audio/` — referenced by
units/weapons; not needed for map generation directly.

## Implications for SanGen
- **Stored icons total ≈ 37 MB / a few hundred files** → 1–2 atlas pages, memory
  is a non-issue (see ASSET_LOADING_SPEC).
- **Prop & unit thumbnails do not exist as files** — they must be **rendered from
  the meshes on demand and cached to disk** (a thumbnail render pass + disk atlas).
- Unit build icon path: `UI/UI/Sprites/Icons/Units/<tpId>.dds` — direct tpId key.
- Stratum textures for the map's 9 layers come from `Environment/<Biome>/Stratum/`.
