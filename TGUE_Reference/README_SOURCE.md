# TGUE Reference Copies

Copied from `D:\Projects\UE5 Projects\TGUE\sys_docs\` on 2026-08-15 for use in
the Map Generator project. Source of truth remains the TGUE repo — these are
reference-only snapshots, not synced.

Pulled for: designing an events tree for Sanctuary map scripts, based on
TGUE's event-sourcing / timeline pattern (TGUE_EventStream).

Files:
- TGUE_EVENTSTREAM_SPEC.md — the Delta Array (event-sourcing ledger) + Time
  Scrubbing (replay-to-microsecond) core concept.
- TGUE_PLUGIN_API_SPEC.md — Event Sourcing Input: mods mutate state only by
  appending to the EventStream; async hooks, 0% idle CPU.
- TGUE_MASTER_FLOW.md — 6-phase tick showing where event data gets sealed/
  processed each frame.
- feature_request_event_sourced_state.md — event-driven callbacks instead of
  polling (geofence example).
- feature_request_event_driven_state_masking.md — bitmask-skip for inactive
  entities.
- feature_request_temporal_decoupling.md — localized sub-stepping / variable
  tick rates per region.
