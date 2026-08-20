# Work-Order — Step 40A: migration foundation — new primitive + `MigrationEntry` struct

*Constitution §7. Executor: SanGen Coder. IO Architecture Expert consult. First of 6 dependency-
ordered tickets (A→{B,C,D,E}→F) building the real V2→V3 migration step. This one must land and
pass before B/C/D/E can be dispatched — they depend on the types/functions it adds.*

## Root problem
Two prerequisites for the real V2→V3 migration step don't exist yet:
1. `IO_MIGRATION_SPEC.md` §3 already ratifies `MigrationEntry{MigrationFunction function; const
   char* name; const char* description; bool bIndependentlySelectable = false;}` as the manifest's
   element type — but `Sanmap_MigrationManifest_IO.h` was never actually updated to match; it's
   still `std::vector<MigrationFunction>` (Step-6-vintage). Confirmed by grep: zero matches for
   `MigrationEntry`/`bIndependentlySelectable` anywhere in `src/`.
2. Three upcoming migrations (`Flow`, `GlobalMarkerSettings`, `EntityCollections`) need to convert
   a legacy 4-element `[r,g,b,a]` array into this format's current `{r,g,b,a}` object shape — no
   existing `JsonPrimitives_IO.h` primitive does this.

## Solution — shape
**1. Add `MigrationEntry` to `Sanmap_MigrationManifest_IO.h`, change `MigrationStep::migrations`'s
element type:**
```cpp
struct MigrationEntry {
    MigrationFunction function;
    const char* name;
    const char* description;
    bool bIndependentlySelectable = false;
};
struct MigrationStep {
    int sourceVersion = 0;
    std::vector<MigrationEntry> migrations;   // was std::vector<MigrationFunction>
    std::vector<const char*> legacyKeysToDelete;
};
```
Update the header's own stale docstring (currently claims "kCurrentSanGenVersion = 2 ... manifest
is EMPTY" — rewrite to describe the real state once this ships).

**2. Update `Sanmap_MigrationRunner_IO.cpp`'s iteration** from
`for (MigrationFunction migration : step->migrations) migration(document);` to
`for (const MigrationEntry& entry : step->migrations) entry.function(document);`.

**3. New primitive in `src/io/JsonPrimitives_IO.h`**, alongside the existing transform primitives:
```cpp
// Converts a legacy 4-element [r,g,b,a] array at `key` into this format's current
// {"r":,"g":,"b":,"a":} object shape — the color shape every V3 field already uses (armyColor,
// FlowMapColor, MarkerColorAlloy/Plasma/Spawn). No-op — total and idempotent — if `key` is
// absent, or if the value is not an array. Fewer than 4 elements pads missing trailing
// components with 0 (r/g/b) or 1 (a) rather than throwing — never a partial-write.
inline void ConvertColorArrayToRgbaObject(nlohmann::json& parent, const char* key) {
    if (!parent.contains(key)) return;
    if (!parent[key].is_array()) return;
    const nlohmann::json array = parent[key];
    static const char* const componentNames[4]    = { "r", "g", "b", "a" };
    static const float       componentDefaults[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    nlohmann::json converted = nlohmann::json::object();
    for (int i = 0; i < 4; ++i)
        converted[componentNames[i]] = (i < static_cast<int>(array.size())) ? array[i] : componentDefaults[i];
    parent[key] = std::move(converted);
}
```
Not a generic "array to named-object" primitive — color-specific, matching this codebase's own
"promote to generic only when a third distinct shape needs it" discipline (no Vector4 field ever
needed this conversion; they already round-trip as objects both sides).

## Target files
- `src/io/Sanmap_MigrationManifest_IO.h` — `MigrationEntry` struct, `MigrationStep` element-type
  change, stale docstring rewrite.
- `src/io/Sanmap_MigrationRunner_IO.cpp` — iteration-loop update.
- `src/io/JsonPrimitives_IO.h` — `ConvertColorArrayToRgbaObject`.
- `src/io/JsonPrimitives_IO_Test.cpp` — new tests for the primitive: converts a real 4-element
  array correctly; pads a short array correctly; is a no-op on an absent key; is a no-op (and
  idempotent) when called twice on an already-converted object.
- `src/io/Sanmap_MigrationManifest_IO.cpp` — the (still-empty) manifest vector's declared type
  must compile against the new `MigrationEntry` element type — confirm it still builds with zero
  entries (this ticket does NOT populate the manifest, that's `STEP40F`).

## Explicit out-of-scope
- Populating the manifest with the real V2→V3 step — `STEP40F`, dispatched last, after B–E ship.
- Any of the 9 actual migration files — `STEP40B` through `STEP40E`.
- `kCurrentSanGenVersion`'s bump from 2 to 3 — `STEP40F`.

## Layer & accuracy class
IO only. Accuracy class: Exact.

## Acceptance test
1. `MigrationEntry`/updated `MigrationStep` compile cleanly with the (still-empty) manifest.
2. `Sanmap_MigrationRunner_IO.cpp`'s updated loop is functionally identical for an empty manifest
   (today's actual state) — every existing migration-runner test still passes unchanged.
3. `ConvertColorArrayToRgbaObject`'s 4 new test cases all pass.
4. Full `SanGenV2` build stays clean; every existing test continues to pass.
