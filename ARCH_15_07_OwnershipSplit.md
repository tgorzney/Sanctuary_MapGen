[← ARCH index](ARCH.md) · [§15 ARCH_15_MapScenarioSystem](ARCH_15_MapScenarioSystem.md) · SanGen ARCH §15.7. **Only the ARCH Expert writes this file.**

### 15.7 Ownership split — who ratifies what for the new `Params::Scenarios` family

Three-way split, mirroring how this pack already divides labor across every prior PARAMS
ratification (`ENTITY_AUTHORING_PARAMS_SPEC`, `ATMOSPHERE_PARAMS_SPEC`):
- **ARCH (this ruling) owns the PARAMS C++ shape and naming** — §15.5/§15.6 above — the same
  pattern §9–§13 already establish (ARCH rules shape even where a type's content originates from
  Format-domain analysis).
- **The Format Expert owns** the follow-up `SANMAP_FORMAT_SPEC` Correction defining the new
  `Scenarios` `.sanmap` section's exact JSON key spelling and confirming round-trip fidelity —
  their charter already lists this exact family of spec (`ENTITY_AUTHORING_PARAMS_SPEC`'s
  manually-placed-entity shapes) as their domain.
- **The IO Architecture Expert owns** the corresponding `MapExporter_Scenarios_IO`/
  `MapImporter_Scenarios_IO` file pair, the new Lua-rendering IO file (§15.4 point 2's render
  step), the runtime-copy mechanism, and the overwrite-safety code shape (§15.4) — per their
  existing charter's Map Scenario note.
- **A new top-level SanGen-owned JSON section being wholly additive needs no
  `IO_MIGRATION_SPEC` version-step** (a migration exists to reshape *existing* data across a
  version bump, not to introduce a new, independent section) — confirm this at implementation
  time; it is the IO Architecture Expert's call, not asserted as final here.

