# STEP121 — Markers Global section: single-line per-type rows, icon button+popup, real Icon Scale/Color/Icon binding

**Layer:** UI. **Domain:** `MarkersTab_Globals_UI.h`/`.cpp`, `MarkersTab_UI.h`/`.cpp`, `Application_AssetPanel_UI.cpp`, `Application_TabState_UI.h`, `Application_PanelEnvironment_UI.cpp`. **Sequence:** no dependency on other undone work-orders; independent of STEP122 (see Out of scope).

**Scope correction from the dispatched brief, verified against live code this session.** The brief's ground truth flagged `MarkerGlobalScaleRow::iconScale` as orphaned UI-only scratch state with a real PARAMS home at `GlobalMarkerSettings::scaleAlloy/scalePlasma/scaleSpawn`. That is confirmed correct — but the same disease is NOT limited to that one field. `MarkerGlobalScaleRow` (`MarkersTab_Globals_UI.h:37-43`) has three data fields — `iconScale`, `previewColor`, `iconId` — and **all three** are orphaned: none is read anywhere outside `MarkersTab_Globals_UI.cpp` (confirmed by grep across `src/`), and each duplicates a real, already-round-tripped, already-render-consumed `GlobalMarkerSettings` field:

| Row field (orphaned) | Real field | Already consumed at render by |
|---|---|---|
| `iconScale` | `GlobalMarkerSettings::scaleAlloy/scalePlasma/scaleSpawn` (`GlobalMarkerSettings_PARAMS.h:21-23`) | nothing yet — STEP122 |
| `previewColor` | `GlobalMarkerSettings::colorAlloy/colorPlasma/colorSpawn` (`GlobalMarkerSettings_PARAMS.h:18-20`) | `ResolveMarkerGroupTypeTintColor` (`GlobalMarkerSettings_PARAMS.h:32-40`), called live from `MapCanvas_IconLayer_CullManual_UI.cpp:149` and `MapCanvas_MarkerDrag_UI.cpp:32` |
| `iconId` | `GlobalMarkerSettings::iconNameAlloy/iconNamePlasma/iconNameSpawn` (`GlobalMarkerSettings_PARAMS.h:15-17`, strings) | `ResolveMarkerIconTemplateIdentifier` (`MapCanvas_IconLayer_CullManual_UI.cpp:209-213`), already wired into the live marker render path |

This changes the ticket's real scope: shipping a polished `ImageButton` for a field that is STILL disconnected (`iconId`) sitting next to a freshly-wired Icon Scale slider in the same row would be a worse, more misleading outcome than today's plain `Icon id: %d` text — a decorated control that looks bound and is not. Since Color and Icon-identity's RENDER READ SIDE is already live (unlike Icon Scale, which needs STEP122), wiring their UI WRITE SIDE here has an immediate, visible effect, not a cosmetic no-op. This ticket therefore binds all three: Icon Scale (per the original brief), Preview Color (a straight `DrawColorSwatch` rebind, same shape as `iconScale`'s fix), and Icon Identity (needs one small new bridge, detailed in §5 — the same shape as the three existing icon bridges in `Application::ResolveIconSelections()`).

## Problem

