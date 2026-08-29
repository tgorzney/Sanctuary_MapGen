[← ARCH index](ARCH.md) · [§14 ARCH_14_PreviewOverlayLayering](ARCH_14_PreviewOverlayLayering.md) · SanGen ARCH §14.17. **Only the ARCH Expert writes this file.**

### 14.17 Map areas are a composited FIELD LAYER — `PreviewLayerKind::MapAreas`, analytic rectangles flattened from PARAMS at `PrepareRun()` (ARCH ruling, human-approved; amends §21.8's draw-pass ruling)

Human-approved ruling, verified against the live code before being written down (not taken on the
proposal's word): `PreviewComposite_Settings_UI.h`, `PreviewComposite_Kernel_UI.h`,
`PreviewComposite_UI.h`, `PreviewComposite_Prepare_UI.cpp`, `PreviewComposite_Cpu_UI.cpp`,
`PreviewComposite_GpuProgram_UI.cpp`, `PreviewComposite_GpuBuffers_UI.cpp`,
`PreviewComposite_UI.glsl`, `PreviewComposite_Sampling_UI.glsl`, `Application_PreviewSetup_UI.cpp`,
`Application_Panels_UI.h`, `Application_Visibility_UI.h`, `Application_UI.cpp`,
`AreasTab_UI.h`/`.cpp`, `AreasTab_List_UI.h`, `MapCanvas_UI.h`,
`MapCanvas_ManualDragSources_UI.h`, `MapCanvas_AreaDraw_UI.cpp`,
`MapCanvas_AreaDragDispatch_UI.cpp`, `MapArea_PARAMS.h`.

---

**1. The general rule this ruling generalizes — §14's overlay-domain / field-layer separation is a
rule about `Data::PlacementInstances`, not a rule about "anything that is not a
`Data::FloatField`."** §14's core distinction (`PREVIEW_COMPOSITING_SPEC` "The core distinction this
section exists to enforce") exists to stop a *placement decision* from being re-decided in the
compositor — the v1 shadow-sim bug where the shader re-tested `rule.MinSlope/MaxSlope/MinHeight/
MaxHeight` and painted markers the bake had rejected (ARCH §3.2). The question that gates
`PreviewFieldLayer` membership is therefore: **does this layer re-decide something a PROC stage
already resolved?** It is NOT "does this layer sample a baked `Data::FloatField`?" Two shipped
`PreviewLayerKind` values already answer "no" to the second question and are legal anyway —
`StratumSplat` (nine weight fields plus per-stratum PARAMS tints; `LayerSourceField` returns
`nullptr` for it, `PreviewComposite_Prepare_UI.cpp:35`) and `Water` (a threshold over the
heightfield parameterized entirely by `Params::Water`, `PreviewComposite_Sampling_UI.glsl:137-146`).
**Ruled: a per-pixel color source flattened from PARAMS with no placement rule behind it —
`StratumSplat`, `Water`, `MapAreas` — is a legal `PreviewFieldLayer` kind.** A layer whose
membership is decided by a PROC stage's accept/reject (markers, units, props, decals, reclaim)
is not, and stays an `OverlayDomainKind_UI` screen-space domain.

**2. What Map Areas are NOT, ruled explicitly so a coder cannot "simplify" toward any of them.**
- **Not a seventh `OverlayDomainKind_UI`.** That enum stays closed at six
  (`Alloy, SpawnsArmies, Units, Props, Reclaim, Decals`, §14.2). An Area produces no
  `Data::PlacementInstance`, has no icon, no atlas page, no LOD threshold, and no sub-layer
  refs — every field on `OverlayLayer_UI` would be inert for it.
- **Not a baked raster mask.** Rasterizing areas into a `Data::FloatField` would put a
  presentation-only, non-serialized rectangle list into the DATA layer and into a stage's
  parameter hash — a Tier A regeneration for a color edit, the exact inversion §14.8 forbids.
- **Not a PROC stage.** `AreasTab_UI.h`'s own SCOPE NOTE 1 already records that an Area feeds no
  generation stage; nothing here changes that.
- **Not an entity in `Data::EntityIdBuffer`.** See item 8.

**3. Enum and GPU `#define` — appended LAST, never inserted.** `PreviewLayerKind`
(`PreviewComposite_Settings_UI.h:22`) becomes
`{ HeightRamp, StratumSplat, Flow, Accumulation, Water, Slope, MapAreas }` — appended after `Slope`,
because the enum's integer values are load-bearing on both sides of the seam (the CPU switch and the
GPU `#define`s `PreviewComposite_GpuProgram_UI.cpp:37-42` generates from the same enum), exactly the
append-only discipline `PreviewBlendMode`'s own STEP200 note already states
(`PreviewComposite_Settings_UI.h:26-30`). A matching line joins `BuildEnumDefinitions()`:
`IntegerDefinition("PREVIEW_LAYER_MAP_AREAS", static_cast<int>(PreviewLayerKind::MapAreas))`. The
numbering is generated from the enum on both sides, so the two cannot drift.

