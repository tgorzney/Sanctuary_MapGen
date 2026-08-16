# v2 Tab Rebuild Plan — every v1 setting, mapped to v2 widgets

*v1 (`gui/`) is the reference. v2 keeps EVERY setting (nothing dropped) and re-implements it
with the v2 shared widget library. Left-column vertical tabs like v1. Rule from you: **every
variable is settable — even sim "constants."** Color pickers = swatch + picker only, no RGBA
number inputs (`NoInputs`).*

---

## Layout (keep v1 shape)
Left column = vertical tab list under three group headers, exactly like v1:
- **TERRAIN & LAYERS** · Symmetry · Heightmap · Slope · Flow · Accumulation · Stratums ·
  Detail Normal · Tint · Holes · Smoothness
- **ENVIRONMENT** · Water · Atmosphere · Markers · Armies · Props · Areas
- **SYSTEM** · Performance · Files / Save
Each left-column row keeps the v1 `[O]/[ ]` visibility toggle where it drove a preview layer.
Right pane = the active tab, split into collapsing **sections**.

## Widget library — what we have vs what to add
Have (M5): `RangeSlider` (dual min/max), `Dial`, `RtToggle`, `VirtualList`, `DraggableList`,
`GradientEditor`, `IconGrid`.
**Add (thin shared widgets, one file each):** `Checkbox`, `Combo` (dropdown), `SliderScalar`
(single int/float), `TextInput`, `FilePathPicker` (button + short-path label), `ColorSwatch`
(picker-only, no inputs), `Levels` (shadows/mid/highlights + output black/white + histogram),
`Section` (collapsing header). Every numeric slider carries an `RtToggle` (realtime vs
on-release) per the v1 pattern.

## Dirty tiers (unchanged v2 model)
`bNeedsMapUpdate` = re-run pipeline (geometry/sim). `bNeedsPreviewRender` = recomposite only.
Each control below tags which it trips.

---

# TERRAIN & LAYERS

## 1 · Symmetry  (all → MapUpdate)
- Algorithm — `Combo` {Fold, Blur, CrossFade, Cylinder3D, Torus3D, NativeHash, Superposition}
- Blur Radius — `SliderScalar` 1–50 *(only if Blur)*
- Cross-Fade Width — `SliderScalar` 0–0.5 *(only if CrossFade)*
- Superposition Blend — `Combo` {Add,Subtract,Multiply,Overlay,Max,Min} *(only if Superposition)*
- Z Scale — `SliderScalar` 0.1–10 *(Cylinder3D)*
- Major Radius — `SliderScalar` 1–20 *(Torus3D)* · Minor Radius — 0.1–5 *(Torus3D)*
- Axis mask — 5 `Checkbox` XOR bits: Point, X, Z, XY, Radial
- **Add (promote):** `SymmetryBlurRadius`, `CrossFadeWidth`, `CylinderZScale`, torus radii,
  `SymmetryDetectionTolerance`, `SnapImperfectSymmetry` (bool) — all exist in params, expose all.

## 2 · Heightmap  (all → MapUpdate)
- Seed — `SliderScalar`/int-input
- Scale Features to Map Size — `Checkbox`
- Map Size — `Combo` {256,512,1024,2048,4096}
- Terrain Max Height (units) — `SliderScalar` 1–4096
- **Add:** Terrain **Min** Height (`TerrainMinHeight`, exists, never exposed) — `SliderScalar`
- Global Gravity — `SliderScalar` 1–20
- **GeoLayer stack** — `DraggableList` of layer groups → see **§ Layer Editor** below.

## § Layer Editor  (the core — used by Heightmap GeoLayers + Detail/Tint/Holes/Smoothness/Props)
Per **GeoLayer group**: header via `DraggableList` (reorder/enable/rename), Add Layer button.
Per **NoiseLayer** (all → MapUpdate unless noted):
- Header row: enable `Checkbox` · drag-reorder · **Import RAW…** `FilePathPicker` ·
  Duplicate · Bake/Unbake toggle · Delete (`DraggableList` row actions)
- Name — `TextInput`
- Stratum Index — `SliderScalar` 0–8
- Opacity — `SliderScalar` 0–1
- **Levels…** — `Levels` widget: Shadows, Midtones (0.01–9.99), Highlights, Output Black,
  Output White + input histogram
