[← ARCH index](ARCH.md) · [§14 ARCH_14_PreviewOverlayLayering](ARCH_14_PreviewOverlayLayering.md) · SanGen ARCH §14.18. **Only the ARCH Expert writes this file.**

### 14.18 Map areas — ONE fill in every state (the live-drag blend-fidelity ruling), tier-gated baked-input uploads, and the 16-entry distinct-color palette (ARCH ruling; amends §14.8, §14.17 items 10/11/12, and §21.8's amended draw pass)

Human-flagged bug plus a human-requested default change, ruled together because the first one cannot
be fixed without changing the cost model the second one lives in. Verified against the live code
before being written down (not taken on the proposal's word):
`MapCanvas_AreaDraw_UI.cpp`, `MapCanvas_AreaDragDispatch_UI.cpp`, `PreviewComposite_Cpu_UI.cpp`,
`PreviewComposite_Color_UI.h`, `PreviewComposite_Sampling_UI.glsl`, `PreviewComposite_Prepare_UI.cpp`,
`PreviewComposite_GpuBuffers_UI.cpp`, `PreviewComposite_Gpu_UI.cpp`, `PreviewComposite_UI.h`,
`PreviewComposite_Settings_UI.h`, `PreviewDriver_PIPELINE.h`/`.cpp`,
`GenerationAssembler_PIPELINE.cpp`, `GpuResource_SYS.h`, `Application_Frame_UI.cpp`,
`Application_UI.cpp`, `AreaColorTable_UI.h`, `AreasTab_List_UI.h`, `AreasTab_UI.cpp`,
`Geometry_PARAMS.h`, `RtToggleWidget_UI.h`, `ARCH_14_16_PerArmyUnitsOverlayRows.md`.

> **PART 3 APPENDED 2026-08-29 (later the same day), items 17-24.** Item 10's benchmark gate is
> **CLOSED — PASS**; Pieces A and B have shipped (STEP216 / STEP217) and the measurement harness with
> them (STEP218). Piece C is unblocked and is now ruled to implementation detail, WITH a mandatory
> cost watchdog. **Item 4's begin-time refresh request and item 8's second bullet are amended by
> Part 3 — do not cite either alone.**

---

## Part 1 — the live-drag fill

**1. The law this ruling adds, stated once and generally: A MAP AREA'S FILL HAS EXACTLY ONE
RENDERER, IN EVERY STATE, INCLUDING MID-GESTURE — the composite.** No second code path may draw an
area's fill, ever, for any reason (performance, latency, "just while dragging"). The shipped
behavior violates this: `DrawAreaOverlayPass` draws the suppressed area with
`drawList->AddRectFilled(...)`, which uses ImGui's one fixed-function alpha-over equation, while the
steady-state fill goes through `BlendPreviewColor` / `combineChannel` with the layer's real
`PreviewBlendMode`. For the default `Overlay` — and for `SoftLight`/`HardLight`,
`ColorBurn`/`ColorDodge`, `Multiply`, `Screen` — the result is a function of the DESTINATION pixel
(`PreviewComposite_Color_UI.h:53-85`: `Overlay` is `d <= 0.5 ? 2*d*s : 1 - 2*(1-d)*(1-s)`). A
destination-blind draw call cannot reproduce a destination-dependent formula. There is no
"approximately right" version of this: the area turns solid the instant it is touched.

This is the same class of defect as §3.2's shadow-sim — a second implementation of a decision the
one owner already makes — applied to blending rather than to placement. **Blend math has exactly two
implementations in this program (the CPU twin and the GLSL twin, parity by expression-for-expression
mirroring, `PreviewComposite_Color_UI.h:50-52`). A third is forbidden.**

**2. Rejected alternative, named so a coder cannot re-propose it: the `ImDrawList` shader callback.**
Sampling the composite texture in an ImGui draw callback and re-running the Overlay formula for the
live rectangle is rejected on three independent grounds, any one of which is sufficient:
- It is the forbidden **third** implementation of the blend math (item 1).
- It blends in **screen** space against the canvas's zoomed/filtered sample of the composite, not in
  the composite's own **cell** space (§14.17 item 4). It therefore cannot be pixel-exact with the
  fill it stands in for, and the mismatch would *snap* visibly at gesture end — trading a constant,
  understood error for an intermittent, mysterious one.
- It needs a GL program and pipeline state outside `PreviewComposite`'s own kernel contract and
  outside `Sys::GpuResourceManager`'s ownership, which §3.2/§5.4 do not permit in the UI layer.

**3. `PreviewCompositeSettings::mapAreaSuppressedIndex` is RETIRED — entirely, not softened.**
Deleted: the field itself; `BuildMapAreaConfigurations`'s `if (index == settings.mapAreaSuppressedIndex) continue;`
(`PreviewComposite_Prepare_UI.cpp:139`); `MapCanvas::SetMapAreaSuppression`; the three call sites in
`TryBeginAreaDrag`/`EndAreaDrag`; `ManualAreaDragSources_UI::mapAreaSuppressedIndex`; and
`SetManualAreaDragSource`'s fifth parameter and its wiring at `Application_UI.cpp:146-147`. The
degenerate-sentinel rule (§14.17 item 4) is untouched — it still fires when `areas` is genuinely
empty. `areaCompositeRefreshCallback` and `SetAreaCompositeRefreshCallback` are KEPT; they become the
per-frame request path.

**4. `ContinueAreaDrag` requests one composite refresh per frame the rectangle actually MOVES.**
Not unconditionally per frame: a held-but-motionless pointer must cost nothing, or
`Application::PumpWindowEvents`'s idle path (`Application_Frame_UI.cpp:61-67`) never gets to settle.
The test needs no new return value from `UpdateAreaDragGesture` — `ContinueAreaDrag` copies the four
floats of `areas[draggedIndex]` (`originX`, `originZ`, `width`, `length`) before the update and
compares after, firing `areaCompositeRefreshCallback()` only on a difference. That is exactly the
idiom `SetAreaToMapSize` already establishes one file over — *"Reports whether the rectangle moved,
so a button press that changes nothing costs no recomposite"* (`AreasTab_List_UI.h:59-60`). Four
float compares against a ~1 ms recomposite is not a tradeoff worth deliberating.
`TryBeginAreaDrag` and `EndAreaDrag` each keep exactly one refresh request (begin: the gesture may
select a different area and must settle immediately; end: the final rectangle must be composited
even if the last frame did not move). `CreateAreaFromDrag`'s single request is unchanged.
*(**AMENDED by item 23-A**: with the suppression index gone, `TryBeginAreaDrag` fires NO refresh
request — selection is provably not a composite input. The end-time request stands, unchanged.)*

**5. §14.8's tier table gains one row — Tier B2, interaction-scoped recomposite.** §14.17 item 11's
"exactly two recomposites per gesture" is superseded. The new row:

| Tier | Trigger | Cost |
| --- | --- | --- |
| B2 — Interaction-scoped recomposite (new) | An area drag/resize that moved the rectangle this frame | One Tier B recomposite per moving frame, with the baked-input uploads gated OFF (item 6). Bounded by item 10's benchmark gate; degrades by THROTTLING, never by adding a second renderer. |

This is the same tradeoff `RealtimeToggle` ON already makes for sliders and dials
(`RtToggleWidget_UI.h`) — with one honest difference that must not be glossed: RT is **opt-in and
defaults OFF** (*"cheap scrubbing is the safe default"*, line 75). Areas get the always-on version
because, unlike a slider, an area has **no correct cheap fallback** — the deferred path here is not
"a slightly stale image," it is "the wrong blend equation." Correctness has no toggle.

**6. THE ENABLING PREREQUISITE, ruled as part of the same change: the composite's BAKED-INPUT
uploads become tier-gated.** Per-frame recomposite is unacceptable on today's compose path, and the
reason is accidental, not essential: `BindComposeBuffers` (`PreviewComposite_GpuBuffers_UI.cpp:66-96`)
re-packs and re-uploads every baked field on **every** compose, whether or not a stage ran. That
file's own header comment already names this as owed work (*"Gating those uploads on the two-tier
dirty flags … is M4-5's wiring"*). Measured in bytes moved per compose, from the live code
(ROUGH-ESTIMATE — arithmetic on real array sizes, not benchmarked):

| `mapSize` | cells | weight pack memcpy + upload (9 fields) | 4 scalar fields | per compose | at 60 Hz |
| --- | --- | --- | --- | --- | --- |
| 256 (default) | 66,049 | 2.38 MB | 1.06 MB | **3.4 MB** | 0.21 GB/s |
| 1024 | 1,050,625 | 37.8 MB | 16.8 MB | **54.6 MB** | 3.3 GB/s |
| 8192 (cap) | 67.1 M | 2.42 GB | 1.07 GB | **3.5 GB** | fatal |

Ruled:
- **The dirty tier is the signal — no new dirty state is invented.** `PreviewDriver` already owns the
  only question that matters, and its own header states the invariant this rests on:
  `RefreshTier::PreviewRender` means *"no stage runs, so nothing re-simulates"*
  (`PreviewDriver_PIPELINE.h:5-7`). Every `MapUpdate` refresh composites immediately after running
  the stages, so a subsequent `PreviewRender` compose is provably looking at byte-identical baked
  fields. A resolution/size change routes through `RequestMapUpdate()` → `MapUpdate` → full upload.
  The first compose ever is `MapUpdate` (`bNeedsMapUpdate = true` at construction).
- `previewCompositeCallback` becomes `std::function<void(RefreshTier)>`; `RunPreviewComposite` passes
  the tier it is servicing. Two binding sites change, both lambdas (`Application_UI.cpp:75`,
  `PreviewIntegration_TestScene_UI.h:34`).
- `Compose` takes an options struct, not a second bool parameter:
  `struct ComposeRequest { bool bNeedsTexelReadback = true; bool bBakedInputsChanged = true; };`
  Defaults reproduce today's behavior exactly, so every existing caller and test is unchanged.
  Two bools in a row at a call site is a legibility trap this ARCH does not accept.
- `EnsureAndBind` gains `bool bUploadRequested` and **self-defends**, so the optimization cannot
  produce a stale buffer even if a future caller wires the flag wrongly:
  `Sys::GpuResourceManager::EnsureBuffer` already *"Returns true when a (re)allocation actually
  happened"* (`GpuResource_SYS.h:74-76`), and a freshly (re)allocated buffer has no contents —
  so the upload runs when `bUploadRequested || bReallocated`.
- **Gated (skipped when `bBakedInputsChanged == false`): `PackSurfaceStratumWeights()` itself** (the
  multi-megabyte memcpy) **and the five baked-input uploads** — `kHeightfield`, `kFlow`,
  `kAccumulation`, `kSlope`, `kSurfaceStratumWeights`. Nothing else.

**7. What must NOT be gated — named explicitly, because each looks gateable and is not.** A coder
"finishing the optimization" on any of these breaks a shipped feature:
- **`BuildEntityPoints()` — NOT gated.** It reads `settings.bEntitiesEnabled`
  (`PreviewComposite_Prepare_UI.cpp:114`), a **presentation** toggle. Gating it on the tier would
  make the entities checkbox stop working. It is O(instances) with two multiplies per instance;
  it stays unconditional.
- **`BuildLayerConfigurations()` and its gradient-LUT bakes — NOT gated.** A ramp edit, an opacity
  change, a layer enable/disable and a blend-mode change are all `PreviewRender`-tier and all land
  here. Cost is a few hundred floats per layer.
- **`BuildMapAreaConfigurations()` — NOT gated.** It is the entire point of the per-frame refresh.
- **The seven small uploads — NOT gated**: entity ids (allocation only), gradient tables, entity
  points, configuration, layer configurations, stratum configurations, map-area rectangles. All are
  kilobytes and all are driven by presentation state.
- **`WaitForCompletion`'s fence spin and the unconditional entity-id readback
  (`PreviewComposite_Gpu_UI.cpp:74,86-87`) stay as they are, in scope for a later ticket, not this
  one.** Skipping the readback requires proving no click can be resolved before the next compose —
  a picking-correctness argument this ruling does not make and a coder must not assume. Named here as
  the next lever if item 10's benchmark misses.

**8. One frame of fill latency is ACCEPTED, and stated so it is not "fixed" by reintroducing a
second renderer.** The frame order is fixed (`Application_Frame_UI.cpp:35-57`): `ServiceDirtyTier()`
runs at step 7, the canvas — which is where the area gesture is dispatched — draws at step 8. A
`ContinueAreaDrag` in frame N therefore composites at step 7 of frame N+1. Consequences, all
accepted:
- The fill trails the pointer by exactly one frame **while the pointer is moving**, and is exact the
  frame after it stops. This is strictly better than the shipped bug, which is wrong 100% of the time
  during a gesture.
- The immediate-mode **border and handles** read live `recipe.areas` in the same frame, so during
  fast motion the border leads the fill by one frame. Accepted; it settles instantly on release.
  *(**CORRECTED — VOID — by item 22.** `DrawAreaOverlayPass` runs BEFORE this frame's
  `ContinueAreaDrag`, so chrome and fill are in exact lockstep. There is no divergence to accept
  outside degraded mode.)*
- `PumpWindowEvents` polls rather than waits while `NeedsPreviewRender()` is set, so the loop stays
  hot for the whole gesture with no wait-timeout hitch.
- **The sanctioned future fix, if the human ever finds the lag objectionable, is to move the canvas's
  gesture dispatch ahead of `ServiceDirtyTier` — never a second fill.**

**9. §14.17 item 12's border rule is amended: clause (b) is deleted, and the immediate-mode FILL
clause is deleted with it.** `DrawAreaOverlayPass` after this ruling draws **chrome only**:
- **Border** — when (a) the MapAreas field layer is enabled AND (c) the area is the selected area.
  Clause (b) ("is the currently suppressed area") is gone with the suppression index. The
  "layer disabled ⇒ no border at all, regardless of selection" rule is UNCHANGED and still law.
- **The 8 handles** — selected-area-only, unchanged (§21.8).
- **Cursor shape** — hover-only, lock-gated, unchanged (§21.8 / STEP212).
- **No `AddRectFilled` anywhere in this pass.** `ResolveAreaColor` is no longer needed by it, and
  `manualAreaDrag.areaColors` may become unused *by this file* (it stays on the struct for the tab).

**10. Benchmark gate, with basis tags (Constitution §7).** The per-frame path ships together with
a measurement, owned by the **SanGen Compute Optimization Expert** (GPU/upload path), reviewed by the
**SanGen UI Optimization Expert** (frame-budget impact). Required: wall-clock of one Tier-B2
recomposite (`Compose` with `bBakedInputsChanged == false`, `bNeedsTexelReadback == false`) at
`previewResolution = 512` for `mapSize ∈ {256, 1024}`, with the fence-wait and the entity-id readback
broken out separately, at 0 and at 100k placement instances. My own pre-implementation expectation,
tagged **ROUGH-ESTIMATE** (derived by reading the code paths and the byte counts in item 6, not
measured): ~0.5–2 ms per gated recomposite at defaults, dominated by the fence stall and the
entity-id readback rather than by the layer dispatches. I am **not confident enough to ship that as
a number** — it is a gate, not a result.
- **Pass** (comfortably inside a 16.7 ms budget with room for the rest of the frame): ship as ruled.
- **Miss**: the only sanctioned degradation is to **throttle the recomposite to a fixed interval**
  (e.g. at most one per N milliseconds during a gesture, with a guaranteed final one at
  `EndAreaDrag`). That trades update *rate* — never *correctness*: the area is in its true blend
  mode in every frame it is drawn. Reintroducing an immediate-mode fill, adding a shader callback,
  or approximating Overlay remain forbidden under item 1.

*(**CLOSED by item 17** — ran as STEP218, verdict PASS. The "Miss" degradation above is promoted by
item 18 from a contingency into a MANDATORY always-present floor.)*

---

## Part 2 — the new-area color palette

**11. The palette: 16 entries, hand-authored, ruled here as concrete data.** Design constraints, each
load-bearing and each derived from the live blend math rather than from taste:
- **Every entry has at least one channel at exactly `0.00` and one at exactly `1.00`.** Under
  `Overlay`, a source channel of `0.5` is the *identity* (`d <= 0.5 → 2·d·0.5 = d`;
  `d > 0.5 → 1 - 2(1-d)(1-0.5) = d`) — a mid-gray area is literally invisible. Channels at the
  extremes have maximum authority over the destination. No entry sits anywhere near
  `(0.5, 0.5, 0.5)`.
- **The ±30° neighbourhood of pure green (120°) is EXCLUDED and reserved for `PlayableArea`.** The
  16 hues run from 153.75° in +18.75° steps to 75.00°, spanning the remaining 300° of the wheel.
  Nearest palette hues to green are 153.75° (33.75° away) and 75.00° (45° away) — no other area can
  be confused with the pinned one.
- Full saturation, full value; 18.75° adjacent spacing is well above the just-noticeable difference
  for saturated hues, and consecutive *assignments* are much further apart than that (item 13).

```cpp
// AreaColorTable_UI.h — RGB only; the fill alpha is kDefaultAreaFillAlpha (item 16), not per-entry.
// Spectrum order, +18.75 degrees per row from 153.75. Assignment does NOT walk this in order
// (item 13) — the order here is for a human auditing the table, not for the cycle.
inline constexpr int kAreaPaletteEntryCount = 16;
inline constexpr float kAreaPaletteColors[kAreaPaletteEntryCount][3] = {
    { 0.00f, 1.00f, 0.56f },   //  0 Spring Aqua   153.75
    { 0.00f, 1.00f, 0.88f },   //  1 Turquoise     172.50
    { 0.00f, 0.81f, 1.00f },   //  2 Cyan          191.25
    { 0.00f, 0.50f, 1.00f },   //  3 Azure         210.00
    { 0.00f, 0.19f, 1.00f },   //  4 Blue          228.75
    { 0.13f, 0.00f, 1.00f },   //  5 Indigo        247.50
    { 0.44f, 0.00f, 1.00f },   //  6 Violet        266.25
    { 0.75f, 0.00f, 1.00f },   //  7 Purple        285.00
    { 1.00f, 0.00f, 0.94f },   //  8 Magenta       303.75
    { 1.00f, 0.00f, 0.63f },   //  9 Fuchsia       322.50
    { 1.00f, 0.00f, 0.31f },   // 10 Rose          341.25
    { 1.00f, 0.00f, 0.00f },   // 11 Red             0.00
    { 1.00f, 0.31f, 0.00f },   // 12 Vermilion      18.75
    { 1.00f, 0.63f, 0.00f },   // 13 Orange         37.50
    { 1.00f, 0.94f, 0.00f },   // 14 Yellow         56.25
    { 0.75f, 1.00f, 0.00f },   // 15 Lime           75.00
};
```

**12. Home: `AreaColorTable_UI.h`, and `kPlayableAreaName` moves DOWN into it.** §14.17 item 9's rule
that this header depends on nothing but `<string>`/`<vector>` is preserved exactly — the table, the
count, the stride and the accessor add no include. `kPlayableAreaName`
(`AreasTab_List_UI.h:46`) is a plain `const char*` and moves here so `ResolveAreaColor` can pin the
reserved area without reaching up into a tab header; `AreasTab_List_UI.h` re-exports it by inclusion,
so `IsPlayableArea`, `EnsurePlayableArea` and every existing call site compile unchanged — the same
mechanism that file already documents for `AreaColorEntry` and `AreaLockEntry`
(`AreasTab_List_UI.h:15-26`). `IsPlayableArea(const Params::MapArea&)` stays put; it needs PARAMS.
This mirrors §14.16-D's `Params::kDefaultArmyColors[8][4]` in *shape* (a plain ordered
`inline constexpr` table consumed by index-rotation) and deliberately differs in *home* and *values*:
army color serializes and its palette is a fixed v1-parity port, while area color has no `_PARAMS`
home at all (§14.17 item 13) and its palette is new, so it is free to be designed against the blend
math instead of inherited.

