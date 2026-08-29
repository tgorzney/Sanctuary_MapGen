[← ARCH index](ARCH.md) · [§22 ARCH_22_NavmapModifierBlockers](ARCH_22_NavmapModifierBlockers.md) · SanGen ARCH §22.4. **Only the ARCH Expert writes this file.**

### 22.4 Partial/single-layer blocker requires a purpose-built prefab — confirmed shipped

**Confirmed working live in-game on Pandemonium Isthmus (2026-08-29, human confirmation: "sea
blocker working")** — a Sea-only blocker built using the pattern below, the same evidentiary bar as
§22.3's confirmed-shipped technique. One nuance preserved: the specific stray-1×1-blocker failure
mode the reasoning below describes was never itself deliberately reproduced and observed failing —
only the working alternative below was built and confirmed; treat that description as reasoned from
source, not reproduced. Full reasoning and citations: `NAVMAP_MODIFIER_BLOCKER_SPEC.md` §4.

**Ruled: §22.3's global-prefab-reuse technique must NOT be used for a partial-layer blocker.**
`_G.PlayableAreaBarrierPrefabID` has all six layers baked into its template. Enabling only the
desired layer's modifier bone on an instance leaves the other layers' modifier children at their
template-creation defaults — **`disabled = false` (active) and `size = float2(1,1)`** — silently
dropping a real, active 1×1-world-unit stray blocker on every other layer at that instance's
position. This is a correctness hazard, not a cosmetic one.

**Ruled: the correct pattern is a new, purpose-built prefab**, constructed the same way the
engine's own `CreatePlayableAreaBarrierPrefab` is constructed, passing only the desired
`layerNames` subset to `AddNavmapModifierTemplates` instead of `GetAllNavigationLayerNames()`.
There is no enable/disable path that removes a layer already baked into a template — a genuinely
different prefab is required, not a configuration of the existing one.

**Ruled: this new prefab may be a small map-local helper function living directly in that map's
own `_data.lua`** — this is NOT mandated to move into an engine file. Recommended (not mandated)
promotion path: stay map-local until the same partial-layer-subset pattern is needed by a
**second** map, then promote the construction helper (not the per-map layer-list choice) to a
shared `common/loading/*.lua` helper — the same proven-twice-before-generalized discipline this
pack applies elsewhere (`ARCH_19_02`'s genericity split; `ARCH_15_10`'s "ported only once shown
universal" reasoning). A single map's need is not sufficient grounds to touch an engine file. This
promotion has NOT happened yet and remains a genuinely open item — Technique B itself is confirmed
shipped, but it has so far been confirmed live only once (Sea-only, Pandemonium Isthmus), not the
"proven twice" bar this promotion path requires.
