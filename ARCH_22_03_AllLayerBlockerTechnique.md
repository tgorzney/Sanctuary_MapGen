[← ARCH index](ARCH.md) · [§22 ARCH_22_NavmapModifierBlockers](ARCH_22_NavmapModifierBlockers.md) · SanGen ARCH §22.3. **Only the ARCH Expert writes this file.**

### 22.3 All-layer blocker via the global `PlayableAreaBarrier` prefab — ratified, confirmed shipped

**Status: confirmed working live in-game, twice** (human confirmation, "It worked," 2026-08-29).
**Use when** a location must block every mobility type together (air + land + sea + amphib +
hover + sub) — e.g. a decorative prop's footprint made fully solid. Zero
`engine`/`mapUtils.lua` changes required. Full sequence and citations:
`NAVMAP_MODIFIER_BLOCKER_SPEC.md` §3.

**Ratified as the correct, sole technique for the all-layer case**: reuse
`_G.PlayableAreaBarrierPrefabID` (§22.2) rather than defining a new prefab —
`Engine.InstantiatePrefab` at each desired location, then
`NavmapModifiers.GetNavmapModifierIDs(instanceId, _G.PlayableAreaBarrierLayers)` +
`SetNavmapModifiersSize` + `SetNavmapModifiersEnabled(true)` — the exact same sequence
`common/playableAreaBarrier.lua`'s own `CreateBarrier`/`SetBarrierSize`/`SetBarrierEnabled` already
use for the map-edge walls.

**Binding host/client split, mirroring `playableAreaBarrier.lua` exactly:** navmap modifiers are
**host-only** (simulation-authoritative); the client sets **only** the local building-placement
grid modifier (`Engine.SetLocalGridModifierSize`/`Enabled`), never `NavmapModifiers.*`. This is not
an optimization choice — it is the same authority split the engine's own barrier code already
enforces for this primitive.

This technique lives in the hand-authored `<MapName>_data.lua` orchestrator today, not in any
SanGen-generated file (§22.9) — no `PARAMS`/`IO`/`UI` change ships with this ruling.