1. **`Icon id: %d` dead text** (`MarkersTab_Globals_UI.cpp:30`) — the picked id is never resolved to anything visual or real.
2. **The picker is a single shared, always-visible grid below all three rows** (`DrawGlobalIconPicker`, `MarkersTab_Globals_UI.cpp:35-47`), gated by a "click a row's label to make it the active target" model (`globals.selectedScaleRowIndex`, `SelectedMarkerScaleRow`, `MarkersTab_Globals_UI.h:56,62-66`) — not a per-row popup.
3. **All three of `MarkerGlobalScaleRow`'s data fields are orphaned** (table above) — every edit in this row is currently invisible to the recipe and to the renderer.
4. **`DrawMarkersTabGlobals` receives no `Params::GlobalMarkerSettings&`** (`MarkersTab_Globals_UI.h:70`, `MarkersTab_Globals_UI.cpp:51`) — there is no path to bind the real field even if the UI wanted to.
5. **No `ImageButton`/`OpenPopup`/`BeginPopup` pattern exists anywhere in this codebase for images** (confirmed: zero matches for `ImageButton` under `src/`). The closest, directly-analogous shipped precedent is `DrawColorSwatch` (`ColorSwatch_UI.cpp:40-65`): `ColorButton` → `OpenPopup("##picker")` on click → unconditional `BeginPopup("##picker")`/`EndPopup()` every frame, wrapping the real picker body. This ticket mirrors that shape exactly, swapping `ColorButton`/`ColorPicker4` for `ImageButton`/`DrawIconGrid`.
6. **Stale header comments assert a now-false premise.** `MarkersTab_Globals_UI.h`'s SCOPE NOTE 1 (lines 5-12) says these fields "have no `_PARAMS` home in the tree" and are "NOT serialized" — false; `GlobalMarkerSettings` is a real, round-tripped (`GlobalMarkerSettings_Migrate_V2_IO.cpp`), render-consumed struct. `MarkersTab_UI.h:27` ("The global section holds NO recipe content") is equally stale post-fix. Both must be corrected, not left contradicting the new code.
7. **Reverse resolution (stored template-identifier string → displayable icon) does not exist anywhere yet.** Every existing icon picker in the tree (`DrawPlacementTemplatePicker`, `PlacementRuleSections_UI.cpp:85-99`; the manual-marker icon override, `MarkersTab_ManualInstance_UI.cpp:190`) is a typed `tpId` field plus an always-visible grid with no "show what's currently picked" preview. This ticket is the first to need that reverse path; it resolves via the already-existing `IconAtlasPairingLookup::Resolve(templateIdentifier).thumbnailIconId` (`IconAtlasPairing_UI.h:41-44`), the exact inverse of `Application::TemplateIdentifierOfIcon` (`Application_Assets_UI.cpp:75-79`) — both index the same `assetBridge.iconTemplateIdentifiers` vector by the same `iconId` (confirmed `iconId == entryIndex` for both, `Application_Assets_UI.cpp:57,64` and `IconAtlasPairing_UI.h:60-66`).

## Fix

### 1. `MarkerGlobalScaleRow` / `MarkersTabGlobals` — delete orphaned data, add per-row popup state

`MarkersTab_Globals_UI.h:37-58`:
```cpp
struct MarkerGlobalScaleRow {
    RealtimeToggle iconScaleToggle{true};
    RealtimeToggle previewColorToggle{true};
    // NEW — STEP121: this row's OWN popup/highlight state, so each row's picker remembers its own
    // scroll position and highlighted cell independently. Replaces the single shared
    // MarkersTabGlobals::iconGridState + selectedScaleRowIndex "click a row to make it the active
    // target" model this ticket retires — only one popup can be open at a time regardless (imgui's
    // own popup-stack behavior), so nothing is lost by giving each row its own state, and the
    // popup can now seed its highlight from THIS row's current icon on open (§3).
    IconGridState  iconGridState;
};

struct MarkersTabGlobals {
    SectionState      section;
    ScalarSliderRange iconScaleRange{ 0.1f, 10.0f, 0.0f };
    MarkerGlobalScaleRow scaleRows[kMarkerGlobalScaleRowCount];
    float iconButtonSizePixels = 48.0f;    // NEW — Constitution §8, the row's icon-button footprint

    std::string           gamedataDirectory;      // SCOPE NOTE 1
    FilePathPickerOptions gamedataOptions;
    ColorSwatchOptions    previewColorOptions;

    float iconGridHeight     = 160.0f;     // now the POPUP's height, shared layout tunable
    bool  bIconScanRequested = false;      // SCOPE NOTE 2 — the host clears it
};
```
Delete `globals.selectedScaleRowIndex` and the standalone `globals.iconGridState` field (both retired by the per-row state above), and delete the now-unused `SelectedMarkerScaleRow` function (`MarkersTab_Globals_UI.h:62-66`) — nothing calls it once the "click a row to select it" model is gone.

