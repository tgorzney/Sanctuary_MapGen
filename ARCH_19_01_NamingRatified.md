[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.1. **Only the ARCH Expert writes this file.**

### 19.1 Final type/wire name — `MarkerLayerBundle`; UI label stays "Group"
The design's own §0 correctly rejected reusing bare "Group" — it already means three different
things in this codebase (`MarkerInstanceGroup`, the `MarkerGroups` wire array which is actually
the C++ `MarkerInstanceLayer` metadata, and the `MarkersStack` `Group(MarkerRuleLayer)→Rule`
wrapper Correction 15 documents). A fourth meaning would be a real AI-legibility hazard, not a
cosmetic naming preference — the Constitution's charter for this whole pack exists specifically
"to prevent hallucination and out-of-scope edits," and a fourth silently-different "Group" is
exactly the kind of collision that produces both.

**Ruled: `Bundle`, not `Cluster`/`Ensemble`/`Formation`.**
- `Cluster` is correctly ruled out — real, live collision (`ClusterByScreenCell`,
  `MapCanvas_IconLayer_Budget_UI.cpp:33`, ARCH §14.9 screen-cell decimation, UI Optimization
  Expert territory).
- `Ensemble`/`Formation` are clean (zero grep hits) but read as domain-flavored English
  ("Formation" in particular reads as a gameplay/military concept an RTS map editor could later
  want for something else entirely — e.g. a future spawn-formation authoring tool). `Bundle` is a
  plain, common software word with no competing in-domain meaning and describes the mechanism
  precisely: it bundles Layers together.
- The design's own working name, `MarkerLayerGroup`, is rejected even though it is textually
  distinct from the three existing meanings — it sits one token away from `MarkerInstanceLayer`/
  `MarkerRuleLayer`/`MarkerInstanceGroup` in the same file family, which is exactly the kind of
  near-miss an AI reading quickly (or a human skimming a diff) mis-resolves. `MarkerLayerBundle`
  is textually and phonetically unrelated to all four sibling type names — a clean, grep-safe,
  eyeball-safe name.

**The type**: `Params::MarkerLayerBundle` (`MapRecipe::markerLayerBundles`). **The future
per-domain twins** (§19.2) follow the identical pattern: `PropLayerBundle`/`DecalLayerBundle`,
`recipe.propLayerBundles`/`recipe.decalLayerBundles` — ratified now as the naming pattern those
later, independently-ticketed features must follow, not re-derived per domain.

**UI-label-vs-type-name split confirmed, not corrected.** The design's own ruling — the display
label stays "Group" (the human's own worked example calls it "Group") even though the type is
named `MarkerLayerBundle` — is agreed as-is. A UI display string and a C++/wire type identity are
different things with different audiences; nothing requires them to match, and forcing them to
match would reintroduce the exact ambiguity `Bundle` was chosen to avoid (a UI-visible word
"Group" that maps to a type NOT spelled "Group" is unambiguous; a UI-visible word "Group" that
maps to a type also spelled "Group" — one of four — is not).
