# Work-Order SPEC-2 — Culling / LOD schema from the engine's own generated docs (DOCS)

*Constitution §7. Executor: **SanGen ARCH Expert** (spec targets under `sangen_arch_pack/`)
and **SanGen Format Expert** (its own charter file). Raised by a developer report;
verified against the engine before authoring. Status: evidence complete; corrections
NOT yet applied.*

## Title
Record the authoritative engine template schema for LOD and culling, and correct three
claims currently believed by the Format Expert domain — two of which originated with me.

## Root problem
A developer reported: *"You can change the culling size of props using something like
`Engine.SetCullingRadius`? There is a culling template you can add to props as well as
lods. If you omit these they will always render."*

That contradicted the Format Expert's working model, which was inferred from reading
`.santp` files rather than from the engine. On investigation **the engine ships its own
generated API and template documentation**, which nobody in this project had been using.
It is ground truth and supersedes inference from asset files.

## NEW SOURCE OF TRUTH (highest priority item in this order)
`engine/LJ/lua/client/generated/doc/engineClasses.lua`  — every engine template class, field-by-field, with descriptions
`engine/LJ/lua/client/generated/doc/engineFunctions.lua` — all **422** documented `Engine.*` Lua functions
Also present: `client/generated/ffi/structs.lua`, `functionWrappers.lua`,
`luaToEngineDelegates.lua`, `engineConstructors.lua`, `generated/lua/enums.lua`.

These are **generated from the engine**. Any future question about what a template field
means is answered here first, and only then by looking at shipped assets.

## Target files
- `sangen_arch_pack/specs/UNIT_PROP_MARKER_DATA_SPEC.md`
- `sangen_arch_pack/specs/ASSET_LOADING_SPEC.md` (LOD/impostor/culling belongs to asset loading)
- `sangen_arch_pack/specs/GAMEDATA_LAYOUT_SPEC.md` (add the generated-doc location)
- *(outside the pack)* `.claude/agents/sangen-format-expert.md`

## Layer & accuracy
`IO / BRIDGE` domain knowledge. Documentation only. Accuracy class: exact — these are
verbatim engine field definitions, not measurements.

## Backend policy
N/A — documentation change, no compute.

## ARCH rules invoked
- Format Expert charter: *"You do not guess — read the format/code/resource before
  concluding."* This order exists because the domain had been reasoning from assets when
  a generated engine spec was on disk the whole time.
- Constitution §6 (input & asset safety) — unchanged, but the LOD/culling defaults below
  are what a validator must reason about.

---

## The authoritative schema (verbatim from `engineClasses.lua`)

