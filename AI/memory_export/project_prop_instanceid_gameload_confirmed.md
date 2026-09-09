---
name: project-prop-instanceid-gameload-confirmed
description: Live game-load test confirmed adding a per-instance field (InstanceId) to every .sanmap prop transform does not break map load
metadata: 
  node_type: memory
  type: project
  originSessionId: 1020d8fc-2688-405c-b760-425353e898e9
  modified: 2026-08-24T20:00:42.192Z
---

On 2026-08-24, tested adding a new `"InstanceId"` integer field directly into every prop transform object (1180 total, across all 17 prop blueprint groups) in a real shipped map: `Pandemonium Isthmus.sanmap` (Sanctuary: Shattered Sun Demo). The map loaded successfully in-game afterward (human tested 1-player, spawn placed correctly).

**Why this matters:** confirms empirically, not just by code-reading inference, that the game's `.sanmap` loader tolerates unrecognized extra keys on per-instance entity objects (not just unrecognized top-level sections, which was already known-safe). This was the missing "production-proven" evidence the SanGen Format Expert flagged as absent when reasoning from precedent alone (see [[project_workorder_consolidation_2026_08]] for related backlog context).

**How to apply:** this de-risks the planned "Assembly" feature and any other design needing a stable per-instance ID on props/markers/decals in `.sanmap`. When picking up that work, the ARCH Expert should have already recorded this into `sangen_arch_pack/specs/SANMAP_FORMAT_SPEC.md` (only the ARCH Expert can write there) — check that spec first; if the entry is missing, this memory is still the source of truth for the empirical result until it's recorded.

**Caveat:** only tested on `props`. Markers/decals use the same generic-decode-then-named-field-read game loader mechanism per Format Expert's analysis, so the same conclusion should hold, but wasn't separately live-tested.
