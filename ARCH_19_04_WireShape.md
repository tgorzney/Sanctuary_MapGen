[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.4. **Only the ARCH Expert writes this file.**

### 19.4 New top-level wire key/shape — `MarkerLayerBundles`, PascalCase, additive, no `SanGenVersion` bump
Per §1.6 (new SanGen-owned top-level section = single-token PascalCase): a new top-level array,
**`MarkerLayerBundles`**, sibling of `MarkerGroups`/`MarkersStack`/`markers`. Mirrors the existing
`PropGroups`/`DecalGroups`/`MarkerGroups` array-of-objects shape and casing convention exactly:

```
MarkerLayerBundles: [ N × {
    Identifier                 (int)      // Params::MarkerLayerBundle::identifier — spelled in
                                           // full per §1.9, NOT "Id" (this new array does not
                                           // repeat MarkerGroups' pre-§1.9 "Id" defect, §19.11)
    Name                       (string)
    ParentBundleIdentifier     (int)      // -1/absent = root
    MarkerTypeName             (string)
    AssemblyIdentifier         (int)      // -1/absent = ungrouped; inert wire field until the
                                           // separate Assembly ticket exists and gives it meaning
} ]
```
Per-Layer back-reference (`MarkerRuleLayer`/`MarkerInstanceLayer` gaining
`parentBundleIdentifier`): direct field injection, lowerCamelCase, merged into the existing
`MarkersStack`/`MarkerGroups` array-of-objects entries the same way `layerIndex`/
`symmetryGroupIdentifier` already merge into `markers[type].transforms[name]` — a genuinely novel
scalar with no competing home (§1.8's direct-injection rule).

**Additive, no `SanGenVersion` bump** — same precedent class as every prior top-level-array
addition this pack has recorded (Corrections 12/14/16/18): a brand-new top-level key an
unrecognized-key-tolerant importer simply doesn't find in an older file, and a brand-new merged
field on an existing collection an older importer simply never reads. Not self-ratified for the
IO Architecture Expert's migration-mechanics sign-off, but nothing here contradicts that
precedent, and this ruling is jointly binding with the Format Expert's normal wire-shape
authority — this ARCH ruling states the shape and casing; final JSON key micro-spelling for any
nested detail this section did not enumerate is the Format Expert's call, same split already used
at §16.4.

**Import validation posture**: `parentBundleIdentifier`/`AssemblyIdentifier` follow the same "no
range to validate, any value degrades to root/ungrouped" posture already ruled for
`symmetryGroupIdentifier` (no clamp needed — a dangling reference is a query-time miss, not a
structural error) — consistent with §19.12's soft-validation ruling for the whole feature.