- Height Blending: Blend Mode `Combo` {Add,Subtract,Multiply,Overlay,Max,Min} · Blend
  Sharpness 0.1–5 · Image Contrast 0–3 · Image Brightness −1..1 · Height Mask `RangeSlider` 0–1
- Symmetry: Use Global `Checkbox`; if off → 5 XOR `Checkbox` bits (Point/X/Z/XY/Radial)
- Noise (hidden if UseImage): Type `Combo` {OpenSimplex2,OpenSimplex2S,Cellular,Perlin,
  ValueCubic,Value,None} · Fractal `Combo` {None,FBm,Ridged,PingPong} · Frequency drag
  0.0001–0.5 · Octaves 1–10 · Gain 0.1–5
- Density Shaping (hidden if UseImage): Land · Plateau · Mountain · Ramp — each `SliderScalar` 0–1
- **Soil Physics** (writes `Stratums[StratumIndex]`): Presets menu {Bedrock,Rock,Clay,Dirt,
  Mud,Sand} · Hardness 0.01–1 · Friction 0.01–1 · Cohesion 0.01–1 · Capacity Mult 0.1–5 ·
  Absorption Rate 0.001–0.5
- **Hydraulic Erosion** sub-panel: Enable `Checkbox`; then Erode Beneath · Droplet Count
  1k–5M · Max Lifetime 5–200 · Evaporation 0.001–0.2 · Viscosity 0.1–10 · Base Absorption
  0.001–0.5 · Capacity Scale 0.1–10 · Use Global Gravity `Checkbox` · Gravity 0.5–20 (if not
  global) · **Precipitation:** Rain Noise `Checkbox` · Freq 0.001–0.1 · Octaves 1–8 ·
  Orographic `Checkbox` · Wind Angle 0–360
- **Deposition** sub-panel: Enable `Checkbox` · Initial Load 0.01–5 · Spawn Height `RangeSlider`
  (min/max terrain height)
- **Add (promote hidden erosion constants — you asked for this):** `BaseErosionRate`,
  `BaseDepositionRate`, `MeanderStrength`, `DivergenceThreshold`, `ThermalIterations`,
  `ThermalRate`, `InitialSedimentLoad` sign-off — all in `ErosionSettings`, currently
  hardcoded → add an "Advanced (constants)" collapsing sub-section exposing each as a slider.

## 3 · Slope  (→ PreviewRender)
- Show overlay — `Checkbox` · Slope Gradient — `GradientEditor` (0–90°, color stops = picker only)

## 4 · Flow
- Show overlay `Checkbox` (Preview) · Precipitation Rate drag 0–10 · Iterations 1–100 ·
  Flow Volume Mult 0.1–10 · Stochastic Variance 0–1 · Slope Adherence 0–1 · Flow Momentum 0–1 ·
  Use GPU `Checkbox` (all → MapUpdate) · Flow Gradient `GradientEditor` (Preview)

## 5 · Accumulation
- Show overlay `Checkbox` (Preview) · Accurate Simultaneous Accumulation `Checkbox` ·
  Spillover Threshold 0–1 (MapUpdate) · Acc Gradient `GradientEditor` (Preview)

## 6 · Stratums / Materials  (mostly → PreviewRender; soil + mask mode → MapUpdate)
- Show overlay `Checkbox` · Select Environment `.sanpack` — `FilePathPicker` · Env path label
- **Per stratum** (repeating `Section`, 9 of them):
  - Name `TextInput` · Environment `Combo` (from sanpack) · Material `Combo` (from sanpack)
  - Albedo / Normal / Composite — three `FilePathPicker`
  - Mask Mode — 3-state toggle {Disabled, ProceduralStart, StaticOverride} (MapUpdate)
  - Preview Base Color · Diffuse Remap · Far Color Remap — three `ColorSwatch` (picker only)
  - Mask Remap Min / Max — drag 0–10 · Tile Size / Far — drag 0.1–1000 · Triplanar / Far
    Triplanar 0.1–100 · Normal Scale / Far 0–5 · Normal Blend / Height Blend 0–1
  - Soil Presets menu + Hardness/Friction/Cohesion/Capacity (same as Layer Editor soil)

