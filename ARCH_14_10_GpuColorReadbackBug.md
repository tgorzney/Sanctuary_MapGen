[← ARCH index](ARCH.md) · [§14 ARCH_14_PreviewOverlayLayering](ARCH_14_PreviewOverlayLayering.md) · SanGen ARCH §14.10. **Only the ARCH Expert writes this file.**

### 14.10 GPU color-texture readback bug (recorded, separate narrow fix, lands first)
`ComposeOnGpu()` (`PreviewComposite_Gpu_UI.cpp:78-81`) unconditionally reads back the full color
texture even on the GPU-resident hot path where nothing downstream consumes it (confirmed:
`Application_UI.cpp` only consumes it `if (!composite.LastRunUsedGpu())`) — up to 256MB wasted
PCIe transfer plus a blocking wait at the 8192² cap, every recompose. The entity-id buffer
readback on the same lines is **not** dead — `MapCanvas_UI.cpp` click-picking reads it
unconditionally on both backends. Fix scope: gate only the color-texture readback on
`!bLastRunUsedGpu`; leave entity-id readback as-is. Independent of, and should land before, the
overlay redesign — a narrow, already-diagnosed defect that compounds with every future Tier B
trigger.