```
---@class LocalTemplate
---@field meshTemplates        MeshTemplate[]
---@field skinnedMeshTemplates SkinnedMeshTemplate[]
---@field lodTemplates         LODTemplate[]
---@field cullingTemplates     CullingTemplate[]
---   (plus line/holes/icons/rangeRings/progressBars/particle/decal/audio/
---    localGridModifier/volumeCollider template arrays)

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

## Correction 1 — culling is a FRUSTUM sphere, not a draw-distance cutoff
**Currently believed (my claim, wrong):** *"`impostor.cullDistance = 500` is a hard wall;
the engine ceiling is 500 and every official prop uses it."*

**Actual:** the engine has no global distance cutoff concept. `CullingTemplate.radius` is a
**bounding-sphere radius for frustum visibility testing** — it governs whether an entity
counts as on-screen, not how far away it stops drawing. Distance behaviour comes entirely
from `LODLevelTemplate.renderDistance`.

What I observed (all 98 shipped blueprints carrying `cullDistance = 500`) was real, but my
interpretation of it was not. Disappearance at range is the **LOD selector running out of
levels**: past the largest `renderDistance` there is no level to select.

**Replace the claim with:** props stop drawing past their highest `renderDistance`; raising
that value is what extends visibility. `cullDistance` in `.santp` sits inside the impostor
block and most plausibly maps to the impostor LOD level's `renderDistance` — *this mapping
is inferred and NOT yet confirmed against the `.santp` deserializer.*

## Correction 2 — `Engine.SetCullingRadius` does not exist
Searched all **422** documented `Engine.*` functions. There is no `SetCullingRadius`, and
nothing matching `cull` for meshes at all — every `cull` hit in `engineFunctions.lua` is
**audio** culling (category budget/priority rules). The only mesh-adjacent cull functions
are `Engine.SetProgressBarCullHeight` / `GetProgressBarCullHeight`, which are progress bars.

The developer's *mechanism* is right — there is a culling template, and it is per-entity —
but the **Lua setter they named is not in the API surface.** Either it is C++-side only, or
it is named differently. **Action: ask the developer to confirm the exact call**, and do not
write a SanGen code path against `Engine.SetCullingRadius` until they do.

## Correction 3 — "omit and they always render" is consistent with the shipped data
The developer's third claim checks out structurally: **0 of 98** shipped prop blueprints
declare a culling template or a `renderDistance`/`isImpostor` field under those engine
names. Confirmed by scanning every `.santp`/`.sanprop` in the Environment pack for
`culling`, `renderDistance`, `isImpostor`, `maxVertices` — no matches.

So props today get whatever the loader defaults to. If a prop has **no LOD template**, there
is no distance selector and the mesh renders unconditionally; if it has **no culling
template**, there is no bounding sphere to fail the frustum test.

**Practical consequence for map authoring:** the way to make a prop always render is to
*omit* the LOD/impostor structure, not to inflate `distance`. Record both approaches; the
inflate-distance route is what has been used so far and is known to at least parse.

## Correction 4 — the misspellings ARE load-bearing; SPEC-1 was right
The `.santp` → engine mapping is **not** C++-side. It is
`engine/LJ/lua/common/loading/propTemplateLoader.lua`, and it reads the santp table
directly. It reads `tp.visuals.impostor.maxVerrtices` — **the double-r spelling, literally.**

So `SPEC-1`'s "do NOT fix these" stands for `maxVerrtices`: correcting it to `maxVertices`
would make the loader read nil. `positonOffset` is separately safe because `snapping` is
never read by this loader at all (editor-side metadata).

*(An earlier draft of this order told the reader to stop treating `maxVerrtices` as
deliberate. That was wrong and is retracted here.)*

## Correction 5 — the real root cause of props culling early: a hardcoded 10
`propTemplateLoader.lua` builds the culling template as:

```lua
local cullingTemplates = {
    EngineClasses.CullingTemplate(TemplateHeadEntityName,
        -- TODO: Proper value for this.
        -- In the engine, we had this calculation for the culling bounds:
        --   math.max(math.max(math.cmax(unit.footprint),
        --                     math.cmax(unit.collisionInfo.collisionSize)) / 1.5f, 1.0f);
        10
    )
}
```

**Every prop gets culling radius 10 regardless of mesh size or instance scale.** A crystal
at scale 12.9 has the same visibility sphere as a pebble. The TODO and the commented-out
engine formula confirm this is unfinished, and it is what the developer means by
"multiply the culling radius you supply to the prop inside `propTemplateLoader` by the size."

**Blocker on adopting the commented formula as-is:** it reads `footprint` and
`collisionInfo.collisionSize`. Every Environment blueprint declares `collider{center,size}`
(dialect A), not `collisionInfo{...}` (dialect B) — and all 17 Pandemonium props carry the
placeholder `(0.459, 0.134, 0.207)` copied from `CrystCluster_A1`. Feeding those in yields
`max(0.46/1.5, 1.0) = 1.0`, i.e. **worse than the current hardcoded 10.** Fixing culling
therefore depends on the blueprints carrying real collider/footprint values first.

## Correction 6 — `impostor.cullDistance` is dead unless impostors are enabled
The loader appends the impostor LOD level **only** inside `if tp.visuals.impostor and
tp.visuals.impostor.enabled then`. When `enabled = false`, `cullDistance` is never read.

Consequence: for the 16 of 17 Pandemonium blueprints with impostors disabled, editing
`cullDistance` has **no effect whatsoever**. Only `visuals.lods[].distance` (→
`LODLevelTemplate.renderDistance`) does anything. Any spec or tooling that presents
`cullDistance` as a visibility knob must say this.

## Correction 7 — `shadowCastingMode` in the `.santp` is ignored
The loader hardcodes it: `(lodIndex == 1) and ShadowCastingMode.On or ShadowCastingMode.Off`,
and `ShadowCastingMode.Off` for the impostor level. The per-LOD `shadowCastingMode` value
written in every shipped blueprint is never consulted. Do not expose it as a tunable.

## Correction 8 — the loader reads a field the Environment blueprints do not have
`propTemplateLoader` reads `tp.collisionInfo.collisionSize` / `tp.collisionInfo.centerOffset`
to build the selection volume collider and the "Selection Bracket" mesh. Environment
blueprints declare `collider`, never `collisionInfo`, so **that entire branch is skipped for
every Pandemonium prop** — no selection collider and no selection bracket are created.

This is the two-dialect split from `SPEC-1` biting at runtime, not just in a parser. Worth
verifying in-game whether Pandemonium props are selectable at all, and reporting upstream.

**Complete list of `.santp` fields this loader reads** (everything else is consumed
elsewhere or not at all): `general.tpId`, `defence.health.{value,max,regen}`,
`visuals.{skeleton,isWreckage,lods,holes}`, `visuals.impostor.{enabled,cullDistance,
horizontalFrames,verticalFrames,textureSize,padding,maxVerrtices,hemiOctahedron}`,
`collisionInfo.{centerOffset,collisionSize}`. Notably **not** read here: `collider`,
`economy`, `footprint`, `tags`, `snapping`, `effects`, `general.name`, `general.class`.

---

## Solution + performance estimate
Documentation only; **no runtime performance impact — N/A**.

Downstream note for whoever implements prop export: reading `engineClasses.lua` once at
build time to generate field-name constants would make SanGen's prop writer schema-accurate
by construction rather than by transcription. Cost is negligible; it is a parse of a 26 KB
annotated Lua file.

## Lossy alternative
None — Corrections 1, 2 and 4 fix statements that are wrong or overstated. If scope must be
cut, apply in this order:
1. **The new source of truth** (generated docs location) — everything else follows from it.
2. **Correction 1** (culling is frustum, not distance) — the actively misleading one.
3. **Correction 4** (misspelling advice) — currently tells a coder to preserve a probable bug.
4. **Corrections 2 and 3** (API name, omit-to-always-render).

## Acceptance test
1. `GAMEDATA_LAYOUT_SPEC.md` names `engine/LJ/lua/client/generated/doc/` as the authoritative
   template/API reference, above asset inference.
2. No spec still claims `cullDistance = 500` is an engine ceiling or a hard draw-distance wall.
3. `LODLevelTemplate` and `CullingTemplate` appear field-by-field in a spec, with
   `renderDistance` described as "first level whose render distance exceeds camera distance".
4. `SPEC-1`'s misspelling line is split — `positonOffset` documented as editor-side,
   `maxVerrtices` flagged unresolved.
5. `sangen-format-expert.md` lists the generated docs as source-of-truth item, ranked above
   "the actual `.sanmap` files, sanpacks, and lua unit/prop data".
6. No `.h`, `.cpp` or `ARCH.md` modified.

## Out of scope
- **Any code change.** No prop writer, validator or culling path is implemented here.
- **`ARCH.md` / `CONSTITUTION.md`** — no law amended.
- **Editing `propTemplateLoader.lua` itself.** That file is in the user's *game install*,
  not SanGen. Changing the hardcoded 10 is a game-side fix for the developers to make (or
  for the user to patch locally at their own risk); it is outside the ARCH layers and must
  not become a SanGen coder order.
- **The `Engine.SetCullingRadius` name.** Still not in the 422 documented functions. The
  developer's later message points at `propTemplateLoader` instead, which resolves the
  mechanism — but if a runtime setter is ever needed, confirm the name first.
- **`03_Desert` (15 blueprints) and `09_Industrial` (1)** — still unaudited from SPEC-1.
- **Whether Pandemonium props are selectable in-game** (Correction 8) — needs an in-game
  check, not a file read.