**13. Assignment happens inside `ResolveAreaColor`'s lazy append — NOT at the two creation call
sites.** This is a correction to the proposal, and the reason is a gap the proposal's own framing
exposes: areas arriving from an **imported `.sanmap` never pass through either creation call site**,
so a creation-site rule leaves every imported map's areas flat green — the exact defect being fixed.
`ResolveAreaColor` is the one funnel every area reaches (the tab's swatch, the canvas, and
`BuildMapAreaConfigurations` at `PreviewComposite_Prepare_UI.cpp:146` all go through it), so the rule
lives there, once. Ruled:
```
ResolveAreaColor(table, name):
    existing entry with this name  -> return it, untouched          (unchanged)
    name == kPlayableAreaName      -> append pinned kPlayableAreaColor, consuming NO ordinal
    otherwise                      -> ordinal = count of existing entries whose name != kPlayableAreaName
                                      append kAreaPaletteColors[(ordinal * kAreaPaletteStride) & 15]
                                      with alpha kDefaultAreaFillAlpha
```
- **`kAreaPaletteStride = 7`.** `gcd(7, 16) == 1`, so the sequence visits all 16 entries before
  repeating: ordinals 0..15 map to table indices `0, 7, 14, 5, 12, 3, 10, 1, 8, 15, 6, 13, 4, 11, 2, 9`
  — Spring Aqua, Purple, Yellow, Indigo, Vermilion, Azure, Rose, Turquoise, Magenta, Lime, Violet,
  Orange, Blue, Red, Cyan, Fuchsia. **The minimum hue separation between two CONSECUTIVELY created
  areas is 112.5°**, versus the 18.75° a naive in-order walk would give. That is the whole reason the
  stride exists, and it is why the table is stored in spectrum order (auditable by a human) while the
  cycle is not.