Add, beside `markerGlobalScaleRowLabels` (line 33-35), a small pure resolver mapping a row index to the real `GlobalMarkerSettings` fields it edits — the direct-binding mechanism, mirroring the posture `MarkersTab_ManualLayerRowBody_UI.cpp:40` already uses for `layer.iconScale` (bind straight to the PARAMS field, no scratch intermediary):
```cpp
struct GlobalMarkerScaleRowFields {
    float*       scale    = nullptr;
    float*       color    = nullptr;   // 4 floats: colorAlloy/colorPlasma/colorSpawn
    std::string* iconName = nullptr;   // iconNameAlloy/iconNamePlasma/iconNameSpawn
};

// rowIndex -> the GlobalMarkerSettings fields that row edits (Alloy=0/Plasma=1/Spawn=2, the same
// order as markerGlobalScaleRowLabels). Out-of-range resolves to every pointer null (Constitution
// §6, mirroring IsMarkerInstanceLayerLocked's out-of-range-safe posture) — the fixed
// kMarkerGlobalScaleRowCount loop in DrawMarkersTabGlobals never passes one, but this helper does
// not trust that.
inline GlobalMarkerScaleRowFields ResolveGlobalMarkerScaleRowFields(
    Params::GlobalMarkerSettings& settings, int rowIndex) {
    switch (rowIndex) {
        case 0: return { &settings.scaleAlloy,  settings.colorAlloy,  &settings.iconNameAlloy };
        case 1: return { &settings.scalePlasma, settings.colorPlasma, &settings.iconNamePlasma };
        case 2: return { &settings.scaleSpawn,  settings.colorSpawn,  &settings.iconNameSpawn };
        default: return {};
    }
}
```
New includes: `#include "../params/GlobalMarkerSettings_PARAMS.h"` and `#include "IconAtlasPairing_UI.h"` (for `IconAtlasPairingLookup`/`kInvalidIconId`, used in §3).

Rewrite SCOPE NOTE 1 (lines 5-12): remove the "no `_PARAMS` home"/"NOT serialized" claim for the scale/color/icon-identity fields — they now bind directly to `Params::GlobalMarkerSettings`, which IS serialized (`GlobalMarkerSettings_Migrate_V2_IO.cpp`). The gamedata root and `bIconScanRequested` remain genuinely caller-owned/unserialized — narrow the note to just those two.

### 2. `DrawMarkersTabGlobals` signature — take the real recipe field and the pairing lookup

`MarkersTab_Globals_UI.h:70`:
```cpp
// `iconManifest`/`pairingLookup` are both nullable: with no resident atlas the icon column shows a
// disabled placeholder button instead of a thumbnail.
void DrawMarkersTabGlobals(MarkersTabGlobals& globals, Params::GlobalMarkerSettings& globalMarkerSettings,
                           const IconAtlasManifest* iconManifest, const IconAtlasPairingLookup* pairingLookup);
```

### 3. Row draw — one line, three columns: icon button, Item Scale, Preview Color

