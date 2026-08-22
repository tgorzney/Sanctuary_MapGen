[← ARCH index](ARCH.md) · [§8 ARCH_08_M4Resolutions](ARCH_08_M4Resolutions.md) · SanGen ARCH §8.4. **Only the ARCH Expert writes this file.**

### 8.4 Scope law — a coder never invents a missing type
Standing rule, generalized from §8.2/§8.3 (both were caught only because the dispatcher
diffed the work-orders against the tree):

> **A work-order's target-file list is exhaustive.** A coder that discovers it needs a type
> which does not exist **stops and reports**; it does not create that type in a folder its
> work-order does not name, and it does not substitute a legacy `core/` type to get a
> build. A missing type is a missing work-order.

Rationale: a type created as a side effect of another task gets its shape from whatever the
one caller happened to need, in whatever layer that caller lived — which is precisely how
the v1 duplicate `StratumSettings` families (hit-list #1) came to exist. New types get a
work-order and, where the shape is not obvious, an ARCH ruling (§8.2, §8.3 are those
rulings).

