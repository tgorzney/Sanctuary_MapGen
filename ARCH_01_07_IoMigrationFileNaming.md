[← ARCH index](ARCH.md) · [§1 ARCH_01_NamingLaw](ARCH_01_NamingLaw.md) · SanGen ARCH §1.7. **Only the ARCH Expert writes this file.**

### 1.7 IO migration file naming — schema version steps (ratifies `IO_MIGRATION_SPEC`)
A `.sanmap` schema version bump (`SanGenVersion`, `SANMAP_FORMAT_SPEC`) is carried
forward by one small file per (domain, version-step): `<Domain>_Migrate_V<N>_IO.h/.cpp`,
migrating a V*N*-shaped JSON fragment to V*N*+1 shape — an instance of the §1.5
`Type_Aspect_LAYER` split-file pattern (`Domain` = Type, `Migrate_V<N>` = Aspect), never
a direct N→M jumper. Each migration is paired with a literal-fixture
`<Domain>_Migrate_V<N>_IO_Test.cpp` and, once green and shipped, is **append-only** —
never edited again. Exactly one file, `Sanmap_MigrationManifest_IO`, is touched to wire a
new version step's ordered migration list; everything else (migrations, tests, any new
JSON-transform primitive) is pure addition. **No self-registration** — static-init order
is a real failure mode and an explicit manifest line beats implicit discovery
(AI-legibility). Full contract, the runner, and the shared `JsonPrimitives_IO.h`
toolkit: `IO_MIGRATION_SPEC`.