- **16 and 7, not 12 and 1.** Sixteen makes the wrap a mask (`& 15`) rather than a signed integer
  division (`% 12`), and gives four more distinct areas before any repeat. Both operands are
  compile-time constants; there is no division anywhere on this path (Constitution §3).
- **The ordinal is derived, never stored.** No counter, no serialization, no new state — the table's
  own contents answer it. Consequence, stated rather than discovered: a deleted area leaves its
  entry behind, so the next area gets a *fresh* ordinal rather than reusing the dead color — which is
  the desirable direction. Beyond 16 distinct area names the cycle repeats; the swatch is
  user-editable and that is the accepted resolution. **No "scan for an unused color" pass** — it buys
  nothing before area 17 and costs a rule.
- **`PlayableArea` needs no collision check.** It cannot collide with any palette entry by
  construction (item 11's ±30° exclusion). That falls out of the palette design and is why the design
  has that constraint.

**14. Neither creation call site changes at all.** `AreasTab_UI.cpp`'s "Add New Area" button and
`MapCanvas_AreaDragDispatch_UI.cpp`'s `CreateAreaFromDrag` get their color for free, from the first
`ResolveAreaColor` touch (whichever of the tab draw or the composite flatten runs first — both walk
`recipe.areas` in vector order, so the append order is deterministic either way). This is a decisive
advantage of item 13 over the creation-site approach: **one implementation instead of two**, covering
created *and* imported *and* migrated areas, with zero new plumbing. The existing rename fix-up
(`AreasTab_UI.cpp:42-47`) already retargets the entry by name, so a renamed area keeps its palette
color — unchanged and still correct.

**15. Blend mode is a LAYER property and stays one — confirmed, and a per-area blend mode is
FORBIDDEN.** The two are fully orthogonal: `PreviewBlendMode` lives on `PreviewFieldLayer`, one per
layer, and the MapAreas layer is a single layer whose default stays `Overlay`
(§14.17 item 10). `PreviewMapAreaRectangle` carries geometry and color only. A per-area blend field
is refused on two independent grounds: it would push the record past its ruled 32 bytes and break the
std430 stride invariant (§14.17 item 4), and it is **architecturally impossible** in the current pass
shape without restructuring — `mapAreaColorAtCell` returns ONE `vec4` for a pixel, which the layer
pass then blends ONCE with the layer's mode (`PreviewComposite_UI.glsl:63-65`, CPU twin
`PreviewComposite_Cpu_UI.cpp:75-77`). Per-rectangle blending would require per-rectangle
read-modify-write of the image. Not a small change; not ruled; not to be attempted opportunistically.

