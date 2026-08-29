[← ARCH index](ARCH.md) · [§22 ARCH_22_NavmapModifierBlockers](ARCH_22_NavmapModifierBlockers.md) · SanGen ARCH §22.9. **Only the ARCH Expert writes this file.**

### 22.9 Ownership/scope ruling — not yet a SanGen-owned construct

**Ruled: this entire ratification is knowledge-pack law about a hand-authored Lua technique, the
same status `MAP_UNIT_SPAWNING_SPEC.md`'s generator-function patterns already hold** — it is not,
today, backed by any `Params::`/`IO`/`UI` type, and this ratification does not create one. Both
techniques (§22.3, §22.4) live in the hand-authored `<MapName>_data.lua` orchestrator, the exact
file `ARCH_15_04_ThreeFileOnDiskShape.md` §15.4 point 1 already rules SanGen may **never write,
under any code path** — consistent with, not in tension with, that ruling.

**Recorded, not designed or scheduled** (mirroring `ARCH_15_09_EngineWhitelistMigrationPath.md`
§15.9's own "intended future simplification, not built" posture): the mask-to-rectangle workflow's
SanGen-native successor (§22.7/§22.7.1) is real prior art worth eventually formalizing into a real
SanGen feature — a masking/placement capability that authors a blocker mask, decomposes it, and
exports `NavmapModifierTemplate` rectangle lists as part of the Map Scenario/per-map orchestration
this ratification documents. **Nothing in this ratification depends on that happening**, and no
coder should build toward it without a real, separately-scoped design consult first (Constitution
§1's layer law and `ARCH_08_04_CoderScopeLaw.md` §8.4 both apply unchanged: a coder never invents a
missing type or stage).
