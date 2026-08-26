[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.13. **Only the ARCH Expert writes this file.**

### 19.13 `markerTypeName` on `MarkerRuleLayer`/`MarkerInstanceLayer` — ratified, additive, extends §19.3
Responds to `work_orders/DESIGN_MarkerTypeSectionsAndInstanceSelection_R1.md` item 2. **Ratified as
designed, no correction.** Both structs (`MarkerRule_PARAMS.h:77` `MarkerRuleLayer`,
`MarkerInstance_PARAMS.h:23` `MarkerInstanceLayer` — both confirmed by direct read to already carry
`parentBundleIdentifier`, §19.3/§19.4) gain:
```cpp
std::string markerTypeName;   // free-form; same string space as MarkerLayerBundle::markerTypeName
```
Default `""`. Wire key **`"MarkerTypeName"`** — identical spelling to the Bundle's own key (§19.4):
same concept, a different owning struct, no reason to diverge. Additive, no `SanGenVersion` bump —
the same precedent class as `parentBundleIdentifier`, already shipped on both structs. IO homes:
`MapExporter_MarkersStack_IO.cpp`/`MapImporter_Markers_IO.cpp` (`MarkerRuleLayer`);
`MapExporter_Markers_IO.cpp`/`MapImporter_Markers_IO.cpp` (`MarkerInstanceLayer`).

No import-time cross-check against the containing Bundle's own `markerTypeName` or the enclosing
Group's name — soft, UI-authored-only posture, the same invariant class §19.12 already ratified for
Bundle→member-type consistency generally (not a new soft-validation rule, an application of the
existing one).