**16. `kDefaultAreaColor` is retired and split in two.** The single flat green constant no longer
means one thing:
- `inline constexpr float kPlayableAreaColor[kAreaColorChannelCount] = { 0.0f, 1.0f, 0.0f, 0.35f };`
  — the pinned reserved color. `AreasTab_UI.cpp:61-66`'s disabled-swatch re-pin retargets to this
  name; it stays (it defends against the swatch), now backed by the resolve-time pin of item 13 so
  the color is correct whether or not the row was ever expanded.
- `inline constexpr float kDefaultAreaFillAlpha = 0.35f;` — the v1-parity coverage alpha, now named
  once and shared by the pinned color and all 16 palette entries (Constitution §8: a named constant,
  never a literal in a member initializer).
- `AreaColorEntry::color`'s member initializer becomes `{ 0.0f, 0.0f, 0.0f, kDefaultAreaFillAlpha }`
  — an inert placeholder that every construction path immediately overwrites. It must NOT be a
  palette entry or a green: an entry that acquires a real color only through `ResolveAreaColor`
  should not be silently plausible if some future path forgets to.

---

## Part 3 — Piece C ruled to implementation detail (2026-08-29, later the same day)

Written after item 10's benchmark ran (STEP218) and the SanGen Compute Optimization Expert reviewed
it. Verified against the live post-STEP216/217/218 tree before being written down:
`MapCanvas_AreaDragDispatch_UI.cpp`, `MapCanvas_AreaDraw_UI.cpp`, `MapCanvas_Draw_UI.cpp`,
`MapCanvas_UI.h`, `MapCanvas_ManualDragSources_UI.h`, `AreaDragGesture_UI.h`,
`PreviewComposite_UI.h`/`.cpp`, `PreviewComposite_Gpu_UI.cpp`, `PreviewComposite_Cpu_UI.cpp`,
`PreviewComposite_Prepare_UI.cpp`, `PreviewComposite_Settings_UI.h`, `PreviewDriver_PIPELINE.h`,
`Application_UI.cpp`, `Application_Frame_UI.cpp`, `MapCanvas_AreaDragSuppression_UI_Test.cpp`,
`PreviewComposite_MapAreas_UI_Test.cpp`, `work_orders/STEP218_GpuComposeBenchmarkHarness_UI.md`.

