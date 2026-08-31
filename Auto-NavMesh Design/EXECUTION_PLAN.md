# Auto-NavMesh — Execution Plan

Working plan, not official documentation. Supersedes anything conflicting in
`NAVMESH_BLOCKER_CONSOLIDATED_REFERENCE.md` (background technical notes) or the four `DESIGN_*.md`
docs in this folder (earlier drafts, kept for their calculations, not their conclusions where they
conflict with this file). No ARCH ratification, no coder dispatch until each phase is proven.

## §0. Corrected understanding (post-dates the earlier design docs)

- **Trigger:** one "Create Navmesh Blockers" button per nav-layer section header (Land/Amphibious/
  Hover/Air/Sea/Submarine), all six visible always, all disabled except the currently-proven ones.
- **Filter: user's canvas selection at click time.** Not `bCollidable`, not any PARAMS flag — not all
  props have accurate collision data, and collision is not accurate to the real mesh regardless.
  Selection is the only eligibility gate.
- **Build order:** Sea first (proves the mesh loader + water-plane intersection end to end) → Air
  next (different algorithm: local terrain slope vs. flight height, not a plane slice) →
  Submarine/Amphibious/Hover/Land later, each its own algorithm, not designed yet.
- **`.sanprop` deprecated.** `.santp` is the only supported prop template format. A `.sanprop`-only
  prop is a skip, same as the engine-lua test templates.
- **Scale is applied correctly without transforming the mesh first** — confirmed by direct algebraic
  derivation (background doc §2.1), not an approximation. No action needed here, just don't
  second-guess it later without re-deriving.
- **Props import status:** raw prop instance data (position/rotation/scale/blueprintPath) imports
  correctly today. A separate, already-known bug leaves the *layer* list empty on any real
  (non-SanGen) map — doesn't block mesh loading, but Phase 0 below builds the tool to verify this
  directly rather than trusting it.

---

## §1. Phases — each independently testable, in order

| # | Phase | Input | Output | Pass/fail test |
|---|---|---|---|---|
| **0** | Verify prop import + build a 3D mesh preview tool in the Props tab | A real (non-SanGen) `.sanmap` and a SanGen-round-tripped one | Confirmation that prop position/rotation/scale/`blueprintPath` are correct on both; a working "select a prop → render its LOD0 mesh" viewer | Open both map types, select a known prop, see its correct position in the recipe AND see its mesh render correctly on screen |
| **1** | `.sanmodel` binary reader (SYS) | Raw file bytes | `positions[]` + `indices[]` | Synthetic buffer with a hand-built known-small mesh → reader reproduces exact vertex/index values byte-for-byte. Then: a real file, viewed through Phase 0's tool, visually matches the prop's known shape |
| **2** | LOD0 path resolution (IO) | A prop's already-evaluated `.santp` table | Resolved `.sanmodel` path (min-`distance` `lods[]` entry) | Single-LOD template, 5-LOD template, and a `.sanprop`-only/Dialect-B template each resolve or skip correctly |
| **3** | Transform math — plane into local space | One instance transform (pos/rot/scale, incl. non-uniform scale) + one world plane | Local-space plane (normal + d) | Numerically compare against a brute-force reference (transform every vertex to world, intersect there) on a small synthetic mesh — results must match to float precision, including a deliberately non-uniform-scale case |
| **4** | Plane-vs-mesh clip | Local mesh + local plane | Local "below" sub-mesh triangles | Synthetic cube (or similar) sliced at a known height produces the exact expected triangle set |
| **5** | Rasterize | World-space clipped triangles | Boolean grid | A single known triangle produces the exact expected set of filled pixels |
| **6** | Mask → rectangles | Boolean grid | `{x,z,sizeX,sizeZ}` list | Reuses the already-proven, already-shipped-in-game algorithm (spec's own §7) unmodified. Mandatory test: rasterize the kept rectangles back, diff against the true mask — missed-pixel count must be exactly zero |
| **7** | Sea button, end to end | Selected props over water + Sea button click | Rectangles written into the Navmesh Tab's Sea layer, visible and editable | Select a handful of real water-adjacent props, click Sea, confirm rectangles appear in roughly the right place and can be dragged/resized/deleted like any other rectangle |
| **8** | Air (after 7 is proven) | Selected props + flight height + slope threshold (60°, same local-gradient method as the existing 30° Land/Amphibious/Hover gate) + enclosed-region correction | Rectangles in the Air layer | Not designed yet — needs: (a) confirm whether all air units share one flight height, (b) the enclosed-blocked-region flood-fill pass, (c) its own test plan once scoped |

Phases 1-6 can be built and unit-tested independently of each other and of the UI — each has a
synthetic, hand-verifiable test that doesn't require a real map or the full app. Phase 0 should
land first regardless of ordering above, since it's the tool that lets you visually verify Phase 1's
real-file output.

---

## §2. What's explicitly NOT being built yet

- Land/Amphibious/Hover/Submarine algorithms (buttons exist, stay disabled).
- Any PIPELINE/PARAMS "baking," determinism classification, or shared-generation-mode handling —
  premature before Phase 7 is proven manually.
- Any ARCH ratification, ticket numbering, or ownership-layer paperwork — per direction, that comes
  after a working implementation, not before.
