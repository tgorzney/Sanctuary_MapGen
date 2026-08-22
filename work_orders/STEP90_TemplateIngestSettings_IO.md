# STEP90 — Template-ingest `AppSettings` fields + shell bridge

**Layer:** IO (new fields) + UI (existing shell bridge, extended). **Domain:** durable, global (not
per-map) preferences — the same family `STEP64_GameInstallLocation_IO.md` already extended.
**Sequence:** ticket 6 of 8 (85–92), **parallel with the 85→86→87→88→89 chain — no code dependency on
any of them.** **Real dependency:** the already-shipped `AppSettings`/`Application` round-trip
pattern this ticket mirrors verbatim: `src/io/AppSettings_IO.h/.cpp` (confirmed real, read this
session — currently carries `sanpackPath`/`assetCacheDirectory`/`environmentPackPath`/
`gameInstallRoot`/`scenarioRuntimeOverridePath`/four `bUseGpu*`/`bWysiwygBaking` fields) and
`src/ui/Application_AppSettings_UI.cpp`'s `LoadAppSettingsAtStartup`/`SaveAppSettingsAtShutdown`
(confirmed real, read this session — this is literally the file STEP64 itself extended, same exact
two-function shape). Ticket 91 depends on THIS ticket for the fields it displays/mutates; this ticket
does not depend on 91.

## Root problem
Q4/Q5 (RESOLVED by the human, restated here for a self-contained ticket): template ingestion runs on
an explicit user action (a button, ticket 91), cached to disk thereafter (ticket 88's cache), reusing
the existing `AppSettings::assetCacheDirectory` setting rather than a new dedicated cache path. This
ticket ships the **durable state a button needs to exist meaningfully across app restarts** — when did
ingestion last run, how many templates did it find, and a durable opt-out — following exactly the
established pattern (field ships before its UI trigger; STEP64's own precedent, quoted verbatim
below) so ticket 91 has a real settings home to read/write rather than inventing caller-owned-only
state that resets every launch.

## Design note: literal reading of Q4, not the design doc's own more elaborate §4.3 text
`DESIGN_SantpFootprintIngestion_R1.md` §4.3's own prose additionally describes an automatic, silent
cache reload at every subsequent launch ("the user does not press the button again"). The human's
actual Q4 resolution, as given to this expert, is stricter and literal: **"explicit user action (a
button), cached to disk thereafter. Not automatic at startup, not lazy-on-first-need."** This ticket
follows the human's literal resolution over the design doc's own earlier, superseded elaboration —
`Application::LoadAppSettingsAtStartup()` gains NO call into `Io::IngestTemplates` anywhere. The
practical reconciliation (documented here so ticket 91 does not have to re-derive it): the SAME
"Ingest game templates" button is pressed every session; its cost is simply very different depending
on whether ticket 88's disk cache is warm (fast, a manifest read) or cold (slow, a full walk +
sandboxed parse) — `IngestTemplates`'s own internal cache-check (ticket 89) already makes this true
for free, with no separate "silent startup path" needed. The durable fields below exist purely so the
System tab can DISPLAY "last ingested at <time>, N templates" immediately at launch, before any button
is pressed this session — an informational convenience, not a trigger.

## Fix

### 1. New fields — `src/io/AppSettings_IO.h`
Add to `struct AppSettings` (currently ends at `bUseGpuMarkers`, confirmed by reading the real file
this session), same flat-scalar style as every existing field:
```cpp
struct AppSettings {
    std::string sanpackPath;
    std::string assetCacheDirectory;
    std::string environmentPackPath;
    std::string gameInstallRoot;
    std::string scenarioRuntimeOverridePath;
    std::string lastTemplateIngestTimestamp;     // NEW — ISO-8601 UTC; empty = never ingested on
                                                  // this app installation (Constitution §6: not an
                                                  // error state, an ordinary "hasn't happened yet")
    int         lastTemplateIngestEntryCount = 0; // NEW — ingestedFootprintRecordCount from the last
                                                  // completed ingest (Io::TemplateIngestReport, STEP89)
    bool        bTemplateIngestEnabled = true;    // NEW — durable opt-out: false hides the ticket 91
                                                  // ingestion controls entirely, never deletes an
                                                  // existing disk cache
    bool bUseGpuTerrain = true;
    bool bUseGpuFlow    = true;
    bool bWysiwygBaking = false;
    bool bUseGpuMarkers = false;
};
```