`MarkersTab_Globals_UI.cpp` — delete `DrawGlobalIconPicker` (lines 35-47) entirely; replace `DrawGlobalScaleRow` (lines 21-32) with two functions. Column layout mirrors STEP118's own precedent for compacting several controls onto one row (`ImGui::Columns` composition, `work_orders/STEP118_MarkerPositionCompactAndRealtimeDefaults_UI.md` Part A) and `LayerEditor_Layer_UI.cpp:92-96`'s `SetColumnWidth` fixed-first-column pattern:
```cpp
void DrawGlobalScaleRowIconButton(MarkerGlobalScaleRow& row, std::string& iconNameField,
                                  const MarkersTabGlobals& globals, const IconAtlasManifest* iconManifest,
                                  const IconAtlasPairingLookup* pairingLookup) {
    const int currentIconId = pairingLookup != nullptr
        ? pairingLookup->Resolve(iconNameField).thumbnailIconId : kInvalidIconId;
    const bool bHasIcon = iconManifest != nullptr && currentIconId >= 0
                       && currentIconId < iconManifest->EntryCount();
    const ImVec2 buttonSize(globals.iconButtonSizePixels, globals.iconButtonSizePixels);

    bool bOpenRequested = false;
    if (bHasIcon) {
        const IconAtlasEntry& entry = iconManifest->entries[static_cast<std::size_t>(currentIconId)];
        const ImTextureID texture = static_cast<ImTextureID>(iconManifest->PageTextureIdentifier(entry.atlasPage));
        bOpenRequested = ImGui::ImageButton("##icon", texture, buttonSize,
                                            ImVec2(entry.uvMinimumX, entry.uvMinimumY),
                                            ImVec2(entry.uvMaximumX, entry.uvMaximumY));
    } else {
        ImGui::BeginDisabled(iconManifest == nullptr);
        bOpenRequested = ImGui::Button("?##icon", buttonSize);
        ImGui::EndDisabled();
    }
    if (bOpenRequested) {
        // Seed the popup's highlight with THIS row's CURRENT icon (or "none"), so it opens showing
        // what is already picked rather than whatever another row last touched.
        row.iconGridState.selectedIconIndex = currentIconId;
        row.iconGridState.selectedIconId    = currentIconId;
        ImGui::OpenPopup("##iconPicker");
    }
    if (ImGui::BeginPopup("##iconPicker")) {
        if (iconManifest == nullptr)
            ImGui::TextUnformatted("No resident icon atlas: run the host's icon scan first.");
        else
            DrawIconGrid("##globalMarkerIconGrid", *iconManifest, row.iconGridState, globals.iconGridHeight);
        ImGui::EndPopup();
    }
}

void DrawGlobalScaleRow(MarkersTabGlobals& globals, int rowIndex, Params::GlobalMarkerSettings& globalMarkerSettings,
                        const IconAtlasManifest* iconManifest, const IconAtlasPairingLookup* pairingLookup) {
    const GlobalMarkerScaleRowFields fields = ResolveGlobalMarkerScaleRowFields(globalMarkerSettings, rowIndex);
    if (fields.scale == nullptr) return;   // Constitution §6 — an out-of-range row draws nothing
    MarkerGlobalScaleRow& row = globals.scaleRows[rowIndex];

    ImGui::PushID(rowIndex);
    ImGui::TextUnformatted(markerGlobalScaleRowLabels[rowIndex]);
    ImGui::Columns(3, "markerGlobalScaleRowColumns", false);
    ImGui::SetColumnWidth(0, globals.iconButtonSizePixels + ImGui::GetStyle().FramePadding.x * 2.0f);
    DrawGlobalScaleRowIconButton(row, *fields.iconName, globals, iconManifest, pairingLookup);
    ImGui::NextColumn();
    DrawSliderScalar("Item Scale", *fields.scale, globals.iconScaleRange, row.iconScaleToggle,
                     WidgetStyle(), "%.2f");
    ImGui::NextColumn();
    DrawColorSwatch("Preview Color", fields.color, globals.previewColorOptions, row.previewColorToggle);
    ImGui::Columns(1);
    ImGui::PopID();
}
```
`DrawMarkersTabGlobals` becomes:
```cpp
void DrawMarkersTabGlobals(MarkersTabGlobals& globals, Params::GlobalMarkerSettings& globalMarkerSettings,
                           const IconAtlasManifest* iconManifest, const IconAtlasPairingLookup* pairingLookup) {
    if (!DrawSectionBegin("Global", globals.section)) return;
    DrawGamedataSource(globals);
    ImGui::Separator();
    for (int rowIndex = 0; rowIndex < kMarkerGlobalScaleRowCount; ++rowIndex)
        DrawGlobalScaleRow(globals, rowIndex, globalMarkerSettings, iconManifest, pairingLookup);
    DrawSectionEnd();
}
```
`ImGui::PushID(placement.iconIndex)` inside `DrawIconGrid`'s own cells (`IconGridWidget_Draw_UI.cpp:27`) is unaffected — the grid is drawn under this row's own `PushID(rowIndex)`, so three simultaneous instances (one per row's popup, mutually exclusive by imgui's own popup-stack rules — the same "only one open at a time" property `DrawColorSwatch` already relies on across every multi-row caller in this codebase) never collide.

