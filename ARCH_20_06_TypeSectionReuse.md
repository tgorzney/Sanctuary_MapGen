[← ARCH index](ARCH.md) · [§20 ARCH_20_PropsDecalsAuthoringParity](ARCH_20_PropsDecalsAuthoringParity.md) · SanGen ARCH §20.6. **Only the ARCH Expert writes this file.**

### 20.6 Type Sections — reuse `§19.14`'s mechanism verbatim; the field itself is named per domain, never `markerTypeName` on a Prop/Decal struct
Reuse the exact mechanism `§19.14` ratified — UI-derived dynamic enumeration over a free-form
string tag on the domain's Layer/Bundle types, no new `Params::TypeSection` struct, no second
sectioning mechanism for Props. But naming law (`§1.1`/`§1.8`: a name states the quantity it
holds, not a role borrowed from a different domain) rules out reusing the literal field name
`markerTypeName` on a Prop or Decal struct — that would misname the data.

- `PropRuleLayer`/`PropInstanceLayer`/`PropLayerBundle` gain `std::string propTypeName;`
  (default `""`, wire key `"PropTypeName"`, additive — same posture as `§19.13`'s
  `"MarkerTypeName"`). Same open, free-form string-space contract as Markers' field; the human's
  ratified starting values are `"Prop"`/`"Reclaim"`.
- **Decals get no equivalent field at all.** The human's ratified Decals shape has exactly one
  implicit type — a `decalTypeName` field would be dead data, always the same value, never
  branched on, which is a worse AI-legibility outcome than omitting it (an inert field invites a
  future "why does this always say Decal?" question with no real answer). The single-bucket
  posture is expressed as a UI presentation choice (one flat list, or one always-present section
  header with nothing behind it) — not a PARAMS concept — left to the UI Expert's discretion.
- Extends `§19.14`'s ordering rule to Props, same pattern, `propTypeName` vocabulary: `"Prop"`,
  `"Reclaim"` first (mirrors `GlobalPropSettings`' own field order, `§20.3`), then every other
  distinct value present, alphabetical, then a final `"(Unassigned)"` bucket for
  `propTypeName == ""`.
- **`PropRule::bReclaimable`/`PropInstanceGroup::bReclaimable`** (existing, asset-derived via
  `PropReclaimableBake_IO.*`) **stay permanently independent of `propTypeName`** — the same
  closure `§19.21` already ruled for `MarkerRule::category` vs. `markerTypeName`. The existing
  Reclaim overlay-domain routing (`§14.6`) keeps reading `bReclaimable`; `propTypeName` is a
  separate, UI-authoring-only organizational tag with no procedural-domain-routing role. A
  future ticket may not silently merge the two.
