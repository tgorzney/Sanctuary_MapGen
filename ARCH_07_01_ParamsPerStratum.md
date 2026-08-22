[← ARCH index](ARCH.md) · [§7 ARCH_07_M3Resolutions](ARCH_07_M3Resolutions.md) · SanGen ARCH §7.1. **Only the ARCH Expert writes this file.**

### 7.1 Where the remaining PARAMS live — one settings type per stratum
`ErosionFlow_PARAMS.h` and `Stratum_PARAMS.h` live in `src/params/` with the `_PARAMS`
suffix, like every other setting.

**There is exactly ONE per-stratum settings type: `Params::Stratum`, in
`src/params/Stratum_PARAMS.h`.** Everything a stratum is configured with — the mask
slope gate, the stored-mask merge mode, the bake/appearance settings (albedo source,
tint, tiling, the material's own mask-texture remap window — §7.2 item 5), and the soil
physics — is reached through it.

- **No rival per-stratum settings type.** A stage takes `const std::vector<Params::Stratum>&`
  (or a span of it). It may **not** take its own private per-stratum array. Two rival arrays
  must be kept in sync by hand, and the same field appears twice — which is exactly how the
  double-remap code defect below was created.
- **Composition is allowed; rival top-level types are not.** `Params::Stratum` aggregates
  small named sub-structs, and a sub-struct may be split into its own `_PARAMS` header
  when the §1.5 ceiling forces it. Such a header is a **member** file of `Stratum_PARAMS.h`,
  never a settings type a stage reaches independently.
- The imported mask **pixel data** (the TGA contents) is not settings: it is a
  `Data::FloatField` in `src/data/` (loaded input, not part of the recipe). Rule:
  *modes/thresholds → PARAMS; loaded pixels → DATA.*

**Standing violations to clear (current tree, M3-2 rework):**
1. `src/params/StratumMask_PARAMS.h` is a rival top-level per-stratum settings type — it
   must fold into `Params::Stratum`.
2. That same file holds `std::vector<float> importedMaskData` — loaded pixels inside
   PARAMS, a second breach of the rule above. The pixels move to a `Data::FloatField`.
3. `Proc::StratumBakeSource` in `src/proc/Bake_Kernel_PROC.h` is a *third* per-stratum
   settings surface living in PROC. Its settings fields fold into `Params::Stratum`; only
   the flattened GPU-layout record (`StratumKernelConfiguration`) stays in PROC.

