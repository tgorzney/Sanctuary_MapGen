# Work-Order SPEC-1 — Prop/blueprint format corrections (DOCS)

*Constitution §7. Executor: **SanGen ARCH Expert** (targets live under
`sangen_arch_pack/`, which only the ARCH Expert may write). Authored by the Format
Expert domain from a full read of the real game data. Status: evidence complete;
corrections NOT yet applied.*

*Numbering note: this is a documentation-correction order, not a milestone
implementation order, hence the `SPEC-` prefix rather than `M<n>-<n>`. Rename to fit
the milestone scheme if preferred.*

## Title
Correct three factually-wrong claims about prop blueprints in the arch specs, and
record the map-load abort mechanism that blocks prop export.

## Root problem
Three specs assert things about prop blueprints that are contradicted by the shipped
game data. Code written against them will fail:

1. `UNIT_PROP_MARKER_DATA_SPEC` says the `.sanpack` files hold **no definitions**.
   They hold **98 prop definitions** — the pack is the primary source.
2. `GAMEDATA_LAYOUT_SPEC` states prop folders are named `<tpId>/`. **24+ props break
   that rule.** Any path synthesized as `<code>/<code>.santp` will 404.
3. `SANMAP_FORMAT_SPEC` records that prop export is disabled because "prop formats are
   outdated" but not the actual failure mode — which is severe and non-obvious: **one
   unresolvable `blueprintPath` aborts the remainder of map load**, silently taking
   resource markers with it. Without this recorded, the exporter will ship the bug
   again.

## Target files
- `sangen_arch_pack/specs/UNIT_PROP_MARKER_DATA_SPEC.md`
- `sangen_arch_pack/specs/GAMEDATA_LAYOUT_SPEC.md`
- `sangen_arch_pack/specs/SANMAP_FORMAT_SPEC.md`
- *(outside `sangen_arch_pack/`, listed for completeness — may be applied by the Format
  Expert directly since the boundary does not cover it:)*
  `.claude/agents/sangen-format-expert.md`

## Layer & accuracy
`IO / BRIDGE` domain knowledge. Documentation only — no layer boundary changes, no
code, no ARCH amendment. Accuracy class: exact (these are file-format facts).

## Backend policy
N/A — no compute, no CPU/GPU dispatch. Documentation change.

## ARCH rules invoked
- Constitution §6 (input & asset safety, pre-alpha data unreliable) — correction 3
  supplies the concrete validate-before-export rule §6 demands for the IO layer.
- ARCH §3.3 / §5 (platform seam) — all three corrections are seam facts.
- Format Expert charter: *"You do not guess — read the format/code/resource before
  concluding."* Every claim below is backed by a file read, not inference.

## Evidence base
Read from `D:\Projects\Sanctuary\Gamedata` (uncompressed sanpack mirror) and
`engine\LJ\lua\common\props`. **98 prop blueprints parsed** across 01_Highlands (54),
02_Evergreen (6), 04_Baikal (3), 10_WhiteDesert (18), Pandemonium (17).
**Not yet audited: 03_Desert (15 folders), 09_Industrial (1).**
Height formula validated against 63,538 prop instances in `The_Forge.sanmap` and
22,528 in `Two_Step_Shuffle.sanmap`.

---

## Correction 1 — `UNIT_PROP_MARKER_DATA_SPEC.md`

**Remove** (§ header block, currently bolded):

> **The `.sanpack` files are assets only** (dds icons, editor brushes,
> models/textures) — NOT the definitions.

**Replace with:**

> Units, markers and areas are defined in lua under `engine/LJ/lua/common/`.
> **Props are not** — `Environment.sanpack` holds 98 prop *definitions*
> (`.santp`/`.sanprop` Lua templates). `props/propsTemplates/` in lua holds only 4
> (`exe0000`–`exe0002` test props + `defaultWreckage`). For props the pack is the
> primary definition source; the lua folder is not.

This also resolves an internal contradiction: the existing `## Props` section already
says environment props live in the pack.

**Add a new subsection under `## Props`:**

