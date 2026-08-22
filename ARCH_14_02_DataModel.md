[← ARCH index](ARCH.md) · [§14 ARCH_14_PreviewOverlayLayering](ARCH_14_PreviewOverlayLayering.md) · SanGen ARCH §14.2. **Only the ARCH Expert writes this file.**

### 14.2 Data model (binding shape)
```cpp
enum class OverlayDomainKind_UI   { Alloy, SpawnsArmies, Units, Props, Reclaim, Decals }; // open/additive
enum class OverlaySubLayerKind_UI { Manual, ProceduralRule };

struct OverlaySubLayerRef_UI { OverlaySubLayerKind_UI kind; int index; bool bEnabled = true; };

struct OverlayLayer_UI {
    std::string name;
    OverlayDomainKind_UI domainKind;
    bool bEnabled = true;
    float opacity = 1.0f;                             // layer-wide alpha multiplier, folded into
                                                        // each instance's tint alpha at draw time —
                                                        // replaces blendMode, §14.13 item 5 (closed)
    std::vector<OverlaySubLayerRef_UI> subLayers;     // any mix/count of Manual + ProceduralRule
    float thumbnailLodThresholdPixels = 5.0f;         // §14.3
    // color[4]/iconScale intentionally NOT always here — §14.5
};
std::vector<OverlayLayer_UI> overlayLayers;           // vector order = Z order, View-toolbar stack
```
A layer's drawn set is the union of every `bEnabled` sub-layer's resolved instances; one opacity
multiplier applies to the whole layer — **not** `Ui::PreviewBlendMode` (UI Expert verdict,
§14.13 item 5, closed: `Ui::PreviewBlendMode` is a two-operand GPU raster-compositing enum wired
into the GPU composite shader as integer defines — meaningless for a textured-quad icon draw
under ImGui's one global blend equation, and a per-layer blend-equation switch would break the
bulk-batched-vertex-write model §14.9 mandates). Reorder/add/remove never touches a fixed enum or
switch statement — this indirection is the entire point of the sub-layer shape.

Sub-layer → data mapping (binding; not to be re-derived per domain in a work-order):

| Domain | Manual sub-layers | Procedural sub-layers |
| --- | --- | --- |
| Props | `recipe.propLayers[i]` (`PropInstanceLayer`) | `recipe.propRules[i]` |
| Decals | `recipe.decalLayers[i]` (`DecalInstanceLayer`) | `recipe.decalRules[i]` |
| Units | one sub-layer per top-level `Army.groups[name]` — **flat**, §14.4 | `recipe.unitRules[i]` |
| Alloy / Spawns-Armies | ⚠️ was blocked — `Params::MarkerInstanceLayer` now exists (ARCH §16); this row's data mapping updates to match §16.1's `recipe.markerLayers[i]`/`recipe.markerRuleLayers[i].rules[j]` shape, superseding the placeholder "single undifferentiated Manual bucket" text below. | `recipe.markerRules[i]`, filtered by `category` (Spawn vs. rest), §14.6 — **superseded by §16.1's `recipe.markerRuleLayers[i].rules[j]`** |
| Reclaim | n/a — no data yet | n/a — no rule type yet; slot reserved, zero cost until it ships |

Sub-layer authoring (add/remove/toggle) lives in each domain's own tab (Props/Decals/Armies/
Markers) — never the View toolbar, which only orders/blends/hides whole `OverlayLayer_UI`s.

