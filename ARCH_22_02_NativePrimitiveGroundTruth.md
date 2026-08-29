[← ARCH index](ARCH.md) · [§22 ARCH_22_NavmapModifierBlockers](ARCH_22_NavmapModifierBlockers.md) · SanGen ARCH §22.2. **Only the ARCH Expert writes this file.**

### 22.2 The native `NavmapModifierTemplate` primitive — recorded ground truth

Recorded as binding ground truth for both techniques (§22.3, §22.4), read directly from
`common/navmapModifiers.lua`, `common/loading/navmapModifierLoader.lua`,
`common/navigationLayers.lua`, `common/loading/playableAreaBarrierLoader.lua`,
`common/playableAreaBarrier.lua`, and `common/loading/unitTemplateLoader.lua` this session. Full
citation table: `NAVMAP_MODIFIER_BLOCKER_SPEC.md`'s ground-truth table.

- A `NavmapModifierTemplate` is a **world-space, axis-aligned rectangle** (`float2 size`, centered
  on the owning prefab instance's position) blocking **one named navigation layer**. **No rotation
  support exists on this primitive** — a diagonal real-world feature can only be staircase-
  approximated by axis-aligned boxes (`NAVMAP_MODIFIER_BLOCKER_SPEC.md` §6 point 6).
- Navigation layers are created **per map**, at load: Land, Amphibious, Hover, Air always;
  Submarine and Sea only if `Engine.HasWater()` is true.
- **The set of layers a prefab can ever block is fixed forever at prefab-template creation time.**
  A prefab needing to block N layers carries N modifier child entities, one per layer, built via
  `AddNavmapModifierTemplates(parentEntityName, layerNames, size, disabled, hierarchyEntities,
  navmapModifierTemplates)` when the template is constructed. An instance can have its already-baked
  layer children resized or enabled/disabled; it can never gain or lose which layers it carries.
- Two real native consumers exist today, ground truth for both (`NAVMAP_MODIFIER_BLOCKER_SPEC.md`
  §2): a unit template's `skirtSize` field (all layers **except** Air — "Structures do not block
  Air"), and the engine's own `PlayableAreaBarrier` prefab (**literally every** layer, created once
  per map, host and client, before any per-map script runs, with its prefab ID and layer list
  cached globally as `_G.PlayableAreaBarrierPrefabID`/`_G.PlayableAreaBarrierLayers`).

Cross-reference, not overlap: `UNIT_PROP_MARKER_DATA_SPEC.md` already records `skirtSize` as a
SanGen-round-tripped structure-data **field**; this section is about the Lua **execution
mechanism** the field feeds, a distinct concern.
