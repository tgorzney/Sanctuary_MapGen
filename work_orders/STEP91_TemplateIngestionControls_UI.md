# STEP91 — System-tab template ingestion controls

**Layer:** UI. **Domain:** the "Ingest game templates" button and its status display, beside the
System tab's existing sanpack/cache pickers. **Sequence:** ticket 7 of 8 (85–92). **Real dependency:**
ticket 89 (`Io::IngestTemplates`, `Io::TemplateIngestReport` — the actual call this button makes) and
ticket 90 (`Application::gameInstallRoot`/`lastTemplateIngestTimestamp`/`lastTemplateIngestEntryCount`/
`bTemplateIngestEnabled` — the durable state this button reads and writes). Also STEP64
(`gameInstallRoot`, already shipped) and the real, shipped `Application_AssetPanel_UI.cpp`/
`Application_AssetBridge_UI.h`/`Application_Frame_UI.cpp` (all read in full this session — the exact
"announce this frame, perform next frame" synchronous-load pattern this ticket mirrors).
`STEP96_FootprintBakeAndStalenessCheck_IO.md` §2 explicitly depends on THIS ticket having wired a live
`Io::TemplateIngestReport` into `ApplicationAssetBridge`/`Application_UI.h` — confirmed by reading it
this session.

## Root problem
Q4 (RESOLVED by the human): ingestion runs on an explicit user action. `DESIGN_SantpFootprintIngestion_R1.md`
§4.3 names the exact real call site: "The System tab already hosts the `sanpackPath` and
`assetCacheDirectory` pickers (`Application_AssetPanel_UI.cpp:85`, `SystemTab_UI.h:28`); an 'Ingest
game templates' button beside them." Confirmed by reading `Application_AssetPanel_UI.cpp` this
session: `Application::DrawAssetPanel()` is the exact function already drawing the `"Sanpack"`
text field and the `"Load Sanpack"` button in this style — the natural, real home for this ticket's
new control (not `SystemTab_UI.h`'s own `DrawSystemTab`, a deliberately `Application`-decoupled, pure/
testable function with no access to `gameInstallRoot`/`threadPool`/`assetBridge`).

## Fix

### 1. `src/ui/Application_AssetBridge_UI.h` — new live-session fields
Confirmed by reading the real file this session: `ApplicationAssetBridge` currently ends with
`bAssetLoadRequested`/`bAssetLoadAnnounced`. Add, same posture:
```cpp
// Ticket 91 — live-session ingestion state. Empty/default until "Ingest game templates" is pressed;
// per Q4's literal resolution (STEP90's own "Design note"), NOTHING populates this automatically at
// startup — it starts empty every session until the button is pressed, even when a warm disk cache
// exists (pressing the button that same session is still required; it is just fast on a cache hit).
Io::TemplateIngestReport templateIngestReport;
bool bTemplateIngestRequested = false;
bool bTemplateIngestAnnounced = false;
```
`#include "../io/TemplateIngest_IO.h"` added to this header's existing include block.

### 2. `src/ui/Application_UI.h` — one new method declaration
Next to the existing `bool ServiceAssetLoadRequest();` (confirmed real, `Application_UI.h:121`):
```cpp
bool ServiceTemplateIngestRequest();      // Application_AssetPanel_UI.cpp
```

