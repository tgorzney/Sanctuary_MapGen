[← ARCH index](ARCH.md) · [§15 ARCH_15_MapScenarioSystem](ARCH_15_MapScenarioSystem.md) · SanGen ARCH §15.2. **Only the ARCH Expert writes this file.**

### 15.2 IO scope ruling — corrects an earlier assumption, does not reverse it
An earlier ratification recorded SanGen Import/Export of the Scenarios file as in scope, under
the assumption that the file would live in the map's **asset folder**
(`Sanctuary_Data/Maps/<MapName>/`) — i.e. inside the shippable `.sanmap` package SanGen's
`MapImporter_*`/`MapExporter_*` already read/write, an extension of the existing per-domain
`.sanmap` JSON convention (§1.6-adjacent). That assumption is now known wrong: the file lives in
the engine's **script tree** (`LJ/lua/maps/<MapName>/`, §15/§2 of `MAP_SCENARIO_SPEC.md`), a
location SanGen's importer/exporter does not address today and which is not part of the `.sanmap`
package at all.

**Ruling: still in scope, reclassified — not a yes/no reversal.** The scope call stands; what
changes is the *kind* of IO surface required. It is **not** an additional section inside the
existing `.sanmap` JSON document (unlike `PropGroups`/`DecalGroups`, `HeightmapStack`, etc.) — it
is a **separate companion artifact**, a `.lua` text file, at a **structurally distinct
filesystem location** from the map asset export folder. It therefore does **not** extend the
existing per-domain `MapImporter_<Domain>Stack_IO.cpp`/`MapExporter_*` convention
(`IO_MIGRATION_SPEC.md` §1) — that convention is scoped to JSON fragments of the one `.sanmap`
document — and needs its own convention, designed from scratch.

**Ownership: the SanGen IO Architecture Expert's domain**, not this ARCH's — how to structure the
new SanGen IO code, including any new file-type convention. Per the existing law already recorded
in `MODDING_SCRIPTING_SPEC.md`: no code is written until a work-order exists and is ratified; the
SanGen Coder writes zero code without one.

~~**❓ Open design question, not resolved by this ratification** — flag to the IO Architecture
Expert / human when this becomes live work: does SanGen literally round-trip the Lua text (parse
and regenerate the tiered scenario tables verbatim, preserving hand-authored comments and the
semantically-load-bearing `COUNT_SCENARIOS` ordering — hard, since Lua is not JSON), or does
SanGen instead own only the *parameterized scenario data* (a PARAMS/JSON structure) and render
that into `.lua` module text on export-only, never reading the `.lua` back in on import? The
latter avoids a Lua parser entirely and fits SanGen's existing PARAMS→IO write direction better,
but the choice is not decided here. Full detail and the "does SanGen know the game install's
`LJ/lua` root at export time" sub-question: `MAP_SCENARIO_SPEC.md` §8.~~

**RESOLVED — §15.3 below.** The question above is answered: option (c), export-only, no Lua
parser. What follows (§15.3–§15.9) is that ratification.