> ### Two incompatible prop-template dialects ship together
> A parser must handle both. Detect on the root table name.
>
> **A — Environment pack** (`Environment/*/Props/**/*.santp`, and Pandemonium's
> `.sanprop`). Root table `propTemplate` (lowercase p). 98/98 audited files.
> - `collider{center,size}`, `footprint{x,y}`, `defence.health{max,value}`
> - `economy{harvestTime, harvest{alloys, plasma}}`
> - `visuals{isWreckage, skeleton, lods[]{distance,shadowCastingMode,model,material},
>   impostor{...}}`
> - `snapping{snapToGrid,snapToGround,snapToWaterLevel,positonOffset,orientation,
>   rotationSnap,randomScaleRange}` — present in 98/98
> - optional `effects[]{type,tag,effect,…}` — `FireEffect` / `SimpleEffect` /
>   `KnockdownEffect`
> - `general{class[],name,tpId}`, `tags[]`
>
> **B — engine lua** (`engine/LJ/lua/common/props/propsTemplates/*/*.santp`).
> Root table `PropTemplate` (capital P).
> - `collisionInfo{centerOffset,collisionSize}` — *not* `collider{center,size}`
> - `harvest{alloys, energy}` — *not* `plasma`
> - `visuals.mesh{isWreckage, lod0distance, lod1distance, lod2distance}` — no `lods[]`,
>   no model/material paths, no impostor, no snapping
>
> **Load-bearing misspellings in shipped files — do NOT "correct" them:**
> `maxVerrtices` (double r), `positonOffset` (missing i).
>
> **Data-quality defects found (report upstream, do not replicate):** 9 of 98 files
> have `general.tpId` != filename — all in Pandemonium. The 8 `CrystCluster_*` files
> declare `CrystalCluster_*` ("Crystal" vs "Cryst"), and `Cliff_03.sanprop` declares
> `tpId = "Cliff_02"`, making `Cliff_02` a **duplicate tpId**. A tpId-keyed index must
> therefore detect and report collisions rather than silently overwrite.
>
> **Not a defect:** props sharing another prop's `.sanmaterial` is normal and
> intentional — 36 instances (e.g. `edbm0142`–`edbm0150` all use
> `edbm0141_lod0.sanmaterial`). A validator must not flag it.

---

## Correction 2 — `GAMEDATA_LAYOUT_SPEC.md`

**Remove** from `## Environment`:

> - `Props/<code>/` — prop mesh folders (`edbm*/edbs*/edml*/edmm*/edms*` = bush/
>   small/large/medium props); heavy 3D assets, **no stored thumbnails**.

**Replace with:**

> - `Props/` — prop folders. Heavy 3D assets, **no stored thumbnails**.
>   **Folder names are NOT derivable from the tpId.** Three distinct conventions ship:
>
>   | Set | Convention | Example |
>   |---|---|---|
>   | 01_Highlands, 02_Evergreen, 04_Baikal | `<tpId>/<tpId>.santp` | `edbm0149/edbm0149.santp` |
>   | 10_WhiteDesert | `<tpId>_<description>/<tpId>.santp` | `edmm0301_chalkrock_01/edmm0301.santp` |
>   | 03_Desert | Quixel asset IDs, no tpId code at all | `Nature_Rock_vd5rfiq_4K_3d_ms/` |
>   | Pandemonium | flat `<Name>.sanprop`, shared `Models/` + `Materials/` siblings | `CrystCluster_B1.sanprop` |
>
>   **Rule: always use the literal `blueprintPath` from the `.sanmap`. Never synthesize
>   `<code>/<code>.santp`** — it breaks on 9 WhiteDesert props, all 15 03_Desert props,
>   and all 17 Pandemonium props.
>
>   Prop-code prefixes (`edbm/edbs/edml/edmm/edms`) remain a useful *size/class* hint
>   where present, but are absent in 03_Desert and Pandemonium.

**Also correct** the biome list note: `Winter` is listed as a biome but has **no
`Props/` directory** — only sets with a `Props/` folder can contribute to a prop index.

---

## Correction 3 — `SANMAP_FORMAT_SPEC.md`

**Amend** the known-gaps entry:

> (2) **props export is disabled** — commented out because "many prop formats
> are outdated, causing maps to fail loading."

**Replace with:**

> (2) **props export is disabled** — commented out because "many prop formats are
> outdated, causing maps to fail loading." **Root cause identified (in-game, 2026-08):
> a single unresolvable `blueprintPath` aborts the remainder of map load.** Props
> parsed before the bad entry still render; everything parsed *after* props — the
> `markers` block above all — silently never spawns. The observable symptom is
> "my alloy points disappeared", not "a prop is missing", which is why this was
> mis-attributed to prop formats being outdated.
>
> **Therefore (Constitution §6): the exporter MUST verify every `blueprintPath`
> resolves to a real file before writing a `.sanmap`.** An unverified path is a
> map-breaking defect, not a cosmetic one. This is the single highest-value
> validation in the IO layer.

**Add** to the notes section:

