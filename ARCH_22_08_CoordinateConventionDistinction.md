[← ARCH index](ARCH.md) · [§22 ARCH_22_NavmapModifierBlockers](ARCH_22_NavmapModifierBlockers.md) · SanGen ARCH §22.8. **Only the ARCH Expert writes this file.**

### 22.8 Pixel↔world coordinate convention for mask-derived rectangles — distinct from the entity-position convention

**Ruled: mask-derived rectangle coordinates use the same heightmap-sampling convention
`SANMAP_FORMAT_SPEC.md` already documents (`row = z; col = (N-1) - x`), applied in the inverse
direction** (pixel → world rather than world → pixel) — `x = (heightmapResolution - 1) - col`,
`z = row`. Empirically validated twice this session (96% agreement against 164 real prop
instances; human confirmation of correct in-game placement). Full detail:
`NAVMAP_MODIFIER_BLOCKER_SPEC.md` §8.

**Ruled, binding: this must never be conflated with the separate `.sanmap` JSON entity-position
convention** (`world.z = length - z - 1`), which `SANMAP_FORMAT_SPEC.md` itself already flags as
having an axis that cannot be fully resolved from shipped maps alone. These are two different
conventions for two different kinds of coordinates — one for raster-texture pixel indices (the
`Textures/` mask family, same family as `heightmap.raw`), one for JSON entity-position fields — and
neither should be used to "correct" or infer the other. `SANMAP_FORMAT_SPEC.md` carries a short
pointer to `NAVMAP_MODIFIER_BLOCKER_SPEC.md` §8 immediately after its own heightmap-sampling
convention, for exactly this reason.
