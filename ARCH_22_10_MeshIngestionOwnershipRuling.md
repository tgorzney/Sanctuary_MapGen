[← ARCH index](ARCH.md) · [§22 ARCH_22_NavmapModifierBlockers](ARCH_22_NavmapModifierBlockers.md) · SanGen ARCH §22.10. **Only the ARCH Expert writes this file.**

### 22.10 Mesh-ingestion + mask-generation formally enter SanGen's owned architecture — §22.9's required design consult is satisfied

**Ratifies, after reading each in full plus everything they cite:** `work_orders/DESIGN_NavmeshBlockerMeshIngest_R1.md`
(Format Expert), `work_orders/DESIGN_NavmeshBlockerMaskGeneration_R1.md` (Generator Expert),
`work_orders/DESIGN_NavmeshBlockerGeometryMath_R1.md` (Compute Optimization Expert),
`work_orders/DESIGN_NavmeshTab_UI_R1.md` (UI Expert), all dated 2026-08-30. These four documents,
together, ARE the "real, separately-scoped design consult" §22.9 required before any coder builds
toward a SanGen-native successor to the hand-run mask-to-rectangle workflow (§22.7/§22.7.1). §22.9's
gate is now **satisfied** for the file/ticket set §22.11-§22.17 name.

**§22.9's own text stands, historical rather than retracted.** It correctly described the
architecture's state on 2026-08-29 — before these four consults existed. It is not contradicted:
the hand-authored Lua half (§22.1-§22.8) is completely unaffected by this ratification, which is
purely additive. The new SanGen-owned pipeline below produces a resolved rectangle list that
**feeds into**, and is consumed by, §22.3/§22.4's existing, unmodified Lua-authoring techniques — a
human or a downstream export routine still writes the final `AddNavmapModifierTemplates`/
`SetNavmapModifiersSize` calls into `<MapName>_data.lua`, never SanGen itself. §22.3's ruling that
SanGen never writes that file (`ARCH_15_04_ThreeFileOnDiskShape.md` point 1) stands completely
unmodified.

**The layer-membership table — the actual "whether/how it enters the layer stack" ruling:**

| Layer | New capability | Ruled in |
|---|---|---|
| `SYS` | `.sanmodel` binary reader | §22.11 |
| `IO` | pack-asset resolve, visual-LOD extraction, run-scoped mesh cache | §22.11 |
| `MATH` | rigid-transform + triangle/plane primitives | §22.14 |
| `PROC` | plane-slice / layer-height / rasterize / decompose kernels | §22.12 |
| `PARAMS` | `NavLayerKind`, `NavmeshBlockerRectangle`/`Layer`/`LayerBundle`, per-layer merge settings, the `PropInstanceGroup::bCollidable` gap-fix | §22.11, §22.15 |
| `PIPELINE` | new one-shot, human-triggered bake responsibility class | §22.13 |
| `UI` | Navmesh Tab, canvas rectangle gesture | §22.15, §22.16, §22.17 |

**Scoped, not a blank check.** This ratification authorizes exactly the tickets the four documents'
own breakdown tables enumerate (the Format doc's implicit mesh-ingestion tickets; the Generator
doc's 227-232; the UI doc's 228-235), corrected/superseded wherever §22.11-§22.17 diverge from those
tables' own open questions. A future extension beyond this scope — most concretely, the
"full mesh silhouette" technique §22.12 explicitly defers for Land/Amphibious/Hover/Air — needs its
own fresh, separately-scoped consult, the same "proven/scoped before generalized" discipline this
pack applies everywhere else (§22.4's own promotion-path precedent, `ARCH_19_02`).

**No new top-level section is opened.** §22 already exists, is already titled "Navmap Modifier
blockers" (not narrowly "the Lua technique"), and already anticipated this exact successor in its
own §22.9/§22.7.1 text — extending it with §22.10-§22.17 keeps one citation surface for the whole
feature rather than splitting a single coherent capability across two top-level sections for no
reader benefit.
