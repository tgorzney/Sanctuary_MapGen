# STEP114 — Per-marker-instance icon override (field + picker UI + render consumer)

**Layer:** PARAMS (field), IO (wire), UI (picker wiring + shell bridge + render consumer). **Domain:**
`Params::MarkerTransform`, `MarkersTab_Manual_UI.h/.cpp`, `MarkersTab_ManualInstance_UI.cpp`,
`Application_AssetPanel_UI.cpp`, `MapCanvas_IconLayer_CullManual_UI.cpp`. **Sequence:** independent
of STEP112/STEP113 (no shared call sites). Relationship to STEP111 analyzed below — **ruling: no
hard dependency, either order.**

**Three parts, one ticket by design — do not ship part 1 alone.** A `MarkerTransform` field with no
picker UI and no render consumer is dead data a designer can never set and would never see rendered
even if a coder mis-typed it by hand into a `.sanmap`. All three parts below (§1 field/wire, §2
picker, §3 render consumer) must land together.

## STEP111 relationship — ruling

STEP111 (separate, concurrently-drafted ticket) adds `tintColorRed/Green/Blue`-shaped fields to
`OverlayVisibleInstance` (`src/ui/MapCanvas_IconLayer_UI.h:38-49`) and edits `AppendCandidate`
(`src/ui/MapCanvas_IconLayer_CullEmit_UI.cpp:47-69`), the ONE place that constructs an
`OverlayVisibleInstance`. This ticket's new manual-marker resolver (§3) calls the EXISTING
`ConsiderManualInstance`/`EmitCandidateIfVisible` path — the same one `ResolvePropsManual`/
`ResolveDecalsManual` already use today (`MapCanvas_IconLayer_CullManual_UI.cpp:84-125`) — supplying
only `templateIdentifier`/`worldX`/`worldZ`/`scale`. It never touches `OverlayVisibleInstance`'s field
list or `AppendCandidate` directly, and Props/Decals' own manual resolvers don't set any tint-RGB
field today either (only `tintAlpha`, `MapCanvas_IconLayer_CullEmit_UI.cpp:62`). **Ruling: this
ticket compiles and functions identically whether STEP111 has landed or not — implement
independently, either order, no sequencing gate.**

## Problem

`Params::MarkerTransform` (`src/params/MarkerInstance_PARAMS.h:44-53`) has no per-instance icon
field — confirmed by reading the struct in full: `name`, `transform`, `alias`, `layerIndex`,
`symmetryGroupIdentifier`, nothing icon-related. v1 had this (`Marker::IconOverride`, a string,
empty = type default) with a working click-to-pick UI (`gui/tabs/Tab_Markers.cpp`'s
`RenderIconPicker`) consumed by `gui/widgets/Widget_MapCanvas.cpp:341-374`'s icon resolution chain
(`iconName = marker.Type` default → overridden by `params.GlobalIconAlloy/Spawn/Plasma` per category
→ overridden again by `marker.IconOverride` if non-empty). v2/v3 has no equivalent.

v2/v3 already has a working click-to-pick grid widget (`DrawIconGrid`,
`src/ui/IconGridWidget_UI.h:109-110`, drawn implementation in `IconGridWidget_Draw_UI.cpp`), wired
today ONLY to the Markers tab's Global Alloy/Plasma/Spawn scale rows
(`src/ui/MarkersTab_Globals_UI.cpp:35-47`, `DrawGlobalIconPicker`) and to the procedural rule
template-id picker (`src/ui/Application_AssetPanel_UI.cpp:62-75`, `ResolveIconSelections`).

