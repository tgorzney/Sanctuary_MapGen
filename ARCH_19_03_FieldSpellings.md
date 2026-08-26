[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.3. **Only the ARCH Expert writes this file.**

### 19.3 `MarkerLayerBundle` field spellings — ratified, applying §1.9's "Id" ban
```cpp
// MarkerLayerBundle_PARAMS.h — NEW FILE, sibling of MarkerRule_PARAMS.h/MarkerInstance_PARAMS.h,
// parent of both (per the design's §2 own reasoning — it shouldn't live inside either).
struct MarkerLayerBundle {
    int identifier             = -1;   // stable, survives reorder/delete — spelled per §1.9,
                                        // matches Params::Assembly::identifier's own spelling
    std::string name;
    int parentBundleIdentifier = -1;   // -1 = root; enables Bundle-in-Bundle nesting
    std::string markerTypeName;        // single-type scope — free-form string space, same as
                                        // MarkerInstanceGroup::name (e.g. "Alloy"), NOT
                                        // MarkerCategory
    int assemblyIdentifier     = -1;   // §19.5 — Assembly-references-Bundle hook, inert until
                                        // Assembly itself exists
};
```
`MapRecipe` gains `std::vector<MarkerLayerBundle> markerLayerBundles;`.

**Back-reference (Layer → Bundle), additive, on the existing types:**
```cpp
// MarkerRuleLayer gains:      int parentBundleIdentifier = -1;
// MarkerInstanceLayer gains:  int parentBundleIdentifier = -1;
```
Named `parentBundleIdentifier`, not `parentGroupIdentifier` — the design's own working spelling
used "Group" here even after renaming the type to avoid that exact word; corrected for
consistency with §19.1's whole point (the new concept should not carry the ambiguous word
anywhere in its own vocabulary, field names included).

**Accessor-lambda genericity confirmed — field names do NOT need to match `Params::Assembly`'s.**
`TreeListWidget_UI<T>` (§19.7) is parameterized by `int IdOf(const T&)`/`int ParentIdOf(const
T&)`/`const std::string& NameOf(const T&)` accessor lambdas, not by field-name coupling —
`MarkerLayerBundle::parentBundleIdentifier` and `Assembly::parentIdentifier` can and do stay their
own most natural, most specific spelling; genericity comes from the lambda the caller supplies,
never from forcing two independently-evolving domain types to share field names.

**Resolves the "Id" abbreviation question raised here and in `DESIGN_Assembly_R1.md` §7,
together, once (§1.9).** Every field above is already spelled correctly per that ruling.
`Assembly`'s own `identifier`/`parentIdentifier` (`DESIGN_Assembly_R1.md` §5) are confirmed
correct as originally drafted — no correction needed there. The design's own working spelling for
Assembly's *per-instance merged field* (`AssemblyId`, on `PropTransform`/`DecalTransform`/
`MarkerTransform`) IS corrected: it becomes **`assemblyIdentifier`**, matching `layerIndex`/
`symmetryGroupIdentifier`'s already-shipped merge convention exactly.