### 3. `src/ui/Application_AssetPanel_UI.cpp` — the button + the two-frame service function
Mirrors `ServiceAssetLoadRequest`/`DrawAssetPanel` exactly (both confirmed real, read this session):
```cpp
bool Application::ServiceTemplateIngestRequest() {
    if (!assetBridge.bTemplateIngestRequested) return false;
    if (!assetBridge.bTemplateIngestAnnounced) {
        const ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
                                ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        if (ImGui::Begin("templateIngest", nullptr,
                         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav))
            ImGui::TextUnformatted("Ingesting game templates... this may take a few seconds.");
        ImGui::End();
        assetBridge.bTemplateIngestAnnounced = true;
        return true;                 // present this frame, then perform on the next
    }
    assetBridge.bTemplateIngestRequested = false;
    assetBridge.bTemplateIngestAnnounced = false;
    assetBridge.templateIngestReport =
        Io::IngestTemplates(gameInstallRoot, tabState.system.assetCacheDirectory, &threadPool);
    lastTemplateIngestTimestamp  = CurrentUtcTimestampIso8601();   // small new local helper — see note
    lastTemplateIngestEntryCount = assetBridge.templateIngestReport.ingestedFootprintRecordCount;
    return false;
}

void Application::DrawAssetPanel() {
    ImGui::TextUnformatted("ASSET ATLAS");
    ImGui::InputText("Sanpack", assetBridge.sanpackPath, IM_ARRAYSIZE(assetBridge.sanpackPath));
    if (ImGui::Button("Load Sanpack")) assetBridge.bAssetLoadRequested = true;
    ImGui::SameLine();
    ImGui::Text("Icons resident: %d", assetBridge.iconManifest.EntryCount());
    ImGui::TextWrapped("%s", assetBridge.assetStatusMessage.c_str());

    ImGui::Separator();
    ImGui::TextUnformatted("GAME TEMPLATES");
    if (!bTemplateIngestEnabled) {
        ImGui::TextWrapped("Template ingestion is disabled (System tab opt-out).");
    } else if (gameInstallRoot.empty()) {
        ImGui::TextWrapped("No game install configured — set one to enable template ingestion.");
    } else {
        if (ImGui::Button("Ingest game templates")) assetBridge.bTemplateIngestRequested = true;
        ImGui::SameLine();
        if (lastTemplateIngestTimestamp.empty()) ImGui::Text("Never ingested.");
        else ImGui::Text("Last ingested: %s (%d templates)",
                         lastTemplateIngestTimestamp.c_str(), lastTemplateIngestEntryCount);
        if (assetBridge.templateIngestReport.totalSourceFileCount > 0)
            ImGui::TextWrapped("%s", assetBridge.templateIngestReport.SummaryText().c_str());
    }
}
```
`CurrentUtcTimestampIso8601()` — a small new free function local to this `.cpp` (or a shared header if
a future ticket also needs it; not asserted here), `std::chrono::system_clock::now()` formatted per
ISO-8601 UTC (`YYYY-MM-DDTHH:MM:SSZ`). No existing timestamp-formatting helper was found anywhere in
`src/` by this session's search — this is genuinely new, small, and scoped to display only (never
parsed back, so a simple `std::strftime`/`std::put_time` implementation is sufficient; no
timezone-arithmetic correctness burden beyond UTC).

### 4. `src/ui/Application_Frame_UI.cpp` — wire the service call
Confirmed real call site, `RunOneFrame()` (read this session):
```cpp
bool Application::RunOneFrame() {
    if (!IsWindowOpen()) return false;
    PumpWindowEvents();
    BeginImguiFrame();
    if (ServiceAssetLoadRequest()) {       // the announce frame is presented on its own
        EndImguiFrame();
        ++frameCount;
        return IsWindowOpen();
    }
    if (ServiceTemplateIngestRequest()) {  // NEW — same announce-then-perform pattern
        EndImguiFrame();
        ++frameCount;
        return IsWindowOpen();
    }
    DrawSettingsWindow();
    ApplyExecutionPolicy();
    ResolveIconSelections();
    ServiceDirtyTier();
    DrawCanvasWindow();
    EndImguiFrame();
    ++frameCount;
    return IsWindowOpen();
}
```
Sequential, not simultaneous: if both an asset load and a template ingest are requested in the same
frame, the asset load's announce-frame returns first; the ingest request is serviced on a later frame.
Never both perform their (synchronous, blocking) work in the same frame.

## Files touched
- `src/ui/Application_AssetBridge_UI.h` — three new fields, one new include.
- `src/ui/Application_UI.h` — one new method declaration.
- `src/ui/Application_AssetPanel_UI.cpp` — `ServiceTemplateIngestRequest()` (new), `DrawAssetPanel()`
  extended, `CurrentUtcTimestampIso8601()` (new, local helper).
- `src/ui/Application_Frame_UI.cpp` — `RunOneFrame()` extended with the new service call.
- NEW `src/ui/Application_AssetPanel_UI_Test.cpp`.

  **⚠️ Correction 2026-08-22 — verified, this file must be NEW, not an extension.** An earlier draft
  of this ticket said to "confirm the exact existing test file name... and extend it," implying one
  already exists. It does not: grepped all of `src/ui/*Test*` for `ServiceAssetLoadRequest`,
  `DrawAssetPanel`, `bAssetLoadRequested`, `LoadAssetAtlas` — the only hit is
  `ApplicationShell_Window_UI_Test.cpp`, which calls generic `RunOneFrame()` for open/close behavior
  only and never exercises the asset-load two-phase logic or panel contents.
  `Application_AssetPanel_UI_Test.cpp` does not exist. Create it fresh, registered in
  `CMakeLists.txt` alongside the other `Application*_UI_Test` targets.
- `CMakeLists.txt` — one new `add_sangen_test(Application_AssetPanel_UI_Test src/ui/Application_AssetPanel_UI_Test.cpp)` line.

## Backend policy
Ingestion itself is synchronous on the UI thread — **a reported limit, not a choice**, the identical
posture `Application_AssetPanel_UI.cpp`'s own top comment already states for `ServiceAssetLoadRequest`
("LOADING IS SYNCHRONOUS, AND THAT IS A REPORTED LIMIT... no async SYS seam exists"). A cold ingest
(546 files, sandboxed Lua evaluation fanned out over `threadPool`) is bounded by ticket 89's own cost
basis (sub-second to a few seconds); a warm cache hit is near-instant. No new async seam is invented
here — out of scope, same as it was for asset loading.