**The grid emits a volatile per-session int, never store it in PARAMS directly.**
`IconGridState::selectedIconId` (`IconGridWidget_UI.h:77-81`) is an index into whatever atlas scan
last ran — `Application_Assets_UI.cpp:57`'s `BuildIconAtlasManifest` assigns `iconEntry.iconId =
static_cast<int>(entryIndex)`, the entry's own position in that scan's `Entries()`, rebuilt fresh on
every atlas load. `DrawGlobalIconPicker` stores this raw int into `MarkerGlobalScaleRow::iconId`
(`MarkersTab_Globals_UI.h:37-43`) — but that field's own header comment (`MarkersTab_Globals_UI.h`
SCOPE NOTE 1: "NOTHING IN THIS FILE IS RECIPE CONTENT... NOT serialized") confirms it is deliberately
**not** a `.sanmap`-round-tripped field. Mirroring that exact write (`row->iconId = ...`) into a
PARAMS field would silently break on the very next atlas rescan or the next process launch.

**The real stable-string bridge is `Application::TemplateIdentifierOfIcon`/`ResolveIconSelections`,
not the Globals-row pattern's direct int write.** `Application_AssetPanel_UI.cpp:51-57`'s
`ApplyIconSelection` is the shipped precedent for turning a picker's int into a durable PARAMS value:
it fires only when the grid's `selectedIconId` is NEW (`!= lastIconId`, guards against re-writing an
unchanged pick every frame), resolves via `TemplateIdentifierOfIcon` (`Application_Assets_UI.cpp:75-79`,
returns the atlas entry's file-STEM string, e.g. `"Alloy"`), and stores that resolved string —
`ResolveIconSelections` (`Application_AssetPanel_UI.cpp:62-75`) runs this for `SelectedMarkerRule`'s
`transform.templateIdentifier` and `SelectedPropRule`'s equivalent, called once per frame,
AFTER the tabs draw (`Application_Frame_UI.cpp:49-51`: `DrawSettingsWindow(); ApplyExecutionPolicy();
ResolveIconSelections();`), so a same-frame click resolves with no lag. `Params::GlobalMarkerSettings`
(`src/params/GlobalMarkerSettings_PARAMS.h:13-23`)'s `iconNameAlloy`/`iconNamePlasma`/`iconNameSpawn`
fields are the exact `std::string` (not fixed-length) shape this new field must mirror — confirmed
they hold values longer than 8 characters in existing test fixtures
(`src/io/MapImporter_IO_Test.cpp:1120-1122`, `"AlloyIconAlt"` = 12 chars), so this field is NOT the
same fixed-8-char `templateIdentifier`/tpId convention `ApplyIconSelection`'s existing signature
truncates into (`Application_AssetPanel_UI.cpp:37-45`, `StoreTemplateIdentifier`) — a new,
untruncated bridge function is needed (§2).

**No render consumer exists for ANY manual marker icon today, override or type-default.** Manually-
placed markers render as plain colored dots (`DrawManualMarkerRoster`,
`src/ui/MapCanvas_MarkerDrag_UI.cpp:67-99`, `ManualMarkerTint`) — no icon concept at all in that path.
The only place a manual marker could enter the icon-overlay pipeline is
`ResolveManualSubLayer`'s domain switch (`src/ui/MapCanvas_IconLayer_CullManual_UI.cpp:129-152`):
```cpp
switch (layer.domainKind) {
    case OverlayDomainKind_UI::Units: ...
    case OverlayDomainKind_UI::Props:
    case OverlayDomainKind_UI::Reclaim: ...
    case OverlayDomainKind_UI::Decals: ...
    default: return;   // Alloy/SpawnsArmies carry no Manual sub-layers this sequence
}
```
`OverlayDomainKind_UI::Alloy`/`SpawnsArmies` (`src/ui/OverlayLayer_Settings_UI.h:18`) both hit
`default: return;` — confirmed zero candidates are ever emitted for manual markers. `Application_
OverlaySetup_Seed_UI.cpp:29-64`'s `SeedMarkerDomains` already pushes `{Manual, layerIndex}` refs into
both `spawnsArmiesLayer.subLayers`/`alloyLayer.subLayers` for every `recipe.markerLayers` index a
manual marker transform actually uses — the SEED side is built and dead-ends at the resolve switch.

## Fix

### 1. New field — `MarkerTransform`, `src/params/MarkerInstance_PARAMS.h:44-53`

```cpp
struct MarkerTransform {
    std::string name;
    InstancedTransform transform;
    std::string alias;
    std::string iconNameOverride;   // NEW — STEP114. Atlas-manifest NAME key, same shape as
                                     // GlobalMarkerSettings::iconName* (NOT the fixed-8-char tpId
                                     // convention MarkerRule::transform.templateIdentifier uses).
                                     // Empty = use the type default resolved from the owning
                                     // MarkerInstanceGroup::name (§3). NEVER an atlas int index —
                                     // IconGridState::selectedIconId is a volatile per-session scan
                                     // order, never stable across a rescan or a save/load
                                     // (Application_Assets_UI.cpp:57's "entry index in Entries()").
    int layerIndex = 0;
    int symmetryGroupIdentifier = 0;
};
```
Additive-only. No `SanGenVersion` bump — same posture as every prior `MarkerTransform`/
`MarkerInstanceLayer` field addition (STEP68's `symmetryGroupIdentifier`, STEP106's `bLocked`),
grounded in `SANMAP_FORMAT_SPEC.md` Correction 14's live-game-load-tolerates-new-fields test.

### 2. Wire — `MapExporter_Markers_IO.cpp` / `MapImporter_Markers_IO.cpp`

Wire key `iconNameOverride`, lowerCamelCase to match `MarkerTransform`'s OWN existing sibling keys
(`alias`, `symmetryGroupIdentifier` — NOT `MarkerGroups`' PascalCase convention, a different array).

`BuildMarkerTransformJson` (`src/io/MapExporter_Markers_IO.cpp:17-37`), after line 35's
`json["symmetryGroupIdentifier"] = markerTransform.symmetryGroupIdentifier;`:
```cpp
json["iconNameOverride"] = markerTransform.iconNameOverride;
```

`ReadMarkerTransformJson` (`src/io/MapImporter_Markers_IO.cpp:40-73`), after line 72's
`ReadJsonInteger(json, "symmetryGroupIdentifier", markerTransform.symmetryGroupIdentifier);`:
```cpp
ReadJsonText(json, "iconNameOverride", markerTransform.iconNameOverride);
```
Absent key (legacy files) keeps the struct default (empty string = type default) — no validation
needed, any string is legal (same posture as `alias`).

### 3. Picker UI — new `SelectedManualMarkerInstance` accessor + shared grid + shell bridge

**Do not reuse `DrawGlobalIconPicker`'s direct `row->iconId = ...` write shape** (§ Problem) — the
SHAPE to mirror is the "one target-selector + one shared grid below it" layout Globals already uses
(`selectedScaleRowIndex` + one `DrawIconGrid` call), not the WRITE mechanism (Globals writes straight
into non-serialized UI state; this field is PARAMS, so the write must go through the shell bridge,
same as `SelectedMarkerRule`/`ApplyIconSelection` already do for `MarkerRule::transform.
templateIdentifier`).

**Target selection is already solved — reuse it, add nothing new.** `ManualMarkersState::
selectedGroupIndex`/`selectedInstanceIndex` (`src/ui/MarkersTab_Manual_UI.h:48-49`) already form an
unambiguous two-level pointer, set by the DraggableList row `Select` signal
(`MarkersTab_Manual_UI.cpp:55-60`'s `ApplyMarkerGroupListSignal`, `MarkersTab_ManualInstance_UI.cpp:
22-25`'s `ApplyMarkerInstanceListSignal`) and ALREADY used to gate other single-target controls (the
Remove Selected button, `MarkersTab_ManualInstance_UI.cpp:56-60`). This is the exact same shape
`SelectedMarkerRule` uses (layer index then rule index) — no new "which row is the icon-pick target"
state is needed.

**a) New state — `ManualMarkersState`, `src/ui/MarkersTab_Manual_UI.h:45-69`**, next to
`layerPickerLabels`:
```cpp
IconGridState iconOverrideGridState;
float         iconOverrideGridHeight = 160.0f;   // Constitution §8 — mirrors
                                                  // MarkersTabGlobals::iconGridHeight's own posture