**4. Buffer binding, buffer name, and the record shape.**
- `CompositeBinding::kMapAreaRectangles = 12` (`PreviewComposite_Kernel_UI.h:28-40`). **Binding 7
  stays vacant** — that file's own comment (lines 26-27) states the hole is deliberate, kept so
  every other index stays stable; filling it would be a silent renumber of a documented gap. 12 is
  the next free index after `kSlope = 11`.
- `CompositeBufferName::kMapAreaRectangles = "previewCompositeMapAreaRectangles"`, beside its
  siblings (`PreviewComposite_Kernel_UI.h:53-65`) — part of the kernel contract, since
  `GpuResourceManager` reallocates only when a *named* buffer's byte size changes.
- The record, beside `PreviewLayerConfiguration`/`PreviewStratumConfiguration` in
  `PreviewComposite_Kernel_UI.h`. **32 bytes / 8 plain 4-byte scalars, no padding member** — a whole
  16-byte multiple, so the std430 array stride needs no implicit padding, which is that header's own
  stated invariant (lines 5-7):

```cpp
// One map area, flattened to the composite's own CELL space and its presentation color.
// 8 scalars = 32 bytes. Areas are PRESENTATION geometry: no placement rule stands behind them,
// so this record re-decides nothing a PROC stage resolved (ARCH §14.17 item 1).
struct PreviewMapAreaRectangle {
    float minimumX = 0.0f;
    float minimumZ = 0.0f;
    float maximumX = 0.0f;
    float maximumZ = 0.0f;
    float colorRed = 0.0f;
    float colorGreen = 0.0f;
    float colorBlue = 0.0f;
    float colorAlpha = 0.0f;
};
```

- **Coordinates are in CELL space, computed CPU-side.** `minimumX = area.originX * cellsPerWorldUnit`,
  `maximumX = (area.originX + area.width) * cellsPerWorldUnit`, and the Z pair likewise, with
  `cellsPerWorldUnit = ReciprocalOrZero(settings.worldUnitsPerCell)` — the SAME reciprocal
  `WorldToPreviewPixel` already takes (`PreviewComposite_Prepare_UI.cpp:87`), so multiply-never-
  divide holds (Constitution §3) and there is no second copy of the world→cell arithmetic. Cell
  space is the right space because it is exactly what both twins already hand
  `layerColorAtPixel`: `sampleX = (pixelX + 0.5) * (vertexSize - 1) / resolution`
  (`PreviewComposite_UI.glsl:59-62`, `PreviewComposite_Cpu_UI.cpp:67-72`). Comparing a cell-space
  rectangle against `sampleX`/`sampleY` needs zero further transform, and it agrees exactly with the
  immediate-mode border's own `WorldToPreviewPixel` path, since both derive from the one
  `PreviewCompositeSettings::worldUnitsPerCell` mirror.
- **No count field is added to `PreviewCompositeConfiguration`.** That record is 20 scalars / 80
  bytes today (`PreviewComposite_Kernel_UI.h:111-133`) and its GLSL twin `CompositeConfiguration` is
  declared **twice** — in `PreviewComposite_Sampling_UI.glsl` and again in `PreviewComposite_UI.glsl`
  (both at their `PREVIEW_BINDING_CONFIGURATION` declarations). A 21st scalar breaks the 16-byte
  multiple and must be mirrored in two GLSL units by hand. Ruled: the shader reads the buffer's own
  `mapAreaRectangles.length()`, and the CPU twin its `.size()`. **Empty list: push one degenerate
  sentinel rectangle with `minimumX > maximumX`** (e.g. `minimumX = 1.0f, maximumX = -1.0f`, every
  other field zero) — it can never contain a sample point, and it preserves the "never a 0-byte
  buffer" idiom `BuildLayerConfigurations` already applies to `gradientLookupTables`
  (`PreviewComposite_Prepare_UI.cpp:72`).
