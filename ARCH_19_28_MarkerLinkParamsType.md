[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.28. **Only the ARCH Expert writes this file.**

> **⚠️ CORRECTED 2026-08-31 — see `ARCH_19_31_PropagatedPropertyMechanisms.md`.** The struct shown
> below and its "no `bHidden` field... deliberate asymmetry with color" paragraph are **superseded**.
> `§19.31`'s 2026-08-31 correction adds `bHidden`, `iconScale`, `bGridSnapEnabled`,
> `gridSnapSizeWorldUnits`, `bSymmetryEnabled`, and `symmetry` to `Params::MarkerLink`, and retracts
> the "hide is a cascading action, not a stored field" reasoning below. A same-day follow-up
> amendment to `§19.31` adds a seventh field, `bLocked`, on the identical read-and-resolve/
> master-slave mechanism (direct human ruling: "everything should be cascaded down to the Groups in
> the Link"). This file's own text is left unedited beneath this notice for historical record of the
> original ratification only — `§19.31` is authoritative on any conflict with what follows.

### 19.28 New PARAMS type `Params::MarkerLink` — ratified, formalizing the advisory ruling in `work_orders/DESIGN_MarkerLink_R1.md` §3.3/§5 item 1
Responds to `work_orders/BRIEF_MarkerLink_R1.md` item 3. **Ratified as strawmanned, no correction.**

**A Link is a stored PARAMS concept, not a UI-derived tier — confirmed, not merely accepted.**
`ARCH_19_14_TypeSectionUiDerived.md` §19.14 rules Type-sections UI-derived for a specific reason
that does not transfer here: Type-sections dedupe an *already-existing* open string field
(`markerTypeName`) that carries no data of its own. A Link is new source-of-truth state (name,
identifier, color-override toggle+color) that exists nowhere else in the recipe — there is
nothing to derive it from. §19.14's own reasoning therefore argues FOR a stored concept in this
case, not against one.

```cpp
// MarkerLink_PARAMS.h — new file, sibling of MarkerLayerBundle_PARAMS.h, same "new tier gets its
// own file" precedent §19.1/§19.3 already established for Bundle.
// ⚠️ Field list superseded by §19.31's 2026-08-31 correction (and its same-day bLocked follow-up
// amendment) — see the notice above this section.
struct MarkerLink {
    int identifier              = -1;    // stable, survives reorder/delete — spelled per §1.9,
                                          // matches MarkerLayerBundle::identifier/
                                          // Params::Assembly::identifier's own spelling
    std::string name;
    // "Links would be where the color override is set" — the human's own words (BRIEF §3). This
    // Link IS the single source of truth these two fields resolve from; never copied down onto a
    // bound Group/Layer (§19.31 — read-and-resolve, not write-through-and-copy).
    bool  bColorOverrideEnabled = false; // mirrors MarkerInstanceLayer::bColorOverrideEnabled
    float color[4]              = {1.0f, 1.0f, 1.0f, 1.0f};   // mirrors MarkerInstanceLayer::color
};
```

`MapRecipe` gains `std::vector<MarkerLink> markerLinks;` — a flat sibling of `markerLayerBundles`
(`MapRecipe_PARAMS.h:119`), same tier, same file.

**No `bHidden` field on `MarkerLink` itself. [RETRACTED by §19.31's 2026-08-31 correction — a
`bHidden` field IS now added; do not implement the paragraph below.]** §19.31 rules visibility
propagates too, but it propagates FROM each bound `MarkerInstanceLayer::bHidden` cascade-style on
toggle, not from a fourth field stored here — see §19.31 for why this was ruled a deliberate
asymmetry with color, not an oversight, at the time this section was originally ratified.

**Not a member of `MarkerLayerBundle_PARAMS.h`.** Kept in its own file for the same reason Bundle
itself was: `MarkerLink` is a peer of Bundle in the hierarchy (a cross-cutting concept a Bundle can
optionally be tagged into, §19.29), not a member of it — folding it into Bundle's own file would
misstate that relationship.