```

**b) New pure accessor — `MarkersTab_Manual_UI.h`**, beside `SelectedMarkerInstance` (line 84-88),
mirroring `SelectedMarkerRule`'s two-level composition exactly:
```cpp
inline Params::MarkerTransform* SelectedManualMarkerInstance(
        std::vector<Params::MarkerInstanceGroup>& markers, ManualMarkersState& state) {
    Params::MarkerInstanceGroup* const group = SelectedMarkerGroup(markers, state.selectedGroupIndex);
    if (group == nullptr) return nullptr;
    return SelectedMarkerInstance(group->transforms, state.selectedInstanceIndex);
}
```

**c) The picker draw — new function in `MarkersTab_ManualInstance_UI.cpp`**, called once at the end
of `DrawMarkerInstanceSection` (mirrors `DrawGlobalIconPicker`'s "drawn once, after the rows, gated
on the current target" placement):
```cpp
void DrawMarkerInstanceIconOverridePicker(std::vector<Params::MarkerTransform>& transforms,
                                          ManualMarkersState& state,
                                          const IconAtlasManifest* iconManifest) {
    Params::MarkerTransform* const selected =
        SelectedMarkerInstance(transforms, state.selectedInstanceIndex);
    if (selected == nullptr) return;
    ImGui::Text("Icon Override: %s", selected->iconNameOverride.empty()
                                     ? "(type default)" : selected->iconNameOverride.c_str());
    if (!selected->iconNameOverride.empty() && ImGui::Button("Clear Icon Override"))
        selected->iconNameOverride.clear();
    if (iconManifest == nullptr) {
        ImGui::TextUnformatted("No resident icon atlas: run the host's icon scan first.");
        return;
    }
    DrawIconGrid("Marker Instance Icon", *iconManifest, state.iconOverrideGridState, state.iconOverrideGridHeight);
}
```
`DrawIconGrid`'s return (a same-frame selection change) needs no local handling here — the shell
resolves `state.iconOverrideGridState.selectedIconId` into the string field (§3e); the tab file never
touches the atlas directly (ARCH §3.1/§8.4: a tab has no `TemplateIdentifierOfIcon`).

Call it from `DrawMarkerInstanceSection` (`MarkersTab_ManualInstance_UI.cpp:167-183`), after the
existing `DrawMarkerInstanceListButtons` call, before the `MakeNamesUnique` check:
```cpp
DrawMarkerInstanceIconOverridePicker(group.transforms, state, iconManifest);
```

**d) Signature threading for `iconManifest`** — `DrawMarkersTab` already receives `iconManifest`
(`MarkersTab_UI.cpp:40-41`) but does not pass it to `DrawManualMarkerLayers`/`DrawManualMarkers`
today. Thread it down:
- `DrawMarkerInstanceSection` (`MarkersTab_Manual_UI.h:149-152`, defined
  `MarkersTab_ManualInstance_UI.cpp:167-169`) gains `const IconAtlasManifest* iconManifest`.
- `DrawManualMarkers` (`MarkersTab_Manual_UI.h:158-161`, defined `MarkersTab_Manual_UI.cpp:105-108`)
  gains the same parameter, passed through to its `DrawMarkerInstanceSection` call
  (`MarkersTab_Manual_UI.cpp:117`).
- `DrawMarkersTab`'s own call (`MarkersTab_UI.cpp:56-57`) passes its already-in-scope `iconManifest`
  parameter through.

**e) Shell bridge — `Application_AssetPanel_UI.cpp`.** New helper beside `ApplyIconSelection`
(lines 51-57), untruncated (this field is not the fixed-8-char tpId convention):
```cpp
bool Application::ApplyIconSelectionToStringField(int selectedIconId, int& lastIconId,
                                                   std::string& target) {
    if (selectedIconId < 0 || selectedIconId == lastIconId) return false;
    lastIconId = selectedIconId;
    const std::string identifier = TemplateIdentifierOfIcon(selectedIconId);
    if (identifier.empty() || identifier == target) return false;
    target = identifier;
    return true;
}
```
Declare in `Application_UI.h`, beside `ApplyIconSelection` (line 149-150):
```cpp
bool ApplyIconSelectionToStringField(int selectedIconId, int& lastIconId,
                                     std::string& target);   // Application_AssetPanel_UI.cpp
