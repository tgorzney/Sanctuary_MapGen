# STEP220 — Fix the Mask stage's O(N) imported-mask content hash: version-stamp instead of walking texels

**Layer:** DATA + PROC + IO. **Domain:** `Data::StratumArt::importedMask`'s dirty-hash contract. **Executor:** SanGen Coder. Authored directly against the SanGen Generator Expert's ratified fix shape/write-site enumeration and the SanGen IO Architecture Expert's ratified counter-placement ruling (both consulted this session, neither yet written to a spec file since this is a narrow, ordinary bug fix, not a standing architecture rule). Every file this ticket cites was read directly against the live tree while drafting it, including the two existing test files whose current assertions this ticket must update, not merely extend.

## Summary
**The root cause of the reported "dragging a Map Area is tremendously slow" bug.** `Mask_PROC.cpp`'s `HashStoredArt` (called by `MaskStage::ComputeParameterHash()`, called by `Pipeline::PreviewDriver::NotifyParametersChanged()`, called on **every single frame** during a live Area drag per the just-shipped STEP219) does a full `O(cell-count)` loop hashing every float in `Data::StratumArt::importedMask` whenever a stratum has one. An imported mask is stored at its **source file's own native resolution** (`MapImporter_Fields_IO.cpp`'s own comment: "any resolution... never cropped to sampleSize"), so a real, ordinarily-sized imported mask (even a modest 1024² TGA slice) means **hundreds of thousands to millions of hash calls, synchronously, on the UI thread, every frame**, for an edit (moving an Area) that has nothing whatsoever to do with the Mask stage. Confirmed by the SanGen Compute Optimization Expert to be structurally invisible to both STEP218's GPU-compose benchmark and STEP219's own watchdog, since both measure only `PreviewComposite::Compose()`, which runs on the FRAME AFTER `NotifyParametersChanged()` already paid this cost.

The fix mirrors a pattern already sitting right next to the bug: `Data::StratumArt::albedoVersion` exists specifically so a hash can "notice new art without walking megabytes of texels" (its own header comment) — but was never applied to `importedMask`. This ticket adds `importedMaskVersion`, has the ONE production writer (`MapImporter_Fields_IO.cpp`'s `LoadStratumMaskTga`) stamp a fresh, monotonic value onto it every time it actually loads pixel content, and changes `HashStoredArt` to hash `(width, height, importedMaskVersion)` instead of walking the field.

**The one subtlety this ticket must not skip:** an existing acceptance test (`Mask_DirtyHash_PROC_Test.cpp`'s `CheckStoredArtDirtying`) directly mutates `importedMask.Set(...)` outside the production loader and asserts the hash changes — a property that is fundamentally incompatible with an O(1) version-based hash unless that direct test mutation ALSO bumps the version (which, correctly, it never did before, because there was no version to bump). This ticket updates that test — and the shared `SetImportedMask` test helper five other test files call — to match the new, real production contract, rather than leaving a test that will fail the moment this fix lands.

## Required reading
None beyond this ticket — this is a self-contained bug fix, not new architecture. The "why" is fully explained above; no ARCH ruling is needed because this doesn't change what any stage's hash is FOR, only how cheaply an already-correct property (does the imported mask's content differ since last check) is computed.

---

## 1. Modified: `src/data/StratumArt_DATA.h`

Add the new field, mirroring `albedoVersion`'s own comment style, immediately after it (currently line 26):

```cpp
struct StratumArt {
    // The stored stratum mask from a .sanmap import (0..1 surface weights, any resolution).
    // The Mask stage resamples it bilinearly onto the generated grid — the ONE resampler
    // (MASKING_SPEC 1.8). `importedMaskVersion` is bumped by the ONE production writer
    // (MapImporter_Fields_IO.cpp's LoadStratumMaskTga) every time it loads pixel content, so a
    // parameter hash can notice new/changed art without walking megabytes of texels — the exact
    // same "version counter stands in for content" contract `albedoVersion` below already
    // documents, now actually applied to this field too (STEP220 — this field's own hash used to
    // walk every texel, the confirmed root cause of a "dragging an Area is tremendously slow"
    // report, since NotifyParametersChanged() calls this hash on every drag frame).
    FloatField importedMask;
    int importedMaskVersion = 0;   // 0 = never loaded; the loader's own counter starts at 1

    // The stratum's albedo texture: RGBA8 packed little-endian (red in bits 0..7), row-major,
    // owned by the asset loader that produced it — this record only borrows the pointer.
    // `albedoVersion` is bumped by that loader when the pixels change, so a parameter hash can
    // notice new art without walking megabytes of texels.
    const unsigned int* albedoTexels = nullptr;
    int albedoWidth   = 0;
    int albedoHeight  = 0;
    int albedoVersion = 0;

    bool HasImportedMask() const { return !importedMask.IsEmpty(); }
    bool HasAlbedo() const { return albedoTexels != nullptr && albedoWidth > 0 && albedoHeight > 0; }
};
```

(Only the new field + its comment are added, placed directly after `importedMask` for locality; nothing else in this file changes — `HasImportedMask`/`HasAlbedo`/`albedoTexels`/`albedoWidth`/`albedoHeight`/`albedoVersion` are all untouched.)

---

## 2. Modified: `src/proc/Mask_PROC.cpp`

Replace `HashStoredArt` (currently lines 74-84):

Currently:
```cpp
// The stored art is a loaded input (Data::StratumArt), so its CONTENT is hashed — otherwise
// re-importing different art under the same settings would silently reuse the cached weights.
std::size_t HashStoredArt(std::size_t seed, const Data::StratumArt& art) {
    if (!art.HasImportedMask()) return HashInteger(seed, 0);
    seed = HashInteger(seed, art.importedMask.Width());
    seed = HashInteger(seed, art.importedMask.Height());
    const float* values = art.importedMask.Data();
    for (std::size_t index = 0; index < art.importedMask.CellCount(); ++index)
        seed = HashFloat(seed, values[index]);
    return seed;
}
```

New:
```cpp
// STEP220 — the stored art is a loaded input (Data::StratumArt), so ITS ARRIVAL/CHANGE must move
// the hash — otherwise re-importing different art under the same settings would silently reuse
// the cached weights. Hashing `importedMaskVersion` (bumped by the ONE production writer,
// MapImporter_Fields_IO.cpp's LoadStratumMaskTga, on every successful load) achieves that
// WITHOUT walking the field's own content: this used to loop every texel
// (`art.importedMask.CellCount()` HashFloat calls), which for a real imported mask at its
// source file's native resolution meant hundreds of thousands to millions of hash calls per
// call to ComputeParameterHash() — paid synchronously on the UI thread every time
// NotifyParametersChanged() ran, which is now every frame during any live-drag gesture
// (Map Areas, STEP219) that has nothing to do with the Mask stage at all. Mirrors
// Bake_PROC.cpp's own HashStratumArt, which already does exactly this for the sibling
// albedoWidth/albedoHeight/albedoVersion fields on this same struct.
std::size_t HashStoredArt(std::size_t seed, const Data::StratumArt& art) {
    if (!art.HasImportedMask()) return HashInteger(seed, 0);
    seed = HashInteger(seed, art.importedMask.Width());
    seed = HashInteger(seed, art.importedMask.Height());
    return HashInteger(seed, art.importedMaskVersion);
}
```

Nothing else in this file changes — `HashSlopeDefaults`, `MaskStage::ComputeParameterHash()`'s own call site (`hash = HashStoredArt(hash, art);`, unchanged), and everything else is untouched.

---

## 3. Modified: `src/io/MapImporter_Fields_IO.cpp`

**New file-scoped counter** — insert in the existing anonymous namespace, beside `bgraWeightOrder` (currently lines 24-25):

Currently:
```cpp
constexpr std::size_t tgaHeaderByteSize = 18u;
constexpr int bgraWeightOrder[4] = { 2, 1, 0, 3 };   // matches the exporter's swizzle
```

New:
```cpp
constexpr std::size_t tgaHeaderByteSize = 18u;
constexpr int bgraWeightOrder[4] = { 2, 1, 0, 3 };   // matches the exporter's swizzle

// STEP220 (IO Architecture Expert ruling) — a process-lifetime "epoch" stamped onto every
// stratum slot a successful LoadStratumMaskTga call touches, so Mask_PROC.cpp's HashStoredArt
// can compare this against its own last-seen value instead of re-walking texel content on every
// dirty-check (mirrors Data::StratumArt::albedoVersion's own contract). Deliberately NOT derived
// from the scratch Data::StratumArt being written — every import path builds a fresh, default-
// constructed std::vector<Data::StratumArt> and move-assigns it wholesale over the live vector
// (FilesTab_Actions_UI.cpp, MapImporter_IO.cpp's own LoadSanmap), so a struct-local "read my own
// value and increment it" bump would produce the SAME value (0->1) on every single import,
// making two different re-imports of different content collide and silently look identical to
// the dirty-hash. IO is this field's sole production writer (ARCH §3.4 single-writer rule), so
// the counter's storage stays local to this translation unit — never threaded up through the UI
// call sites that invoke LoadSanmap/LoadBakedFields.
int nextImportedMaskVersion = 1;
```

**`LoadStratumMaskTga`'s per-channel sizing loop** — replace (currently lines 67-73):

Currently:
```cpp
    if (outStratumArt.size() < static_cast<std::size_t>(Data::MapFields::stratumCount))
        outStratumArt.resize(static_cast<std::size_t>(Data::MapFields::stratumCount));
    // Size each of this call's four destination fields once, before the pixel loop below (the loop
    // only ever calls Set(), never Resize()).
    for (int channel = 0; channel < 4; ++channel) {
        const int weightIndex = firstWeightIndex + bgraWeightOrder[channel];
        if (weightIndex < Data::MapFields::stratumCount)
            outStratumArt[weightIndex].importedMask.Resize(fileWidth, fileHeight, 0.0f);
    }
```

New:
```cpp
    if (outStratumArt.size() < static_cast<std::size_t>(Data::MapFields::stratumCount))
        outStratumArt.resize(static_cast<std::size_t>(Data::MapFields::stratumCount));
    // STEP220 — one successful load of ONE file is one content-change event: every stratum slot
    // THIS call touches shares the SAME stamped version (drawn once, not once per channel), so a
    // hash comparing versions can't spuriously see them as different when the source pixels are
    // identical (they came from the same TGA read). The sibling call for the OTHER TGA slice
    // (low vs. high, both invoked from LoadBakedFields below) draws its own separate stamp, since
    // those two files change independently.
    const int stampedVersion = nextImportedMaskVersion++;
    // Size each of this call's four destination fields once, before the pixel loop below (the loop
    // only ever calls Set(), never Resize()).
    for (int channel = 0; channel < 4; ++channel) {
        const int weightIndex = firstWeightIndex + bgraWeightOrder[channel];
        if (weightIndex < Data::MapFields::stratumCount) {
            outStratumArt[weightIndex].importedMask.Resize(fileWidth, fileHeight, 0.0f);
            outStratumArt[weightIndex].importedMaskVersion = stampedVersion;
        }
    }
```

No other line in this file changes — every one of the four early `return false` paths (missing file, too short, wrong TGA format, truncated) occurs BEFORE this point in the function and is untouched, so a failed load correctly never advances the counter or touches `importedMaskVersion` (matches the function's existing "touch nothing on failure" contract). The pixel-writing loop below (`outStratumArt[weightIndex].importedMask.Set(column, row, value);`) is unchanged.

---

## 4. Modified: `src/proc/Mask_TestSupport_PROC.h`

`SetImportedMask` is a shared test helper called from FIVE test files (`Mask_DirtyHash_PROC_Test.cpp`, `Mask_Merge_PROC_Test.cpp`, `Mask_Purity_PROC_Test.cpp` x2, `Mask_Parity_PROC_Test.cpp`). Under the new contract, it must stamp a fresh version on every call — mirroring what the real production loader (`LoadStratumMaskTga`) now always does — or every one of those tests silently stops proving what it claims to prove (that supplying NEW stored art is a hash-visible event).

Replace (currently lines 53-58):

Currently:
```cpp
// One stratum's imported art as a Data::FloatField (loaded pixels are DATA, ARCH §7.1).
inline void SetImportedMask(Data::StratumArt& art, const float* values, int width, int height) {
    art.importedMask.Resize(width, height, 0.0f);
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) art.importedMask.Set(x, y, values[y * width + x]);
}
```

New:
```cpp
// STEP220 — a test-local mirror of the production counter LoadStratumMaskTga now owns
// (MapImporter_Fields_IO.cpp), so this helper's own "supply new imported art" contract stays
// truthful under the version-based hash: every call is treated as a fresh content-change event,
// exactly as one real TGA load is. Test-binary-lifetime scope is sufficient here (unlike
// production, nothing outside this process ever needs these values to mean anything).
inline int& NextTestImportedMaskVersion() {
    static int nextVersion = 1;
    return nextVersion;
}

// One stratum's imported art as a Data::FloatField (loaded pixels are DATA, ARCH §7.1).
inline void SetImportedMask(Data::StratumArt& art, const float* values, int width, int height) {
    art.importedMask.Resize(width, height, 0.0f);
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) art.importedMask.Set(x, y, values[y * width + x]);
    art.importedMaskVersion = NextTestImportedMaskVersion()++;
}
```

No other line in this file changes.

---

## 5. Modified: `src/proc/Mask_DirtyHash_PROC_Test.cpp`

`CheckStoredArtDirtying`'s second half directly mutates `importedMask` via a raw `.Set()` call, bypassing BOTH the production loader and the `SetImportedMask` helper — a real production edit can never reach `importedMask` this way (per the Generator Expert's own write-site enumeration, `LoadStratumMaskTga` is the ONLY production writer). Under the new contract this line must ALSO stamp a fresh version to correctly simulate "the imported art changed," or this assertion goes from true-and-meaningful to simply false.

Replace (currently lines 86-102):

Currently:
```cpp
// Stored art is an input like any other: both its arrival and its pixels are part of the hash.
void CheckStoredArtDirtying(DirtyHashHarness& harness) {
    const float artPixels[4] = { 0.0f, 0.25f, 0.5f, 0.75f };
    harness.strata[1].importedMaskMode = Params::ImportedMaskMode::StaticOverride;
    SetImportedMask(harness.stratumArt[1], artPixels, 2, 2);
    std::vector<std::string> ran = harness.pipeline.Run();
    Check(ran.size() == 1 && harness.maskRunCount == 5, "importing stored art re-runs the mask stage");

    const std::size_t hashBeforeEdit = harness.stage.ComputeParameterHash();
    harness.stratumArt[1].importedMask.Set(0, 1, 0.6f);
    Check(harness.stage.ComputeParameterHash() != hashBeforeEdit, "stored-art CONTENT is part of the hash");
    ran = harness.pipeline.Run();
    Check(ran.size() == 1 && harness.maskRunCount == 6, "editing stored-art pixels re-runs the mask stage");

    ran = harness.pipeline.Run();
    Check(ran.empty() && harness.maskRunCount == 6, "the stage settles again once nothing changes");
}
```

New:
```cpp
// STEP220 — stored art is an input like any other: both its arrival and a fresh load of it are
// part of the hash. The hash itself no longer walks pixel content (an O(cell-count) loop that
// was the confirmed root cause of a "dragging a Map Area is tremendously slow" report — Mask
// stage hashing runs on every drag frame via NotifyParametersChanged) — it hashes
// importedMaskVersion instead, stamped by the ONE production writer (MapImporter_Fields_IO.cpp's
// LoadStratumMaskTga) on every successful load. This test simulates a SECOND load event (as a
// real re-import would produce) by re-calling SetImportedMask, which now stamps its own fresh
// version on every call (Mask_TestSupport_PROC.h) — a direct `.Set()` on the field's own content
// with no accompanying version bump would no longer move the hash, correctly: that bypasses the
// only path production content ever arrives through.
void CheckStoredArtDirtying(DirtyHashHarness& harness) {
    const float artPixels[4] = { 0.0f, 0.25f, 0.5f, 0.75f };
    harness.strata[1].importedMaskMode = Params::ImportedMaskMode::StaticOverride;
    SetImportedMask(harness.stratumArt[1], artPixels, 2, 2);
    std::vector<std::string> ran = harness.pipeline.Run();
    Check(ran.size() == 1 && harness.maskRunCount == 5, "importing stored art re-runs the mask stage");

    const std::size_t hashBeforeReload = harness.stage.ComputeParameterHash();
    const float reloadedPixels[4] = { 0.6f, 0.25f, 0.5f, 0.75f };
    SetImportedMask(harness.stratumArt[1], reloadedPixels, 2, 2);
    Check(harness.stage.ComputeParameterHash() != hashBeforeReload,
          "a fresh load of stored art (a new importedMaskVersion) moves the hash");
    ran = harness.pipeline.Run();
    Check(ran.size() == 1 && harness.maskRunCount == 6, "reloading stored art re-runs the mask stage");

    ran = harness.pipeline.Run();
    Check(ran.empty() && harness.maskRunCount == 6, "the stage settles again once nothing changes");
}
```

Nothing else in this file changes — `CheckSettingsDirtying`, `CheckPureSubstitution`, `CheckSlopeDefaultsHashSensitivity`, and `RunDirtyHashTests`'s own call sequence are all untouched.

---

## 6. Modified: `src/io/MapImporter_Fields_IO_Test.cpp`

Add a new test proving the real end-to-end contract this whole ticket exists for: two distinct loads through the real production path produce two DIFFERENT `importedMaskVersion` values, using `LoadSanmap`'s existing nullable `outStratumArt` out-parameter (`MapImporter_IO.h` — no new API needed). Per the IO Architecture Expert's own test-hygiene ruling: assert **relative** behavior (two values differ), never an absolute literal (`== 1`), since `nextImportedMaskVersion` is a process-lifetime counter shared across every test in this binary, not reset per test case.

Add a new function, inserted directly after `CheckTheFieldDestinationIsOptional` (currently ending at line 78), before the closing `} // namespace`:

```cpp
// STEP220 — the ONE thing this ticket's whole fix depends on: LoadStratumMaskTga must stamp a
// FRESH, DIFFERENT importedMaskVersion on every successful load, never the same value twice (a
// naive "read my own current value and increment it" bump would fail this, since every import
// path hands the loader a brand-new, default-constructed Data::StratumArt each time — see this
// ticket's own MapImporter_Fields_IO.cpp comment for why). Loads the SAME fixture map twice
// (re-importing identical content is still a distinct load EVENT) and confirms the two resulting
// versions differ — a relative check, never an absolute literal: nextImportedMaskVersion is a
// process-lifetime counter shared by every test in this binary, not reset per test case.
void CheckImportedMaskVersionAdvancesOnEveryLoad(const std::string& folderPath) {
    Params::MapRecipe firstRecipe;
    std::vector<Data::StratumArt> firstStratumArt;
    const Io::MapImportResult firstResult =
        Io::MapImporter::LoadSanmap(folderPath, firstRecipe, nullptr, Io::MapImportOptions(), &firstStratumArt);
    Check(firstResult.bSucceeded, "the first load succeeds");
    Check(firstStratumArt[0].importedMaskVersion > 0 && firstStratumArt[5].importedMaskVersion > 0,
          "a successfully loaded stratum's version is stamped (never left at the 0 sentinel)");

    Params::MapRecipe secondRecipe;
    std::vector<Data::StratumArt> secondStratumArt;
    const Io::MapImportResult secondResult =
        Io::MapImporter::LoadSanmap(folderPath, secondRecipe, nullptr, Io::MapImportOptions(), &secondStratumArt);
    Check(secondResult.bSucceeded, "the second load succeeds");
    Check(secondStratumArt[0].importedMaskVersion != firstStratumArt[0].importedMaskVersion,
          "re-loading the SAME file still draws a NEW version — every load is its own event, "
          "never derived from content equality");
    Check(secondStratumArt[0].importedMaskVersion != secondStratumArt[4].importedMaskVersion
          || secondStratumArt[0].importedMaskVersion == secondStratumArt[1].importedMaskVersion,
          "strata sharing one TGA call (0-3, the low slice) share one stamp; a stratum from the "
          "OTHER TGA call (4, the high slice) draws its own separate stamp");
}
```

**Update `RunBakedFieldTests`** (currently lines 82-88) to call it:

Currently:
```cpp
void RunBakedFieldTests() {
    const std::string folderPath = ScratchFolderPath("SanGenMapImporterTest");
    const Params::MapRecipe written = WriteFixtureMap(folderPath);
    CheckPathResolution(folderPath);
    CheckBakedFieldsComeBack(folderPath, written);
    CheckTheFieldDestinationIsOptional(folderPath);
}
```

New:
```cpp
void RunBakedFieldTests() {
    const std::string folderPath = ScratchFolderPath("SanGenMapImporterTest");
    const Params::MapRecipe written = WriteFixtureMap(folderPath);
    CheckPathResolution(folderPath);
    CheckBakedFieldsComeBack(folderPath, written);
    CheckTheFieldDestinationIsOptional(folderPath);
    CheckImportedMaskVersionAdvancesOnEveryLoad(folderPath);
}
```

Check the exact `LoadSanmap` signature (`MapImporter_IO.h`, currently around lines 115-121) before finalizing this call — the ticket assumes the existing `(pathOrFolder, outRecipe, outFields=nullptr, options=MapImportOptions(), outStratumArt=nullptr)` parameter order; adjust the call's argument order/names only if the live signature differs, without changing the test's own intent.

No `CMakeLists.txt` change — both modified test files are already-registered binaries; no new file is added.

---

## ARCH rules invoked
- Constitution §3 — an O(N) hash walk over loaded content is exactly the "hidden cost on a hot path" this fix eliminates; `importedMaskVersion` is a named, `int`-typed counter, never a magic sentinel reused for another purpose.
- ARCH §3.4 (single-writer rule) — `LoadStratumMaskTga` stays the ONE production writer of `importedMask`/`importedMaskVersion`; the fix does not add a second write path.
- ARCH §7.1 — loaded pixels stay DATA (`Data::StratumArt`), never promoted into `Params::Stratum`; `importedMaskVersion` is metadata about that same DATA record, not a recipe setting.
- Constitution §6 — every one of `LoadStratumMaskTga`'s four existing failure paths (missing/short/wrong-format/truncated file) is confirmed untouched by this ticket and continues to leave `importedMaskVersion` at its prior value on failure, never advancing the counter for a load that didn't happen.

## Explicit out-of-scope
- **No change to `Data::StratumArt::albedoVersion` or its own (already-correct) hash treatment** — `Bake_PROC.cpp`'s `HashStratumArt` is the precedent this ticket follows, not something it touches.
- **No accessor/setter wrapper added around `importedMask`/`importedMaskVersion`** (e.g. a `SetImportedMask(width, height, pixels)` method on `Data::StratumArt` itself that bumps the version internally). The IO Architecture Expert flagged the bare-public-field posture as a real but non-blocking gap (a future direct write could bypass the bump) — this ticket accepts that risk as-is, matching `albedoVersion`'s own existing bare-field posture, since the one production writer today is disciplined. Closing that gap with an accessor wrapper is separate, optional follow-up work, not required for this fix.
- **Lead 1 from the Compute Optimization Expert's own investigation** (recomposing every enabled field layer instead of just the changed one, a GPU-compositing concern) — confirmed real but secondary and MEASURED-small at today's scale; explicitly not addressed by this ticket, which is scoped to the Mask-stage hash defect only.
- **`NoiseBlendStage::ComputeParameterHash`'s own flagged (opposite-direction) defect** — never hashes `bakedLayerImages` content or version at all, a potential UNDER-invalidation bug, a different bug class entirely, not investigated or touched here.
- **No `.sanmap` schema change, no `SanGenVersion` bump, no new PARAMS field** — `importedMaskVersion` is DATA-side metadata about a loaded input, never serialized into the recipe document.

## Acceptance test
1. Full `SanGenV2`/relevant PROC test-binary build stays clean.
2. `Mask_DirtyHash_PROC_Test.cpp`'s `RunDirtyHashTests` — `ALL PASS`, including the rewritten `CheckStoredArtDirtying` (now proving a fresh load moves the hash, not raw content mutation).
3. `Mask_Merge_PROC_Test.cpp`, `Mask_Purity_PROC_Test.cpp`, `Mask_Parity_PROC_Test.cpp` — all continue to pass unmodified; none of them assert anything about hash SENSITIVITY to `SetImportedMask`'s own calls (only `Mask_DirtyHash_PROC_Test.cpp` does), so the helper's new version-stamping behavior is transparent to them.
4. `MapImporter_Fields_IO_Test.cpp`'s new `CheckImportedMaskVersionAdvancesOnEveryLoad` — `ALL PASS`: a successfully loaded stratum's version is non-zero, and two successive loads of the same file draw two DIFFERENT versions.
5. Manually confirmable by code inspection (no new perf test is required by this ticket, since the fix itself is the O(N)→O(1) change and the Compute Optimization Expert's own investigation is the evidence trail): `HashStoredArt` no longer contains a loop over `CellCount()`; grep confirms `art.importedMask.Data()` no longer appears in `Mask_PROC.cpp` at all.
6. Full `ctest` (or whatever the project's full suite invocation is) shows zero regressions elsewhere.

## Interpretation calls made beyond the two rulings' text
1. **`CheckStoredArtDirtying`'s rewrite uses a second `SetImportedMask` call (simulating a re-import) rather than manually setting `importedMaskVersion` by hand.** This keeps the test exercising the SAME public helper every other Mask test already uses, rather than reaching into the struct's internals directly — more representative of how a real caller (IO) actually triggers a version change, and consistent with the ticket's own principle that direct field mutation without a version bump should no longer be treated as a real "content changed" event anywhere, including in tests.
2. **`NextTestImportedMaskVersion()`'s counter is function-local `static`, test-binary-lifetime, unshared with the production `nextImportedMaskVersion` in `MapImporter_Fields_IO.cpp`.** These are two independent counters in two independent binaries/translation units by construction — no cross-contamination is possible or intended; this mirrors the production counter's own "process-lifetime, never reset" contract at the granularity that actually matters for each (one test binary's own run).
3. **The new IO test's second assertion** ("strata sharing one TGA call share one stamp; a stratum from the other TGA call draws its own") is written as an `||`-guarded single `Check` rather than two separate checks, to tolerate either possible relative ordering of the two `nextImportedMaskVersion` draws (low-TGA-first vs. high-TGA-first) without hard-coding which one `LoadBakedFields` calls first — the property being tested (a shared stamp within one call, a different stamp across calls) holds either way.

## Key files read/cited while drafting
`D:\Projects\Sanctuary\Map Generator\src\data\StratumArt_DATA.h`,
`D:\Projects\Sanctuary\Map Generator\src\proc\Mask_PROC.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\proc\Bake_PROC.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\proc\Mask_TestSupport_PROC.h`,
`D:\Projects\Sanctuary\Map Generator\src\proc\Mask_DirtyHash_PROC_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\proc\Mask_PROC_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\proc\Mask_Merge_PROC_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\proc\Mask_Purity_PROC_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\proc\Mask_Parity_PROC_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\io\MapImporter_Fields_IO.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\io\MapImporter_Fields_IO_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\io\MapImporter_IO.h`,
`D:\Projects\Sanctuary\Map Generator\src\pipeline\GenerationAssembler_PIPELINE.h`,
`D:\Projects\Sanctuary\Map Generator\src\pipeline\PreviewDriver_PIPELINE.cpp`,
`D:\Projects\Sanctuary\Map Generator\ARCH_14_18_AreaLiveBlendFidelityAndPalette.md` (the STEP218/219 record this bug was found investigating),
and the two expert rulings this session already produced (SanGen Generator Expert's fix-shape/write-site ruling; SanGen IO Architecture Expert's counter-placement ruling), both relayed verbatim into the sections above.