### 2. Round trip — `src/io/AppSettings_IO.cpp`
Same flat, direct-1:1, total/never-throwing pattern every existing field already uses (confirmed by
reading the real `ToJson`/`FromJson` this session):
```cpp
// ToJson, after the existing scenarioRuntimeOverridePath line:
document["lastTemplateIngestTimestamp"]  = settings.lastTemplateIngestTimestamp;
document["lastTemplateIngestEntryCount"] = settings.lastTemplateIngestEntryCount;
document["bTemplateIngestEnabled"]       = settings.bTemplateIngestEnabled;

// FromJson, after the existing scenarioRuntimeOverridePath line:
ReadJsonText(document, "lastTemplateIngestTimestamp", outSettings.lastTemplateIngestTimestamp);
ReadJsonInteger(document, "lastTemplateIngestEntryCount", outSettings.lastTemplateIngestEntryCount);
ReadJsonBoolean(document, "bTemplateIngestEnabled", outSettings.bTemplateIngestEnabled);
```
A document missing any of the three keys leaves the caller's default (empty string / 0 /
`true`) — same partial-document degrade-gracefully contract every existing field already has.

### 3. Shell bridge — `src/ui/Application_UI.h` + `src/ui/Application_AppSettings_UI.cpp`
Three plain caller-owned members on `Application` (`src/ui/Application_UI.h`, next to the existing
`gameInstallRoot`/`scenarioRuntimeOverridePath` pair from STEP64), same posture — **no button, no
picker wired in this ticket**, exactly how STEP64 itself shipped `gameInstallRoot` with "no picker
yet":
```cpp
std::string lastTemplateIngestTimestamp;    // ISO-8601 UTC; empty = never ingested; ticket 91 writes
int         lastTemplateIngestEntryCount = 0;
bool        bTemplateIngestEnabled = true;   // opt-out; ticket 91 reads to hide its own controls
```
Extend `LoadAppSettingsAtStartup`/`SaveAppSettingsAtShutdown` with the mirrored three lines each —
direct copy, no translation, no validation call (same shape as STEP64's own `gameInstallRoot`/
`scenarioRuntimeOverridePath` lines, confirmed real, read this session):
```cpp
// LoadAppSettingsAtStartup, after the existing scenarioRuntimeOverridePath line:
lastTemplateIngestTimestamp  = loaded.lastTemplateIngestTimestamp;
lastTemplateIngestEntryCount = loaded.lastTemplateIngestEntryCount;
bTemplateIngestEnabled       = loaded.bTemplateIngestEnabled;

// SaveAppSettingsAtShutdown, after the existing scenarioRuntimeOverridePath line:
current.lastTemplateIngestTimestamp  = lastTemplateIngestTimestamp;
current.lastTemplateIngestEntryCount = lastTemplateIngestEntryCount;
current.bTemplateIngestEnabled       = bTemplateIngestEnabled;
```

## Files touched
- `src/io/AppSettings_IO.h` — three new fields.
- `src/io/AppSettings_IO.cpp` — `ToJson`/`FromJson` extended.
- `src/ui/Application_UI.h` — three new members.
- `src/ui/Application_AppSettings_UI.cpp` — `LoadAppSettingsAtStartup`/`SaveAppSettingsAtShutdown` extended.
- `src/io/AppSettings_IO_Test.cpp` — extend `TestRoundTripSurvivesExactly` (and the missing/corrupt-
  document tests) with the three new fields, mirroring STEP64's own extension of this exact test.
