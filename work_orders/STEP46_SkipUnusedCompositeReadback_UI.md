# STEP46 — Skip the wasted composite-texture GPU readback on the production hot path

**Layer:** UI. **Domain:** Preview composite (`PreviewComposite`).

## Problem
`PreviewComposite::ComposeOnGpu()` (`src/ui/PreviewComposite_Gpu_UI.cpp:74-81`) unconditionally
does, after every GPU recomposite:
```cpp
WaitForCompletion(manager);                       // blocking spin-yield fence wait
manager.ReadbackTexture(compositeTexture, compositeTexels.data(),
                        compositeTexels.size() * sizeof(unsigned int));
manager.ReadbackBuffer(CompositeBufferName::kEntityIdentifiers, entityIdentifierBuffer.Data(),
                       entityIdentifierBuffer.CellCount() * sizeof(unsigned int));
```
The **texture** readback (`ReadbackTexture`) is unused on the production hot path. Confirmed via
`Application::BindCompositeToCanvas` (`src/ui/Application_UI.cpp:80-85`): it only reads
`composite.CompositeTexels()` when `!composite.LastRunUsedGpu()` — on the normal GPU-resident
path, the canvas draws `composite.CompositeTexture()` (the GL handle) directly and never touches
the CPU-side texels. So on every production recomposite where the GPU backend is active, that
readback transfers up to `previewResolution² × 4` bytes (256MB at the 8192² cap) over PCIe and
blocks on `WaitForCompletion` for data nothing consumes.

**The entity-id buffer readback (`ReadbackBuffer`) is NOT part of this problem — do not touch
it.** `MapCanvas::ApplyClick` (`src/ui/MapCanvas_UI.cpp:33-37`) reads the entity-id buffer
unconditionally on both backends for click-picking; it stays exactly as-is.

## Why this isn't a one-line delete
`CompositeTexels()` is a tested parity contract, not dead API surface. Multiple test files call
the same `Compose()`/`ComposeOnGpu()` entry point production uses and then read
`CompositeTexels()` directly to verify GPU output — e.g. `PreviewComposite_Gpu_UI_Test.cpp:108-110`
(`HasVariedTexels`/`TexelsWithinTolerance` against the Cpu twin), plus
`PreviewComposite_Wysiwyg_UI_Test.cpp`, `PreviewIntegration_Picking_UI_Test.cpp`,
`ParameterTabs_DirtyTier_UI_Test.cpp`, `PreviewIntegration_DirtyTier_UI_Test.cpp`,
`ApplicationShell_UI_Test.cpp`, `ApplicationShell_Window_UI_Test.cpp`,
`PreviewComposite_UI_Test.cpp`. None of these pass any extra argument today — they all rely on the
texture readback happening by default. Gating the readback on `LastRunUsedGpu()` *inside*
`ComposeOnGpu()` would silently break every one of these.

## Fix
Add a defaulted parameter that keeps every existing call site's behavior byte-for-byte identical,
and let only the production call site opt out:

```cpp
// PreviewComposite_UI.h
void Compose(bool bNeedsTexelReadback = true);
void ComposeOnGpu(bool bNeedsTexelReadback = true);   // ComposeOnCpu unaffected — its texels ARE
                                                        // the primary output, always needed.
```
```cpp
// PreviewComposite_UI.cpp
void PreviewComposite::Compose(bool bNeedsTexelReadback) {
    if (gpuResourceManager != nullptr) { ComposeOnGpu(bNeedsTexelReadback); return; }
    ComposeOnCpu();
}
```
```cpp
// PreviewComposite_Gpu_UI.cpp
void PreviewComposite::ComposeOnGpu(bool bNeedsTexelReadback) {
    ...                                    // unchanged through the dispatch sequence
    WaitForCompletion(manager);
    if (bNeedsTexelReadback) {
        manager.ReadbackTexture(compositeTexture, compositeTexels.data(),
                                compositeTexels.size() * sizeof(unsigned int));
    }
    manager.ReadbackBuffer(CompositeBufferName::kEntityIdentifiers, entityIdentifierBuffer.Data(),
                           entityIdentifierBuffer.CellCount() * sizeof(unsigned int));
    bLastRunUsedGpu = true;
}
```
Only production's call site changes, to opt out explicitly:
```cpp
// Application_UI.cpp, WireCallbacks()
previewDriver.SetPreviewCompositeCallback([this] { composite.Compose(/*bNeedsTexelReadback=*/false); });
```
Update the header comment on `Compose()`/`ComposeOnGpu()` (`PreviewComposite_UI.h:47-52`) to
document the parameter's purpose (skips the texture readback only, entity-id readback is
unaffected) — don't let the "reports which [backend] it used" comment go stale.

⚠️ **Do not touch `WaitForCompletion`** — the entity-id readback still needs the fence wait, so
the blocking wait itself isn't removable here, only the now-conditional texture transfer after it.

## Files touched
- `src/ui/PreviewComposite_UI.h` — `Compose`/`ComposeOnGpu` signatures + doc comment
- `src/ui/PreviewComposite_UI.cpp` — `Compose()` threads the parameter through
- `src/ui/PreviewComposite_Gpu_UI.cpp` — `ComposeOnGpu()` gates the texture readback only
- `src/ui/Application_UI.cpp` — `WireCallbacks()` passes `false` at the one production call site

No test file should need to change — every existing call site keeps the default (`true`) and
keeps observing populated `CompositeTexels()` exactly as today.

## Verify
Full solo rebuild + `ctest -C Debug`, full suite green with zero test edits. That itself is the
acceptance check: if any existing test needed modification to pass, the default-argument
threading was done wrong.
