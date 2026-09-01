# STEP239 — The "Links" UI tier: +Link button, propagation, delete-link

**Layer:** UI. **Domain:** new `MarkersTab_Links_UI.h/.cpp`, `MarkersTab_UI.cpp` (new `+Link`
button + `bAddLinkClicked` handling), `MarkersTab_ManualInstanceSelection_UI.h/.cpp` (new
`PartitionSelectedManualInstancesByType`), `MarkersTab_ManualLayerHelpers_UI.h` (new resolver
functions), `MarkersTab_ManualLayerRowBody_UI.cpp` (disabled-condition + swatch-read extension).
**Sequence:** depends on STEP237 (done). Independent of STEP240 (disjoint files).

Ratifies `ARCH_19_29_LinkIdentifierBackReferences.md`, `ARCH_19_31_PropagatedPropertyMechanisms.md`.
See `DESIGN_MarkerLink_R1.md` §3.4–§3.7 for full grounding.

## Session coordination

Check `ListAgents`/message peer sessions before touching EACH file above.

## Fix

1. **Propagation resolvers** in `MarkersTab_ManualLayerHelpers_UI.h` (adjacent to the existing
   `EffectiveManualMarkerLayerColor`, not a replacement):
   ```cpp
   bool EffectiveManualMarkerLayerColorOverrideEnabled(const Params::MarkerInstanceLayer& layer,
                                                       const std::vector<Params::MarkerLink>& links);
   const float* EffectiveManualMarkerLayerColor(const Params::MarkerInstanceLayer& layer,
                                                const std::vector<Params::MarkerLink>& links);
   bool EffectiveManualMarkerLayerHidden(const Params::MarkerInstanceLayer& layer,
                                        const std::vector<Params::MarkerLink>& links);
   ```
   `linkIdentifier >= 0` and resolves → the Link's own field; else → the Layer's own field,
   unchanged from today. **Read-and-resolve only** — never write the Link's value back onto the
   Layer's own field.
2. `MarkersTab_ManualLayerRowBody_UI.cpp`'s `DrawManualMarkerLayerColorOverrideHeaderControl`:
   `ImGui::BeginDisabled(state.bUseGroupColor || layer.linkIdentifier >= 0)`; swatch reads
   `EffectiveManualMarkerLayerColor`/`EffectiveManualMarkerLayerColorOverrideEnabled` instead of the
   raw fields. Same disabled-treatment for the `bHidden` visibility toggle via
   `EffectiveManualMarkerLayerHidden`.
3. New `PartitionSelectedManualInstancesByType(markers, selectedIdentifiers)` in
   `MarkersTab_ManualInstanceSelection_UI.h/.cpp` — groups selected identifiers by
   `Params::CanonicalMarkerTypeSectionName(group.name)`, mirrors `BuildManualInstanceLayerIndex`'s
   per-frame-map-build shape.
4. New `+Link` button in every Marker-Type section header (`MarkersTab_UI.cpp`), enabled whenever
   `state.selectedManualInstanceIdentifiers` is non-empty (unlike `+Group`/`+Layer`, always acts on
   the WHOLE tab-wide selection regardless of which section's copy is clicked). On click:
   - Mint a new `Params::MarkerLink` (`identifier`, default name), push to `recipe.markerLinks`.
   - `PartitionSelectedManualInstancesByType`, then for each represented type: create a
     `MarkerLayerBundle` (root-scoped, `linkIdentifier` set) + its first `MarkerInstanceLayer`
     (`name` = Link's name, `linkIdentifier` set), reassign that type's selected instances into the
     new Layer via the existing `ReassignManualInstanceLayers`.
5. New `MarkersTab_Links_UI.h/.cpp` — one `DrawSectionBegin`/`DrawSectionEnd` pair per
   `Params::MarkerLink` in `recipe.markerLinks`, sibling loop to the Type-section loop (not nested,
   not folded into Type-section enumeration). Distinct header hue via a new
   `LinkSectionHeaderStyle()` (`WidgetStyle` with a named `trackColor` constant distinct from every
   Type-section default). Header: double-click-to-rename (mirrors
   `DrawMarkerLayerBundleNodeHeaderExtra`'s scratch-buffer rename) — **rename performs a one-shot
   cascade-write** into every currently-bound Bundle's `name` field (walk `markerLayerBundles` for
   `linkIdentifier == link.identifier`), then the Bundle's `name` stays independently editable
   afterward; the Link's own color-override toggle+swatch (bound directly to
   `Params::MarkerLink::bColorOverrideEnabled`/`color`); an "X" delete. Body (expanded): read-only
   per-type instance-count summary.
6. **Delete-Link**: for every `MarkerLayerBundle`/`MarkerInstanceLayer` with matching
   `linkIdentifier`, clear it to `-1` (ungroup the link relationship only — Group/Layer/instances
   untouched); erase the `Params::MarkerLink` entry last.

## Verify

- New tests: `+Link` on a cross-type selection creates one Group+Layer per represented type, each
  tagged with the same `linkIdentifier`, and moves each type's instances in.
- Propagation resolvers: a Link-bound Layer's color/hidden read from the Link, not its own field, while
  `linkIdentifier >= 0`; reverts to its own field once un-linked.
- Rename cascades to every bound Bundle's `name` once, then each stays independently editable.
- Delete-Link clears both back-reference tiers, erases only the `MarkerLink` entry, leaves every
  instance/Layer/Group intact.
- Existing `MarkersTab_UI_Test`, `MarkersTab_ManualInstanceSelection_UI_Test`,
  `MarkersTab_ManualLayerRowBody_UI_Test` (or equivalent) stay green.

## Out of scope

- Icon-size UI rework (STEP240).
- The Standing Recorded Defect (hardcoded Type-section loop) — separate, unrelated ticket.
