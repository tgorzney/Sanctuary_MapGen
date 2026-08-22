[← ARCH index](ARCH.md) · SanGen ARCH §2. Part of the ratified v2 architecture; the Constitution (`sangen_arch_pack/CONSTITUTION.md`) and this file's preamble in `ARCH.md` bind alongside it. **Only the ARCH Expert writes this file.**

## 2. Layer → directory map (Constitution §1/§2)

One directory per layer, the folder and the file suffix always agree, so a misplaced
file is visible instantly. The `core/` + `gui/` split and the dead `core/data/` are
retired.

```
src/
  math/     *_MATH                 pure stateless math (SIMD, noise, vector, morton, spatial)
  data/     *_DATA                 computed SoA output (heightfield, masks, flow, resolved prop/marker/unit instances)
  params/   *_PARAMS               adjustable settings / the recipe (layer stack, rules, constants, seed) — serializes to the schema v3 PascalCase sections (§1.6, `SANMAP_FORMAT_SPEC`)
  proc/     *_PROC  +  *.glsl      processors; each CPU .cpp paired with its GPU .glsl
  pipeline/ *_PIPELINE             the conductor — dirty-hash DAG, stage order, backend policy
  io/       *_IO                   .sanmap / SupCom import-export, sanpack reader — the platform seam
  ui/       *_UI                   imgui-bypass tabs + widgets, 100k-entity preview
  sys/      *_SYS                  runtime primitives — threads, allocation, GPU resources, dispatch mechanism, logging
```

- **DATA vs PARAMS are separate folders** — computed output (`data/`) never mixes with
  the adjustable settings/recipe (`params/`). Input (PARAMS) vs output (DATA).
- **GPU lives beside its CPU twin** in `proc/`, not a separate `gpu/` tree.
- A large class's split files stay in their layer folder (`PreviewRenderer_*_UI.cpp`).
