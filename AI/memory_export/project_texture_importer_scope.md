---
name: project-texture-importer-scope
description: "Future work — sanpack/asset texture importer for icons, thumbnails, unit data. Not started; deferred to its own conversation."
metadata: 
  node_type: memory
  type: project
  originSessionId: c9570a09-b69a-4776-9ec0-619d31582c81
  modified: 2026-08-21T02:30:49.956Z
---

Planned future feature (explicitly deferred by the user as "a different conversation," raised
during the preview-overlay-compositing design work, [[project_realworld_verification_gap]]
unrelated): a texture/asset importer that reads `.sanpack` (zip) files and unit template Lua data
out of the real game install, to source icons/thumbnails/unit blueprints for SanGen's preview
overlay layers (markers, props, units, decals).

**Design intent confirmed by the user**: ask the user for the game install root ("Demo folder")
**once**, via a single folder-picker, then derive every needed subpath internally from the known
fixed layout — do not ask separately per asset type/subfolder.

**Real paths on the user's machine** (Steam Demo build, for grounding when this conversation
happens — will differ on other machines, must not be hardcoded, only the *relative* structure
below the root is reusable):
- Root: `E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo`
- Unit templates (Lua stat data): `engine\LJ\lua\common\units\unitsTemplates\`
- Strategic icons: `engine\Sanctuary_Data\Gamedata\Gameplay.sanpack\Gameplay\StrategicIcons`
- Unit thumbnails: `engine\Sanctuary_Data\Gamedata\UI.sanpack\UI\Sprites\Icons\Units`

**Why**: single-root ask avoids repeatedly asking the user to navigate to specific subfolders 
for each asset kind; the relative layout under the game root is fixed and can be composed once
the root is known.

**How to apply**: when this conversation happens, this is the starting design constraint for the
folder-picker UX. Also relevant: SanGen already has a working disk-cache mechanism for built
atlases (`ASSET_LOADING_SPEC.md` "Disk cache" section, `AssetAtlasCache_*_IO.cpp`) — the importer
should feed into that existing pipeline, not build a new one. `.sanpack` files include `.dds`
among other texture formats.
