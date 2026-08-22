[← ARCH index](ARCH.md) · [§16 ARCH_16_MarkerLayerSymmetry](ARCH_16_MarkerLayerSymmetry.md) · SanGen ARCH §16.10. **Only the ARCH Expert writes this file.**

### 16.10 Consult routing summary — what remains before a coder work-order can be written
This ratification closes the PARAMS shape (§16.1, with the naming fix in §16.5/§16.7), the
module-boundary question (§16.3, resolved without needing MATH relocation or a new dependency
exception), and confirms two items need no further ARCH action (§16.8, §16.9). Three items remain
open, each explicitly routed, not decided here:
1. **Format Expert:** the exact `MarkersStack` nested-rules-array JSON key spelling (§16.4); the
   `MarkerGroups` top-level wire key's exact shape, confirming it parallels `PropGroups`/
   `DecalGroups` (§12, §16.1); and the STEP49 export-time-warning interaction with the §16.8
   orphan ruling.
2. **IO Architecture Expert:** the `MarkersStack` migration mechanics for the breaking per-rule
   → per-layer symmetry-field move, including the lossy-collapse sub-problem named in §16.6.
3. **Generator Expert — item corrected below.** ~~confirm `Placement_Rules_PROC.cpp`'s marker
   symmetry-resolution call site can walk the new `MarkerRuleLayer` tier instead of reading
   `rule.*` directly (mechanical once §16.1/§16.6 are wired)~~

   **Correction (found and reported by `work_orders/STEP79_MarkerRuleLayerProcConsumer_PROC.md`,
   verified against the two named source files before this edit):** this item undercounted the PROC
   consumers and understated the work. There are **two** independent PROC consumers of the per-rule
   symmetry triplet, not one, and both must migrate together:
   - `Placement_Rules_PROC.cpp`'s `AppendMarkerRules` — the compile-time consumer; breaks loudly
     the instant §16.1/§16.6's shape lands, so it cannot be missed.
   - `Placement_Hash_PROC.cpp`'s `HashMarkerRule`/`ComputeParameterHash` — a **silent** consumer
     with its own independent loop and its own `rule.bSymmetryUseGlobal`/`.symmetryMask` reads. If
     left on the old flat shape it does not fail to compile; it fails to fail. The dirty-hash
     dependency DAG (Constitution §1 PIPELINE) silently stops reacting to marker-symmetry-layer
     edits — a stale-preview bug indistinguishable from "nothing changed" until a human notices the
     preview didn't update.

   "Mechanical" also undersells the migration itself, for two reasons neither consumer's file
   escapes: (a) `ARCH_01_05_FileSizeCeilings.md` §1.5's soft/hard ceilings are breached by the naive
   in-place edit — `AppendMarkerRules` is 39 lines in a 143-line `Placement_Rules_PROC.cpp`, one line
   under the 40-line function ceiling and seven under the 150-line file ceiling, and the new
   two-level walk does not fit in that margin, forcing a real file split
   (`Placement_MarkerRules_PROC.cpp` + `Placement_RuleAppend_PROC.h`), not a same-file edit;
   (b) Constitution §4's Deterministic sub-mode imposes a hard seed-decorrelation requirement — the
   existing flat, per-rule `ruleIndex` counter must become **one** counter threaded through both
   levels of the new nested `markerRuleLayers → layer.rules` walk, advancing even for suppressed
   rules and fully-suppressed layers, or every marker rule after a skipped one silently reseeds
   (wrong scatter positions for every affected `.sanmap`, with no compile error and no failing
   count-only test to catch it).

   Full shape, both consumers' after-states, the flat-seed-counter proof, and the file split are
   specified in `work_orders/STEP79_MarkerRuleLayerProcConsumer_PROC.md`, dispatched as a single
   inseparable unit with `STEP66_MarkerRuleLayer_PARAMS.md` (neither lands without the other).