### 4. Thread `recipe.globalMarkerSettings` and the pairing lookup down to the tab

`MarkersTab_UI.h:140-143` and `MarkersTab_UI.cpp:40-42` — `DrawMarkersTab` gains one new nullable parameter, `pairingLookup`, inserted before `placedMarkers` (which the one production call site already supplies positionally):
```cpp
void DrawMarkersTab(Params::MapRecipe& recipe, MarkersTabState& state,
                    Pipeline::PreviewDriver* previewDriver,
                    const IconAtlasManifest* iconManifest = nullptr,
                    const IconAtlasPairingLookup* pairingLookup = nullptr,
                    const Data::PlacementInstances* placedMarkers = nullptr);
```
`IconAtlasPairingLookup` reaches this header transitively via the already-present `#include "MarkersTab_Globals_UI.h"` (§1 adds the direct include there). `MarkersTab_UI.cpp:44`:
```cpp
DrawMarkersTabGlobals(state.globals, recipe.globalMarkerSettings, iconManifest, pairingLookup);
```
Call site, `Application_PanelEnvironment_UI.cpp:36-37`:
```cpp
DrawMarkersTab(recipe, tabState.markers, &previewDriver, ActiveIconManifest(), &IconPairingLookup(),
              &assembler.Placements().markers);
```
`Application::IconPairingLookup()` already exists (`Application_UI.h:81`, returns `const IconAtlasPairingLookup&`) — no new accessor needed.

Correct `MarkersTab_UI.h`'s two stale comments: lines 14-19 ("nothing in the tree maps an icon id back to a game `tpId`") still describes `DrawPlacementTemplatePicker`'s rule/prop template pickers correctly (untouched by this ticket) but is no longer true of the Global section specifically — append a one-line note that the Global section (STEP121) is the one exception, resolved via `IconAtlasPairingLookup`. Line 27 ("The global section holds NO recipe content") becomes "The global section's Scale/Color/Icon controls edit `recipe.globalMarkerSettings` directly (STEP121); the gamedata root and the icon-scan request remain caller-owned UI state — see `MarkersTab_Globals_UI.h`."

### 5. Icon-identity write bridge — `Application::ResolveIconSelections()`

The picker only raises `IconGridState::selectedIconId` (an int); resolving it to the `templateIdentifier` string `GlobalMarkerSettings::iconNameAlloy/Plasma/Spawn` needs `Application::TemplateIdentifierOfIcon`, which only `Application` can call (`MarkersTab_Globals_UI.cpp` has no `Application` access, ARCH §3.2). Mirror the three existing bridges exactly (`Application_AssetPanel_UI.cpp:75-96`).

New tracking fields, `Application_TabState_UI.h`, beside `lastManualMarkerIconId` (line 82):
```cpp
// STEP121 — the Markers tab Global section's three per-category icon pickers, sibling of the
// three above; each bridges independently (three different GlobalMarkerSettings string fields).
int lastGlobalAlloyIconId  = -1;
int lastGlobalPlasmaIconId = -1;
int lastGlobalSpawnIconId  = -1;
```
New branches in `Application::ResolveIconSelections()` (`Application_AssetPanel_UI.cpp:75-96`), added after the existing manual-marker-instance branch (lines 89-94), before `if (bRecipeMoved) previewDriver.NotifyParametersChanged();`:
```cpp
// STEP121 — unlike Icon Scale (still unread until STEP122), this bridge's READ side is already
// live: ResolveMarkerIconTemplateIdentifier (MapCanvas_IconLayer_CullManual_UI.cpp:209-213)
// already resolves these three fields at render time, so a pick here changes the rendered marker
// icon on the very next preview redraw.
bRecipeMoved = ApplyIconSelectionToStringField(
    tabState.markers.globals.scaleRows[0].iconGridState.selectedIconId,
    tabState.lastGlobalAlloyIconId, recipe.globalMarkerSettings.iconNameAlloy) || bRecipeMoved;
bRecipeMoved = ApplyIconSelectionToStringField(
    tabState.markers.globals.scaleRows[1].iconGridState.selectedIconId,
    tabState.lastGlobalPlasmaIconId, recipe.globalMarkerSettings.iconNamePlasma) || bRecipeMoved;
bRecipeMoved = ApplyIconSelectionToStringField(
    tabState.markers.globals.scaleRows[2].iconGridState.selectedIconId,
    tabState.lastGlobalSpawnIconId, recipe.globalMarkerSettings.iconNameSpawn) || bRecipeMoved;
```
`Application::ApplyIconSelectionToStringField` (`Application_AssetPanel_UI.cpp:62-70`, declared `Application_UI.h:153`) is already generic over any `std::string&` target — no signature change needed, only three new call sites.

