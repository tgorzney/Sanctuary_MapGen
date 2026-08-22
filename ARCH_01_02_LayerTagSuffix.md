[← ARCH index](ARCH.md) · [§1 ARCH_01_NamingLaw](ARCH_01_NamingLaw.md) · SanGen ARCH §1.2. **Only the ARCH Expert writes this file.**

### 1.2 Layer tag is a SUFFIX (TGUE convention)
A file's **suffix** declares its Constitution §1 layer, and it must match the file's
folder (§2). Descriptive-name first, layer last:

| Suffix | Layer | Example files |
| --- | --- | --- |
| `_MATH` | MATH / SIMD | `Vector_MATH.h`, `Noise_MATH.h`, `Morton_MATH.h`, `Spatial_MATH.h` |
| `_DATA` | DATA — computed output | `Heightfield_DATA.h`, `MapFields_DATA.h`, `FlowMap_DATA.h`, `Props_DATA.h` |
| `_PARAMS` | PARAMS — adjustable settings | `Layers_PARAMS.h`, `MarkerRules_PARAMS.h`, `Geometry_PARAMS.h`, `Enums_PARAMS.h` |
| `_PROC` | PROC processors | `NoiseBlend_PROC.cpp`, `Erosion_PROC.cpp`, `Mask_PROC.cpp`, `Placement_PROC.cpp` |
| `_PIPELINE` | PIPELINE (conductor) | `Generation_PIPELINE.cpp`, `StageGraph_PIPELINE.h`, `DirtyHash_PIPELINE.h` |
| `_IO` | IO / BRIDGE | `SanmapImport_IO.cpp`, `SanmapExport_IO.cpp`, `SanpackReader_IO.cpp` |
| `_UI` | UI | `MaterialTab_UI.cpp`, `RangeSliderWidget_UI.h`, `MapCanvas_UI.cpp` |
| `_SYS` | SYS (runtime primitives) | `ArenaAllocator_SYS.h`, `ThreadPool_SYS.h`, `Dispatch_SYS.h`, `Log_SYS.h` |

This replaces the old prefixes (`Gen_`, `Tab_`, `Widget_`, `Params_`, `Sanmath_`).