**17. Item 10's benchmark gate is SATISFIED — PASS — and the measurement's own blind spot is
recorded as part of the pass, not omitted from it.** The numbers, tagged **MEASURED** (STEP218
binary; **Debug** build, one machine, one run, `previewResolution = 512`, 50 iterations,
`ComposeRequest{bNeedsTexelReadback=false, bBakedInputsChanged=false}`), averages in ms:

| `mapSize` | instances | bindAndDispatch | fenceWait | entityIdReadback | sum-of-maxes |
| --- | --- | --- | --- | --- | --- |
| 256 | 0 | 0.0672 | 0.4634 | 1.0026 | 2.65 |
| 256 | 100,000 | 0.4398 | 0.7545 | 1.0317 | **3.88** |
| 1024 | 0 | 0.0724 | 0.5644 | 1.0495 | 5.83 (one 4.63 readback outlier) |
| 1024 | 100,000 | 0.4290 | 0.7529 | 1.0500 | 3.42 |

The Compute Optimization Expert's verdict — worst representative case 3.88 ms against a 16.7 ms
budget (23% consumed, ~12.8 ms headroom), the 5.83 ms row a single-sample Debug/scheduler spike
rather than steady state — is **accepted**. Piece C is unblocked.

**The blind spot, ruled into the record because a later reader will otherwise treat 3.88 ms as the
whole per-frame cost.** STEP218's first timing window opens *after* `PrepareRun()`
(`PreviewComposite_Gpu_UI.cpp:52` vs. `:64`), by that ticket's own interpretation call 2. The
published totals therefore **exclude `PrepareRun()` entirely** — every one of
`BuildConfigurationRecord`, `BuildStratumConfigurations`, `BuildLayerConfigurations`,
`BuildMapAreaConfigurations`, `BuildEntityPoints` and the `compositeTexels.assign(resolution², 0u)`
zero-fill (`PreviewComposite_UI.cpp:81-93`). Two of those are not small:
- `BuildEntityPoints()` is O(instances) and is **not gated** (item 7, correctly). Each instance calls
  `WorldToPreviewPixel`, which itself calls `ReciprocalOrZero(settings.worldUnitsPerCell)` and
  `PixelsPerPreviewCell()` — both loop-invariant, neither hoisted, and the first containing a
  division (`PreviewComposite_Prepare_UI.cpp:87-93, 111-120`). In a Debug build with no inlining,
  100k iterations of that is **not** free.
- `compositeTexels.assign(resolution², 0u)` is a 1 MB fill at 512², executed on **every** compose,
  including the `bNeedsTexelReadback == false` shape where nothing will ever read it.

Both land inside the item-5 Tier-B2 per-frame path. Their size is **not measured and I will not
publish a number for it** (ROUGH-ESTIMATE: sub-ms at 0 instances, plausibly several ms in a Debug
build at 100k — stated as a shape, not a figure, exactly as item 10's own pre-benchmark estimate
was). The published sum is therefore a **lower bound**, and the shortfall is largest precisely in the
row the expert called "worst representative." This does not reopen the gate — the phases that were
measured carry the 60 Hz conclusion — but it is the decisive reason item 18's watchdog is
**mandatory rather than optional**, and the decisive reason it brackets `Compose()` **whole** instead
of summing item 10's three phases. A safety floor measured through the same keyhole as the benchmark
would inherit the benchmark's blind spot.

**18. The Tier-B2 cost watchdog is MANDATORY law.** Item 10's "Miss" clause is promoted from a
contingency into an always-present floor: the per-frame recomposite ships **with** the throttle
armed, on every machine, in every build. Rationale, stated so it is not softened later: the PASS
rests on one Debug run on one machine with no integrated-GPU or cross-machine data, and item 17 shows
the measurement did not cover the whole call. A floor that only exists if someone later notices a
problem is not a floor. Three named constants, `inline constexpr`, homed beside
`kAreaHandleScreenRadiusPixels`/`kAreaMinimumExtentWorldUnits` in the throttle header of item 20:

```cpp
inline constexpr double kAreaRecompositeCostBudgetMillis       = 8.0;    // one sample above this is "expensive"
inline constexpr int    kAreaRecompositeBreachFrameCount        = 5;     // consecutive expensive samples before throttling
inline constexpr double kAreaRecompositeThrottleIntervalMillis  = 33.0;  // degraded mode: at most one recomposite per this
```

Basis, tagged so a future tuner knows what each number is anchored to:
- `8.0` — **MEASURED-derived**: ~2.06× the measured worst representative case (3.88 ms) and ~48% of
  the 16.7 ms budget. Two-times-worst-measured is the margin that absorbs item 17's unmeasured
  `PrepareRun` remainder and a Release-vs-Debug or GPU-class difference without firing on ordinary
  variance.
- `5` — the smallest count that provably cannot fire on the exact one-frame outlier this dataset
  already exhibited (the 4.63 ms `1024`/0-instance readback spike). A single-frame trigger would have
  throttled on that sample; five consecutive frames at 60 Hz is ~83 ms of *sustained* overrun.