- `src/ui/ApplicationShell_AppSettings_UI_Test.cpp` — extend the seed/reload assertions with the three
  new fields, mirroring how `gameInstallRoot`/`scenarioRuntimeOverridePath` were added
  (STEP64's own precedent, confirmed real by this ticket's citations above).
- No `CMakeLists.txt` change — both extended test files are already registered.

## Backend policy
N/A — three field copies, called at most once per app launch/shutdown. No compute dispatch.

## ARCH rules invoked
- `STEP19_AppSettings_IO` / `STEP64_GameInstallLocation_IO.md` — the precedent this ticket extends
  rather than duplicates verbatim: same flat/direct-1:1 JSON shape, same total/never-throwing load,
  same "seed at startup, flush at clean shutdown" bridge, same "field ships before its UI trigger"
  posture already used twice (`bUseGpuMarkers`, then `gameInstallRoot`/`scenarioRuntimeOverridePath`).
- Constitution §6 — total, never-throwing round trip; a missing key degrades to the compiled default,
  never a thrown exception; `lastTemplateIngestTimestamp.empty()` is an ordinary state, not an error.
- Constitution §8 — total tweakability: `bTemplateIngestEnabled` is an ordinary, always-flippable
  toggle, never a one-way/provenance-gated setting.

## Explicit out-of-scope
- **Any UI control** — the button, the display of these fields, and the actual `Io::IngestTemplates`
  call that would UPDATE `lastTemplateIngestTimestamp`/`EntryCount` at runtime are all ticket 91.
  This ticket ships the durable home only, exactly STEP64's own "no picker yet" precedent.
- **A startup auto-ingest call** — explicitly rejected; see "Design note" above.
- **Deleting the disk cache when `bTemplateIngestEnabled` is toggled false** — the opt-out only hides
  ticket 91's controls; it never touches ticket 88's cache file. Reversible, no data loss either way.
- **`ValidateGameInstallRoot`/`gameInstallRoot` itself** — STEP64, already shipped, unedited here.

## Acceptance test
Extended `src/io/AppSettings_IO_Test.cpp`: `TestRoundTripSurvivesExactly` sets non-default
`lastTemplateIngestTimestamp`/`lastTemplateIngestEntryCount`/`bTemplateIngestEnabled` and asserts all
three survive `Save`→`Load` exactly. `TestMissingDirectoryDegradesToDefaults`/
`TestCorruptJsonDegradesToDefaults` assert all three land at their compiled defaults (`""`/`0`/`true`)
alongside the existing checks.

Extended `src/ui/ApplicationShell_AppSettings_UI_Test.cpp`: the seeded fixture sets all three new
fields to non-default values; assert `application.lastTemplateIngestTimestamp`/
`application.lastTemplateIngestEntryCount`/`application.bTemplateIngestEnabled` equal the seed
immediately after construction; after mutating all three and calling `SaveAppSettingsAtShutdown()`,
assert `Io::LoadAppSettings(scratchDirectory)` reflects the mutated values, not the original seed —
proves the shutdown flush, not just construction, carries them (same two-phase proof STEP64's own
extension of this test already established).

Full solo rebuild + `ctest -C Debug`: previously-passing suite stays green with zero unrelated test
files edited.

## Verify
- `src/io/AppSettings_IO_Test`, `src/ui/ApplicationShell_Layout_UI_Test` (which links
  `ApplicationShell_AppSettings_UI_Test.cpp`) stay green with the extended assertions.
- Grep `src/ui/Application_AppSettings_UI.cpp`'s `LoadAppSettingsAtStartup` for any call to
  `Io::IngestTemplates`/`Io::ValidateGameInstallRoot` — must be none (confirms "no startup auto-
  ingest" was actually honored, not just stated).
- Full solo rebuild + `ctest -C Debug`: 100% pass, zero unrelated test files edited or broken.
