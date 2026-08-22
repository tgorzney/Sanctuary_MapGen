[← ARCH index](ARCH.md) · [§1 ARCH_01_NamingLaw](ARCH_01_NamingLaw.md) · SanGen ARCH §1.1. **Only the ARCH Expert writes this file.**

### 1.1 Literal, fully-spelled names — no abbreviations
Every file, type, method, variable, and parameter uses **complete descriptive
words**. No abbreviations, no truncations, no single-letter names.
- `centerX / centerY` (not `cx/cy`), `deltaX / deltaY` (not `dx/dy`),
  `frequency` (not `freq`), `config` (not `cfg`), `blueprintPath` (not `bp`),
  `radiusSeed` (not `rSeed`).
- **Only exceptions:** file extensions (`.sanmap`, `.dds`, `.glsl`); identifiers the
  file format or game dictates — `.sanmap` JSON keys, stratum names, and the
  format-derived PARAMS fields §1.8 governs — verbatim so import/export round-trips
  (§1.8 also lists its own named exceptions, e.g. `tpId` → `templateIdentifier`, the
  spelling actually used everywhere in `src/`); and the universally-standard hardware
  acronyms `Cpu` and `Gpu`. Our own code around them spells fully.
- **Booleans keep the `b` prefix** (`bNeedsMapUpdate`) — retained precedent; the word
  after it is still fully spelled.
- **A name must state the quantity, not the role.** A field holding a physical
  proportion is not called a "mask"; a field holding a visibility weight is not called
  a "proportion". (This rule is written in blood — see §7.2.)