- `33.0` — ~30 Hz, one half of the 60 Hz target. It is a *rate* concession only: the area is in its
  true blend equation in every frame it is drawn, in both modes (item 10's "trades update rate, never
  correctness").

These three are **not** Constitution §8 tunables. §8 governs creative and algorithmic values a
designer may want to play with; this is a safety floor whose value is an argument about a frame
budget, and a user-lowered budget or a user-disabled watchdog is a way to make the program stutter,
not a creative result. They are compile-time law.

**19. Ownership: `PreviewComposite` measures, `MapCanvas` decides. `SetAreaCompositeRefreshCallback`'s
signature does NOT change.** This is the question the ruling exists to settle; both rival homes are
rejected on the record so neither is re-proposed.

- **REJECTED — the refresh callback reports a duration.** It cannot. `areaCompositeRefreshCallback`
  is bound to `previewDriver.NotifyParametersChanged()` (`Application_UI.cpp:189`), which only sets
  `bNeedsPreviewRender`; the compose it requests runs at step 7 of the *next* frame
  (`Application_Frame_UI.cpp:52`). A function cannot return the cost of work that has not happened
  yet, and returning the *previous* compose's cost from something named "refresh" is a lie-shaped
  API. `std::function<void()>` stays exactly as it is.
- **REJECTED — `PreviewDriver` owns the throttle.** A throttle there would gate *every*
  `PreviewRender` refresh — a slider, a checkbox, a ramp edit, a layer reorder — on a condition
  created by an area drag. PIPELINE also has no concept of a gesture, and making
  `previewCompositeCallback` return a duration would install a UI-interaction policy in the layer
  whose whole charter is "owns *when* a composite happens and never *what* it is"
  (`PreviewDriver_PIPELINE.h:11-13`).
- **RULED — `PreviewComposite::Compose()` brackets itself.** One `std::chrono::steady_clock` pair
  around the whole body of `Compose` (`PreviewComposite_UI.cpp:36-39`), recorded into a new
  `double lastComposeMillis` member, exposed as `double LastComposeMillis() const`. Three reasons,
  each independently sufficient:
  1. It is the only place that measures **both** backends — including the two silent fallbacks to
     the Cpu twin inside `ComposeOnGpu` (`PreviewComposite_Gpu_UI.cpp:55, 59`, program-compile or
     texture failure). That fallback is invisible to `ComposeGpuTiming` by construction, and it is
     exactly the catastrophic per-frame case a watchdog exists for.
  2. The bracket includes `PrepareRun()`, closing item 17's blind spot.
  3. `MapCanvas` already holds `const PreviewComposite* composite` (`MapCanvas_UI.h:366`, injected by
     `SetPreviewComposite`, deliberately const). The reader therefore needs **zero** new plumbing:
     no new setter, no new source pointer, no callback change, no Application-side bookkeeping.
- **Cost of measuring: two clock reads per compose, unconditional.** ROUGH-ESTIMATE ~40-60 ns
  (QPC-backed `steady_clock`) against a ≥1 ms compose — under 0.006%. Deliberately **not** gated
  behind an "enable timing" flag: a gate would make the safety floor's own input conditional, and
  item 7's whole lesson is that a conditional on a hot path is what breaks silently later.
- **`ComposeGpuTiming` / `ComposeOnGpu`'s `outTiming` are UNTOUCHED.** STEP218's per-phase
  breakdown stays exactly as shipped and remains the benchmark's instrument. `LastComposeMillis()`
  does not duplicate it and is not derived from it: different bracket (the whole call, including
  `PrepareRun`), different lifetime (always-on scalar vs. opt-in diagnostic), different consumer
  (a live watchdog vs. an offline harness). Both may exist; neither replaces the other.

**20. The decision is a pure function in its own tiny header, so it is testable without a GPU, a
gesture, or a stopwatch.** New `AreaRecompositeThrottle_UI.h` — header-only, depends on nothing
(no imgui, no composite, no PARAMS), the same shape this subsystem already uses for
`AreaColorTable_UI.h` / `AreaLockTable_UI.h` / `AreaDragGesture_UI.h`:

```cpp
struct AreaRecompositeThrottleState {
    int    breachFrameCount      = 0;      // consecutive samples over budget
    double lastRequestTimeMillis = 0.0;
    bool   bSamplePending        = false;  // "a compose I asked for has since run; the next sample is mine"
    bool   bDeferredMove         = false;  // "the rectangle moved but the throttle ate the request"
};

// Folds one gesture frame into `state` and answers whether a recomposite may be requested NOW.
// `lastComposeMillis` is PreviewComposite::LastComposeMillis(); it is only consumed when
// state.bSamplePending says the sample belongs to this gesture.
bool ShouldRequestAreaRecomposite(AreaRecompositeThrottleState& state, bool bRectangleMoved,
                                  double lastComposeMillis, double nowMillis);
```

The function body, ruled step by step so it is transcribed rather than designed:
1. If `state.bSamplePending`: `state.breachFrameCount = lastComposeMillis > kAreaRecompositeCostBudgetMillis ? state.breachFrameCount + 1 : 0;`
   then `state.bSamplePending = false`.
2. `if (bRectangleMoved) state.bDeferredMove = true;`
3. `if (!state.bDeferredMove) return false;` — a motionless held pointer with nothing outstanding
   costs nothing, exactly as item 4 requires.
4. `if (state.breachFrameCount >= kAreaRecompositeBreachFrameCount && nowMillis - state.lastRequestTimeMillis < kAreaRecompositeThrottleIntervalMillis) return false;`
   — throttled; `bDeferredMove` deliberately stays set (item 21).
5. `state.lastRequestTimeMillis = nowMillis; state.bSamplePending = true; state.bDeferredMove = false; return true;`

Why a **counter** and not a rolling window of the last five costs: "5 consecutive samples over
budget" is exactly a consecutive-breach count. A ring buffer would store four values no rule ever
reads and would let a future edit accidentally redefine the condition as an average. One `int`.

Why the sample is honest: `MapCanvas`'s gesture runs at frame step 8 and `ServiceDirtyTier()` at
step 7, so when `ContinueAreaDrag` runs at frame N+1 the compose requested at frame N has *already*
completed this frame. `LastComposeMillis()` read at step 1 above is precisely that compose. The one
impurity — another edit tripping a `MapUpdate` in the same frame, making the sample an over-read — is
**accepted and stated**: it can only make the watchdog throttle *earlier*, it needs five consecutive
such frames, and no other edit is plausible while a canvas drag holds the mouse.

**Recovery is ruled here, closing the implementation-level call the Compute Optimization Expert left
open: clear mid-gesture, immediately, symmetric.** Any sample at or under budget resets
`breachFrameCount` to 0 (step 1 above already does this), which un-throttles on the next frame; and
`TryBeginAreaDrag` resets the whole struct so every gesture starts un-throttled. **No per-gesture
re-arm latch, no separate exit threshold, no hysteresis band.** Entry already carries five frames of
hysteresis; the worst exit behavior is an alternation between throttled and un-throttled that
averages somewhere between 30 and 60 Hz — and *every frame in both states shows the true blend
equation*, so flapping costs update rate only, which is the exact tradeoff item 10 sanctioned. A
fourth tuning constant to damp a harmless oscillation is not bought.

`nowMillis` comes from `ImGui::GetTime() * 1000.0`. Ruled over `std::chrono` at the call site because
it is the frame clock the gesture already lives on, it introduces no second time base into the UI
layer, and it is **deterministic in tests**: the existing area-gesture fixtures already drive
`io.DeltaTime = 1.0f/60.0f` per frame (`MapCanvas_AreaDragSuppression_UI_Test.cpp:24`), so the 33 ms
interval is exercised by counting frames — never a sleep, never a flaky wall-clock assertion. The
throttle's own logic, being a pure free function, is unit-tested with fabricated costs and fabricated
times and needs no GL context at all.

**21. `bDeferredMove` is load-bearing, not bookkeeping — without it the throttle can strand a stale
fill for the rest of the gesture.** The failure it prevents, stated concretely: frame N moves the
rectangle while throttled, so the request is suppressed; the user then holds the pointer still.
`bRectangleMoved` is false from then on, so a "request only on a moving frame" rule never fires
again, and the fill stays at the pre-move rectangle until `EndAreaDrag`. Item 8's guarantee — "exact
the frame after it stops" — would be false in degraded mode, which is the one mode nobody is watching.
The latch fixes it: the first frame past the interval fires the request even though nothing moved
that frame. Note this does not weaken item 4's idle protection — once the latch clears,
`NeedsPreviewRender()` is false again and `PumpWindowEvents` returns to `glfwWaitEventsTimeout`.

**22. Item 8's one-frame latency is CONFIRMED unchanged by per-moved-frame recomposition; item 8's
second bullet is CORRECTED and VOID.**

- **Confirmed.** The one-frame lag is produced solely by the fixed position of `ServiceDirtyTier()`
  (step 7) relative to the canvas's gesture dispatch (step 8) in `Application::RunOneFrame`
  (`Application_Frame_UI.cpp:52-53`). Recompose *frequency* does not enter that argument: whichever
  frames fire a request, the composite serviced at step 7 of frame N can only reflect a
  `recipe.areas` write made no later than frame N-1. Frequency changes how OFTEN the fill is
  refreshed; the step order changes how LATE each refresh is. The two are independent, and firing
  every moved frame instead of only at begin/end changes the *first* and not the *second*.
- **Corrected.** Item 8's second bullet claimed the immediate-mode border leads the fill by one frame
  during fast motion. That is false under Piece C, verified against `MapCanvas_Draw_UI.cpp`:
  `DrawAreaOverlayPass` is called at line 51, **before** the region's `ImGui::InvisibleButton`
  (line 57) and therefore before this frame's `ContinueAreaDrag` (line 199). The chrome pass reads
  `recipe.areas` at its frame-N-1 value — the same value the composite serviced at step 7 was built
  from. **Border, handles and fill are in exact lockstep, all one frame behind the pointer.** There
  is no divergence to accept. The bullet described the shipped suppression design and does not
  survive it.
- **The divergence does appear in degraded mode**, and is stated so it is diagnosed instead of
  re-reported as a bug: while throttled, chrome stays one frame behind while the fill may be up to
  ~33 ms behind. That gap is the visible signature of the throttle.

**23. The concrete final shape. This is the delta a work-order transcribes; nothing below is left to
be re-derived.**

**A. `MapCanvas_AreaDragDispatch_UI.cpp`**
- **DELETE `SetMapAreaSuppression` entirely** (lines 38-47) and its declaration/comment
  (`MapCanvas_UI.h:358-361`).
- **`TryBeginAreaDrag`** — delete both `if (bAreaDragActive) SetMapAreaSuppression(...)` calls
  (lines 72, 98) and **fire no refresh request at all**, amending item 4's begin clause. Justified,
  not assumed: with the suppression index gone, a begin changes no composite input — `BeginAreaDragGesture`
  does not move the rectangle, and selection is *not* a composite input (item 9 puts border and
  handles in the immediate-mode chrome pass; `BuildMapAreaConfigurations` reads no selection). A
  begin-time recomposite would produce a byte-identical image, which is the precise case item 4's own
  cited idiom refuses to pay for. **The single condition that reinstates it, named so it is checkable
  rather than guessed: if the MapAreas composite ever gains a selection-dependent visual.** On a
  successful begin, reset the throttle state (`areaRecompositeThrottle = AreaRecompositeThrottleState();`).
- **`ContinueAreaDrag`** — after the existing guards: snapshot the four floats of
  `(*manualAreaDrag.areas)[manualAreaDrag.state.areaIndex]` (`originX`, `originZ`, `width`, `length`);
  call `UpdateAreaDragGesture(...)` unchanged; compare the four; then
  `if (ShouldRequestAreaRecomposite(areaRecompositeThrottle, bRectangleMoved, composite->LastComposeMillis(), ImGui::GetTime() * 1000.0) && areaCompositeRefreshCallback) areaCompositeRefreshCallback();`
  Guard the snapshot on `state.areaIndex` being in range (`AreaDragGesture_UI.h:32-40` — the same
  defensive check `UpdateAreaDragGesture` already documents).
- **`EndAreaDrag`** — replace `SetMapAreaSuppression(-1)` with an **unconditional**
  `if (areaCompositeRefreshCallback) areaCompositeRefreshCallback();`, fired regardless of throttle
  state and regardless of whether the last frame moved (item 4's end clause, unchanged in substance
  and explicitly exempt from item 18's interval). Then reset the throttle state.
- **`CreateAreaFromDrag`** — unchanged, its one request stays (item 4, item 14).
- Add `#include <imgui.h>` and `#include "AreaRecompositeThrottle_UI.h"`; the file's header comment
  is rewritten (its first 7 lines currently describe the retired suppression mechanic as current law).

**B. `MapCanvas_AreaDraw_UI.cpp`**
- Delete the `suppressedIndex` read (lines 39-40) and the **entire** `if (suppressedIndex >= 0 ...)`
  block (lines 48-63) — with it go the `AddRectFilled`, the `ResolveAreaColor` call and the
  `manualAreaDrag.areaColors` use. Drop `#include "AreasTab_List_UI.h"` **only if** `ResolveAreaColor`
  was its sole use in this file (verify at edit time; `IsAreaLocked` is a member, not from that header).
- The **border** moves into the existing selected-area block, gated per item 9 on
  `IsMapAreasLayerEnabled(composite->Settings())` and a valid `selectedIndex` — clause (b) gone.
  Border and the 8 handles now share one `if (selectedIndex ...)` scope and one pair of `ToScreen`
  corner projections instead of two.
- Handles: unchanged. Cursor-shape section: unchanged (`IsAreaLocked` gate intact).
- `IsMapAreasLayerEnabled` is unchanged and keeps its home in this file's anonymous namespace.

**C. `MapCanvas_ManualDragSources_UI.h`** — delete `int* mapAreaSuppressedIndex` (line 64) and its
comment block (lines 60-63).

**D. `MapCanvas_UI.h`** — `SetManualAreaDragSource` drops its fifth parameter and the assignment
(lines 166-171); delete the `SetMapAreaSuppression` declaration; add one
`AreaRecompositeThrottleState areaRecompositeThrottle;` member beside `bAreaDragActive` (line 443);
rewrite `SetAreaCompositeRefreshCallback`'s contract comment (lines 174-176 currently promise the
exact opposite of the new law: *"Fired exactly twice per drag … never once per ContinueAreaDrag
frame"*). Note this class is already over its §21.7 size ceiling: the net change here is **-1 method,
-1 parameter, +1 member**, which is the correct direction.

**E. `PreviewComposite_Settings_UI.h`** — delete `int mapAreaSuppressedIndex = -1;` (line 109).

**F. `PreviewComposite_Prepare_UI.cpp`** — delete line 139's
`if (index == settings.mapAreaSuppressedIndex) continue;`.

**G. `Application_UI.cpp`** — the `SetManualAreaDragSource` call (lines 157-159) drops its fifth
argument; its comment (lines 151-156) drops the `mapAreaSuppressedIndex` clause.

**H. `PreviewComposite_UI.h` / `PreviewComposite_UI.cpp`** — item 19's self-measurement: a
`double lastComposeMillis = 0.0;` member, a `double LastComposeMillis() const` accessor next to
`LastRunUsedGpu()`, `<chrono>` in the `.cpp`, and the clock pair around `Compose()`'s body. Nothing
else in either file changes; `ComposeRequest`, `ComposeGpuTiming` and `ComposeOnGpu` are untouched.

**I. `AreaRecompositeThrottle_UI.h`** — new, per item 18/20 (the three constants, the state struct,
the one pure function).

**J. Tests.**
- `MapCanvas_AreaDragSuppression_UI_Test.cpp` is **RETIRED and REPLACED** — its entire premise
  ("exactly two recomposites per gesture … never one per ContinueAreaDrag frame") is now the opposite
  of law. New `MapCanvas_AreaDragRecomposite_UI_Test.cpp` asserts: zero requests at begin; one per
  moved frame; **zero** on a held-but-motionless frame; exactly one at release; and
  `CreateAreaFromDrag`'s single request unchanged. Rename its declaration/call
  (`MapCanvas_UI_Test.cpp:31-32, 59`) and its `CMakeLists.txt` registration (line 593) with it.
- New `AreaRecompositeThrottle_UI_Test.cpp` — headless, no GL, no imgui: five fabricated 9 ms samples
  engage the throttle; a fabricated 2 ms sample clears it immediately; a throttled moving frame
  followed by motionless frames still fires once past 33 ms (item 21's stranding case); a
  never-moving gesture fires nothing.
- `MapCanvas_AreaAltCenterResizeModifier_UI_Test.cpp:129, 144` — drop the local
  `mapAreaSuppressedIndex` and the fifth argument.
- `PreviewComposite_MapAreas_UI_Test.cpp:103-118` — **delete** both suppression assertions outright.
  They assert retired law; they are not a regression to preserve.

**24. Explicitly OUT of Piece C — named so the scope cannot creep during implementation.**
- **No change to `ComposeGpuTiming`, `ComposeOnGpu`'s `outTiming`, or the STEP218 benchmark binary.**
- **The fence spin and the entity-id readback stay** (item 7). Still the named next lever; still
  requires the picking-correctness argument this ruling does not make.
- **Item 17's two newly-identified levers are NOT in Piece C** and must not be attempted
  opportunistically inside it: (a) hoisting `WorldToPreviewPixel`'s two loop-invariant calls (and its
  division) out of `BuildEntityPoints`'s per-instance loop; (b) skipping
  `compositeTexels.assign(resolution², 0u)` when `bNeedsTexelReadback == false`. Both are
  ROUGH-ESTIMATE-sized only, and (b) in particular needs a proof that no reader of `CompositeTexels()`
  can observe the skip — the same class of argument item 7 refused to make for the readback. They are
  a separate ticket, owned by the SanGen Compute Optimization Expert, gated on a **Release**-build
  re-run of STEP218 with the timing window widened to include `PrepareRun()`.
- **No per-area blend mode** (item 15). **No second fill renderer, ever** (item 1). **No shader
  callback** (item 2).
- No `.sanmap` schema, `Params::MapArea`, picking or `SanGenVersion` change of any kind.

---

**Dispatchable as three independently shippable pieces** (a coder work-order may split or combine
them): **(A)** the tier-gated baked-input uploads — `ComposeRequest`, the
`std::function<void(RefreshTier)>` callback, `EnsureAndBind`'s self-defending upload flag (items 6-7).
Pure performance, no visual change, testable on its own by asserting the composite's texels are
identical with and without the flag. **(B)** the palette (items 11-16) — pure presentation, no
dependency on (A) or (C). **(C)** the one-fill rule — retire the suppression index, per-frame refresh
on movement, border/fill amendment (items 3, 4, 8, 9). **(C) must not land before (A)**: without the
upload gating, a per-frame recomposite moves 55 MB per frame on a 1024 map. (B) may land any time.

**Status, 2026-08-29 (later the same day): (A) SHIPPED (STEP216). (B) SHIPPED (STEP217). Item 10's
gate SHIPPED and CLOSED-PASS (STEP218, item 17). (C) is UNBLOCKED and FULLY SPECIFIED — items
1-5 and 8-9 as amended by items 17-24, which a work-order transcribes rather than re-derives.**
