[← ARCH index](ARCH.md) · [§16 ARCH_16_MarkerLayerSymmetry](ARCH_16_MarkerLayerSymmetry.md) · SanGen ARCH §16.7. **Only the ARCH Expert writes this file.**

### 16.7 Naming confirmed — `MarkerRuleLayer` / `MarkerInstanceLayer`
The design's own recommendation is **confirmed, not merely provisional.** Both names use a
qualifier word ("Rule" / "Instance") specifically to avoid the exact collision ARCH §12 already
flagged and avoided for `PropInstanceLayer`/`DecalInstanceLayer`: bare "Layer" is already
overloaded in this codebase between the procedural Group→Layer(rule) hierarchy's inner-rule
tier and a manual metadata record, and reusing it unqualified for either new marker type would
reintroduce that exact ambiguity. `MarkerRuleLayer` additionally correctly parallels the
already-shipped `GeoLayer` naming pattern (an outer container whose name still carries "Layer")
for the procedural-container role, while `MarkerInstanceLayer` directly parallels
`PropInstanceLayer`/`DecalInstanceLayer` for the manual-metadata role — both names pick up
existing conventions rather than inventing new ones, satisfying Constitution §2's naming law.