```
New tracking field, `Application_TabState_UI.h:76-79`, beside `lastMarkerIconId`/`lastPropIconId`:
```cpp
int lastManualMarkerIconId = -1;
```
Extend `ResolveIconSelections()` (`Application_AssetPanel_UI.cpp:62-75`), a third bridge alongside
the existing two:
```cpp
Params::MarkerTransform* const manualMarkerInstance =
    SelectedManualMarkerInstance(recipe.markers, tabState.markers.manual);
if (manualMarkerInstance != nullptr)
    bRecipeMoved = ApplyIconSelectionToStringField(
        tabState.markers.manual.iconOverrideGridState.selectedIconId,
        tabState.lastManualMarkerIconId, manualMarkerInstance->iconNameOverride) || bRecipeMoved;
```
Placed after the existing `propRule` block, before `if (bRecipeMoved) previewDriver.
NotifyParametersChanged();` — same fold-in shape as the two existing bridges.

### 4. Render consumer — `MapCanvas_IconLayer_CullManual_UI.cpp`

**a) Type-default resolution.** No PARAMS/render consumer resolves `GlobalMarkerSettings::
iconNameAlloy/Plasma/Spawn` today (confirmed by exhaustive grep across `src/ui` and `src/proc` —
zero hits outside PARAMS/IO round-trip; this struct is currently orphaned data, IO-only). This
ticket is the FIRST consumer. Mirrors v1's exact resolution order (`Widget_MapCanvas.cpp:341-370`):
override wins if set, else map the owning group's name to the matching `GlobalMarkerSettings` field,
else fall back to the raw group name (a miss on that just logs-once-and-draws-nothing, the same
posture every other unresolved `templateIdentifier` already gets in this file).

New file-local helper, `MapCanvas_IconLayer_CullManual_UI.cpp`'s anonymous namespace:
```cpp
std::string ResolveMarkerIconTemplateIdentifier(const Params::MarkerTransform& transform,
                                                const Params::MarkerInstanceGroup& group,
                                                const Params::GlobalMarkerSettings& globalMarkerSettings) {
    if (!transform.iconNameOverride.empty()) return transform.iconNameOverride;
    if (group.name == Params::kSpawnMarkerGroupName || group.name == "Spawns")
        return globalMarkerSettings.iconNameSpawn;
    if (group.name == "Alloy" || group.name == "Alloys")
        return globalMarkerSettings.iconNameAlloy;
    if (group.name == "Plasma" || group.name == "Plasmas")
        return globalMarkerSettings.iconNamePlasma;
    return group.name;   // v1 Widget_MapCanvas.cpp:341 precedent — raw type name as last resort
}
```
`"Alloys"`/`"Spawn"`/`"Expansion"`/`"Generic"` are `markerCategoryLabels`
(`MarkersTab_Rules_UI.h:22-24`) — the "Add Marker Type" combo seeds `group.name` with exactly one of
these (`MarkersTab_Manual_UI.cpp:81-83`); `"Plasma"` is not in that combo's vocabulary today (only
`GlobalMarkerSettings` names it — ARCH_11's "a real planned resource type") but `group.name` is
free-form text (`ENTITY_AUTHORING_PARAMS_SPEC.md`'s open-set finding), so a hand-renamed/imported
group named `"Plasma"` is a legal, real input this resolver must still handle correctly.

**b) The resolve function**, replacing the dead `default: return;` for these two enum values:
```cpp
void ResolveMarkersManual(const DrawOverlayIconLayersInput& input, const OverlayLayer_UI& layer,
                          int layerIndex, int subLayerArrayIndex, int* stableOrderCounter,
                          LayerWorldAabb_UI* outAabb, const ViewWorldRect_UI* viewRect,
                          IconLayerCullDiagnostics_UI* diagnostics,
                          std::vector<OverlayVisibleInstance>& outCandidates) {
    // Alloy/SpawnsArmies partition by the owning group's name, mirroring ResolvePropsManual's
    // bReclaimable en-bloc gate (line 89 of this file) — evaluated once per GROUP, not per transform.
    const bool bWantSpawnGroups = (layer.domainKind == OverlayDomainKind_UI::SpawnsArmies);
    for (const Params::MarkerInstanceGroup& group : input.recipe->markers) {
        const bool bIsSpawnGroup = group.name == Params::kSpawnMarkerGroupName;
        if (bIsSpawnGroup != bWantSpawnGroups) continue;
        for (std::size_t index = 0; index < group.transforms.size(); ++index) {
            const Params::MarkerTransform& transform = group.transforms[index];
            // Positional match — MarkerTransform::layerIndex is NOT resolved through a stable
            // layerId indirection the way Props/Decals now are (this file's own header comment,
            // lines 9-15); markers never got that migration, and SeedMarkerDomains
            // (Application_OverlaySetup_Seed_UI.cpp:29-51) seeds subLayerArrayIndex as the SAME
            // plain recipe.markerLayers position layerIndex already uses everywhere else in the
            // marker domain (IsMarkerInstanceLayerLocked, QuantizeMarkerPositionToLayerGrid).
            if (transform.layerIndex != subLayerArrayIndex) continue;
            const std::string templateIdentifier =
                ResolveMarkerIconTemplateIdentifier(transform, group, input.recipe->globalMarkerSettings);
            ConsiderManualInstance(input, layer, layerIndex, templateIdentifier,
                                   transform.transform.positionX, transform.transform.positionZ,
                                   transform.transform.scaleX, PlacementCollectionKind_UI::Markers,
                                   static_cast<std::int32_t>(index), stableOrderCounter, outAabb,
                                   viewRect, diagnostics, outCandidates);
        }
    }
}
```

**c) Wire it into the switch** (`ResolveManualSubLayer`, lines 129-152):
```cpp
switch (layer.domainKind) {
    case OverlayDomainKind_UI::Units: ...
    case OverlayDomainKind_UI::Alloy:
    case OverlayDomainKind_UI::SpawnsArmies:
        ResolveMarkersManual(input, layer, layerIndex, subLayerArrayIndex, stableOrderCounter,
                             outAabb, viewRect, diagnostics, outCandidates);
        return;
    case OverlayDomainKind_UI::Props:
    case OverlayDomainKind_UI::Reclaim: ...
    case OverlayDomainKind_UI::Decals: ...
    default: return;
}
```

**d) Dots stay, as a fallback/secondary visual — stated explicitly, not left ambiguous.**
`DrawManualMarkerRoster`'s colored-dot rendering (`MapCanvas_MarkerDrag_UI.cpp:67-99`) is UNTOUCHED
by this ticket and continues to draw on every frame regardless of whether an icon also renders
through the overlay pipeline. **Both will draw simultaneously once this ticket lands** — the dot is
this tab's own pick-radius/drag-affordance visual (used by `HitTestManualMarkers` for the drag
gesture, a completely separate mechanism from the icon-overlay pipeline's own click-picking), not a
"is this thing visible" indicator this ticket is meant to replace. Retiring the dot in favor of the
icon alone, or suppressing one when the other is visible, is a follow-up UX decision explicitly out
of scope here — not silently resolved by this ticket either way.

### Known limitation inherited, not introduced — `instanceIndex` collision risk under
`PlacementCollectionKind_UI::Markers`

`TryResolveDomainCollection` (`MapCanvas_IconLayer_CullHelpers_UI.cpp:18-33`) maps BOTH `Alloy` and
`SpawnsArmies` to `PlacementCollectionKind_UI::Markers` — the SAME collection tag the PROCEDURAL
marker walker already uses (`ResolveProceduralSubLayer`, `MapCanvas_IconLayer_CullProcedural_UI.cpp`),
whose `instanceIndex` is a flat position into the BAKED `Data::PlacementInstances::markers` buffer.
This ticket's `ResolveMarkersManual` supplies a per-GROUP-LOCAL `index` as `instanceIndex` under that
SAME collection tag — `OverlayInstanceKey_UI{Markers, 0, true}` can now legitimately mean either "the
first procedurally-baked marker" or "the first manual marker in whichever group is being walked,"
indistinguishable to `OverlayInstanceKeysEqual` (`MapCanvas_IconLayer_UI.h:29-32`). **This exact
pattern already exists, unexercised, for Props/Units/Decals** (`ResolvePropsManual`/
`ResolveUnitsManual`/`ResolveDecalsManual` all reuse their procedural counterpart's collection tag
the same way) — `MapCanvas_IconLayer_Cache_UI.cpp`'s own header comment states "Today only markers
have a working picker (STEP48)", meaning the collision has been latent everywhere else and this
ticket is the first to make it LIVE (markers are the one domain with a real click-picker today).
**Corrected safety argument (ARCH signoff) — the real mechanism is narrower than "can a manual
marker become selected," and it IS live once this ticket ships.** `MapCanvas::ApplyClick`
(`MapCanvas_UI.cpp:34-56`) picks only against the baked `Data::SpatialGrid`/`PlacementInstances`, so
`selectedInstanceKey` itself can never resolve to a manual marker — that part of the original
argument holds. But `OverlayInstanceKeysEqual` (`MapCanvas_IconLayer_UI.h:29-32`) compares only
`{collection, instanceIndex}` with no layer/manual-vs-procedural discriminator, and
`instance.bSelected` is set for EVERY emitted candidate whose key matches, procedural or manual
(`MapCanvas_IconLayer_CullEmit_UI.cpp:66-67`). So once this ticket ships, a manual marker whose
group-local `index` happens to equal the currently-selected procedural marker's baked index
(near-certain for low indices, e.g. index 0) WILL spuriously get `bSelected = true`. Traced
consequence: `bSelected` only affects decimation priority (`MapCanvas_IconLayer_Budget_UI.cpp:19`,
"never clustered/capped") and cache-bypass routing (`SplitSelected`/`RebuildAndCache`) — there is no
distinct highlight-ring visual keyed off `bSelected` in this pass, so the defect is a minor,
non-visible decimation-priority/cache-inefficiency bug, not a "wrong marker glows selected"
regression or any data-corruption/wrong-marker-moved risk.

**Ruling (ARCH signoff): deferring is genuinely safe — do not block this ticket on it.** Severity is
low, it is architecturally inherited (the identical latent pattern already exists for
`ResolvePropsManual`/`ResolveUnitsManual`/`ResolveDecalsManual`, unexercised only because those
domains have no real picker yet), and a `PlacementCollectionKind_UI::ManualMarkers` enumerator is the
right long-term fix — it cleanly disambiguates the key space and matches this file's own
forward-looking comment ("Generic on `collection` so Props/Units/Decals need no cache rework once
they get their own pickers later," `MapCanvas_IconLayer_UI.h:21-23`). File as its own follow-up
ticket (UI Optimization Expert or ARCH Expert) — not this ticket's to fix.

## ⚠️ Sequencing note — required, added at ARCH signoff

STEP111 (a separate, concurrently-drafted ticket — icon overlay RGB tint) ALSO edits
`ConsiderManualInstance` (`src/ui/MapCanvas_IconLayer_CullManual_UI.cpp:26-40`), adding three
trailing tint params between `instanceIndex` and `stableOrderCounter`. This ticket's §4b
`ResolveMarkersManual` is written against the PRE-STEP111 (current, no-tint) 14-argument signature.
Both tickets independently claim "no shared call sites"; that is false for this one function. See
STEP111's own "Sequencing note" section for the full resolution — in short: if STEP111 lands first,
`ResolveMarkersManual`'s call needs `1.0f, 1.0f, 1.0f` (white) appended; if this ticket lands first,
STEP111's implementer updates `ResolveMarkersManual`'s call site as part of its own edit pass. Either
order is fine; just don't let whichever ticket lands second discover this via a compile error with
no explanation.

## Out of scope

- **STEP111's color-field work.** Referenced above (relationship ruling), not duplicated. This
  ticket's new `ResolveMarkersManual` sets no tint-RGB field, only `templateIdentifier`/position/scale.
- **The `instanceIndex` collision under `PlacementCollectionKind_UI::Markers`.** Documented above,
  explicitly not fixed — inherited from the existing Props/Units/Decals pattern, flagged for a
  separate ARCH/UI-Optimization ruling.
- **`DrawManualMarkerRoster`'s dot rendering.** Untouched; both dot and icon draw simultaneously once
  this ticket lands (stated explicitly above — not silently superseded).
- **Wiring `GlobalMarkerSettings` into the PROCEDURAL marker render path.** Confirmed today that no
  consumer resolves it at all (§4a) — this ticket makes it the type-default source for MANUAL markers
  only; whatever resolves procedural markers' own `templateIdentifier` at bake time is untouched and
  unexamined here (PIPELINE/PROC territory, not this ticket's domain).
- **A "Pick Icon" affordance inline per-row** (drawn inside `DrawSelectedMarkerInstance`'s own body
  for every expanded row). The picker is drawn once, gated on `state.selectedGroupIndex`/
  `selectedInstanceIndex` (the SAME target-selection the Remove button already uses) — mirroring
  Globals' one-shared-grid shape, not a per-row grid instance.
- **Any change to `MarkerGlobalScaleRow`/`DrawGlobalIconPicker`.** Untouched; that pattern is cited
  only as what NOT to structurally copy (the direct non-serialized int write), not edited.
- **`Params::MarkerCategory`.** Untouched; manual marker "type" resolution here is purely
  `MarkerInstanceGroup::name` string matching, the same open-set posture the rest of the manual
  marker domain already uses (no retyping to the closed procedural-rule enum).

## Files touched

- `src/params/MarkerInstance_PARAMS.h` — `iconNameOverride` on `MarkerTransform`
- `src/io/MapExporter_Markers_IO.cpp` — `BuildMarkerTransformJson` writes `"iconNameOverride"`
- `src/io/MapImporter_Markers_IO.cpp` — `ReadMarkerTransformJson` reads `"iconNameOverride"`
- `src/ui/MarkersTab_Manual_UI.h` — `ManualMarkersState` gains `iconOverrideGridState`,
  `iconOverrideGridHeight`; new `SelectedManualMarkerInstance` accessor; `DrawMarkerInstanceSection`/
  `DrawManualMarkers` declarations gain `const IconAtlasManifest* iconManifest`
- `src/ui/MarkersTab_Manual_UI.cpp` — `DrawManualMarkers` threads `iconManifest` through to
  `DrawMarkerInstanceSection`
- `src/ui/MarkersTab_ManualInstance_UI.cpp` — new `DrawMarkerInstanceIconOverridePicker`;
  `DrawMarkerInstanceSection` gains `iconManifest` parameter and calls the new picker
- `src/ui/MarkersTab_UI.cpp` — `DrawManualMarkers` call passes `iconManifest` through
- `src/ui/Application_UI.h` — new `ApplyIconSelectionToStringField` declaration
- `src/ui/Application_AssetPanel_UI.cpp` — new `ApplyIconSelectionToStringField` definition;
  `ResolveIconSelections` gains the third (manual-marker) bridge
- `src/ui/Application_TabState_UI.h` — new `lastManualMarkerIconId` on `ApplicationTabState`
- `src/ui/MapCanvas_IconLayer_CullManual_UI.cpp` — new `ResolveMarkerIconTemplateIdentifier`,
  `ResolveMarkersManual`; `ResolveManualSubLayer`'s switch gains the `Alloy`/`SpawnsArmies` case

## Verify

Acceptance bar: the field round-trips (including legacy-file default), a picked icon resolves to a
stable string (not a volatile int) through the SAME shell-bridge shape `MarkerRule`'s picker already
uses, and `ResolveManualSubLayer` emits a correctly-icon-resolved candidate for a manual marker —
override-set and type-default-fallback cases both covered — with new unit tests for every new pure
function.

- **New unit test — `MarkerTransform.iconNameOverride` round-trip**, extend
  `src/io/MapImporter_IO_Test.cpp`'s `FillFixtureMarkersAndChains` (line 1182+, sets
  `markerTransform.alias`/`symmetryGroupIdentifier` around lines 1203-1205 — same fixture STEP68's
  own coverage extended) with `markerTransform.iconNameOverride = "CustomAlloyIcon";`; extend
  `CheckMarkersAndChains` (line 646+, checks `alias`/`symmetryGroupIdentifier` around lines 679-683)
  with `Check(loadedMarker.iconNameOverride == originalMarker.iconNameOverride, "the marker's
  iconNameOverride survives, sibling of alias/symmetryGroupIdentifier");`.
- **New unit test — legacy default**: hand-construct a `markers` transform JSON object with no
  `"iconNameOverride"` key present, call `ReadMarkerTransformJson` directly, assert
  `markerTransform.iconNameOverride.empty()` (struct default, untouched).
- **New unit test — `ApplyIconSelectionToStringField`** (new small pure-logic test, alongside
  existing `ApplyIconSelection` coverage — check `ApplicationShell_IconBridge_UI_Test.cpp` for the
  existing test shape and add beside it): a fresh `selectedIconId` (`!= lastIconId`, `>= 0`) with a
  resolvable `TemplateIdentifierOfIcon` result writes the string and returns true; a repeated
  `selectedIconId` (`== lastIconId`) returns false and leaves `target` untouched; a negative
  `selectedIconId` returns false; an unresolvable id (`TemplateIdentifierOfIcon` returns empty)
  returns false and leaves `target` untouched, mirroring `ApplyIconSelection`'s own existing
  no-op-on-miss test case.
- **New unit test — `SelectedManualMarkerInstance`**: an empty `markers` vector resolves to
  `nullptr`; a valid `selectedGroupIndex`/`selectedInstanceIndex` pair resolves to the exact
  transform's address; an out-of-range `selectedGroupIndex` resolves to `nullptr` without touching
  `selectedInstanceIndex`'s own bounds; a valid `selectedGroupIndex` with an out-of-range
  `selectedInstanceIndex` resolves to `nullptr` (mirrors `SelectedMarkerRule`'s existing two-index
  test shape, `MarkersTab_RuleLayers_UI_Test.cpp:103-116`).
- **New unit test — `ResolveMarkerIconTemplateIdentifier`**: a non-empty `transform.iconNameOverride`
  always wins regardless of `group.name`; an empty override with `group.name == "Spawn"` resolves to
  `globalMarkerSettings.iconNameSpawn`; `group.name == "Alloys"` resolves to `iconNameAlloy`;
  `group.name == "Plasma"` resolves to `iconNamePlasma`; an unrecognized `group.name` (e.g.
  `"Expansion"` or `"Generic"`) resolves to `group.name` itself, verbatim.
- **New unit test — `ResolveMarkersManual` end to end**, added to
  `src/ui/MapCanvas_IconLayer_Cull_UI_Test.cpp` (part of the existing `MapCanvas_IconLayer_UI_Test`
  binary, `CMakeLists.txt:567-575` — no new test binary needed), using the shared
  `IconLayerTestFixture`/`SeedAtlasEntry` fixture (`MapCanvas_IconLayer_TestFixture_UI.h`), mirroring
  `ReclaimManualFixture`'s exact shape (`MapCanvas_IconLayer_Cull_UI_Test.cpp:88-126`) but built over
  `fixture.recipe.markers`/`fixture.recipe.markerLayers`/`fixture.recipe.globalMarkerSettings`
  instead of `recipe.props`/`recipe.propLayers`:
  - A one-`MarkerInstanceLayer`, two-group (`"Alloys"`, `Params::kSpawnMarkerGroupName`) fixture, each
    group with one in-view (`positionX=positionZ=2.0f`, matching the fixture's own "in-view" world
    coordinate convention) transform on `layerIndex = 0`; seed the atlas for `globalMarkerSettings.
    iconNameAlloy`'s default `"Alloy"` value. Resolving an `Alloy`-domain layer with `{Manual, 0}`
    yields exactly 1 candidate (the Alloys-group transform); resolving a `SpawnsArmies`-domain layer
    with the same `{Manual, 0}` yields exactly 1 candidate (the Spawn-group transform) — proves the
    group-name partition, mirroring `CheckManualReclaimPartitionCorrectness`'s union/intersection
    shape (lines 130-144).
  - A transform with a non-empty `iconNameOverride` set to a SEPARATE seeded atlas entry: assert the
    emitted candidate's `atlasPage`/`uvMinimum*` match the OVERRIDE entry, not the type-default
    `"Alloy"` entry — proves override-wins-over-type-default end to end through the real resolve path
    (not just the isolated `ResolveMarkerIconTemplateIdentifier` unit test above).
  - A transform on `layerIndex = 1` against a `{Manual, 0}` sub-layer ref: assert zero candidates —
    proves the positional `layerIndex != subLayerArrayIndex` filter.
- **Existing suites stay green**: `MapImporter_IO_Test`, `MapExporter_IO_Test` (every non-
  `iconNameOverride` fixture/check byte-identical); `MapCanvas_IconLayer_UI_Test`,
  `ApplicationShell_IconBridge_UI_Test`, `MarkersTab_Manual_UI_Test`, `MarkersTab_UI_Test` (no
  existing assertion this ticket doesn't itself add is touched).

---

**Relevant paths** (verified against the live tree this session):
`D:\Projects\Sanctuary\Map Generator\src\params\MarkerInstance_PARAMS.h`,
`D:\Projects\Sanctuary\Map Generator\src\io\MapExporter_Markers_IO.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\io\MapImporter_Markers_IO.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkersTab_Manual_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkersTab_Manual_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkersTab_ManualInstance_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkersTab_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkersTab_Globals_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkersTab_Globals_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\Application_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\Application_AssetPanel_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\Application_Assets_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\Application_TabState_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\Application_OverlaySetup_Seed_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_IconLayer_CullManual_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_IconLayer_CullEmit_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_IconLayer_CullHelpers_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_IconLayer_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_MarkerDrag_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\IconGridWidget_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\IconAtlasPairing_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\params\GlobalMarkerSettings_PARAMS.h`,
`D:\Projects\Sanctuary\Map Generator\src\io\MapImporter_IO_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_IconLayer_Cull_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_IconLayer_TestFixture_UI.h`,
`D:\Projects\Sanctuary\Map Generator\CMakeLists.txt`,
`D:\Projects\Sanctuary\Map Generator\gui\widgets\Widget_MapCanvas.cpp` (v1 reference only, read-only).