## File-size ceiling — documented exception (ARCH signoff, Constitution §7)
`Application_AssetPanel_UI.cpp` (172 lines), `ApplicationShell_IconBridge_UI_Test.cpp` (167 lines),
and `MarkersTab_UI.h` (146 lines, likely crossing 150 with this ticket's additions) are already over
ARCH §1.5's hard 150-line ceiling BEFORE this ticket touches them, with no prior documented exception
on record. This ticket's own additions to each are small (a few new lines apiece); making the ratchet
deliberate rather than silent — do not use this ticket as the trigger to split any of the three, that
is a separate cleanup ticket's job.

## Out of scope
- **Icon Scale's render-composition wiring** (nothing reads `scaleAlloy/scalePlasma/scaleSpawn` at render time yet) — STEP122, drafted in parallel. This ticket makes the binding real (§1/§3); STEP122 makes it visible.
- **Widening the icon atlas beyond `.dds`** — a real, separately-noted gap, untouched here.
- **`DrawPlacementTemplatePicker`'s rule/prop template pickers** (`PlacementRuleSections_UI.cpp:85-99`) and the manual-marker icon-override picker (`MarkersTab_ManualInstance_UI.cpp:190`) — both remain always-visible grids with a typed `tpId` field, NOT converted to the button+popup shape. Only the Markers tab's Global section changes shape this ticket.
- **Any change to `DrawGamedataSource`/`bIconScanRequested`** — untouched; SCOPE NOTE 2's posture stands.
- **Props/Armies tabs' own global sections**, if any exist with a similar shape — not touched; this ticket is Markers-Global-only.
- **A visual "atlas not yet scanned" hint beyond the existing "Scan for Icons" button/status text and the popup's own null-manifest message** — no new banner added.

## Files touched
- `src/ui/MarkersTab_Globals_UI.h` — `MarkerGlobalScaleRow` loses `iconScale`/`previewColor`/`iconId`, gains `iconGridState`; `MarkersTabGlobals` loses `selectedScaleRowIndex`/`iconGridState`, gains `iconButtonSizePixels`; `SelectedMarkerScaleRow` deleted; new `GlobalMarkerScaleRowFields`/`ResolveGlobalMarkerScaleRowFields`; `DrawMarkersTabGlobals` signature gains `globalMarkerSettings`/`pairingLookup`; SCOPE NOTE 1 corrected; new includes (`GlobalMarkerSettings_PARAMS.h`, `IconAtlasPairing_UI.h`)
- `src/ui/MarkersTab_Globals_UI.cpp` — `DrawGlobalIconPicker` deleted; `DrawGlobalScaleRow` rewritten (Columns layout, direct PARAMS binding); new `DrawGlobalScaleRowIconButton`; `Icon id: %d` text removed
- `src/ui/MarkersTab_UI.h` — `DrawMarkersTab` gains `pairingLookup` parameter; two stale comments corrected
- `src/ui/MarkersTab_UI.cpp` — `DrawMarkersTab` definition/body updated; `DrawMarkersTabGlobals` call passes `recipe.globalMarkerSettings`/`pairingLookup`
- `src/ui/Application_PanelEnvironment_UI.cpp` — `DrawMarkersTab` call passes `&IconPairingLookup()`
- `src/ui/Application_TabState_UI.h` — three new `lastGlobal*IconId` fields
- `src/ui/Application_AssetPanel_UI.cpp` — `ResolveIconSelections()` gains three new `ApplyIconSelectionToStringField` bridges
- `src/ui/MarkersTab_UI_Test.cpp` — `RunSelectionFenceChecks` loses its `SelectedMarkerScaleRow` assertions (lines 101-105); new `RunResolveGlobalMarkerScaleRowFieldsChecks`
- `src/ui/ApplicationShell_IconBridge_UI_Test.cpp` — new `RunGlobalMarkerIconBridgeChecks`, registered in `RunShellIconBridgeChecks()`

## Verify

- **New unit test — `ResolveGlobalMarkerScaleRowFields`**, `MarkersTab_UI_Test.cpp` (alongside the existing `RunSelectionFenceChecks`, replacing its deleted `SelectedMarkerScaleRow` assertions):
  ```cpp
  void RunResolveGlobalMarkerScaleRowFieldsChecks() {
      Params::GlobalMarkerSettings settings;
      const GlobalMarkerScaleRowFields alloy = ResolveGlobalMarkerScaleRowFields(settings, 0);
      Check(alloy.scale == &settings.scaleAlloy && alloy.color == settings.colorAlloy
            && alloy.iconName == &settings.iconNameAlloy, "row 0 resolves to the Alloy fields");
      const GlobalMarkerScaleRowFields plasma = ResolveGlobalMarkerScaleRowFields(settings, 1);
      Check(plasma.scale == &settings.scalePlasma && plasma.iconName == &settings.iconNamePlasma,
            "row 1 resolves to the Plasma fields");
      const GlobalMarkerScaleRowFields spawn = ResolveGlobalMarkerScaleRowFields(settings, 2);
      Check(spawn.scale == &settings.scaleSpawn && spawn.iconName == &settings.iconNameSpawn,
            "row 2 resolves to the Spawn fields");
      const GlobalMarkerScaleRowFields outOfRange = ResolveGlobalMarkerScaleRowFields(settings, 3);
      Check(outOfRange.scale == nullptr && outOfRange.color == nullptr && outOfRange.iconName == nullptr,
            "an out-of-range row resolves every pointer to null, not a stale/aliased one");
  }
  ```
  `RunSelectionFenceChecks` keeps its `bIconScanRequested`/`ResolvedPlacedMarkerSelection` assertions (lines 106-111) unchanged; only the three `SelectedMarkerScaleRow` lines (101-105) are removed since that function no longer exists.
- **New unit test — the three Global icon bridges**, `ApplicationShell_IconBridge_UI_Test.cpp`, mirroring `RunManualMarkerIconOverrideBridgeChecks` (lines 102-143) exactly, driven end to end through the real `LoadAssetAtlas()`/`IconPairingLookup()` path already exercised by `RunShellIconBridgeChecks()`:
  ```cpp
  void RunGlobalMarkerIconBridgeChecks(Application& application) {
      const int iconId = IconIdOfTemplate(application, "ucl3001");
      Check(iconId >= 0, "the known unit thumbnail resolved to a template identifier");
      if (iconId < 0) return;

      // Negative (default -1, no pick made yet): a no-op for all three rows.
      application.ResolveIconSelections();
      Check(application.Recipe().globalMarkerSettings.iconNameAlloy == "Alloy"
            && application.Recipe().globalMarkerSettings.iconNamePlasma == "Plasma"
            && application.Recipe().globalMarkerSettings.iconNameSpawn == "Spawn",
            "no pick made yet leaves all three GlobalMarkerSettings icon names at their defaults");

      // Fresh, resolvable pick on the Alloy row (index 0) only.
      application.TabState().markers.globals.scaleRows[0].iconGridState.selectedIconId = iconId;
      application.ResolveIconSelections();
      Check(application.Recipe().globalMarkerSettings.iconNameAlloy == "ucl3001",
            "a fresh pick on row 0 writes iconNameAlloy");
      Check(application.Recipe().globalMarkerSettings.iconNamePlasma == "Plasma"
            && application.Recipe().globalMarkerSettings.iconNameSpawn == "Spawn",
            "and leaves the other two rows' fields untouched");
      Check(application.Driver().NeedsMapUpdate(), "a real recipe edit trips a regeneration");

      // Repeated: re-drawing the same selection writes nothing.
      application.Driver().Refresh();
      application.Recipe().globalMarkerSettings.iconNameAlloy = "HandTyped";
      application.ResolveIconSelections();
      Check(application.Recipe().globalMarkerSettings.iconNameAlloy == "HandTyped",
            "a repeated (== lastGlobalAlloyIconId) selection leaves a hand-typed value untouched");

      // Plasma (index 1) and Spawn (index 2) rows bridge independently, same shape.
      application.TabState().markers.globals.scaleRows[1].iconGridState.selectedIconId = iconId;
      application.TabState().markers.globals.scaleRows[2].iconGridState.selectedIconId = iconId;
      application.ResolveIconSelections();
      Check(application.Recipe().globalMarkerSettings.iconNamePlasma == "ucl3001"
            && application.Recipe().globalMarkerSettings.iconNameSpawn == "ucl3001",
            "the Plasma and Spawn rows each bridge to their own GlobalMarkerSettings field");
  }
  ```
  Register the call inside `RunShellIconBridgeChecks()` (`ApplicationShell_IconBridge_UI_Test.cpp:147-164`) alongside the existing four.
- **Existing suites stay green**: `MarkersTab_RuleLayers_UI_Test.cpp` (shares the binary/`main()` with `MarkersTab_UI_Test.cpp`, untouched by this ticket), `RunPlanLimitChecks`/`RunRealtimeDefaultChecks` (both read fields this ticket does not remove — `globals.iconScaleRange`, `scaleRows[0].iconScaleToggle`/`previewColorToggle` — byte-identical), and every other `ApplicationShell_IconBridge_UI_Test.cpp` check (`RunManifestShapeChecks`, `RunSelectionResolutionChecks`, `RunIconPairingLookupWiringChecks`, `RunManualMarkerIconOverrideBridgeChecks`) unchanged.
- **No manual/visual acceptance claimed by this ticket** (per this project's no-agent-manual-testing posture) beyond what the tests above assert headlessly; the Coder/human confirms by eye that the row reads icon-button/Item Scale/Preview Color on one line, the popup opens on click and shows the row's own current icon highlighted, and Icon Scale's numeric effect remains invisible in the preview until STEP122 lands (expected, not a regression).

Relevant files read this session (all absolute paths under `D:\Projects\Sanctuary\Map Generator\`): `src\ui\MarkersTab_Globals_UI.h`, `src\ui\MarkersTab_Globals_UI.cpp`, `src\ui\MarkersTab_UI.h`, `src\ui\MarkersTab_UI.cpp`, `src\ui\MarkersTab_UI_Test.cpp`, `src\ui\MarkersTab_ManualLayerRowBody_UI.cpp`, `src\ui\ColorSwatch_UI.cpp`, `src\ui\ColorSwatch_UI.h`, `src\ui\IconGridWidget_UI.h`, `src\ui\IconGridWidget_Draw_UI.cpp`, `src\ui\MapCanvas_IconLayer_CullEmit_UI.cpp`, `src\ui\IconAtlasPairing_UI.h`, `src\ui\Application_AssetPanel_UI.cpp`, `src\ui\Application_Assets_UI.cpp`, `src\ui\Application_TabState_UI.h`, `src\ui\Application_UI.h`, `src\ui\Application_PanelEnvironment_UI.cpp`, `src\ui\ApplicationShell_IconBridge_UI_Test.cpp`, `src\ui\PlacementRuleSections_UI.cpp`, `src\params\GlobalMarkerSettings_PARAMS.h`, `src\params\MapRecipe_PARAMS.h`, `src\ui\SliderScalar_UI.cpp`, `src\ui\SliderScalar_Track_UI.cpp`, `src\ui\LayerEditor_Layer_UI.cpp`, `work_orders\STEP106_MarkerLayerLockAndGridSnap_PARAMS.md`, `work_orders\STEP118_MarkerPositionCompactAndRealtimeDefaults_UI.md`, `build\_deps\imgui-src\imgui.h`.