- **New builder `void PreviewComposite::BuildMapAreaConfigurations();`**, declared beside
  `BuildStratumConfigurations`/`BuildLayerConfigurations` on `PreviewComposite_UI.h` and called from
  `PrepareRun()`, with a new `std::vector<PreviewMapAreaRectangle> mapAreaRectangles;` member beside
  `stratumConfigurations`. Upload/bind is one more `EnsureAndBind(...)` line in
  `BindComposeBuffers` (`PreviewComposite_GpuBuffers_UI.cpp:84-92`), shaped exactly like the
  `kStratumConfigurations` line above it. GLSL side: one `struct MapAreaRectangle` + one
  `layout(std430, binding = PREVIEW_BINDING_MAP_AREAS) readonly buffer` declaration in
  `PreviewComposite_Sampling_UI.glsl` only (the pass unit reaches it only through
  `layerColorAtPixel`, so the buffer is declared once — that file's own stated convention, line 130),
  plus one `IntegerDefinition("PREVIEW_BINDING_MAP_AREAS", …)` in `BuildBindingDefinitions()`
  (`PreviewComposite_GpuProgram_UI.cpp:58-71`).

**5. `LayerSourceField` gets an explicit `case`, never the `default:` fall-through.**
```cpp
case PreviewLayerKind::MapAreas:     return nullptr;
```
beside `case PreviewLayerKind::StratumSplat: return nullptr;`
(`PreviewComposite_Prepare_UI.cpp:30-38`). Letting it reach `default: return &mapFields.heightfield;`
would silently give the layer a heightfield auto-domain it has no use for. Two consequences, both
already handled by existing code and neither to be "fixed" by a coder: `bAutoDomainFromField` is
skipped by the existing `sourceField != nullptr` guard (line 58), and the CPU twin's null deref at
`PreviewComposite_Cpu_UI.cpp:108-109` is unreachable **only because** `LayerColorAtPixel` early-
returns for this kind before it — the exact same invariant `StratumSplat` already relies on. The
`MapAreas` branch therefore MUST be added to `LayerColorAtPixel` in the same change as the
`LayerSourceField` case, never separately.

**6. Overlap / Z rule — forward iteration, LAST containing match wins.** One rule, shared by the
hit-test and the visual, so click-to-select and what-you-see can never disagree. This is the rule
`TryBeginAreaDrag`'s own body hit-test already implements
(`MapCanvas_AreaDragDispatch_UI.cpp:46-49`, "forward iteration, last match wins") and it matches
v1's "later in the vector is drawn topmost" convention §21.8 already ported. Both twins therefore
scan forward and keep overwriting, rather than returning on first hit:
```glsl
vec4 mapAreaColorAtCell(float sampleX, float sampleY) {
    vec4 result = vec4(0.0);
    for (int index = 0; index < mapAreaRectangles.length(); ++index) {
        MapAreaRectangle area = mapAreaRectangles[index];
        if (sampleX < area.minimumX || sampleX > area.maximumX) continue;
        if (sampleY < area.minimumZ || sampleY > area.maximumZ) continue;
        result = vec4(area.colorRed, area.colorGreen, area.colorBlue, area.colorAlpha);
    }
    return result;
}
```
The degenerate sentinel (item 4) fails the first test unconditionally, so the empty case costs one
rejected iteration and returns `vec4(0.0)`. The CPU twin is the same loop over `mapAreaRectangles`,
returning `PreviewColor()` for a miss — the same early-out shape the `Water` branch already uses.

**7. Blend math needs zero new machinery.** `layerColorAtPixel` already returns
`vec4(rgb, coverageAlpha)` and the pass blends with
`amount = layerOpacity(layerIndex) * layerColor.a` (`PreviewComposite_UI.glsl:63-65`; CPU twin
`PreviewComposite_Cpu_UI.cpp:75-77`). Outside every area the layer answers alpha 0 and contributes
nothing — no per-layer "coverage mask" concept, no new blend mode, no new uniform. The area's own
`AreaColorEntry` alpha (default `0.35`) is the coverage; `PreviewFieldLayer::opacity` multiplies it
as a layer-wide scale exactly as for every other layer.

**8. Picking is untouched.** Areas never enter `Data::EntityIdBuffer` and never produce a
`PreviewEntityPoint`. The entity-id passes write instance indices into
`Data::PlacementInstances`, and an Area has no such index. Area picking remains **entirely** §21.8's
CPU-side hit test (`IsWorldPointInsideArea` / `HitTestAreaHandles`) against live `recipe.areas` —
one source of truth, no second picking path. A coder must not add an "area id" to the entity-id
buffer to make selection "consistent"; item 6's shared Z rule is what makes it consistent.

**9. Data sources and the `AreaColorEntry` ownership move.** Two inputs, each entering by the route
its own category already dictates:
- **`recipe.areas` (PARAMS) enters as a new constructor const-ref**, mirroring `strata` exactly:
  `PreviewComposite(const Params::Geometry&, const Params::Water&, const std::vector<Params::Stratum>&,
  const std::vector<Params::MapArea>&, const Data::MapFields&, const Data::PlacementInstances&,
  Data::EntityIdBuffer&)`, stored as `const std::vector<Params::MapArea>& areas;` beside `strata`.
  Wired in `Application_UI.cpp:30-31`'s member-init list as `recipe.areas`, the same live vector the
  tab and the canvas already edit. Read-only: the composite never writes an Area.
- **The color table (PRESENTATION) moves into the settings struct.** Ruled: `AreaColorEntry`'s
  single owner becomes `PreviewCompositeSettings::areaColors`
  (`std::vector<AreaColorEntry> areaColors;`), and `AreasTabState::areaColors`
  (`AreasTab_UI.h:34`) is **removed**, not duplicated. This is the same category argument that puts
  `gradientRamps` and `clearColor` there: it is presentation state that never serializes into
  `mapGeneratorData` (`PreviewComposite_Settings_UI.h:6-8`), and the tab, `MapCanvas`, and the new
  composite input all need the same *mutable* table. `AreasTab`/`MapCanvas` keep reading and writing
  the same vector through `ResolveAreaColor` verbatim — name-keyed resolution and its
  lazy-append-on-first-touch are unchanged (`AreasTab_List_UI.h:86-93`).
- **Header mechanics, ruled rather than left to the coder.** `PreviewComposite_Settings_UI.h`
  currently includes only `<vector>` and `GradientRamp_PARAMS.h`; it must NOT grow a dependency on
  `AreasTab_List_UI.h` (which pulls `ColorSwatch_UI.h`, `RtToggleWidget_UI.h`, `UniqueNameList_UI.h`
  and `MapArea_PARAMS.h` — a tab header leaking into the composite's own settings). Ruled: a new
  minimal header `src/ui/AreaColorTable_UI.h` holds `kAreaColorChannelCount`, `kDefaultAreaColor`,
  `AreaColorEntry` and `ResolveAreaColor`, depending on nothing but `<string>`/`<vector>`;
  `AreasTab_List_UI.h` includes it (keeping every existing call site compiling unchanged) and carries
  one `static_assert(kAreaColorChannelCount == kColorSwatchChannelCount, …)` so the two channel-count
  constants can never drift — the same "name the count and assert against it" idiom
  `kPreviewBlendModeCount` already establishes (`PreviewComposite_Settings_UI.h:36-37`).
  `MapCanvas_ManualDragSources_UI.h:11`'s `#include "AreasTab_List_UI.h" // AreaColorEntry` retargets
  to the new header.
- **Threading consequence, stated so it is not discovered mid-implementation:**
  `DrawAreasTab(Params::MapRecipe&, AreasTabState&, Pipeline::PreviewDriver*)` gains one parameter,
  `std::vector<AreaColorEntry>& areaColors`, and its one call site
  (`Application_PanelEnvironment_UI.cpp:57`) passes `composite.Settings().areaColors`. Everything
  below it (`DrawAreaDetail`'s `ResolveAreaColor(state.areaColors, …)`, the rename fix-up at
  `AreasTab_UI.cpp:40-41`) rebinds to that reference; no logic changes.

**10. Defaults.**
- **New areas default to Green, blend mode Overlay.** `kDefaultAreaColor` in `AreaColorTable_UI.h` is
  `{ 0.0f, 1.0f, 0.0f, 0.35f }` — RGB green, and the pre-existing `0.35` fill alpha preserved
  verbatim (that alpha is v1 parity, `AreasTab_List_UI.h:76-80`; only the RGB changes). It is a
  named constant, not a literal in the struct's member initializer (Constitution §8), and
  `AreaColorEntry::color` initializes from it.
- **`Params::MapArea` named `"PlayableArea"` is always Green and non-editable.** The default IS
  green, so a freshly-resolved PlayableArea entry is already correct with no special case; the only
  thing that could change it is the tab's own swatch. Ruled: the Areas tab draws that swatch inside
  `ImGui::BeginDisabled`/`EndDisabled` when `IsPlayableArea(area)` (`AreasTab_List_UI.h:36`) and
  re-pins the resolved entry to `kDefaultAreaColor` before drawing. This mirrors the existing row-lock
  logic that already keys "cannot be removed" off the same predicate
  (`IsAreaRemovable() == false`), and `BeginDisabled` is already an established codebase idiom
  (§15.5's own read-only-slider ruling). No other path can mint a PlayableArea color: the canvas
  create path names areas via `NextAreaName` (`MapCanvas_AreaDragDispatch_UI.cpp:87`) and the table
  never serializes.
- **The layer is seeded topmost and enabled.** `ConfigureDefaultPreview`
  (`Application_PreviewSetup_UI.cpp:60-80`) pushes it LAST, after Accumulation:
  `MakeFieldLayer(PreviewLayerKind::MapAreas, PreviewBlendMode::Overlay, -1, 0.0f, 1.0f, 1.0f)` —
  `gradientRampIndex = -1` for the same reason `StratumSplat` uses it (no ramp; the color comes from
  the per-area tint), and the domain pair is inert.
- **"Enabled by default" is expressed through the panel catalogue, not by fighting it.**
  `MakeFieldLayer` deliberately forces `bEnabled = false` and lets `ApplyPanelVisibility` decide
  (`Application_PreviewSetup_UI.cpp:35`, run at construction, `Application_UI.cpp:58`). The Areas row
  today is `{ …, true, false, PreviewVisibilityTarget::None, PreviewLayerKind::HeightRamp }`
  (`Application_Panels_UI.h:86-87`) — a `[O]` toggle that drives nothing, one of the six rows
  `Application_Visibility_UI.h:10-15`'s SCOPE NOTE names. Ruled: that row becomes
  `{ …, true, true, PreviewVisibilityTarget::FieldLayer, PreviewLayerKind::MapAreas }`. That single
  edit both makes the layer enabled on the first frame and gives the previously-inert toggle its real
  target — the SCOPE NOTE's own stated resolution path ("making one of them paint needs that stage
  plus its layer kind"), now satisfied for Areas. Update that note's "six rows" count to five and
  drop Areas from its list.
- **Nothing baked yet:** `BuildLayerConfigurations` early-returns with `layerCount = 0` when
  `!mapFields.IsSized()` (`PreviewComposite_Prepare_UI.cpp:48`), so areas do not paint before the
  first bake. Correct and unchanged — there is no image to paint onto.

**11. Drag-performance rule — exactly two recomposites per gesture (amends §21.8).** A GPU
recomposite per drag frame would violate the Tier B cost model (`ARCH_14_08_DirtyFlagTiers.md`) for
an interaction §21.8 deliberately placed in the cheap tier. Ruled:
- **New transient field** `int mapAreaSuppressedIndex = -1;` on `PreviewCompositeSettings`.
  Presentation state, never serialized; documented as transient interaction state.
- **`BuildMapAreaConfigurations()` skips the suppressed index**, emitting no rectangle for it. An
  out-of-range value suppresses nothing (`index == suppressed` is inherently safe), which is the
  correct degradation if a list reorder ever races a gesture.
- **`TryBeginAreaDrag`** — on a successful begin (either branch) sets it to the dragged area's index
  and requests ONE recomposite. **`ContinueAreaDrag`** — unchanged from §21.8: `recipe.areas` is
  written live every frame (§21.8 correction 4), and **zero** recomposites are requested; the live
  visual is the existing bespoke immediate-mode pass in `MapCanvas_AreaDraw_UI.cpp`, which draws that
  one area's fill during the gesture. **`EndAreaDrag`** — resets to `-1` and requests one more.
  Net: 2 per drag/resize gesture, not one per frame. **`CreateAreaFromDrag`** requests one (a new
  area must appear), with no suppression change.
- **An index, not a `bEnabled` toggle.** Toggling `fieldLayers[i].bEnabled` for the duration of a
  drag would clobber the user's own View-popup/left-column enable state and desynchronize
  `ApplicationVisibilityState` from the composite. The suppressed index is a separate, single-purpose
  slot that cannot collide with a user setting.
- **Plumbing, ruled.** `ManualAreaDragSources_UI` gains `int* mapAreaSuppressedIndex = nullptr;`
  (mutable, null-safe) and `SetManualAreaDragSource` gains a matching fifth parameter, wired at
  `Application_UI.cpp:146-147` to `&composite.Settings().mapAreaSuppressedIndex` — a stable address on
  a member of a member, the same posture `&recipe.areas` already uses. `MapCanvas` cannot reach it
  through its existing `const PreviewComposite* composite` (`MapCanvas_UI.h:339-340`, deliberately
  const — the canvas never composites), and that constness is preserved, not relaxed. The recomposite
  request is a new `std::function<void()> areaCompositeRefreshCallback` with setter
  `SetAreaCompositeRefreshCallback`, mirroring `SetSelectionChangedCallback`'s existing injection
  shape verbatim (`MapCanvas_UI.h:86-90,347-348`) and bound in `Application::WireCallbacks()` to a
  lambda calling `previewDriver.NotifyParametersChanged()` — the identical derive-the-tier call the
  left column's visibility toggle already makes for a presentation-only edit
  (`Application_LeftColumn_UI.cpp:57-60`). Unset callback = no refresh, never a crash. One small
  private helper, `void MapCanvas::SetMapAreaSuppression(int areaIndex)`, writes the pointer
  null-safely and fires the callback only when the value actually changed, so the three call sites
  above never hold two copies of that condition.

**12. Border rule (amends §21.8's "fill+border every area every frame").** With the fill now the
composite's job in the steady state, the immediate-mode border is edit-time-only feedback. Ruled:
`MapCanvas_AreaDraw_UI.cpp` draws an area's border **only when all three hold** — (a) the MapAreas
field layer is enabled (`bEnabled` on the `PreviewFieldLayer` of kind `MapAreas` in
`PreviewCompositeSettings::fieldLayers`), AND (b) that area is currently the
`mapAreaSuppressedIndex` (its fill is not in the composite this frame), AND (c) it is the selected
area. **If the MapAreas layer is disabled entirely, the border never draws, regardless of
selection** — a disabled layer means "do not show me areas," and a border is showing an area. Note
(b) and (c) coincide in practice today (only the dragged area is suppressed, and a drag always
selects), but both are stated because they answer different questions and a future gesture could
separate them. The 8 handles and the cursor-shape feedback keep their existing §21.8 rules
(selected-area-only, hover-only) unchanged. The immediate-mode **fill** likewise draws only for the
suppressed area, for the same reason — otherwise a drag would paint the composite's fill and the
canvas's fill on top of each other for every non-dragged area.
`MapCanvas` reads (a) through a new read-only injected pointer or through its existing
`const PreviewComposite*` (`composite->Settings()` has a `const` overload,
`PreviewComposite_UI.h:50`) — the const path is preferred, since it needs no new plumbing at all.

**13. `Params::MapArea` and the `.sanmap` schema are completely unchanged.** No new field, no new
wire key, no `SanGenVersion` bump, no importer/exporter change. Everything this ruling adds is
presentation state in the same category as `PreviewCompositeSettings` and the pre-existing
`AreaColorEntry` table — which STEP21 ruling #4 already decided has no `_PARAMS` home
(`AreasTab_List_UI.h:16-20`). That decision stands; this ruling only moves where the non-serialized
table lives inside UI, never whether it serializes.

**14. Documentation defect to fix in the same change.** `PreviewComposite_Settings_UI.h:16-22`'s
comment states "Which BAKED field a layer colorizes. Every entry names a field `Data::MapFields`
actually carries." That is **already false** for `StratumSplat` (no single field; `LayerSourceField`
answers `nullptr`) and arguably for `Water` (a PARAMS-parameterized threshold over the heightfield),
and this ruling makes it plainly false. Corrected wording must say what item 1 rules: a layer names
a per-pixel color source — a baked field, a PARAMS-flattened analytic source, or a combination — and
what it may never be is a re-decision of a placement rule. The `Slope`-is-sampled-not-derived
sentence (lines 18-21) is correct and stays verbatim.

---

**Dispatchable as three independently shippable pieces** (a coder work-order may split or combine
them): **(A)** the `AreaColorTable_UI.h` extraction + the `areaColors` ownership move + the
`DrawAreasTab` parameter (pure refactor, no visual change, lands first); **(B)** the field layer
itself — enum, define, binding, record, both twins, builder, `LayerSourceField` case, catalogue row,
defaults, comment fix; **(C)** the drag-suppression + border rules, which are inert until (B) lands.
(C) must not land before (B): without the composite fill, suppressing the dragged area's fill would
make it vanish mid-drag.