> ### Sampling terrain height for entity placement (empirically confirmed)
> `Textures/heightmap.raw` is headerless little-endian `uint16`, `N×N` where
> `N = heightmapResolution`. World-space Y for an entity at world `(x, z)`:
>
> ```
> row = z
> col = (N - 1) - x            // the documented flip, applied to the index
> y   = bilinear(heightmap[row][col]) * height / 65536.0
> ```
>
> `height` is the map's terrain vertical extent (`128` on Pandemonium, `410` on
> Two_Step_Shuffle, `1600` on The_Forge) — **read it from the map, never hardcode.**
>
> Validation: **median absolute error 0.0105** world units over 63,538 prop instances
> in `The_Forge.sanmap`; mean 0.311. Alternatives ruled out decisively — un-flipped
> `(z, x)` gives mean error 28.1, transposed `(x, z)` gives 22.8.
>
> **Open question, deliberately not resolved here:** which axis carries the flip cannot
> be determined from shipped maps. `(N-1-z, x)` and `(z, N-1-x)` disagree on only 28 of
> 22,528 Two_Step_Shuffle instances, and on those the winner is a 53.6% coin flip,
> because every official map tested is symmetric. The documented convention
> (`world.z = length - z - 1`) stands; do not "fix" it on the strength of a
> better-looking residual. A decisive test needs an asymmetric map.

**Also amend** the feature-survey line:

> Blueprints are `.santp` paths

to note that **`.sanprop` is still a live extension** — the entire Pandemonium set
ships as `.sanprop` containing dialect-A Lua. An extension allow-list of `.santp` only
will reject valid blueprints.

---

## Correction 4 — `.claude/agents/sangen-format-expert.md` *(outside the pack)*

In **Truths you enforce**, amend the fix-targets line:

> Fix-targets: identity-quaternion export (rotation unimplemented); props export
> disabled; single-pass memory-mapped sanpack ingestion (never 2 GB in RAM); validate
> every external file (Constitution §6).

**Append:**

> Props export is blocked by one specific defect: **an unresolvable `blueprintPath`
> aborts the rest of map load and silently kills the `markers` block.** Every exported
> blueprintPath must be resolved against the real pack before write. Resolve paths
> **literally** — prop folder naming is inconsistent across biome sets, so never
> synthesize `<tpId>/<tpId>.santp`. Two prop-template dialects ship simultaneously
> (`propTemplate` vs `PropTemplate`); a reader must branch on the root table name.

## Solution + performance estimate
Documentation edit; **no runtime performance impact — N/A by nature of the change.**

Downstream performance basis, for whoever implements the validation this order
mandates: resolving ~114 blueprint paths against an in-memory pack directory is a hash
lookup per path — microseconds total, negligible against export I/O. There is no
performance argument for skipping it.

## Lossy alternative
None applicable — a factual correction has no lossy variant. Corrections 1 and 2 fix
statements that are *false*; partial application leaves code being written against
wrong facts.

If scope must be cut, apply in this priority order:
1. **Correction 3** (map-load abort) — prevents shipping a map-breaking exporter.
2. **Correction 2** (folder naming) — prevents a path-synthesis bug across 41+ props.
3. **Correction 1** (definitions location + dialects).
4. **Correction 4** (agent charter).

## Acceptance test
1. `grep -c "assets only" UNIT_PROP_MARKER_DATA_SPEC.md` → `0`.
2. `UNIT_PROP_MARKER_DATA_SPEC.md` contains both `propTemplate` and `PropTemplate` and
   states the root-table-name discriminator.
3. `GAMEDATA_LAYOUT_SPEC.md` no longer claims `Props/<code>/` is the universal layout,
   and contains the words "literal" and "never synthesize".
4. `SANMAP_FORMAT_SPEC.md` states that an unresolvable `blueprintPath` aborts map load
   and that the exporter must validate paths before write.
5. `SANMAP_FORMAT_SPEC.md` contains the height formula and the `/65536` divisor, and
   preserves `world.z = length - z - 1` unchanged.
6. No `.h`, `.cpp` or `ARCH.md` file is modified by this order.

## Out of scope
- **Any code change.** No exporter/validator is implemented here; this order only
  records the facts a later implementation order will cite.
- **`ARCH.md` and `CONSTITUTION.md`** — no law is amended. Correction 3 applies
  existing §6, it does not extend it.
- **03_Desert (15 blueprints) and 09_Industrial (1)** — not yet audited. The 03_Desert
  Quixel naming is confirmed from the directory listing, but their `.santp` contents
  are unread. A follow-up order should close this gap before any claim of "all prop
  blueprints" is made.
- **Fixing the upstream data defects** (`CrystalCluster_*` tpIds, the duplicate
  `Cliff_02`) — those are dev-side pack bugs to report, not SanGen's to patch.
- **Repacking `Environment.sanpack`** — out of scope for docs and for SanGen.
- **The heightmap flip-axis question** — deliberately left open above; needs an
  asymmetric map to settle.
