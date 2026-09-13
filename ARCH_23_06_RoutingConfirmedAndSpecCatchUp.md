[← ARCH index](ARCH.md) · [§23 ARCH_23_ProceduralStratumMaskFilterChain](ARCH_23_ProceduralStratumMaskFilterChain.md) · SanGen ARCH §23.6. **Only the ARCH Expert writes this file.**

### 23.6 Downstream routing confirmed; `MASKING_SPEC.md` content catch-up flagged, not done here

**Routing from the design doc's §4, confirmed unchanged except where §23.1–§23.5 above narrowed
the question each expert is being asked:**

- **Format Expert** — `.sanmap` wire shape for `maskFilters` (now a variable-length array per
  §23.1, not a fixed `[8]`), extending `StratumGenerationSettings`
  (`MASKING_SPEC.md` §1.7 Correction 12). Also needs the wire shape for `bSlopeUseGlobal`'s new
  home on `SlopeGateSettings` (§23.1 Correction 2) and for the new `HeightBandSettings`/
  `RoughnessSettings` field lists (§23.5).
- **IO Architecture Expert** — migration: an old `.sanmap` carries only the flat slope-gate
  fields; on load, synthesize `maskFilters = [{ type: SlopeGate, blendMode: AlphaBlend,
  alpha: 1.0, slope: <old fields, including the old root-level bSlopeUseGlobal now moved onto
  slope> }]`, one entry, so existing maps render identically post-migration. Its own migration
  version step, per `IO_MIGRATION_SPEC.md`.
- **Compute Optimization Expert** — evaluation strategy for the now-`std::vector`-shaped,
  per-stratum, tagged-union filter chain: branchy per-cell dispatch vs. per-type batched passes,
  the PROC-side GPU kernel-config record's own fixed per-stratum filter-slot cap (the number
  §23.1 explicitly declined to set at the PARAMS layer), the Roughness filter's windowed-variance
  cost/caching question (§23.3's closing note), and the GPU kernel shape
  (`DISPATCH_INTERFACE_SPEC`) for a chain whose length varies per stratum.
- **UI Expert** — authoring UI for the ordered per-stratum filter list (type picker, blend-mode
  picker, alpha slider, per-type settings, reorder/add/remove), likely reusing
  `DraggableList`/`TreeListWidget_UI<T>`. Finally gives the slope gate (and its new siblings) the
  authoring surface `StratumsTab_UI.h` SCOPE NOTE 4 flags as currently missing entirely. §23.5's
  ruling that all three filter types' gate-window portion shares one field shape (window,
  dual-sided feather, smoothstep, invert, strength) means this can plausibly be one shared
  sub-panel layout parameterized by filter type, not three bespoke panels — left to the UI
  Expert's own judgment, not decided here.

**`MASKING_SPEC.md` content catch-up — flagged, not performed in this pass.** This ratification
supersedes `MASKING_SPEC.md`'s own "v2 mask-of-masks (still open, §8 tweakability)" closing note
(end of its Part 2) and parts of §1.2/§1.7 (the single hardcoded slope gate becomes one case of a
general chain). Per this pack's established convention (`sangen_arch_pack/INDEX.md`'s own running
log — e.g. the §16/§19 pattern of "the Format Expert's follow-up... has since landed" recorded as
a later, separate pass), the spec's own prose is domain-owned content the Generator Expert drafts
and the ARCH Expert physically writes into `sangen_arch_pack/specs/MASKING_SPEC.md` — that pass
has not happened yet. Recorded here, and to be recorded in `sangen_arch_pack/INDEX.md`'s running
log, so neither document silently drifts stale in the meantime: **`MASKING_SPEC.md` §1.2, §1.7,
and its Part 2 "v2 mask-of-masks" note are now superseded by ARCH §23 and not yet updated to say
so.**

**No item in this ratification is a coder work-order.** STEP269+ (PARAMS shape, PROC/Mask-stage
chain evaluator, IO round-trip + migration, UI authoring panel) are written once the Format/IO
Architecture/Compute Optimization/UI consults above land, per
`HANDOFF_TRACK_ProceduralStratumMasking.md` §C item 6 — not guessed here.