## ARCH rules invoked
- `DESIGN_SantpFootprintIngestion_R1.md` §4.3 — Q4's resolution and the exact real call site this
  ticket implements.
- `ARCH_18_02_IngestedDataDeterminism.md` §18.2 — this ticket's button triggers `IngestTemplates`
  (IO-layer, Visual-class) only; it never touches PROC, never triggers a PARAMS bake by itself
  (STEP96's own separate "Resolve Footprint" button does that, reading `assetBridge.templateIngestReport`
  this ticket populates — the dependency STEP96 §2 names explicitly).
- Constitution §6 — every disabled/empty state (`!bTemplateIngestEnabled`, empty `gameInstallRoot`)
  shows an actionable, specific message, never a silently-missing control.
- `ARCH_02_LayerDirectoryMap.md` — `Application` is confirmed (by this ticket's own precedent reading)
  the one legal unit touching both IO and UI/SYS at once; this ticket adds no new violation of that
  boundary.

## Explicit out-of-scope
- **Calling `Io::PopulateWorldFootprintSizeTable` or storing an `Io::WorldFootprintSizeTable`
  instance anywhere in the shell** — STEP58's own icon-LOD consumer wiring (STEP51/STEP52) remains
  unbuilt; this ticket stores the raw `TemplateIngestReport` only, which is exactly what
  `STEP96_FootprintBakeAndStalenessCheck_IO.md`'s bake button and ticket 92's bake function actually
  need (STEP96 §2's own explicit forward dependency, confirmed by reading it this session).
  Symmetric to STEP58's own precedent: "wiring this table into `Application_AssetBridge_UI.h`...is
  STEP51's or STEP52's job at their own dispatch time, not invented here."
- **An asynchronous/background ingest** — synchronous only, a reported limit (see Backend policy).
- **A game-install-root picker** — `FileDialog_IO.h`'s `SelectDirectoryPath` already exists elsewhere;
  wiring a "browse for game install" button for `gameInstallRoot` itself is not this ticket's job
  (STEP64 shipped the field with "no picker yet," still true after this ticket).
- **Any staleness warning display** — that is `STEP96_FootprintBakeAndStalenessCheck_IO.md`'s own §3.1
  UI surface, unbuilt/open-question there, not invented here.
- **`bTemplateIngestEnabled`'s own toggle checkbox** — reading the flag to hide controls is in scope;
  a widget to FLIP it is not requested here (mirrors STEP19's own "field ships before its checkbox"
  precedent for `bUseGpuMarkers`) — flagged as a small, easy follow-up, not built now.

## Acceptance test
New `src/ui/Application_AssetPanel_UI_Test.cpp` (verified 2026-08-22: no existing test covers
`DrawAssetPanel`/`ServiceAssetLoadRequest` today — see the "Files touched" correction above; this is
a new headless `Application` fixture test, not an extension):
- With `gameInstallRoot` empty: `DrawAssetPanel()` shows the "No game install configured" message,
  never draws the "Ingest game templates" button (or draws it disabled, coder's call on the exact
  ImGui idiom — the assertion is on VISIBILITY/ENABLEMENT, not literal pixels).
- With `bTemplateIngestEnabled == false`: shows the opt-out message, button absent/disabled
  regardless of `gameInstallRoot`.
- With both set: clicking "Ingest game templates" sets `assetBridge.bTemplateIngestRequested == true`;
  the FIRST following `RunOneFrame()`-equivalent call announces (returns/short-circuits, matching
  `ServiceAssetLoadRequest`'s own two-phase test precedent); the SECOND performs the ingest,
  populates `assetBridge.templateIngestReport` (non-default `totalSourceFileCount`), and sets
  `lastTemplateIngestTimestamp` (non-empty, ISO-8601-shaped) and `lastTemplateIngestEntryCount`
  (matching `templateIngestReport.ingestedFootprintRecordCount`).
- A second ingest against an unchanged install produces `bLoadedFromDiskCache == true` on the
  resulting report (proves the button reuses ticket 88's cache rather than forcing a rebuild).
- `SaveAppSettingsAtShutdown()` after an ingest persists `lastTemplateIngestTimestamp`/
  `lastTemplateIngestEntryCount` — reload via `Io::LoadAppSettings` reflects the post-ingest values
  (proves ticket 90's wiring is actually exercised end-to-end by this ticket's button, not just
  independently unit-tested).
- Full solo rebuild + `ctest -C Debug`: previously-passing suite stays green; extended tests pass.

## Verify
- Extended test(s) pass, especially the two-frame announce/perform proof and the cache-reuse proof.
- Grep `src/proc/` for any new reference introduced by this ticket — must be zero (this is a UI-only
  ticket; `ARCH_18_02` is not even reachable from here, confirmed by construction).
- Full solo rebuild + `ctest -C Debug`: 100% pass, zero unrelated test files edited or broken.
