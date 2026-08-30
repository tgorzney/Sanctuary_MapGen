[← ARCH index](ARCH.md) · [§22 ARCH_22_NavmapModifierBlockers](ARCH_22_NavmapModifierBlockers.md) · SanGen ARCH §22.11. **Only the ARCH Expert writes this file.**

### 22.11 Mesh-ingestion shape — SYS reader, the `visuals.lods[]` IO fork, the mesh cache, and the manual-prop `bCollidable` gap

Ratifies `work_orders/DESIGN_NavmeshBlockerMeshIngest_R1.md` (Format Expert) as designed, with one
binding fork resolution and one real gap closed.

**1. `Sys::SanmodelMesh`/`ReadSanmodelMesh` confirmed as designed** — positions+indices only (that
doc's Q1 → option (a); `ARCH_22_14_GeometryMathAndDispatch.md`'s own advisory independently agrees
no normals are needed for a plane-intersection consumer). Revisit only if a future PROC design
states a real need, not speculatively. The bit-reinterpret discipline (`std::bit_cast`/`memcpy`
only, **never** `static_cast<int>`/`std::round`, for the segment `count` fields and every segment-7
index) is binding, and that document's own acceptance-test sentence — construct a synthetic buffer,
verify byte-for-byte index reproduction — is copied verbatim into the eventual work-order's
acceptance criteria.

**2. `visuals.lods[]` extraction — RULED: option (a), the additive sibling file.** A new
`src/io/TemplateVisualLod_IO.h/.cpp` independently re-detects the root table and extracts the
minimum-`distance` `lods[]` entry, never touching the shipped, already-ARCH-ratified
`Io::TemplateRecord`/`ParseTemplateSource`. **Reasoning:** `ARCH_18_03_CatalogDataOwnership.md`'s
deferred-scope list never named mesh/visual-LOD paths either way (confirmed by direct re-read this
session) — nothing in it technically forbids extending `TemplateRecord`. But doing so anyway would
reopen a shipped type's scope for one feature's convenience, exactly the class of scope creep §18.3
exists to prevent even where its text is silent. The sibling file costs one small (five-line)
duplicated root-table branch and buys zero risk to a shipped type. **§18.3 itself is not amended** —
this ruling settles the fork without touching that file.

**Also ruled: extract the shared helper.** `TemplateDialect_IO.cpp`'s anonymous-namespace-private
`DetectRootTable` becomes a shared `Io::DetectTemplateRootTable(const Sys::LuaTableValue& globals,
TemplateDialectKind& outKind) -> const Sys::LuaTableValue*`, reused by both `TemplateDialect_IO.cpp`
and the new sibling file. A five-line branch duplicated across two files in the same module is
exactly what `ARCH_01_05_FileSizeCeilings.md`'s naming/DRY discipline prefers named once — the
Format doc's own flag 5 correctly identified this; ARCH confirms it as binding rather than leaving
it optional.

**3. `PackAssetResolve_IO` (pack-relative path → bytes) confirmed as designed** — new, generalizes
cleanly beyond `.sanmodel`, no ARCH concern. The "confirm the sanpack entry-name spelling via
`SanpackReader::DirectoryEntries()` before trusting it" caveat is binding on the implementing coder
at implementation time, not resolved here — exactly the class of "confirm before trusting file
structure" caution Constitution §6 already requires, not a gap in this design.

**4. Per-template mesh cache — RULED: in-memory, run-scoped only** (that doc's Q3 → option (a)). No
disk cache. Same reasoning the Format doc itself gives (Constitution §7's basis-tag discipline — no
measured cost yet) and the same posture this pack already took for the sibling
footprint-ingestion cache before real corpus numbers existed.

**5. The manual-prop `bCollidable` gap — ruled and closed, not merely routed onward.** Confirmed by
direct read this session (`PropInstance_PARAMS.h`, `Placement_Manual_PROC.cpp`): `Params::
PropInstanceGroup`/`PropTransform` carries **no** collidable-equivalent field at all — this is not a
wiring omission inside `MakeManualInstance`, it is a genuinely missing PARAMS field. Every
hand-placed prop is therefore permanently invisible to any `bCollidable`-gated consumer, including
the mask-generation stage's own filter (`ARCH_22_12_MaskGenerationAlgorithmAndScope.md`).

**RULED:** add `bool bCollidable = false;` to `Params::PropInstanceGroup`, wire key `"Collidable"`,
group-level granularity — the exact same shape/precedent as the already-shipped `bReclaimable` field
on the same struct (additive, no `SanGenVersion` bump, mechanical IO round-trip mirroring
`bReclaimable`'s existing exporter/importer handling). `Placement_Manual_PROC.cpp::
ResolveManualPropsAndDecals` is authorized to populate `instance.bCollidable = group.bCollidable;`
in the same loop that already resolves `instance.manualLayerId` per group. This is small,
precedented, and mechanical — dispatched as a prerequisite to (or folded into) ticket 227
(`NavmeshBlocker_PlaneSlice_PROC`), since without it the mask-generation stage would silently
produce zero output for every manually-placed prop — a correctness gap, not a missing nice-to-have.
The Format doc's own open question ("should blocker-eligibility even be `bCollidable`-gated at
all?") is answered by this fix removing the reason to ask it differently — see
`ARCH_22_12_MaskGenerationAlgorithmAndScope.md` for the filter ruling itself.
