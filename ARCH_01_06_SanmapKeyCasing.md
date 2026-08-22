[← ARCH index](ARCH.md) · [§1 ARCH_01_NamingLaw](ARCH_01_NamingLaw.md) · SanGen ARCH §1.6. **Only the ARCH Expert writes this file.**

### 1.6 `.sanmap` top-level key casing — game-native vs SanGen-owned (ratifies work-order SPEC-4 Correction 0)
The `.sanmap` format already had a live, useful split, formalized here as binding
naming law: **camelCase top-level key = game-native field** (`width`, `armies`,
`markers`, …); **PascalCase top-level key = SanGen-owned section**
(`GeneralMapSettings`, `HeightmapStack`, `Symmetry`, `SlopeDefaults`, `Flow`,
`Accumulation`, `MarkersStack`, `PropsStack`, `DecalsStack`, `UnitsStack`,
`DetailNormal`, `SanGenVersion`, `PropGroups`, `DecalGroups` — the ratified schema
v3, `SANMAP_FORMAT_SPEC`).

- Every SanGen-owned top-level key is **single-token PascalCase, no spaces** —
  `GeneralMapSettings`, never `"General Map Settings"`.
- A field **merged into an existing format-native collection** (e.g. `armies[key]`)
  stays **lowerCamelCase** to match its siblings — `armyColor`, `alias`, never
  `"Army Color"`. It does not become a new SanGen section just because SanGen added
  it; it is a sibling field inside the format's own dictionary.
- This governs `.sanmap` **top-level JSON keys and format-collection member keys
  only**. It does not change §1.1 (identifiers the format/game dictates stay
  verbatim) or the C++ naming law inside `src/`; a PARAMS type's C++ member name
  still follows §1.1/§1.8 (`camelCase`/`b` prefix, and §1.8's data-kind rule for
  format-derived fields) regardless of how the same value is spelled at the JSON
  top level.

