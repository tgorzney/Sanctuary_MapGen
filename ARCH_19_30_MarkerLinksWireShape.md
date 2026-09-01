[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.30. **Only the ARCH Expert writes this file.**

### 19.30 `MarkerLinks` wire array shape — ratified, confirming the Format Expert's micro-spelling ruling holds
Responds to `work_orders/DESIGN_MarkerLink_R1.md` §3.3/§5 item 3. **Confirmed at the ARCH-shape
level; the Format Expert's micro-spelling correction is verified against real precedent and
adopted verbatim, no further correction.**

```
MarkerLinks: [ N × {
    Identifier            (int)
    Name                  (string)
    ColorOverrideEnabled  (bool)
    Color                 ({r,g,b,a})
} ]
```

**`Color` as an object `{r,g,b,a}`, not a bare 4-element array — verified against real, shipped
precedent, not merely restated.** Every existing SanGen-owned PascalCase color field backing a
C++ `float color[4]` already uses this object shape: `PropGroups`/`DecalGroups`'s own `Color`
field and `MarkerGroups`'s `Color` (`SANMAP_FORMAT_SPEC.md:785,892`) both serialize
`{r,g,b,a}`; `Army::armyColor` does too (`"armyColor": {"r":1.0,"g":0.2,"b":0.2,"a":1.0}`,
`SANMAP_FORMAT_SPEC.md:1013`). The original design strawman's "4 × float" (a bare array) would
have been the first field in this format to break that convention — rejected in favor of the
shape above.

**`Identifier`, spelled in full — not `Id`.** Applies §1.9's ban directly; matches
`MarkerLayerBundles[i].Identifier`'s own already-ratified spelling (§19.4), not the pre-§1.9
`MarkerGroups[i].Id` legacy defect (`ARCH_01_09_IdAbbreviationBan.md`).

**Per-tier back-reference — direct field injection on the two existing wire arrays, no new file
needed for these two fields:**
```
MarkerLayerBundles[i].LinkIdentifier   (int)   // merges alongside AssemblyIdentifier
MarkerGroups[i].LinkIdentifier         (int)   // merges alongside ParentBundleIdentifier/MarkerTypeName
```
`LinkIdentifier` — PascalCase on wire, full word per §1.9, lowerCamelCase `linkIdentifier` in
C++ — the identical `assemblyIdentifier`/`AssemblyIdentifier` merge precedent (§19.3/§19.5),
confirmed, not a new spelling rule.

**Additive, no `SanGenVersion` bump.** Same precedent class as every prior addition in this
family (Corrections 12/14/16/18/19) — a brand-new top-level array plus two merged scalar fields on
existing arrays, all absent-safe on read. A dangling `LinkIdentifier` (on either tier) degrades to
"not Link-bound" (`-1`/absent, soft, logged-or-silent per the IO Architecture Expert's own call,
not re-litigated here) — same posture as `AssemblyIdentifier`/`ParentBundleIdentifier`'s own
dangling-reference rule, applied again.

**`MarkerLinks` must be registered in `Sanmap_KnownTopLevelKeys_IO`** (or its equivalent) at
coder-dispatch time — otherwise it round-trips verbatim under `UnknownImport` instead of parsing
into `recipe.markerLinks`. Flagged here so it is binding, not left to inference; the remaining IO
precision items (import ordering, dangling-reference logging, test-file pairing) stay routed to
the IO Architecture Expert per the design's own §3.8, unchanged by this ratification.

`SANMAP_FORMAT_SPEC.md` gains a new Correction for this shape (`MarkerLinks` top-level array, the
two merged `LinkIdentifier` fields) at the next Format Expert pass — not landed by this ARCH
session, per the same "ARCH ratifies the shape, Format Expert lands the spec-file correction text"
division of labor §19.11's own correction bundle already followed.
