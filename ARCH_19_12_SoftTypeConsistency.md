[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.12. **Only the ARCH Expert writes this file.**

### 19.12 Bundle→marker-type consistency stays soft (UI-enforced only) — ratified, no import-time hard validation
**Ratified as designed.** Nothing structurally validates that a `MarkerInstanceLayer`'s actual
transforms all belong to the Bundle's declared `markerTypeName` — `layerIndex` is a bare untagged
int, same as today, with no type tag of its own to cross-check against. **Ruled: enforce this only
at the UI/authoring flow** (the "Add Marker" action inside a Bundle-scoped Layer creates instances
in the matching `MarkerInstanceGroup`), never as an import-time hard validation/rejection rule.

**Why this is correct, not merely convenient.** It is consistent with every comparable invariant
already ratified in this format: an out-of-range `layerIndex` degrades with a loud, logged clamp,
never a refusal (Correction 14); a dangling `symmetryGroupIdentifier`/`parentIdentifier`/
`assemblyIdentifier` degrades to ungrouped/root, never a refusal; a cycle in a parent-chain is
logged and treated as root, never a refusal (Assembly's own already-decided convention, restated at
§19's Bundle tier). Constitution §6 draws the line precisely where this ruling needs it to:
"a file that fails to parse, is not a JSON object, or fails the size/header checks is still refused
outright — only a version marker's value stops being refusal-worthy" governs hard structural/format
failures; a Bundle's declared type not matching its members' actual type is authoring-convenience
metadata going stale, not a structural or format failure, and gets the same soft-degrade treatment
every other soft/authoring-time-only invariant in this format already receives.

**What "soft" means concretely.** A hand-edited or foreign `.sanmap` with a `MarkerLayerBundle`
whose `markerTypeName` doesn't match what its member Layers actually contain loads without warning
or refusal; the UI simply displays whatever is there. If a future ticket wants a *loud* (but still
non-blocking) authoring-time consistency check — e.g. a Markers-tab warning icon on a Bundle whose
members diverge from its declared type — that is new UI-layer scope, not an import-time change, and
needs its own work-order; this ruling does not preclude it, it only rules out import-time rejection
or auto-correction.
