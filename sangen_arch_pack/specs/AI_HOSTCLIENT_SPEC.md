# AI_HOSTCLIENT_SPEC — map AI-analyzability & host/client shared generation

Source: `engine/LJ/lua/AI/*` (AIMarkerGenerator, AIFunctions, strategy/target/location
managers), `engine/LJ/lua/{host,client}/`, the blueprint validators, and
`DETERMINISM_SPEC`. Two coupled concerns the ARCH must plan for: **(A)** a generated
map must be *analyzable and playable by the game's AI*, and **(B)** the *host/client
shared-generation* protocol for competitive play. Parts of `AI/`, `host/`, `client/`
still need a deep read (flagged at the end); this spec fixes the invariants and the
integration contract now.

## A. AI-analyzability — the invariants SanGen output must satisfy
The game AI does not read the heightmap raw; it builds a spatial analysis via
`AIMarkerGenerator.lua`, and a generated map is only viable if that analysis
succeeds. SanGen must **generate toward these invariants**, not just make pretty
terrain:
- **Pathability per movement layer.** The AI derives a terrain path map with
  `IsPathable` / `CanUnitMoveOnTerrainType` per movement layer (Land/Air/Water/
  Seabed — see `UNIT_PROP_MARKER_DATA_SPEC` layer enums). SanGen's terrain (slope
  limits, water depth, collidable props) must leave **connected, pathable regions**
  for the layers a match needs — no accidentally sealed spawns.
- **Start positions.** `GetStartPositions` must find valid, reachable, fair spawns.
  SanGen's spawn markers must land on pathable, buildable, symmetric ground.
- **Resource & expansion markers.** `GetAlloyPositions`, `GetLandExpansions`,
  `GetNavalExpansions` need alloy (mex) markers and expansion sites placed so the AI
  can reach and contest them; markers are connected by pathing. SanGen's marker
  placement (`PLACEMENT_SCATTER_SPEC`) must produce these as *analyzable* spots
  (correct marker type — `Spawn` res=false, `Alloys` res=true — and reachable), not
  just visually plausible dots.
- **Flood-fill area detection.** The AI flood-fills to find contiguous areas; tiny
  disconnected pockets or one-tile land bridges confuse it. Generation should favor
  coherent regions and validate connectivity as an output check.
- **Reclaim / collidable props are gameplay.** Props with collision affect pathing
  and reclaim value (`DETERMINISM_SPEC` scope), so their placement feeds the AI's
  spatial model — decorative props do not.

**SanGen responsibility:** add a post-generation **AI-analyzability validation pass**
that mirrors what `AIMarkerGenerator` does (per-layer pathability flood-fill, spawn/
alloy reachability, symmetry fairness) and flags a map that would fail AI analysis —
the same validate→report discipline as the blueprint validators
(`MODDING_SCRIPTING_SPEC`), applied to generated terrain instead of blueprints.

## B. Host/client shared generation
Extends `DETERMINISM_SPEC` into the multiplayer flow. The engine splits **`host/`
(authoritative)** from **`client/` (presentation)**; the shared-gen protocol rides on
that split.
- **Protocol.** The host distributes only **settings + seed** (tiny — the recipe's
  serialized `.sanmap` schema v3 SanGen-owned sections, `SANMAP_FORMAT_SPEC` —
  `GeneralMapSettings`/`HeightmapStack`/`MarkersStack`/etc., not the legacy
  `mapGeneratorData` blob those sections replaced — plus seed); every client runs the
  **identical deterministic generation locally** and arrives at a bit-identical
  gameplay map. No large heightmap/texture transfer.
- **Authority boundary.** Only **gameplay-authoritative outputs** must match
  bit-for-bit across machines (heightmap incl. erosion, marker/spawn/mex/expansion
  positions, playable area, collidable props/reclaim). Visual outputs (stratum masks,
  tint, decoration) may differ per client and stay on the fast GPU path — the host
  does not need them to agree.
- **Determinism path.** Shared-gen forces the CPU **Exact + Deterministic** class:
  portable minimax transcendentals (`MATH_SIMD_SPEC`), disciplined float, ordered
  reductions, fixed-point for erosion/flow feedback state — exactly
  `DETERMINISM_SPEC`. GPU is disabled for the authoritative bake.
- **Version pinning.** Because clients regenerate rather than download, the
  generator **version + preset version** (`PresetVersion`, `FORMAT_VERSION`, and now
  `SanGenVersion` — `SANMAP_FORMAT_SPEC`/`IO_MIGRATION_SPEC`, the field that actually
  gates which schema shape the distributed settings are in) must be pinned and
  checked across all clients — a mismatched SanGen build could produce a divergent
  map from the same seed, or (post `IO_MIGRATION_SPEC`) silently migrate the
  distributed settings differently on an out-of-date client. The protocol must
  reject version-skewed clients rather than let one silently migrate on its own.
- **Verification gate.** Shared-gen stays experimental until the cross-machine
  bit-exact test (`DETERMINISM_SPEC`) passes on all gameplay outputs, erosion tested
  hardest. Until then, fall back to transferring the baked map.

## Interaction of A and B
Shared-gen (B) only matters if the regenerated map is playable (A): every client's
locally-generated map must be identically **AI-analyzable**, so the analyzability
invariants are themselves part of the deterministic, gameplay-authoritative set. A
map that generates deterministically but seals a spawn on one path layer is still
broken — both passes run.

## Deep-read items still open (for the ARCH Expert)
`AI/` internals — `AIFunctions.lua` (233 KB), `AITargetManager` (96 KB),
`AIStrategyManager`, `AILocationCreator/Manager`, `ProfilerAI` — for the exact
pathability/marker contracts SanGen must satisfy; the full `host/` vs `client/`
authority split and the real map-distribution/lobby flow; how AI mods
(`AI-Sanctuary/-Lukas/-Uveso`) consume map markers; and the complete
`builtInDocumentation.lua` API surface. These pin the precise invariants and the
shared-gen handshake before implementation.

## Ties
Builds on `MODDING_SCRIPTING_SPEC` (AI system, validators, host/client), extends
`DETERMINISM_SPEC` (shared-gen), and constrains `PLACEMENT_SCATTER_SPEC` (markers must
be AI-analyzable) and `SANMAP_FORMAT_SPEC`/`IO_MIGRATION_SPEC` (settings+seed = the
schema v3 SanGen-owned sections, version-pinned via `SanGenVersion`).