## 7 · Detail Normal  (Preview)
- Show overlay `Checkbox` · Detail Normal Size `Combo` {256,512,1024,2048,4096} ·
  layer stack → **§ Layer Editor** (`DetailNormalLayers`)

## 8 · Tint  (Preview) — Show overlay `Checkbox` · layer stack (`TintLayers`)
## 9 · Holes (Preview) — Show overlay `Checkbox` · layer stack (`HoleLayers`)
## 10 · Smoothness (Preview) — Show overlay `Checkbox` · layer stack (`SmoothnessLayers`)

---

# ENVIRONMENT

## Water  (all → PreviewRender)
- Water Level — `RangeSlider` (terrain min..max) · Deep Water Depth — `RangeSlider` 0–50 ·
  Water Gradient — `GradientEditor`
- Shore & Wind: Wind Speed 0–1 · Wind Direction 0–360 · Shore Waves Remap 0–1 · Shore Depth
  Offset −10..10 · Shore Depth Str 0–5 · Shore Dist Offset −5..5 · Shore Dist Str 0–5
- Wave Blueprint — `TextInput`

## Atmosphere  (lighting/tints → Preview; skybox mode/fog/wind/sun-pos → MapUpdate)
- **Sun:** Right Ascension 0–360 · Declination −90..90 · Intensity drag 0–100000 · Tint
  `ColorSwatch` · Temperature 1000–10000 · Angular Dia 0.1–5 · Volumetric Mult 0–10 ·
  Volumetric Dimer 0–1 · Sun Position drag3 · Sun Cookie Path `TextInput` · Cookie Size drag2
- **Skylight:** Intensity drag 0–100000 · Tint `ColorSwatch` · Temp 1000–10000
- **Exposure & Skybox:** Exposure 0–20 · Exp Comp −5..5 · Skybox Path `TextInput` · Rotation
  0–360 · Intensity Mode `Combo` {Exposure,Lux,Multiplier} · Skybox Exposure 0–20 · Mult 0–100 ·
  Lux drag 0–100000
- **Legacy Fog:** Atten Dist 0–10 · Base H −100..500 · Max H 0–1000 · Max Dist 0–10000 · Anisotropy 0–1
- **Background Fog:** Intensity 0–10 · Range 0–10000 · Min 0–1 · Sky Intensity 0–10 · Color
  `ColorSwatch` · Color Intensity 0–10 · Fadeout Range drag 0–500000 · Fadeout Power 0–10
- **Height Fog:** Intensity 0–10 · Range drag2 · Start · End · Power 0.01–10
- **Linear Fog:** Intensity 0–10 · Start · End · Power 0.01–10 · Cam Intensity 0–1 · Cam Start · Cam End
- **Wind (Global):** Speed 0–10 · Direction 0–360

## Markers
- Global: Enable Procedural `Checkbox` · Use GPU `Checkbox` · Browse Gamedata `FilePathPicker`
  · Scan for Icons button · debug log panels (read-only)
- Global scale rows (Alloy/Plasma/Spawn): icon `IconGrid` picker · scale `SliderScalar` 0.1–10
  · color `ColorSwatch`
- **Procedural layers** — `DraggableList`; per **rule** (repeating): Name `TextInput` · Enabled
  `Checkbox` · Type `Combo` (KnownMarkerTypes) · Base Color `ColorSwatch` · Count 1–1000 · Use
  All Positions `Checkbox` · Use Density `Checkbox` · Density 0–1 · Random Selection `Checkbox`
  · Height Bounds `RangeSlider` · Slope Bounds `RangeSlider` 0–90 · Area Radius Min 1–200 ·
  Check Max Radius `Checkbox` · Area Radius Max 1–500 · Area Height Range 0.1–10 · Clearance
  Spacing 0–500 · Map Edge Padding 0–200 · Focus Gradient `Combo` {None,Center,Edge,Torus} ·
  Gradient Radius 0.1–1000 · Strength 0.1–5 · Contrast 0.1–5 · Delete
