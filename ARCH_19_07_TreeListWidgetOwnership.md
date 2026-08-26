[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.7. **Only the ARCH Expert writes this file.**

### 19.7 `TreeListWidget_UI<T>` — one shared, domain-agnostic widget; Markers' Ticket B builds it, generically, first
**Ruled: the Markers Group/Bundle Ticket B (the tab UI, `DESIGN_MarkerGroupLayerRestructure_R1.md`
§6) builds `TreeListWidget_UI<T>`, not Assembly's own (still-unratified, still-undispatched)
ticket.** This follows directly from the human's own explicit delivery priority
(`BRIEF_MarkerGroupLayerRestructure_R1.md` "Delivery sequencing": "Get Markers working and usable
first") — Markers is the feature actually being dispatched next; Assembly is design-only with no
scheduled ticket. Building the widget against whichever consumer ships first, rather than
speculatively against the one that has not, avoids a second round of "build it again, generically
this time" work.

**It is built fully generic from day one, not Markers-specific-then-generalized-later.** Per
`DESIGN_Assembly_R1.md` §1's own sketch (data shape, signal contract, drop-zone geometry, the
caller-owned `TreeListState` map) genericized over the node type `T` via accessor lambdas — `int
IdOf(const T&)`, `int ParentIdOf(const T&)`, `const std::string& NameOf(const T&)` — exactly as
`DESIGN_MarkerGroupLayerRestructure_R1.md` §4 specifies, so `Params::Assembly` and
`MarkerLayerBundle` are both legal instantiations with zero field-name coupling (§19.3). It lives
in `TreeListWidget_UI.h`, a UI-framework primitive sibling of `DraggableListWidget_UI.h` — never
inside a `MarkersTab_*` file, so Assembly's later ticket includes it unchanged.

**The additive extension is part of the same build, not a follow-up.** The design's own finding —
Assembly's leaf rows are read-only (select/drag only), while a Bundle's Layer-node rows need their
full existing settings body inline (`DrawRuleLayerSettings`/`DrawLayerRowBody`, already
per-row/non-selected-gated per STEP110) — means the widget contract needs one small addition from
the start: an optional `DrawExpandedLeafBody(leafKey)` callback, no-op/unused for Assembly's
leaves, wired for a Bundle's Layer leaves. Ticket B builds this callback into
`TreeListWidget_UI<T>`'s initial contract, not as a later signature change — Assembly's own
eventual ticket then needs zero widget-library changes, only its own call site.

**Leaf-key type stays per-consumer, not shared** — `MarkerGroupLeafKey_UI = {kind:
Procedural|Manual, layerIndex}` for the Bundle tree, `AssemblyMemberKey_UI` for the Assembly tree
(`DESIGN_Assembly_R1.md` §2) — different leaf payloads, same pattern, no forced type unification;
this is consistent with §19.2's "genericity lives in the mechanism, not the data shape" rule.
