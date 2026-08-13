# SanGen ARCH — (not yet authored)

The authoritative SanGen v2 architecture. This file is intentionally empty: it
is authored and ratified in the SanGen ARCH Expert's dedicated setup
conversation, after the Expert has read the entire codebase and all resources.

**Only the SanGen ARCH Expert writes this file.** See the Setup Plan for the
process and `sangen_arch_pack/CONSTITUTION.md` for the always-loaded law.

## Opening hit-list (Appendix A of the Setup Plan)
1. Reconcile the two data-model families — dead `core/data/*` + `GenParams_*`
   vs live `params/Params_*`.
2. Dismember the `GenerationParams` god object; evict GPU/GL state from DATA.
3. Unify the CPU/GPU twins behind one dispatch interface; retire rival toggles.
4. Eliminate the preview's shadow reimplementation of the sim (WYSIWYG).