- **Placed markers** — `DraggableList` of layers; per marker (`VirtualList`): Alias `TextInput`
  · Position drag3 (0–4096) · Icon `IconGrid` · Use Global Symmetry `Checkbox` + 5 XOR bits ·
  Type `Combo` · Spawn→Army assignment (color-button grid) · Delete

## Armies  (→ MapUpdate)
- Browse Gamedata `FilePathPicker` · Add Army button
- Per army: Team Color `ColorSwatch` · Add Units (opens unit `IconGrid` modal) · Faction
  `Combo` {UEF,Cybran,Aeon} · Starting Alloys drag 0–100000 · Starting Energy drag 0–1000000 ·
  Remove · per-group + per-unit delete buttons
- Add-Units modal: unit `IconGrid` (multi-select) · Count int-input · Cancel/Confirm

## Props  (→ Preview for colors; layer stacks → MapUpdate)
- Manual prop layers: Use Group Color `Checkbox` · Group Color `ColorSwatch` · Layer Icon
  Scale 0.1–10; per group: Color `ColorSwatch` · Icon Scale 0.1–10 · transforms (read-only list)
- Procedural Props stack + Decal Rules stack → **§ Layer Editor** (Prop/Decal mode: Blueprint
  `TextInput` · Slope Range `RangeSlider` · Height Range `RangeSlider` · Avoid Water · Near
  Cliffs · Physics Simulate · Collision Tag)

## Areas  (→ Preview)
- Add New Area button · Lock Areas `Checkbox`
- Per area (`DraggableList`): Name `TextInput` · X · Y drag · Width/Length drag 1–(2×MapSize) ·
  Color `ColorSwatch` (alpha bar) · Set to Map Size button · Remove (PlayableArea locked)

---

# SYSTEM

## Performance  (→ MapUpdate)
- Use GPU Terrain `Checkbox` · Use GPU Flow `Checkbox` · WYSIWYG Baking `Checkbox` · GPU
  Preview Iterations 1–100
- **Maps to v2 System tab / DispatchPolicy** — these are the determinism/backend toggles.

## Files / Save  (this is PARITY_BACKLOG PB-1/PB-2 — the load/save that's missing)
- **Open Sanmap File** (`.sanmap` → importer) · Import SupCom Lua · Export Sanmap Only ·
  Export All (Project+Textures) · Export Heightmap RAW · Slope PNG · Flow PNG · Stratums TGA ·
  import-debug log panel
- **Removed:** Open/Save Generator File (`.json`) — all generator info now lives in the
  `.sanmap`, so the separate settings file is gone. `MetadataExporter::Load/SaveSettings` (json)
  is not ported; the sanmap is the single source of truth.
- All need `FilePathPicker` + the IO layer (`FileDialog_IO`, `MapImporter_IO`, `MapExporter_IO`).

---

# Keep / Remove / Add summary
- **Keep:** everything above — no v1 setting is dropped.
- **Remove:** the `.json` Generator File save/open (all generator data lives in the `.sanmap`
  now — sanmap is the single source of truth). Plus dead statics (`activeArmyForUnits` etc.),
  display-only debug text, and the no-op "Import Supcom FAF Map" placeholder button.
- **Add (promote to settable, per your rule):** TerrainMinHeight; all symmetry tuning
  (tolerance, snap); the hidden `ErosionSettings` constants (BaseErosionRate, BaseDepositionRate,
  MeanderStrength, DivergenceThreshold, ThermalIterations, ThermalRate); anything else currently
  hardcoded in a stage → surfaced under an "Advanced (constants)" section in its owning tab.

# Build order (suggested)
1. Shared widgets: Checkbox, Combo, SliderScalar, TextInput, ColorSwatch, FilePathPicker,
   Levels, Section (parallel — disjoint files).
2. **§ Layer Editor** (biggest; unblocks Heightmap + Detail/Tint/Holes/Smoothness/Props).
3. Tabs, one per coder (disjoint): Symmetry, Slope, Flow, Accumulation, Stratums, Water,
   Atmosphere, Markers, Armies, Props, Areas, Performance.
4. Files/Save + IO importer/exporter (PB-1/PB-2) — sequential, shared IO.
5. Left-column tab host + preview-layer visibility toggles.
