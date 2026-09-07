# Work-Order SPEC-3 — Prop culling is presence-gated, not value-gated (DOCS)

*Constitution §7. Executor: **SanGen ARCH Expert** (spec targets under `sangen_arch_pack/`)
and **SanGen Format Expert** (its own charter). Status: **solution confirmed in-game
2026-08-16**; corrections NOT yet applied. Supersedes the culling half of `SPEC-2`.*

## Title
Record the confirmed rule for making a prop always render, and retract the "raise
`renderDistance` / raise the culling radius" guidance that `SPEC-2` introduced.

## Root problem
`SPEC-2` concluded that props vanish because the LOD selector runs out of levels, and that
raising `renderDistance` extends visibility. **That was tested and is wrong.**

Sequence of what was actually tried, in order, each verified on disk and in-game:

| # | Change | Result |
|---|---|---|
| 1 | `visuals.lods[0].distance` 400 → 1500 (all 17 blueprints, repacked) | still culled |
| 2 | `impostor.cullDistance` 500 → 1500 | **no effect possible** — loader only reads it when `impostor.enabled`, which is false on 16 of 17 |
| 3 | `visuals.lods[0].distance` 1500 → 100000 (repacked) | still culled |
| 4 | `CullingTemplate` radius 10 → 100000 for these tpIds | still culled |
| 5 | **Omit `LODTemplate` and `CullingTemplate` entirely (pass `nil`), keep `MeshTemplate`** | **WORKS — props never cull** |

Symptom before the fix was culling on **both** axes at once — distance (zoom out) *and*
frustum (pan/rotate) — which is the tell that the values were being ignored, not merely set
too low.

## THE RULE
> **A prop's LOD and culling behaviour is gated on whether the attachment EXISTS, not on the
> values inside it.** With a `LODTemplate` present the engine applies distance selection on
> its own terms; with a `CullingTemplate` present it applies a frustum sphere. Arbitrarily
> large `renderDistance` / `radius` values do not disable either mechanism. The only way to
> make a prop always render is to omit both attachments while still emitting a
> `MeshTemplate`.

This matches the developer's original wording exactly — *"There is a culling template you
can add to props as well as lods. If you omit these they will always render."* It was
correct as stated; it was mis-implemented on our side as "set the values very high."

## The working shape (`propTemplateLoader.lua`)
```lua
local alwaysRender = <predicate>

local cullingTemplates = nil
if not alwaysRender then
    cullingTemplates = { EngineClasses.CullingTemplate(TemplateHeadEntityName, 10) }
end

-- inside `if tp.visuals.lods ~= nil then` …
if not alwaysRender then
    table.insert(lodTemplates, EngineClasses.LODTemplate(TemplateHeadEntityName, lodLevelTemplates, skeletonPath))
end
-- the MeshTemplate built from tp.visuals.lods[1] is left untouched

if lodTemplates and #lodTemplates == 0 then lodTemplates = nil end   -- nil, not {}
```
Two details that matter:
- **`nil`, not `{}`.** An empty table may still register the attachment. Pass nil so it is
  genuinely absent. (Not independently A/B tested — see Out of scope.)
- **The mesh must still be created.** `MeshTemplate` is built inside the `tp.visuals.lods`
  branch, so `lods` must remain populated in the `.santp`. Removing `lods` from the
  blueprint yields **no mesh at all**, not an always-rendering prop.

## Target files
- `sangen_arch_pack/specs/UNIT_PROP_MARKER_DATA_SPEC.md`
- `sangen_arch_pack/specs/ASSET_LOADING_SPEC.md`
- *(outside the pack)* `.claude/agents/sangen-format-expert.md`

## Layer & accuracy
`IO / BRIDGE` domain knowledge. Documentation only. Accuracy class: exact — this is an
observed in-game result, not an inference.

## Backend policy
N/A — documentation change.

## ARCH rules invoked
- Format Expert charter: *"You do not guess — read the format/code/resource before
  concluding."* `SPEC-2` read the code correctly but **inferred** behaviour from field names
  and shipped values rather than testing. The lesson to record: reading the schema tells you
  what fields exist, not what the engine does with them.
- Constitution §6 — unchanged.

## Corrections to apply
1. **Retract `SPEC-2` Correction 1's remedy.** Keep the finding that `CullingTemplate.radius`
   is a frustum sphere rather than a draw distance; **delete** the advice that "raising the
   largest `renderDistance` extends visibility." It does not.
2. **Add THE RULE above** to `UNIT_PROP_MARKER_DATA_SPEC` and `ASSET_LOADING_SPEC`.
3. **Record the mesh/LOD coupling**: `MeshTemplate` is only built when `tp.visuals.lods` is
   non-empty, so a blueprint can never express "always render" on its own. It requires a
   loader change. Any SanGen feature offering an always-render toggle must say so.
4. **Record that `impostor.cullDistance` is dead when `impostor.enabled = false`** (already in
   `SPEC-2` Correction 6; re-state it here since it wasted a test cycle).
5. **Format Expert charter**: add "engine behaviour is confirmed in-game, not inferred from
   schema field names" to its Truths, and note the presence-gated rule.

## Solution + performance estimate
Documentation only; **no runtime performance impact — N/A**.

Downstream note: omitting the LOD attachment means the prop renders LOD0 at every distance.
For the 17 Pandemonium props (1,180 instances) this is fine. It would **not** be safe as a
blanket default — `Two_Step_Shuffle` ships 22,528 prop instances, and forcing full-detail
meshes on all of them at any range would be a real cost. Any always-render facility must be
opt-in per prop.

## Lossy alternative
None — the rule is binary and confirmed. If scope must be cut, apply correction 1 (the
retraction) first: leaving the wrong remedy in the spec actively costs test cycles, as it did
here across four attempts.

## Acceptance test
1. No spec still advises raising `renderDistance` or the culling radius to prevent culling.
2. `UNIT_PROP_MARKER_DATA_SPEC` contains the presence-gated rule, in those terms.
3. The mesh/LOD coupling is documented — "removing `lods` gives no mesh".
4. `sangen-format-expert.md` states that engine behaviour is verified in-game, not inferred.
5. No `.h`, `.cpp` or `ARCH.md` modified.

## Out of scope
- **Which omission did it.** Both attachments were removed in the same change, so it is not
  known whether omitting the LOD template alone, the culling template alone, or only both
  together produces the fix. A one-line A/B would settle it; worth doing before this is
  offered as a SanGen feature.
- **`nil` vs `{}`.** Chosen defensively, not tested.
- **Editing `propTemplateLoader.lua`** — that file is in the user's game install, not SanGen.
  The local patch is a workaround; the upstream fix (the hardcoded radius `10` and its TODO)
  belongs to the game developers.
- **Whether official props would benefit.** Untested; the patch is deliberately scoped to the
  17 Pandemonium tpIds so official maps are unaffected.
